#include <arpa/inet.h>  //提供sockaddr_in, 替代socket中的sockaddr
#include<sys/types.h>
#include <string.h>
#include"../hook.h"
#include"../iomanager.h"
#include"../log.h"


server::Logger::ptr g_logger = SERVER_LOG_ROOT();
void test_sleep(){
    server::IOManager iom;
    iom.schedule([](){
        sleep(2);
        SERVER_LOG_INFO(g_logger) << server::Fiber::GetThis()->GetFiberId();
        SERVER_LOG_INFO(g_logger) << "sleep 2";
    });
    iom.schedule([](){
        sleep(10);
        SERVER_LOG_INFO(g_logger) << server::Fiber::GetThis()->GetFiberId();
        SERVER_LOG_INFO(g_logger) << "sleep 3";
    });
    SERVER_LOG_INFO(g_logger) << "test_sleep";
}

void test_sock(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "220.181.38.148", &addr.sin_addr.s_addr);
    SERVER_LOG_INFO(g_logger) << "begin connect " ;
    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));

    SERVER_LOG_INFO(g_logger) << "connect rt = " << rt << " errno = " << errno;

    if(rt){
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    SERVER_LOG_INFO(g_logger) << "send rt = " << rt << " errno = " << errno;

    if(rt <= 0){
        return;
    }

    std::string buff;
    buff.resize(4096);
    rt = recv(sock, &buff[0], buff.size(), 0);
    SERVER_LOG_INFO(g_logger) << "recv rt = " << rt << " errno = " << errno;

    if(rt <= 0){
        return;
    }
    buff.resize(rt);
    SERVER_LOG_INFO(g_logger) << "recv message : " << buff;
 
}
int main(int argc, char** argv){
    //test_sock();
    server::IOManager iom;
    iom.schedule(test_sock);
    return 0;
}
