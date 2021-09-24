#include <dlfcn.h>
#include"config.h"
#include"log.h"
#include"hook.h"
#include"fd_manager.h"
#include"fiber.h"
#include"iomanager.h"


server::Logger::ptr g_logger = SERVER_LOG_NAME("system");
namespace server{

    static server::ConfigVar<int>::ptr g_tcp_connect_timeout = server::Config::lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

    static thread_local bool t_hook_enable = false;
    
    //定义一个宏
    #define HOOK_FUN(XX) \
        XX(sleep) \
        XX(usleep) \
        XX(nanosleep) \
        XX(socket) \
        XX(connect) \
        XX(accept) \
        XX(read) \
        XX(readv) \
        XX(recv) \
        XX(recvfrom) \
        XX(recvmsg) \
        XX(write) \
        XX(writev) \
        XX(send) \
        XX(sendto) \
        XX(sendmsg) \
        XX(close) \
        XX(fcntl) \
        XX(ioctl) \
        XX(getsockopt) \
        XX(setsockopt) 

    void hook_init(){
        static bool is_init = false;
        if(is_init){
            return;
        }
    #define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX);
    #undef XX
    }

    static uint64_t s_connect_timeout = -1;
    struct _HookIniter{
        _HookIniter(){
            hook_init();
            s_connect_timeout = g_tcp_connect_timeout->getValue();
            g_tcp_connect_timeout->addListener([](const int & old_value, const int& new_value){
                SERVER_LOG_INFO(g_logger) << "tcp connect timeout changed from " << old_value << " to " << new_value;
                s_connect_timeout = new_value;
            });
        }   
    };

    static _HookIniter s_hook_initer;

    bool is_hook_enable(){
        return t_hook_enable;
    }
    void set_hook_enable(bool flag){
        t_hook_enable = flag;
    } 
}

struct timer_info
{
    int cancelled = 0;
};

