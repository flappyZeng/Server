#include"fiber.h"
#include"config.h"
#include<atomic>   //提供atomic模板，自带线程安全，使用时默认是原子操作
#include"macro.h"
#include"log.h"
#include<scheduler.h>

namespace server{

server::Logger::ptr g_logger = SERVER_LOG_NAME("system");

static std::atomic<uint64_t>s_fiber_id = {0};
static std::atomic<uint64_t>s_fiber_count = {0};

static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "stack_size allocated for fiber");  //默认协程栈大小为1M

class MallocStackAllocator{
public:
    static void*  Alloc(size_t size){
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size){
        free(vp);
    }
};

//设置默认的分配器（Malloc），当然也可以使用mmap
using StackAllocator = MallocStackAllocator;

//主协程不分配栈空间，也没有回调函数
// 主协程调度子协程，子协程只能执行或者将执行权力返回给主协程，子协程没有生产协程的权力
Fiber::Fiber(){
    m_state = EXEC;
    SetThis(this);

    //getcontext将当前执行协程的上下文赋值给m_ucontext,成功返回0，出错返回-1
    if(getcontext(&m_ucontext)){
        SERVER_ASSERT2(false, "getcontext error");
    }
    //SERVER_LOG_DEBUG(g_logger) << "Fiber: fiber"; 
    ++ s_fiber_count;
}

Fiber::Fiber(std::function<void()>cb, size_t stack_size, bool use_caller)
    :m_fiberId(++s_fiber_id)
    ,m_cb(cb){
        ++s_fiber_count;
        if(getcontext(&m_ucontext)){
            SERVER_ASSERT2(false, "getcontext error");
        }
        m_stack_size = stack_size ? stack_size : g_fiber_stack_size->getValue();
        m_stack = StackAllocator::Alloc(m_stack_size);  //分配内存
        m_ucontext.uc_stack.ss_sp = m_stack;
        m_ucontext.uc_stack.ss_size = m_stack_size;
        m_ucontext.uc_link = nullptr;
        if(!use_caller){
            makecontext(&m_ucontext, &MainFunc, 0); //设置当前协程的上下文，并设置回调函数
        }else{
            makecontext(&m_ucontext, &CallerMainFunc, 0);
        }
        //SERVER_LOG_DEBUG(g_logger) << "Fiber: fiber_id = " << m_fiberId; 
}

Fiber::Fiber(Fiber&& fiber)
    :m_fiberId(fiber.m_fiberId)
    ,m_stack(fiber.m_stack)
    ,m_stack_size(fiber.m_stack_size)
    ,m_state(fiber.m_state)
    ,m_ucontext(fiber.m_ucontext)
    ,m_cb(fiber.m_cb){
        fiber.m_stack = nullptr;
        fiber.m_cb = nullptr;
        SERVER_LOG_DEBUG(g_logger) << "Fiber: fiber" ; 
    }

Fiber::~Fiber(){
    --s_fiber_count;
    //SERVER_LOG_DEBUG(g_logger) << "~Fiber: fiber_id = " << m_fiberId; 
    if(m_stack){
        //SERVER_LOG_DEBUG(g_logger)  << m_fiberId << " fiber state:" << m_state; 
        SERVER_ASSERT(m_state == INIT || m_state == TERM || m_state == EXCEPT);
        StackAllocator::Dealloc(m_stack, m_stack_size);
    }else{
        //主协程
        SERVER_ASSERT(!m_cb);
        SERVER_ASSERT(m_state == EXEC);
        Fiber* cur = t_fiber;
        if(cur == this){  //主协程才有资格析构
            SetThis(nullptr);
        }
    }
    
}

//重置协程，将协程的运行调用函数修改为制定函数，INIT, TERM并且重置状态为INIT
void Fiber::reset(std::function<void()> cb){
    SERVER_ASSERT(m_stack);
    //SERVER_LOG_DEBUG(g_logger) << "m_state= " << m_state; 
    SERVER_ASSERT(m_state == INIT || m_state == TERM || m_state == EXCEPT);  //如果执行出错了，也可以继续？
    m_cb = cb;
    if(getcontext(&m_ucontext)){
        SERVER_ASSERT2(false, "getcontext error");
    }
    m_ucontext.uc_stack.ss_sp = m_stack;
    m_ucontext.uc_stack.ss_size = m_stack_size;
    m_ucontext.uc_link = nullptr;

    makecontext(&m_ucontext, &MainFunc, 0);

    m_state = INIT;
}

void Fiber::swapIn(){
    //SERVER_LOG_DEBUG(g_logger) << "Fiber: fiber_id = " << m_fiberId; 
    SetThis(this);
    //SERVER_LOG_DEBUG(g_logger) << "Fiber: fiber_id = " << m_fiberId; 
    SERVER_ASSERT(m_state != EXEC);
    m_state = EXEC;
    if(swapcontext(&Scheduler::GetMainFiber()->m_ucontext, &m_ucontext)){
        SERVER_ASSERT2(false, "swapcontext error");
    }
}

