#include"./server.h"

server::Logger::ptr  g_logger = SERVER_LOG_NAME("system");

void run_in_fiber(){
    SERVER_LOG_INFO(g_logger) << "run in fiber begin";
    server::Fiber::YieldToHold();
    SERVER_LOG_INFO(g_logger) << "run in fiber end";
    server::Fiber::YieldToHold();
}
void test_fiber(){
    server::Fiber::GetThis();
    SERVER_LOG_INFO(g_logger) << "main begin";
    {
        server::Fiber::ptr fiber(new server::Fiber(run_in_fiber));
        fiber->swapIn();
        SERVER_LOG_INFO(g_logger) <<"mian after swapin";
        fiber->swapIn();
        SERVER_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    SERVER_LOG_INFO(g_logger) << "main after end2";
}
int main(int argc, char** argv){
    server::Thread::SetName("main");
    std::vector<server::Thread::ptr> threads;
    for(size_t i = 0; i < 3; ++i){
        threads.emplace_back(new server::Thread(test_fiber, "thread" + std::to_string(i)));
    }
    for(auto thread : threads){
        thread->join();
    }
    return 0; 
}