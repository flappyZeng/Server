//协程实现
#pragma once

#include<ucontext.h>  //提供协程库的基本实现
#include<memory>
#include<functional>  //提供函数对象库（代替函数指针）
#include"thread.h"  //提供信号量及互斥量



namespace server{

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;
    enum State{
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };

private:
    Fiber();   //禁止外界调用默认构造函数

public:
    Fiber(std::function<void()>cb, size_t stack_size = 0, bool use_caller = false);
    Fiber(Fiber&& fiber);  //移动构造函数实现
    ~Fiber();

    void reset(std::function<void()>cb);
    State getState() const {return m_state;}
    void setState(Fiber::State state) {m_state = state;}
    uint64_t getFiberId() const {return m_fiberId;}
    void swapIn();  //切换当前协程开始执行
    void swapOut();  //当前协程切换到后台，让出执行权力

    void call();  //非主协程和其他协程相互切换的方式
    void back();
public:
    //设置当前协程
    static void SetThis(Fiber* fiber);
    //返回当前协程
    static Fiber::ptr GetThis();
    //协程切换到后台，并且设置为READY;
    static void YieldToReady();
    //协程切换到后台，并且设置为Hold;
    static void YieldToHold();
    //返回当前系统的总协程数
    static uint64_t TotalFibers();
    //返回当前的协程ID
    static uint64_t GetFiberId();
    static void MainFunc();
    static void CallerMainFunc();

private:
    uint64_t m_fiberId = 0;        //协程ID
    uint64_t m_stack_size = 0;     //协程栈大小
    State m_state = INIT;             //协程状态

    ucontext_t m_ucontext;  //协程实现基础对象
    void* m_stack = nullptr;  //协程栈基址

    std::function<void()> m_cb;  //协程的回调函数

};

}