template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,uint32_t event, int timeout_so, Args&&... args){
    if(!server::t_hook_enable){
        return fun(fd, std::forward<Args>(args)...);
    }

    SERVER_LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ">";
    server::FdCtx::ptr ctx = server::FdMgr::getInstance()->get(fd);
    //不是socket句柄(文件句柄不存在)
    if(!ctx){
        return fun(fd, std::forward<Args>(args)...);
    }
    //文件句柄已经关闭了
    if(ctx->isClose()){
        errno = EBADF;
        return -1;
    }
    //不是socket句柄或者被用户设置为非阻塞
    if(!ctx->isSocket() || ctx->getUserNonblock()){
        return fun(fd, std::forward<Args>(args)...);
    }

    //是socket，开始执行操作
    //获取fd的超时,区分读写io
    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info>tinfo(new timer_info);

    retry:
     SERVER_LOG_ERROR(g_logger) << "start  " << hook_fun_name;
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    SERVER_LOG_ERROR(g_logger) << "n = " << n << " " << hook_fun_name;
    //没能执行成功，无限重试
    while(-1 == n && errno == EINTR){
        n = fun(fd, std::forward<Args>(args)...);
    }
    //读了很多次后没等到对应的事件发生，需要基于epoll做异步操作
    if(-1 == n &&  errno == EAGAIN){
        server::IOManager* iom = server::IOManager::GetThis();
        server::Timer::ptr timer;
        std::weak_ptr<timer_info>winfo(tinfo);
        //存在超时期限
        if(to != (uint64_t)-1){
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event](){
                auto t = winfo.lock();
                //条件不满足或者事件已经取消
                if(!t || t->cancelled){
                    return ;
                }
                //设置为超时事件
                t->cancelled = ETIMEDOUT;
                //取消事件
                iom->cancelEvent(fd, (server::IOManager::Event)(event));
            }, winfo);
        }
        //添加事件
        //SERVER_LOG_ERROR(g_logger) << hook_fun_name << fd << " dd event " <<  event;
        int rt = iom ->addEvent(fd, (server::IOManager::Event)(event));
        //SERVER_LOG_ERROR(g_logger) << hook_fun_name << fd << "add event over" <<  event;
        //添加失败
        if(rt){
            SERVER_LOG_ERROR(g_logger) << hook_fun_name << "addEvent(" << fd << "," << event << ") error";
            //定时器还在的话就取消
            if(timer){
                timer->cancel();
            }
            return -1;
        } else{
            //SERVER_LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ", start yield>";
            server::Fiber::YieldToHold();
            //SERVER_LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ", yield back>";
            if(timer){
                timer->cancel();
            }
            if(tinfo->cancelled){
                errno = tinfo->cancelled;
                return -1;
            }
            //如果成功添加的话，就返回异步执行
            goto retry;
        }
    }
    return n;
}
extern "C"{
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds){
    if(!server::t_hook_enable){
        return sleep_f(seconds);
    }
    server::Fiber::ptr fiber = server::Fiber::GetThis();  //获取当前线程对应的主协程
    server::IOManager* iom = server::IOManager::GetThis();  //获取当前线程对应的调度器；
    iom->addTimer(seconds * 1000, std::bind((void(server::Scheduler::*)
    (server::Fiber::ptr, int thread))&server::IOManager::schedule, iom, fiber, -1
    ));
    server::Fiber::YieldToHold();
    return 0;
}
int  usleep(useconds_t usec){
    if(!server::t_hook_enable){
        return usleep_f(usec);
    }
    server::Fiber::ptr fiber = server::Fiber::GetThis();  //获取当前线程对应的主协程
    server::IOManager* iom = server::IOManager::GetThis();  //获取当前线程对应的调度器；
    //原来： [iom, fiber](){iom->schedule(fiber)  }
    iom->addTimer(usec / 1000, std::bind((void(server::Scheduler::*)
    (server::Fiber::ptr, int thread))&server::IOManager::schedule, iom, fiber, -1
    ));

    server::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem){
    if(!server::t_hook_enable){
        return nanosleep_f(req, rem);
    }
    int time_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
    server::Fiber::ptr fiber = server::Fiber::GetThis();  //获取当前线程对应的主协程
    server::IOManager* iom = server::IOManager::GetThis();  //获取当前线程对应的调度器；
    iom->addTimer(time_ms, std::bind((void(server::Scheduler::*)
    (server::Fiber::ptr, int thread))&server::IOManager::schedule, iom, fiber, -1
    )); 

    server::Fiber::YieldToHold();
    return 0;
}


//socket函数
int socket(int domain, int type, int protocol){
    if(!server::t_hook_enable){
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if(fd == -1){
        return fd;
    }
    server::FdMgr::getInstance()->get(fd, true);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr,socklen_t addrlen, uint64_t timeout_ms){
    if(!server::t_hook_enable){
        return connect_f(fd, addr, addrlen);
    }
    int n = connect_f(fd, addr, addrlen);
    if(0 == n){
        return n;
    }else if(-1 != n || errno != EINPROGRESS){
        return n;
    }
    server::IOManager* iom = server::IOManager::GetThis();
    server::Timer::ptr timer;
    std::shared_ptr<timer_info>tinfo(new timer_info);
    std::weak_ptr<timer_info>winfo(tinfo);

    if((uint64_t)-1 != timeout_ms){
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom](){
            auto t = winfo.lock();
            if(!t){
                return;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(fd, server::IOManager::WRITE);
        },winfo);
    }
    int rt = iom->addEvent(fd, server::IOManager::WRITE);
    if(rt == 0){
        server::Fiber::YieldToHold();
        if(timer){
            timer->cancel();
        }
        if(tinfo->cancelled){
            errno = tinfo->cancelled;
            return -1;
        }
    }else{
        if(timer){
            timer->cancel();
        }
        SERVER_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }
    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)){
        return -1;
    }
    if(!error){
        return 0;
    }else{
        errno = error;
        return -1;
    }
}
//connect函数
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    return connect_with_timeout(sockfd, addr, addrlen, server::s_connect_timeout);
}

//accept函数
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
    int fd = do_io(sockfd, accept_f, "accept", server::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0){
        server::FdMgr::getInstance()->get(fd, true);
    }
    return fd;
}


//read

//read函数
ssize_t read(int fd, void *buf, size_t count){
    return do_io(fd, read_f, "read", server::IOManager::READ, SO_RCVTIMEO, buf, count);
}

