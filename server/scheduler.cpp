#include"hook.h"
#include"log.h"
#include"macro.h"
#include"scheduler.h"

namespace server{
    static server::Logger::ptr g_logger = SERVER_LOG_NAME("system");

    static thread_local Scheduler* t_scheduler = nullptr;
    static thread_local Fiber* t_fiber = nullptr;        //主协程

    Scheduler::Scheduler(size_t thread, bool use_caller, const std::string& name)
        :m_name(name){
        SERVER_ASSERT(thread > 0);   //线程调度器调度的线程数起码要至少为1个

        //将当前线程加入调度队列
        if(use_caller){
            server::Fiber::GetThis();  //获得主协程
            thread--;  //当前线程要加入调度队列，要求的线程数减1

            SERVER_ASSERT(GetThis() == nullptr);   //不允许线程中存在多个调度器
            t_scheduler = this;
            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true)); //m_cb = this.run; //root_fiber不需要判断是不是主协程，也就没有必要做协程切换。

            Thread::SetName(m_name);

            t_fiber = m_rootFiber.get();

            m_rootThreadId = GetThreadId();
            m_threadIds.push_back(m_rootThreadId);  //当前线程加入调度队列
        }
        else{
            m_rootThreadId = -1;
        }
        m_threadCount = thread;
    }

    Scheduler::~Scheduler(){
        SERVER_ASSERT(m_stopping);   //协程调度器要属于停止状态
        if(GetThis() == this){
            t_scheduler = nullptr;
        }
    }

    Scheduler* Scheduler::GetThis(){
        return t_scheduler;
    }

    Fiber* Scheduler::GetMainFiber(){
        return t_fiber;
    }

    void Scheduler::start(){
        //SERVER_LOG_INFO(g_logger) <<" start scheduler " ;
        MutexType::Lock lock(m_mutex);
        if(!m_stopping){  //默认stopping是true，如果stopping是true的话，说明调度器还未启动
            return;
        }
        //启动调度器
        m_stopping = false;
        SERVER_ASSERT(m_threads.empty()); //保证线程池为空
        m_threads.resize(m_threadCount);
        
        for(size_t i = 0; i < m_threadCount; ++i){
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());   //这个地方的getID返回的是什么；
        }
        lock.unLock();
    }

    void Scheduler::stop(){  //还没看懂写的啥玩意
        m_autoStop = true;
        //SERVER_LOG_INFO(g_logger) <<" stop thread: " <<  GetThis()->m_name;
        if(m_rootFiber && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT)){
            //SERVER_LOG_INFO(g_logger) << this << " stopped";
            m_stopping = true;

            if(stopping()){   //加入任务全部执行完毕，并且协程队列为空，则退出了
                return;
            }
        }
        //bool exit_on_this_fiber = false;

        //保证scheduler的退出一定是在创建scheduler的线程之下
        if(m_rootThreadId != -1){
            SERVER_ASSERT(GetThis() == this);
        }else{
            SERVER_ASSERT(GetThis() != this);
        }

        m_stopping =  true;
        for(size_t i = 0; i < m_threadCount; ++i){
            tickle();
        }


        if(m_rootFiber){
            tickle();
        }

        if(m_rootFiber){
            if(!stopping()){
                // if(m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::EXCEPT){  //&& 和 || 不要乱写，时刻注意
                //     m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
                //     SERVER_LOG_INFO(g_logger) << "root fiber is term , reset it";
                //     t_fiber = m_rootFiber.get();
                // }
                m_rootFiber->call();
            }
        }
        //回收线程
        std::vector<Thread::ptr>thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }
        for(auto i : thrs){
            i->join();
        }
    }

    void Scheduler::SetThis(){
        t_scheduler = this;
    }

    void Scheduler::run(){
        //SERVER_LOG_INFO(g_logger) << "run";
        set_hook_enable(true);
        SetThis();
        if(GetThreadId() != m_rootThreadId){
            t_fiber = Fiber::GetThis().get();
        }
        Fiber::ptr idel_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        //idel_fiber->setState(Fiber::TERM);
        Fiber::ptr cb_fiber;

        FiberAndThread ft;
        while(true){
            ft.reset();  //重置
            bool is_active = false;  //标志有没有可用任务正在执行，如果有，其他的协程就不应该陷入idel。
            bool tickle_me = false;
            {
                MutexType::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                while(it != m_fibers.end()){
                    if(it->thread != -1 && it->thread != GetThreadId()){   //不是主协程对应的线程也不是正在执行的线程，当前设置的线程id不等于期望的线程id（任务有要求）
                        ++it;
                        tickle_me = true;
                        continue;
                    }
                    SERVER_ASSERT(it->fiber || it->cb);
                    if(it->fiber && it->fiber->getState() == Fiber::EXEC){  //正在执行
                        ++it;
                        continue;
                    } 
                    ft = *it;
                    m_fibers.erase(it);   //取出消息，并将消息从队列中删除
                    ++m_activeThreadCount;
                    is_active= true;
                    break;  //这里应该要跳出循环把
                }
            }
            //SERVER_LOG_INFO(g_logger) << "center";
            if(tickle_me){
                tickle();
            }
            //SERVER_LOG_INFO(g_logger) << "event start -------------------------------";
            if(ft.fiber && ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT){  //拿到的协程并没有结束执行
                //++m_activeThreadCount;   多线程条件下，多个协程可能执行状态不同，导致其他的fiber对这个正在执行的线程数的判断出现错误，导致协程结束，应该把这个状态变化前置。
                //SERVER_LOG_INFO(g_logger) << "fiber start" <<server::Fiber::GetFiberId() ;

                ft.fiber->swapIn();  //开始执行；
                --m_activeThreadCount;
                //SERVER_LOG_INFO(g_logger) << "fiber back";
                //SERVER_LOG_INFO(g_logger) << "now fiber id: " << server::Fiber::GetThis()->GetFiberId() << " state: " << server::Fiber::GetThis()->getState();
                //SERVER_LOG_INFO(g_logger) << "back fiber id: " << ft.fiber->GetFiberId() << " state: " << ft.fiber->getState();
                
                //SERVER_LOG_INFO(g_logger) << "ft.fiber" << ft.fiber->getState() << " " << Fiber::TERM;
                if(ft.fiber->getState() == Fiber::READY){
                    //内部调用了YeildToReady  
                    schedule(ft.fiber);  //这里其实放ft和ft.fiber都可以，一个调用赋值构造函数，一个调用构造函数, ft重新加入调度队列
                } 
                else if(ft.fiber->getState() != Fiber::TERM  && ft.fiber->getState() != Fiber::EXCEPT){   //既没有出错也没有结束
                    
                    ft.fiber->setState(Fiber::HOLD);    //那就暂时阻塞掉这个协程,让出执行
                }
                //SERVER_LOG_INFO(g_logger) << "back fiber id: " << ft.fiber->GetFiberId() << " state: " << ft.fiber->getState();
                //SERVER_LOG_INFO(g_logger) << "fiber state" << ft.fiber->getState();
                //SERVER_LOG_INFO(g_logger) << "fiber use_count" << ft.fiber.use_count();
                ft.reset();  //重置ft对象
                //SERVER_LOG_INFO(g_logger) << "fiber end";
            }  
            else if(ft.cb){
                //SERVER_LOG_INFO(g_logger) << "call back start";
                if(cb_fiber){
                    cb_fiber->reset(ft.cb);  //将cb_fiber的run函数重设为ft.cb
                }else{
                    cb_fiber.reset(new Fiber(ft.cb));  //智能指针reset
                    //SERVER_LOG_INFO(g_logger) << "fiber init   "  << cb_fiber->GetFiberId();
                }
                ft.reset();  //重置ft对象
                
                //++m_activeThreadCount;  这个值前置。防止其他的协程重新退出。
                //SERVER_LOG_INFO(g_logger) << "fiber run   "  << cb_fiber->GetFiberId();
                cb_fiber->swapIn();   //执行协程
                --m_activeThreadCount; 

                if(cb_fiber->getState() == Fiber::READY){
                     //内部调用了YeildToReady
                    schedule(cb_fiber);  //cb_fiber重新加入调度队列
                    cb_fiber.reset();
                } else if(cb_fiber->getState() == Fiber::TERM || cb_fiber->getState() == Fiber::EXCEPT){  //执行结束了
                    cb_fiber->reset(nullptr);  //将cb_fiber的cb置空
                }else{  //其他状态
                    cb_fiber->setState(Fiber::HOLD);
                    cb_fiber.reset();
                }
                //SERVER_LOG_INFO(g_logger) << "call back end";
            }else{  
                //SERVER_LOG_INFO(g_logger) << "idel start";
                if(is_active){  //有其他的协程执行，那么他就不应该执行idle。
                    --m_activeThreadCount;
                    continue;
                }
                //m_fiber里面什么都没有，就执行idel协程
                //SERVER_LOG_INFO(g_logger) << "a";
                if(idel_fiber->getState() == Fiber::TERM){
                    //SERVER_LOG_INFO(g_logger) << "idel fiber term";
                    break;
                }
                //SERVER_LOG_INFO(g_logger) <<"idel start"  << Fiber::GetFiberId() ;
                ++m_idleThreadCount ;     
                idel_fiber->swapIn();
                --m_idleThreadCount;
                //SERVER_LOG_INFO(g_logger) <<"idel end" ;
                if(idel_fiber->getState() != Fiber::TERM && idel_fiber->getState() != Fiber::EXCEPT){
                    idel_fiber->setState(Fiber::HOLD);
                }
                //idel_fiber.reset();
                
            }
        }
    }

    void Scheduler::tickle(){
        //SERVER_LOG_INFO(g_logger) << server::BackTrace();
        SERVER_LOG_INFO(g_logger) << "tickle";
    }

    bool Scheduler::stopping(){
        MutexType::Lock lock(m_mutex);
        return (m_stopping && m_autoStop
            && m_fibers.empty()
            && m_activeThreadCount == 0);
    }

    void Scheduler::idle(){ 
        SERVER_LOG_INFO(g_logger) << "idle";
        //不能让他直接退出
        while(!stopping()){
            server::Fiber::YieldToHold();
        }
    }


}