//线程/协程调度器
#pragma once

#include<memory>
#include<vector>
#include<list>
#include"fiber.h"
#include"thread.h"
#include<atomic>

namespace server{
    class Scheduler{
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        typedef Mutex MutexType;
        
        //usecaller的作用是，是否将调用调度器的线程也纳入调度范围内
        Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
        virtual ~Scheduler();

        static Scheduler* GetThis();   //获取调度器
        static Fiber* GetMainFiber();  //调度器的主协程

        //获取变量
        std::string getName() const {return m_name;}
        
        void start();
        void stop();

        //调度函数
        template<class FiberOrCb>
        void schedule(FiberOrCb fc, int  thread = -1){
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                need_tickle = scheduleNoLock(fc, thread);
            }
            if(need_tickle){
                tickle();
            }
        }

        //批量调度函数
        template<class InputIterator>
        void schedule(InputIterator begin, InputIterator end){
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while(begin != end){
                    //原来使用的是need_tickle = scheduleNoLock(&*begin) || need_tickle; 
                    need_tickle = scheduleNoLock(&*begin) || need_tickle;  
                    ++begin;
                }
            }
            if(need_tickle){
                tickle();
            }
        }

    protected:
        virtual void tickle();    //用来提醒调度器可以进行调度(fiber or cb)执行
        void run();
        virtual bool stopping();  //不知名子类的stopping函数
        void SetThis();        //设置当前调度器为主调度器(运行中的调度器)
        virtual void idle();    //用于给空闲线程空执行

        bool haveIdelThreads() {return m_idleThreadCount > 0;}
    private:
        template<class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc, int thread = -1){
            bool need_tickle = m_fibers.empty();  ///判断消息队列(m_fiber)是否为空
            FiberAndThread ft(fc, thread);
            if(ft.fiber || ft.cb){
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }
    private:
        //符合类型，支持Fiber和function
        struct FiberAndThread{
            Fiber::ptr fiber;
            std::function<void()> cb;
            int thread;

            FiberAndThread(Fiber::ptr f, int thr)
                :fiber(f), thread(thr){
            }

            FiberAndThread(Fiber::ptr* f, int thr)
                : thread(thr){
                //,fiber(std::move(*f)){
                fiber.swap(*f);   //swap的作用可以减少传入智能指针的引用计数,可以使用std::move代替
                f = nullptr;
            }
            FiberAndThread(std::function<void()> f, int thr)
                :cb(f), thread(thr){

            }

            FiberAndThread(std::function<void()> *f, int thr)
                :thread(thr){ //, cb(std::move(*f)){
                cb.swap(*f);
                f = nullptr;
            }

            FiberAndThread()
                :thread(-1){
            };

            void reset(){
                
                fiber = nullptr;
                cb = nullptr;
                thread = -1;
            }
        };
    private:
        MutexType m_mutex;
        std::vector<Thread::ptr> m_threads;  //线程池
        std::list<FiberAndThread> m_fibers;   //协程对象/function对象
        Fiber::ptr m_rootFiber;              //主协程
        std::string m_name;                    //名字
    
    protected:
        //给继承类用的其他参数
        std::vector<int>m_threadIds;                     //记录调度器内所有线程的ID
        size_t m_threadCount = 0;                        //线程数量
        std::atomic<size_t> m_activeThreadCount = {0};   //活跃线程数量   //原子类型，可以保证线程安全
        std::atomic<size_t> m_idleThreadCount = {0};     //空闲线程数量 
        bool m_stopping = true;                          //执行状态
        bool m_autoStop = false;                         //是否主动停止
        int m_rootThreadId = 0;                          //主线程ID
    };
}