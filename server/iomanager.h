#pragma once

#include"scheduler.h"
#include"timer.h"


namespace server{
class IOManager  : public Scheduler, public TimerManager{
public:
    typedef std::shared_ptr<IOManager>ptr;
    typedef RWMutex  MutexType ;   //读写锁为默认的锁

    //支持的事件分类
    enum Event{
        NONE = 0x0,   
        READ = 0x1,     //这个应该和epoll相对应EPOLLIN = 1
        WRITE = 0X4     //这个应该和epoll相对应EPOLLOUT = 4
    };

private:
    //事件类型
    struct FdContext{
        typedef Mutex MutexType;   //锁类型，使用互斥锁
        struct EventContext{
            Scheduler* scheduler  = nullptr;    //事件执行指定的调度器
            Fiber::ptr fiber;                   //事件执行的指定协程
            std::function<void()> cb;           //事件的回调函数

        };
        //根据event类型返回对应的读/写事件
        EventContext& getEventContext(Event event);
        //重置内部事件
        void resetContext(EventContext& event_ctx);
        //触发给定的事件
        void triggerEvent(Event event);

        EventContext read;    //读事件
        EventContext write;   //写事件
        int fd = 0;           //文件描述符号；（事件关联的句柄）
        Event events = NONE;   //已注册的事件
        MutexType mutex;
    };

public:
    IOManager(size_t threads = 1, bool use_caller =  true, const std::string& name = " ");
    ~IOManager();

    //事件管理函数
    //返回值：1 success , -1 error
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);   //将事件删除
    bool cancelEvent(int fd, Event event);  //当前被取消的事件可能存在一定的触发条件，取消事件是将事件取消并强制触发其要求的触发条件

    bool cancelAll(int fd);

    static IOManager* GetThis();   //获取当前线程的IOManager；

    bool stopping(uint64_t& now_time);
protected:
    //Scheduler中的虚函数
    void tickle() override;
    bool stopping() override;
    void idle()  override;
    void contextResize(size_t size);

    //Timer里面的虚函数
    void onTimerInsertedAtFront() override;
private:
    int m_epfd;
    int m_tickleFds[2];

    std::atomic<size_t>m_pendingEventCount = {0};  //目前等待执行的事件数量
    std::vector<FdContext*> m_fdContexts;
    MutexType m_mutex;
};

}

