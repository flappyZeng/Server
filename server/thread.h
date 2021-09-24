#pragma once

//线程的做法
// pthread_xxx  -> c++11之前实现线程的方式，有读写锁
// std::thread  -> c++11提供，但是不能提供读写锁

#include<pthread.h>
//#include<thread>
#include<functional>
#include<memory>
#include<string>
#include<semaphore.h>
#include<iostream>
#include<atomic>

#include"noncopyable.h"

namespace server{

    //信号量
    class Semaphore :  Noncopyable{
    public:
        //构造函数以及析构函数
        Semaphore(uint32_t sem = 0);
        ~Semaphore();

        //PV操作
        void wait();
        void notify();
    private:
        //拷贝构造函数，赋值运算符置为delete，防止拷贝
        Semaphore(Semaphore&) = delete;
        Semaphore(Semaphore&&) = delete;
        Semaphore& operator=(Semaphore& ) = delete;
    private:
        sem_t m_semaphore;
    };

    //互斥量
    //为了防止程序员在编写代码的之后只加锁而忘记了解锁，因此将互斥量定义为一个模板类，在构造函数里面加锁，在析构函数里面解锁
    template<class T>
    struct ScopeLockImpl{
    public:
        ScopeLockImpl(T& mutex)
            :m_mutex(mutex){
            //这里不能直接调用lock函数，否则会导致功能失效；
            m_mutex.lock();
            m_isLocked = true;
        }
        ~ScopeLockImpl(){
            unLock();
        }
        void lock(){
            if(!m_isLocked){
                m_mutex.lock();
                m_isLocked = true;
            }
        }

        void unLock(){
            if(m_isLocked){
                m_mutex.unLock();
                m_isLocked = false;
            }
        }
    private:
        T& m_mutex;
        bool m_isLocked;
    };

    //读锁实现
    template<class T>
    struct ReadScopeLockImpl{
    public:
        ReadScopeLockImpl(T& mutex)
            :m_mutex(mutex){
            //这里不能直接调用lock函数，否则会导致功能失效；
            m_mutex.rdLock();
            m_isLocked = true;
        }
        ~ReadScopeLockImpl(){  
            unLock();
        }

        void lock(){
            //判断防止死锁
            if( !m_isLocked){
                m_mutex.rdLock();
                m_isLocked = true;
            }
        }
        void unLock(){
            if(m_isLocked){
                m_mutex.unLock();
                m_isLocked = false;
            }
        }
    private:
        T& m_mutex;
        bool m_isLocked;

    };

    //写锁实现
    template<class T>
    struct WriteScopeLockImpl{
    public:
        WriteScopeLockImpl(T& mutex)
            :m_mutex(mutex){
            //这里不能直接调用lock函数，否则会导致功能失效；
            m_mutex.wrLock();
            m_isLocked = true;
        }
        ~WriteScopeLockImpl(){
            unLock();
        }

        void lock(){
            if(!m_isLocked){
                m_mutex.wrLock();
                m_isLocked = true;
            }
        }
        void unLock(){
            if(m_isLocked){
                m_mutex.unLock();
                m_isLocked = false;
            }
        }
    private:
        T& m_mutex;   //记住，这里传的是引用
        bool m_isLocked;

    };


    //通用锁互斥量
    class Mutex : Noncopyable{
    public:
        typedef ScopeLockImpl<Mutex> Lock;
        Mutex(){
            pthread_mutex_init(&m_lock, nullptr);
        }
        ~Mutex(){
            pthread_mutex_destroy(&m_lock);
        }
        void lock(){
            pthread_mutex_lock(&m_lock);
        }
        void unLock(){
            pthread_mutex_unlock(&m_lock);
        }
    private:
        pthread_mutex_t m_lock;
    };

    //空锁，用来调试
    class NullMutex : Noncopyable{
    public:
        typedef ScopeLockImpl<NullMutex> Lock;
        NullMutex() {};
        ~NullMutex() {}

        void lock() {}
        void unLock() {}
    };

    //读写锁，方便服务器端读多写少的情形
    class RWMutex : Noncopyable{
    public:
        typedef ReadScopeLockImpl<RWMutex> ReadLock;
        typedef WriteScopeLockImpl<RWMutex> WriteLock;
        RWMutex(){
            //pthread_rwlock_inin :两个参数，参数1是被初始化的读写锁指针，后者是用来初始化参数1的读写错指针，缺省为nullpr则代表使用缺省的读写锁属性，成功则返回0；
            pthread_rwlock_init(&m_lock, nullptr);
        }
        ~RWMutex(){
            //销毁读写锁，和pthread_rwlock_init配套使用
            pthread_rwlock_destroy(&m_lock);
        }
        void rdLock(){
            //添加读锁
            pthread_rwlock_rdlock(&m_lock);
        }
        void wrLock(){
            //添加写锁
            pthread_rwlock_wrlock(&m_lock);
        }
        void unLock(){
            //解锁
            pthread_rwlock_unlock(&m_lock);
        }
    private:
        //一个读写锁类型量
        pthread_rwlock_t m_lock;
    };

    //空读写锁，方便调试
    class NullRWMutex : Noncopyable{
    public:
        typedef ReadScopeLockImpl<NullRWMutex> ReadLock;
        typedef WriteScopeLockImpl<NullRWMutex> WriteLock;

        NullRWMutex() {};
        ~NullRWMutex() {};

        void rdLock() {};
        void rwLock() {};
        void unLock() {};
    };

    //自旋锁(SpinLock)  --> 常规写入大概是普通锁Mutex的3倍左右 
    class SpinLock : Noncopyable{
    public:
        typedef ScopeLockImpl<SpinLock> Lock;
        SpinLock(){
            //两个参数PTHREAD_PROCESS_PRIVATE = 0 表示进程内部共享， PTHREAD_PROCESS_SHARED = 1表示任意进程共享(enum变量)
            pthread_spin_init(&m_lock, PTHREAD_PROCESS_PRIVATE);
        }
        ~SpinLock(){
            pthread_spin_destroy(&m_lock);
        }
        void lock(){
            pthread_spin_lock(&m_lock);
        }

        void unLock(){
            pthread_spin_unlock(&m_lock);
        }
    private:
        pthread_spinlock_t m_lock; 
    };

    class CASLock : Noncopyable{
    public:
        CASLock(){
            m_mutex.clear();
        }
        ~CASLock(){

        }

        void lock(){
            while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
        }

        void unLock(){
            std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
        }
    private:
        volatile std::atomic_flag m_mutex;
    };
    class Thread{
    public:
        typedef std::shared_ptr<Thread> ptr;
        Thread(std::function<void()> cb, const std::string& name);
        ~Thread();

        pthread_t getId() const {return m_id;}
        const std::string&  getName() const {return m_name;}

        void join();

        static Thread* GetThis();
        static const std::string& GetName();
        static void SetName(const std::string& name);
        static void* run(void* arg);
    private:
        //阻止线程发生拷贝操作
        Thread(const Thread&) = delete;
        Thread(const Thread&&) = delete;
        Thread& operator=(const Thread&) = delete;

    private:
        //tid_t id;
        pid_t m_id = -1;  //线程id 
        pthread_t m_thread = 0; 
        std::function<void()> m_cb;
        std::string m_name;
        Semaphore m_semaphore;  //信号量
    };
}