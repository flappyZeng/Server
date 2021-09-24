#include<assert.h>
#include"./server.h"

server::Logger::ptr g_logger = SERVER_LOG_NAME("system");
void test_assert(){
    SERVER_LOG_INFO(g_logger) << server::BackTrace(10, 0);
    //assert(0);  //assert报错只能输出当前路径日志，无法知悉上层调用test_assert的位置  -> test_utils: tests/test_utils.cpp:5: void test_assert(): Assertion `0' failed.
    SERVER_ASSERT(false);
}

int main(){
    test_assert();
    return 0;
}