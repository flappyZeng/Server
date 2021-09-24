#include"../iomanager.h"
#include"../socket.h"
#include"../server.h"

#include<sstream>


static server::Logger::ptr g_logger = SERVER_LOG_ROOT();


void test_socket(){
    server::Address::ptr addr =  server::Address::LookupAnyIPAddress("www.baidu.com");
    if(addr){
        SERVER_LOG_INFO(g_logger) << "get address : " << addr->toString();
    }else{
        SERVER_LOG_ERROR(g_logger) << "get address failed ";
        return;
    }
    server::Socket::ptr sock = server::Socket::CreateTCP(addr);
    addr->setPort(80);
   
    if(!sock->connect(addr)){
        SERVER_LOG_ERROR(g_logger) << "connect(" << addr->toString() << ") failed";
    }else{
        SERVER_LOG_INFO(g_logger) << "connect(" << addr->toString() << ") connected";
    }
    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0){
        SERVER_LOG_ERROR(g_logger) << "send failed rt = " << rt;
        return;
    }
    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());
    if(rt <= 0){
        SERVER_LOG_ERROR(g_logger) << "recieve failed rt = " << rt;
        return;
    }
    buffs.resize(rt);
    SERVER_LOG_INFO(g_logger) << buffs;

}   

int main(int argc, char** argv){
    server::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}