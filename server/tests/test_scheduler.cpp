#include"../server.h"

server::Logger::ptr g_logger = SERVER_LOG_ROOT();

void test_fiber(){
    static int count = 5;
    SERVER_LOG_INFO(g_logger) << "test scheduler  " << server::GetThreadId();
    if(--count >= 0){
        server::Scheduler::GetThis()->schedule(&test_fiber, server::GetThreadId());
    }
}
void test_fibera(){
    SERVER_LOG_INFO(g_logger) << "test scheduler";
}
int main(int argc, char** argv){
    server::Thread::SetName("main");
    SERVER_LOG_INFO(g_logger) << "main start";
    server::Scheduler sc(3, false , "test");
    //sc.schedule(&test_fiber);
    sc.start();
    SERVER_LOG_INFO(g_logger) << "add start";
    sc.schedule(&test_fiber);
    SERVER_LOG_INFO(g_logger) << "add end";
    sc.stop();
    SERVER_LOG_INFO(g_logger) << "main end";
    return 0;
}