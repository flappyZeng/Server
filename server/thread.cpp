#include"thread.h"
#include"log.h"

namespace server{

    //Semaphore类实现
     //构造函数以及异构函数
    Semaphore::Semaphore(uint32_t sem){
        //sem_init初始化一个信号量，参数1是信号量指针，参数2约定该信号量是否进程间共享，0表示不共享， 参数3表示信号量的初始值
        //返回值： 返回0表示操作成功，其他值(-1)表示操作失败
        if(sem_init(&m_semaphore, 0, sem)){  
            throw std::logic_error("sem_init error");
        }
    }

    Semaphore::~Semaphore(){   
        //和sem_init匹配使用，销毁sem_init()初始化的信号量。 
        sem_destroy(&m_semaphore);
    }

    //PV操作
    void Semaphore::wait(){ 
        //P操作，消费信号量，如果信号量大于0的话，可以消费，否则阻塞当前线程
        //sem_wait尝试将信号减一，如果成功的话返回， 还有两个函数，sem_trywait() ：只是尝试，失败的话返回-1， sem_timewait() :在给定时间内等待，如果成功，返回0；
        if(sem_wait(&m_semaphore)){
            throw std::logic_error("sem_wait error");
        }
    }
    void Semaphore::notify(){
        //V操作，生产信号量， sem_post()尝试将信号量加1,成功的话返回0，否则返回-1， 
        if(sem_post(&m_semaphore)){
            throw std::logic_error("sem_post error");
        }
    }

    //静态方法
    //Thread静态方法

    //线程局部变量
    static thread_local Thread* t_thread = nullptr;
    static thread_local std::string t_thread_name = "UNKNOWN";

    static server::Logger::ptr system_log = SERVER_LOG_NAME("system");

    Thread* Thread::GetThis(){
        return t_thread;
    }
    const std::string& Thread::GetName(){
        return t_thread_name;
    }
    void Thread::SetName(const std::string& name){
        if(t_thread){
            t_thread->m_name = name;
        }
        t_thread_name = name;
    }

    void* Thread::run(void *arg){
        Thread* thread = (Thread*)arg;
        t_thread = thread;   //切换当前局部线程为目标线程
        SetName(thread->m_name);
        thread->m_id = server::GetThreadId();  //获取当前线程id；
        //pthtead_self() :获取当前线程的id
        pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());  //参数： 传入线程id , 线程命名（据说不能大于16个字符（有待考证）  用来同步系统线程名称

        //防止thread的智能指针引用添加
        std::function<void()>cb;
        cb.swap(thread->m_cb);

        //thread初始化开始运行， 解锁
        thread->m_semaphore.notify();   //保证线程已经run起来了
        cb();

        return 0;
    }

    //Thread类实现
    Thread::Thread(std::function<void()> cb, const std::string& name)
        :m_name(name)
        ,m_cb(cb){
        if(name.empty()){
            m_name = "UNKNOWN";
        }
        int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);  //pthread_create的返回值表示成功，返回0；表示出错，返回表示-1
        if(rt){
            SERVER_LOG_ERROR(system_log) << "pthread_create thread fail, rt = " << rt 
                << " name = " << name;
            throw std::logic_error("pthread_create error");
        }
        m_semaphore.wait();  //加锁
    }

    Thread::~Thread(){
        //调用pthread_detach分离线程
        if(m_thread){
            pthread_detach(m_thread);
        }
    }

    void Thread::join(){
        //使用pthread_join阻塞线程，等待系统回收
        if(m_thread){
            int rt = pthread_join(m_thread, nullptr);   //调用成功返回0. 
            if(rt){
                SERVER_LOG_ERROR(system_log) << "pthread_jion thread fail, rt = " << rt 
                << " name = " << m_name;
            throw std::logic_error("pthread_join error");
            }
            m_thread = 0;
        }
    }
}