void Fiber::swapOut(){
    //SERVER_LOG_INFO(g_logger) << t_fiber->GetFiberId();
    SetThis(Scheduler::GetMainFiber());   //将调度器的主协程切换为当前的主协程
    //SERVER_LOG_INFO(g_logger) << t_fiber->GetFiberId();
    if(swapcontext(&m_ucontext, &Scheduler::GetMainFiber()->m_ucontext)){
        SERVER_ASSERT2(false, "swapcontext error");
    }
}

void Fiber::call(){
    SetThis(this);
    m_state = EXEC;
    if(swapcontext(&t_threadFiber->m_ucontext, &m_ucontext)){
        SERVER_ASSERT2(false, "swapcontext error");
    }
}

void Fiber::back(){
    SetThis(t_threadFiber.get());
        if(swapcontext(&m_ucontext, &t_threadFiber->m_ucontext)){
            SERVER_ASSERT2(false, "swapcontext error");
        }
}

void Fiber::SetThis(Fiber* fiber){
    t_fiber = fiber;
}

Fiber::ptr Fiber::GetThis(){
    if(t_fiber){
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber());
    SERVER_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber -> shared_from_this();
}

void Fiber::YieldToHold(){  //这个地方没必要真的把状态改成hold
    Fiber::ptr cur = GetThis();
    SERVER_ASSERT(cur->m_state == EXEC);
    cur->swapOut();
}

void Fiber::YieldToReady(){
    Fiber::ptr cur = GetThis();
    SERVER_ASSERT(cur->m_state == EXEC);
    cur -> m_state = READY;
    cur->swapOut();
}

uint64_t Fiber::TotalFibers(){
    return s_fiber_count;
}

uint64_t Fiber::GetFiberId(){
    //这里fiberID可能是空
    if(!t_fiber){
        GetThis();
    }
    return t_fiber->m_fiberId;
}

void Fiber::MainFunc(){
    //协程执行m_cb函数
    Fiber::ptr cur = GetThis();
    SERVER_ASSERT(cur);
    //SERVER_LOG_ERROR(g_logger) << "main Fiber run: " << cur->getState();
    try{
        //SERVER_LOG_ERROR(g_logger) << "run cb: " << cur->GetFiberId();
        cur->m_cb();
        //SERVER_LOG_ERROR(g_logger) << "end cb: " << cur->GetFiberId();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
        //SERVER_LOG_ERROR(g_logger) << "end cb out: " << cur->GetFiberId()  << " "<< cur->getState();
    }
    catch(std::exception& e){
        cur->m_state = EXCEPT;
        SERVER_LOG_INFO(g_logger) << "\n" << BackTrace();
        SERVER_LOG_ERROR(g_logger) << "Fiber Except: " << e.what();
    }
    catch(...){
         cur->m_state = EXCEPT;
        SERVER_LOG_ERROR(g_logger) << "Fiber Except: ";
    }
    //std::cout << cur.use_count() << std::endl;   这里输出的值是2。
    //上述执行过程中，因为cur = GetThis(),导致协程智能指针的引用计数+1，但是之后并没有释放操作，导致子协程无法回收，解决办法如下
    auto raw_ptr = cur.get();
    cur.reset();
    //main函数执行完了之后应该切换回主协程
    //SERVER_LOG_ERROR(g_logger) << "terminal: "  << raw_ptr->GetFiberId() << " " << raw_ptr->getState() ;
    raw_ptr->swapOut();

    SERVER_ASSERT2(false, "never reach");
}

void Fiber::CallerMainFunc(){
    //协程执行m_cb函数
    Fiber::ptr cur = GetThis();
    //SERVER_LOG_INFO(g_logger) << "caller main Fiber run: " << cur->getState();
    SERVER_ASSERT(cur);
    try{
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }
    catch(std::exception& e){
        cur->m_state = EXCEPT;
        SERVER_LOG_ERROR(g_logger) << "Fiber Except: " << e.what();
    }
    catch(...){
         cur->m_state = EXCEPT;
        SERVER_LOG_ERROR(g_logger) << "Fiber Except: ";
    }
    //std::cout << cur.use_count() << std::endl;   这里输出的值是2。
    //上述执行过程中，因为cur = GetThis(),导致协程智能指针的引用计数+1，但是之后并没有释放操作，导致子协程无法回收，解决办法如下
    auto raw_ptr = cur.get();
    cur.reset();
    //main函数执行完了之后应该切换回主协程
    //SERVER_LOG_ERROR(g_logger) << "terminal: "  << raw_ptr->GetFiberId() << " " << raw_ptr->getState() ;
    raw_ptr->back();
    //SERVER_LOG_ERROR(g_logger) << "terminal: "  << raw_ptr->GetFiberId() << " " << raw_ptr->getState() ;
    
    SERVER_ASSERT2(false, "never reach");
}
}