//readv函数
ssize_t readv(int fd, const struct iovec *iov, int iovcnt){
    return do_io(fd, readv_f, "readv", server::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

//recv函数
ssize_t recv(int sockfd, void *buf, size_t len, int flags){
    SERVER_LOG_INFO(g_logger) << "receive from bufferss";
    return do_io(sockfd, recv_f, "recv", server::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

//recvfrom函数
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen){
    return do_io(sockfd, recvfrom_f, "recvfrom", server::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

//recvmsg函数
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags){
    return do_io(sockfd, recvmsg_f, "recvmsg", server::IOManager::READ, SO_RCVTIMEO, msg, flags);
}


//wtite

//write函数
ssize_t write(int fd, const void *buf, size_t count){
    return do_io(fd, write_f, "write", server::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

//writev函数
ssize_t writev(int fd, const struct iovec *iov, int iovcnt){
    return do_io(fd, writev_f, "writev", server::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

//send函数
ssize_t send(int sockfd, const void *buf, size_t len, int flags){
    return do_io(sockfd, send_f, "send", server::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags);
}

//sendto函数
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen){
    return do_io(sockfd, sendto_f, "sendto", server::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
}

//sendmsg函数
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags){
    return do_io(sockfd, sendmsg_f, "sendmsg", server::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

//control

//close函数
int close(int fd){
    if(!server::t_hook_enable){
        return close_f(fd);
    }
    server::FdCtx::ptr ctx = server::FdMgr::getInstance()->get(fd);
    if(ctx){
        auto iom = server::IOManager::GetThis();
        if(iom){
            iom->cancelAll(fd);
        }
        server::FdMgr::getInstance()->del(fd);
    }
    return close_f(fd);
}

//fctnl函数  //获取用户的非阻塞信息
int fcntl(int fd, int cmd, ... /* arg */ ){
    va_list va;
    va_start(va, cmd);
    switch (cmd)
    {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                server::FdCtx::ptr ctx = server::FdMgr::getInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()){
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if(ctx->getSysNonblock()){
                    arg |= O_NONBLOCK;
                }else{
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
        
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                server::FdCtx::ptr ctx = server::FdMgr::getInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()){
                    return arg;
                }
                if(ctx->getUserNonblock()){
                    return arg | O_NONBLOCK;
                }
                else{
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
        case F_SETPIPE_SZ:
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg);

            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ:
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_ex  arg = va_arg(va, struct f_owner_ex);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;

        default:
            return fcntl_f(fd, cmd);
    }
}

//ioctl函数
int ioctl(int fd, unsigned long request, ...){
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(FIONBIO == request){
        bool user_nonblock = !!*(int *) arg;
        server::FdCtx::ptr ctx = server::FdMgr::getInstance() ->get(fd);
        if(!ctx || ctx->isClose() || !ctx->isSocket()){
            return ioctl_f(fd, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(fd, request, arg);
}

//socketopt

//getsockopt函数
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen){
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

//setsockopt函数
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen){
    if(!server::t_hook_enable){
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if(SOL_SOCKET == level){
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO){
            server::FdCtx::ptr ctx = server::FdMgr::getInstance()->get(sockfd);
            if(ctx){
                const timeval* tv = (const timeval*)optval;
                ctx->setTimeout(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}

/*
extern sleep_fun sleep_f;
extern usleep_fun usleep_f;
extern nanosleep_fun nanosleep_f;
extern socket_fun socket_f;
extern connect_fun connect_f;
extern accept_fun accept_f;
extern read_fun read_f;
extern readv_fun readv_f;
extern recv_fun recv_f;
extern recvfrom_fun recvfrom_f;
extern recvmsg_fun recvmsg_f;
extern write_fun write_f;
extern writev_fun writev_f;
extern send_fun send_f;
extern sendto_fun sendto_f;
extern sendmsg_fun sendmsg_f;
extern close_fun close_f;
extern fcntl_fun fcntl_f;
extern ioctl_fun ioctl_f;
extern getsockopt_fun getsockopt_f;
extern setsockopt_fun setsockopt_f;
*/