#include"./server.h"
#include<unistd.h>  //提供sleep函数

server::Logger::ptr g_logger = SERVER_LOG_ROOT();
server::RWMutex w_mutex;
//pthread_rwlock_t lock;
server::Mutex r_mutex;

int count = 0;

void fun1(){
    
    SERVER_LOG_INFO(g_logger) << "name: " << server::Thread::GetName()
                              <<" this->name: " << server::Thread::GetThis() ->getName()
                              << "  id: " << server::GetThreadId()
                              <<"  this.id: " << server::Thread::GetThis()->getId();
    //不加锁，可能会出现同步问题求出来的count可能不是50000，原因如下：
    /*
        for循环过程中，count的更新分为3步 ：读count  -> 寄存器加count + 1 --> 写回count = count + 1 这三步由于多线程的问题，任意一步都有可能被抢占，导致某一次判断过程中，还未来的及赋值就被其他线程抢占
        导致count被少加，无法同步。一个例子：
            线程1： 取出count = x     (被抢占)        count + 1       count = count + 1(count = x + 1)       (被抢占)
            线程2：                取出count = x       (被抢占)                                        count = count + 1(count = x + 1)
        上述两个线程，对count自增两次，但是在内存中都只是写入了count 为 x + 1，导致少加了 
    */
    for(int i = 0; i < 100000; ++i){
        server::RWMutex::WriteLock lock(w_mutex);
        //server::Mutex::Lock lock(r_mutex);
        //pthread_rwlock_wrlock(&lock);
        count ++;
        //pthread_rwlock_unlock(&lock);
        //clock.unLock();
    }
}

void fun2(){

}

int main(int argc, char** argv){
    std::cout << 0 << std::endl;
    SERVER_LOG_INFO(g_logger) << "thread begin";
    std::vector<server::Thread::ptr> threads;
    for(int i = 0; i < 10; ++i){
        server::Thread::ptr thread(new server::Thread(&fun1, "name_" + std::to_string(i)));
        threads.push_back(thread);
    }
    for(int i = 0; i < 10; ++i){
        //std::cout << threads[i]->getName() << std::endl;
        threads[i]->join();
    }
    SERVER_LOG_INFO(g_logger) << "thread end";
    SERVER_LOG_INFO(g_logger) << "count = " << count;
    return 0;
}