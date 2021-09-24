#include"../iomanager.h"
#include"../server.h"
#include<sys/types.h>
#include<sys/socket.h>
#include <arpa/inet.h>  //提供sockaddr_in, 替代socket中的sockaddr
#include<fcntl.h>


server::Logger::ptr g_logger = SERVER_LOG_ROOT();

void test_fiber(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);  //建立socket， 第一个参数代表使用IP,第二个参数代表使用TCP
    
    fcntl(sock, F_SETFL, O_NONBLOCK);   //设置sock为异步io

    sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr));
    addr.sin_family = AF_INET;  //IP协议
    addr.sin_port = htons(80);  //使用80端口
    inet_pton(AF_INET, "39.156.69.79", &addr.sin_addr.s_addr);  //连接百度的ip
    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))){
    }else if(errno == EINPROGRESS){
        server::IOManager::GetThis()->addEvent(sock, server::IOManager::READ, [](){
        SERVER_LOG_INFO(g_logger) << "connect";
        });
    }else{
        SERVER_LOG_INFO(g_logger) << "errno " << strerror(errno);
    }
}

void test1(){
    server::IOManager iom(2, false);
    iom.schedule(&test_fiber);

}
server::Timer::ptr s_timer;
void test_timer(){
    server::IOManager iom(2);
    s_timer =  iom.addTimer(500, [](){
        static int i = 0;
        SERVER_LOG_INFO(g_logger) << "test timer  i=" <<i;
        if(++i == 3){
            s_timer->reset(1000, true);
        }
        if(i == 10){
            s_timer->cancel();
        }
    }, true);
}

int main(int argc, char** argv){
    test_timer();
    return 0;
}