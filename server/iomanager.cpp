#include"iomanager.h"
#include"macro.h"
#include"log.h"
#include<unistd.h>  //提供pipe函数
#include<sys/epoll.h>  //提供epoll相关函数
#include<fcntl.h>  //提供fcntl函数，用于修改句柄的属性
#include<errno.h>
#include<string.h>


namespace server{

    static server::Logger::ptr g_logger = SERVER_LOG_NAME("system");

    IOManager::FdContext::EventContext& IOManager::FdContext::getEventContext(IOManager::Event event){
        switch (event)
        {
        case IOManager::READ:
            return read;
            break;
        case IOManager::WRITE:
            return write;
        default:
            SERVER_ASSERT2(false, "getcontext error");
            break;
        }
    }
    void IOManager::FdContext::resetContext(EventContext& ctx){
        ctx.scheduler = nullptr;
        ctx.fiber = nullptr;
        ctx.cb = nullptr;
    }

    void IOManager::FdContext::triggerEvent(IOManager::Event event){
        SERVER_ASSERT(events & event);   //给定的事件一定要存在
        events = (Event)(events & (~event));
        EventContext& event_ctx = getEventContext(event);
        if(event_ctx.cb){
            event_ctx.scheduler->schedule(event_ctx.cb);
        }else{
            event_ctx.scheduler->schedule(event_ctx.fiber);
        }
        event_ctx.scheduler = nullptr;
        event_ctx.fiber = nullptr;
        return;
    }

