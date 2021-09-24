#pragma once 

//实现一个定时器

#include"thread.h"
#include<set>
#include<vector>

namespace server{
class TimerManager;  //通过TimerManager创建计时器

class Timer : public std::enable_shared_from_this<Timer>{

friend class TimerManager;

public:
    typedef std::shared_ptr<Timer> ptr;  //智能指针
    bool cancel();
    bool refresh();
    //重置时间
    bool reset(uint64_t ms, bool from_now);

private:
    /*
    触发时间 ：ms
    回调函数 ：cb
    是否循环调用： recuring
    定时器管理器： manager
    */
    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager);   //设定为私有，只能被管理器调用
    Timer(uint64_t next);

private:
    bool m_recurring = false;     //循环计时器
    uint64_t m_ms = 0;            //触发时间
    uint64_t m_next = 0;          //精确的执行时间
    std::function<void()> m_cb;   //定时器需要执行的回调函数
    TimerManager* m_manager;      //定时器所属的定时器管理器

private:
    struct Comparator{
        bool operator()(const Timer::ptr lhs, const Timer::ptr rhs) const;
    };
};

class TimerManager{
friend class Timer;

public:
    typedef RWMutex MutexType;

    TimerManager();
    virtual ~TimerManager();

    //添加定时器
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recuring = false);
    //添加条件定时器，若是weak_ptr为nullptr,则不会调用这个计时器（计时器失效）
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void>weak_cond, bool recuring = false);


    void addTimer(Timer::ptr val, MutexType::WriteLock& lock);
    
    //获取下一个定时器的执行时间
    uint64_t getNextTimer();

    //返回并取出已经过了执行时间的定时器的回调函数
    void listExpiredCb(std::vector<std::function<void()> >& cbs);

    //判断是否有定时器
    bool hasTimer();
protected:
    virtual void onTimerInsertedAtFront() {};

private:
    //时钟同步，判断有没有人改了系统时间
    bool detectClockRollover(uint64_t now_ms);
private:
    MutexType m_mutex;
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    bool m_tickled = false;  //记录当前定时器是否需要提醒，也就是在同一个定时器下，多个被插入到了队列的前面，只提醒一次
    uint64_t m_previousTime = 0;  //先前时间轴

};

}