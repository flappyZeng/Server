#include"../log.h"
#include"../tcp_server.h"
#include"../iomanager.h"
#include"../bytearray.h"


server::Logger::ptr g_logger = SERVER_LOG_ROOT();

class EchoServer : public server::TcpServer{
public:
    EchoServer(int type);  //type决定为文本打开为文本形式(type = 1), 二进制形式(type = 0);
    virtual void handleClient(server::Socket::ptr client) override;

private:
    int m_type;
};

EchoServer::EchoServer(int type)
    :m_type(type){
}

void EchoServer::handleClient(server::Socket::ptr client){
    SERVER_LOG_INFO(g_logger) << "handleClient" << *client;
    server::ByteArray::ptr ba(new server::ByteArray());
    while(true){
        ba->clear();
        std::vector<iovec>iovs;
        ba->getWriteBuffers(iovs, 1024);

        int rt = client->recv(&iovs[0], iovs.size());
        if(rt == 0){
            SERVER_LOG_INFO(g_logger) << " client close: " << *client;
            break;
        }else if(rt < 0){
            SERVER_LOG_ERROR(g_logger) << "client error rt = " << rt 
                << "errno = " << errno << " errstr = " << strerror(errno);
        }
        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0);
        if(m_type == 1) {//text
            std::cout <<  ba->toString();
        }else{
            std::cout <<  ba->toHexString();
        }
    }
}

int type  = 1;
void run(){
    EchoServer::ptr es(new EchoServer(type));
    auto addr = server::Address::LookupAny("0.0.0.0:8020");
    while(!es->bind(addr)){
        sleep(2);
    }
    es->start();
}
int main(int argc, char** argv){
    if(argc < 2){
        SERVER_LOG_ERROR(g_logger) << "used as[" << argv[0] << "-t or [" << argv[0] << " -b";
        return 0;
    }
    if(!strcmp(argv[1], "-b")){
        type = 0;
    }

    server::IOManager iom(2);
    iom.schedule(&run);
    return 0;
}