    IOManager::IOManager(size_t threads, bool use_caller, const std::string& name )
        :Scheduler(threads, use_caller, name){
        m_epfd = epoll_create(5000);  //监听epoll
        SERVER_ASSERT(m_epfd > 0);   //成功调用的话要返回大于0的值

        //使用管道的方式进行处理
        int rt = pipe(m_tickleFds);
        SERVER_ASSERT(!rt);  //成功调用返回零值

        epoll_event event;  //epoll事件的结构体
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET;  //EPOLLIN: 水平触发， 文件可读触发  EPOLLET： 边缘触发， 仅在文件描述符fd的状态发生变化的时候才会触发。

        event.data.fd = m_tickleFds[0];  //0是管道的只读端

        //系统调用可以用来对已打开的文件描述符进行各种控制操作以改变已打开文件的的各种属性,对指定的文件执行给定的命令
        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);   //F_SETFL是用于设置文件状态标识的命令行代号， O_NONBLOCK 将读句柄的属性改称非阻塞
        SERVER_ASSERT(rt  != -1);

        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);  //将这个事件添加到m_epfd的监听范围
        SERVER_ASSERT(!rt);  //成功返回零

        contextResize(32);

        start();
    }
    
    IOManager::~IOManager(){
        stop();

        //关闭分配的句柄
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]); 

        //回收事件结构体的内存
        for(size_t i = 0; i < m_fdContexts.size(); ++i){
            if(m_fdContexts[i]){
                delete m_fdContexts[i];
            }
        }
    }
    void IOManager::contextResize(size_t size){
        m_fdContexts.resize(size);
        for(size_t i = 0; i < size; ++i){
            if(!m_fdContexts[i]){
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }
    }

    int IOManager::addEvent(int fd, Event event, std::function<void()> cb ){
        FdContext* fd_ctx = nullptr;
        MutexType::ReadLock lock(m_mutex); 
        if(m_fdContexts.size() > fd){
            fd_ctx = m_fdContexts[fd];
            lock.unLock();
        }else{
            lock.unLock();
            MutexType::WriteLock lock2(m_mutex);
            //这样写导致可能调用多次
            /*
            while(m_fdContexts.size() < fd){
                contextResize(m_fdContexts.size() * 1.5);
            }
            */
           contextResize(fd * 1.5);
            fd_ctx = m_fdContexts[fd];
        }
        //SERVER_LOG_INFO(g_logger) << "fd_ctx event : "<<  this << " "<< fd_ctx->events << " fd = " << fd << "fd_ctx" << &fd_ctx;
        FdContext::MutexType::Lock lock3(fd_ctx->mutex);
        if(fd_ctx->events & event){  //两个线程在重复添加事件。
            SERVER_LOG_INFO(g_logger) << "addEvent assert fd=" << fd << " event=" << event << " fd_ctx.event=" << fd_ctx->events;
            SERVER_ASSERT(!(fd_ctx->events & event));
        }

        //使用epoll_ctl修改
        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;  //有则修改，无则添加
        epoll_event epevent;
        epevent.events = EPOLLET | fd_ctx->events | event;
        epevent.data.ptr = fd_ctx;   //回调fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if(rt){
            //返回非0值，代表添加事件失败
            SERVER_LOG_INFO(g_logger) << "epollctl(" << m_epfd << ", " << op << ", " << fd << ", " << epevent.events << ");" << rt << 
                                        "("  << errno << ") (" << strerror(errno) << ").";
            return -1;
        }
        
        ++m_pendingEventCount;
        fd_ctx->events = (Event)(fd_ctx->events | event);
        //SERVER_LOG_INFO(g_logger) << "fd_ctx event22- : " << fd_ctx->events << " fd = " << fd << " fd_ctx" << &fd_ctx;
        FdContext::EventContext& event_ctx = fd_ctx->getEventContext(event);

        //返回的事件一定要为空
        SERVER_ASSERT(!(event_ctx.scheduler || event_ctx.fiber || event_ctx.cb));
        event_ctx.scheduler = Scheduler::GetThis();
        if(cb){
            event_ctx.cb.swap(cb);
        }else{
            event_ctx.fiber = Fiber::GetThis();
            SERVER_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
        }
        return 0;
    }
    bool IOManager::delEvent(int fd, Event event){
        SERVER_LOG_INFO(g_logger) <<"deal event";
        MutexType::ReadLock lock(m_mutex);
        if(m_fdContexts.size() <= fd){
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        lock.unLock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if(!(fd_ctx->events & event)){  //要删除的事件不存在，那就没有必要删除
            return false;
        }

        Event new_event = (Event) (fd_ctx->events & event);
        int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;  //这里好像没有要，这个值一定是零 <--错了，可能是非零值，例如同时存在读写事件，删除读事件。
        epoll_event epevent;
        epevent.events = EPOLLET | new_event;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if(rt){
            //返回非0值，代表添加删除失败
            SERVER_LOG_INFO(g_logger) << "epollctl(" << m_epfd << ", " << op << ", " << fd << ", " << epevent.events << ");" << rt << 
                                        "("  << errno << ") (" << strerror(errno) << ").";
            return false;
        }

        //下面这段不是很懂作用是啥 -> 将event限制在新的event下
        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;

        return true;
    }
    bool IOManager::cancelEvent(int fd, Event event){
        SERVER_LOG_INFO(g_logger) <<"cancel event";
        MutexType::ReadLock lock(m_mutex);
        if(m_fdContexts.size() <= fd){
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        lock.unLock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if(!(fd_ctx->events & event)){  //要删除的事件不存在，那就没有必要删除
            return false;
        }

        Event new_event = (Event) (fd_ctx->events & event);
        int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;  //这里好像没有要，这个值一定是零 <--错了，可能是非零值，例如同时存在读写事件，删除读事件。
        epoll_event epevent;
        epevent.events = EPOLLET | new_event;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if(rt){
            //返回非0值，代表添加删除失败
            SERVER_LOG_INFO(g_logger) << "epollctl(" << m_epfd << ", " << op << ", " << fd << ", " << epevent.events << ");" << rt << 
                                        "("  << errno << ") (" << strerror(errno) << ").";
            return false;
        }

        //下面这段不是很懂作用是啥 -> 将event限制在新的event下
        fd_ctx->triggerEvent(event);  //触发原来的事件并且将事件改为对应的事件。
         --m_pendingEventCount;
        return true;
    }

    bool IOManager::cancelAll(int fd){
       // SERVER_LOG_INFO(g_logger) <<"cancel all event" <<  fd;
        //SERVER_LOG_INFO(g_logger) << BackTrace();
        MutexType::ReadLock lock(m_mutex);
        if(m_fdContexts.size() <= fd){
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        lock.unLock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if(!fd_ctx->events){  //要删除的事件不存在，那就没有必要删除
            return false;
        }
        SERVER_LOG_INFO(g_logger) <<"cancel all event over" << fd;
        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if(rt){
            //返回非0值，代表添加删除失败
            SERVER_LOG_INFO(g_logger) << "epollctl(" << m_epfd << ", " << op << ", " << fd << ", " << epevent.events << ");" << rt << 
                                        "("  << errno << ") (" << strerror(errno) << ").";
            return false;
        }
 
        //触发是原来的读写事件。
        if(fd_ctx->events & READ){
            SERVER_LOG_INFO(g_logger) <<"trugger read";
            fd_ctx->triggerEvent(READ);  //触发原来的事件并且将事件改为对应的事件。
            --m_pendingEventCount;
        }
        if(fd_ctx->events & WRITE){
             SERVER_LOG_INFO(g_logger) <<"trugger write";
            fd_ctx->triggerEvent(WRITE);  //触发原来的事件并且将事件改为对应的事件。
            --m_pendingEventCount;
        }
        SERVER_ASSERT(fd_ctx->events == 0);
        return true;
    }

    IOManager* IOManager::GetThis(){
        return dynamic_cast<IOManager*>(Scheduler::GetThis());
    }

    //Scheduler的三个虚函数
    void IOManager::tickle() {
        //SERVER_LOG_INFO(g_logger) << "tickle" ;
        if(haveIdelThreads()){
            //向管道写入一个标志符
            int rt = write(m_tickleFds[1], "T", 1);
            SERVER_ASSERT(rt == 1);
        }
    }

    bool IOManager::stopping(uint64_t& time_out){
        //SERVER_LOG_INFO(g_logger) << "nextTimer";
        time_out = getNextTimer();
        return stopping();
    }
    bool IOManager::stopping() {
        return Scheduler::stopping() 
            && m_pendingEventCount == 0
            && !hasTimer();   //定时器任务也没有了

    }

    void IOManager::idle(){
        //SERVER_LOG_INFO(g_logger) << "idel swap in";
        //捕获因为IO带来的事件（read/write)
        epoll_event* events = new epoll_event[64]();
        //自定义shared_ptr的析构函数
        std::shared_ptr<epoll_event> event_ptr(events, [](epoll_event* events){
            delete[] events;
        });
        //开始捕获消息
        //SERVER_LOG_INFO(g_logger) << "m_peddingcount=" << m_pendingEventCount;
        while(true){
            //调度器停止了，那么就退出（没有事件可以调度），调度器也处于退出状态
            uint64_t next_timeout = 0;
            //SERVER_LOG_INFO(g_logger) << "count tasks" << m_pendingEventCount << " " << hasTimer();
            if(stopping(next_timeout)){
                //SERVER_LOG_INFO(g_logger) << "name=" << getName() << "idle stopping exit";
                break;
            }
            //SERVER_LOG_INFO(g_logger) << "next_ timeout: " << next_timeout;
            int rt = 0;
            do {
                //epoll_wait等待的时间阈值，大于这个阈值没有事件发生也要退出。
                static const int MAX_TIMEOUT = 5000;
                if(next_timeout != ~0ul){ //有超时事件
                    next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
                }else{
                    next_timeout = MAX_TIMEOUT;
                }
                //epoll_wait等待管道是否有消息写入
                rt = epoll_wait(m_epfd, events, 64, (int)next_timeout);
                //SERVER_LOG_INFO(g_logger) << "epoll wait rt=" << rt << " errno=" << strerror(errno);
            }while(rt < 0 && errno == EINTR);
            //SERVER_LOG_INFO(g_logger) << "epoll wait rt=" << rt << " errno=" << strerror(errno);
            //rt为等待到的事件数，处理对应的事件
            std::vector<std::function<void()> > cbs;
            listExpiredCb(cbs);
            if(!cbs.empty()){
                //SERVER_LOG_INFO(g_logger) << "add cbs, cbs.size=" << cbs.size();
                schedule(cbs.begin(), cbs.end());
                //SERVER_LOG_INFO(g_logger) << "add cbs over, cbs.size=" << cbs.size();
            }
            for(size_t i = 0; i < rt; ++i){
                epoll_event& event = events[i];
                //SERVER_LOG_INFO(g_logger) << event.data.fd << " " << event.events;
                //处理掉通知事件，不是明确是读/写IO事件
                if(event.data.fd == m_tickleFds[0]){
                    uint8_t dummy = 0;
                    while(read(m_tickleFds[0], &dummy, 1) == 1);
                    continue;
                }
                //处理IO事件
                //所有经过epoll_wait处理的事件的event.data.ptr都会被设置程fd_ctx;
                FdContext* fd_ctx = static_cast<FdContext*>(event.data.ptr);
                FdContext::MutexType::Lock lock(fd_ctx->mutex);
                //确认event的events是是否有（EPOLLERR）还是 中断（EPOLLHUP），如果是出错，就应该要清理连接，读取/或者写入未处理的所有消息（处理掉）
                if(event.events & (EPOLLERR | EPOLLHUP)){
                    event.events |= (EPOLLIN | EPOLLOUT);  //将读写事件加入
                }
                int real_event = NONE;
                //读取读事件以及写事件
                if(event.events & EPOLLIN){
                    real_event |= READ;
                }
                if(event.events & EPOLLOUT){
                    real_event |= WRITE;
                }
                //fd_ctx的事件被处理掉了。
                if((fd_ctx ->events & real_event) == NONE){
                    continue;
                }
                //SERVER_LOG_INFO(g_logger) << "server";
                int left_event = (fd_ctx->events) & (~real_event);  //处理完之后剩余的事件
                int op = left_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_event;
                
                //将剩余的事件修改并写入epoll
                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
                if(rt2){
                    //返回非0值，代表添加删除失败
                    SERVER_LOG_INFO(g_logger) << "epollctl(" << m_epfd << ", " << op << ", " << fd_ctx->fd << ", " << event.events << ");" << rt << 
                                        "("  << errno << ") (" << strerror(errno) << ").";
                    
                    //如果没有改成，那么这个事件也就不能处理；
                    continue;
                }

                //处理读事件
                if(real_event & READ){
                    //SERVER_LOG_INFO(g_logger) << "trigger read";
                    fd_ctx->triggerEvent(READ);
                    --m_pendingEventCount;
                }
                //处理写事件
                if(real_event & WRITE){
                    //SERVER_LOG_INFO(g_logger) << "trigger write";
                    fd_ctx->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }
            //SERVER_LOG_INFO(g_logger) << "out";
            //处理完一轮之后，让出执行权,下面的操作可以避免智能指针计数增加
            Fiber::ptr cur = Fiber::GetThis();
            auto raw_ptr = cur.get();
            cur.reset();
            raw_ptr->swapOut();
        }
    }

    void IOManager::onTimerInsertedAtFront(){
        //SERVER_LOG_INFO(g_logger) << "onTimer";
        tickle();
    }

    
}