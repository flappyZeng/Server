#include"config.h"
#include"log.h"
#include"tcp_server.h"




namespace server{
    static server::Logger::ptr g_logger = SERVER_LOG_NAME("system");

    server::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout = server::Config::lookup("tcp_server.read_timeout", (uint64_t)60 * 1000 * 2);

    TcpServer::TcpServer(IOManager* worker, IOManager* accept_worker)
        :m_worker(worker)
        ,m_accept_worker(accept_worker)
        ,m_recvTimeout(g_tcp_server_read_timeout->getValue())
        ,m_name("server/1.0.0")
        ,m_stop(true){

    }   

    TcpServer::~TcpServer() {
        //回收所有的socket句柄
        for(auto& sock : m_socks){
            sock->close();
        }
        m_socks.clear();
    }

    bool TcpServer::bind(Address::ptr addr){
        std::vector<Address::ptr> addrs, fails;
        addrs.push_back(addr);
        return bind(addrs, fails);
    }

    bool TcpServer::bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails){
        for(auto addr : addrs){
            Socket::ptr sock = Socket::CreateTCP(addr);
            //SERVER_LOG_INFO(g_logger) << *addr;
            if(!sock->bind(addr)){
                SERVER_LOG_ERROR(g_logger) << "bind fail errno = " << errno 
                            << " errstr = " << strerror(errno)
                            << " addr = [" << addr->toString() << "]" ;
                fails.push_back(addr);
                continue;

            }
            if(!sock->listen()){
                SERVER_LOG_ERROR(g_logger) << "listen fail errno = " << errno
                        << " errstr = "<< strerror(errno) 
                        << " addr = [" << addr->toString() << "]" ;
                fails.push_back(addr);
                continue;
            }
            m_socks.push_back(sock);
        }
        //SERVER_LOG_ERROR(g_logger) << "bind int tcpserver";
        if(!fails.empty()){
            m_socks.clear();
            return false;
        }
        for(auto& sock : m_socks){
            SERVER_LOG_INFO(g_logger) << "server bind success : " << *sock;
        }
        return true;

    }

    bool TcpServer::start(){
        if(!isStop()){
            return true;
        }
        m_stop = false;
        for(auto& sock : m_socks){
            //异步accpet
            m_accept_worker->schedule(std::bind(&TcpServer::startAccept, shared_from_this(), sock));
        }
        return true;
    }

    void TcpServer::stop(){
        SERVER_LOG_INFO(g_logger) << "stop ";
        m_stop = true;
        auto self =shared_from_this();
        m_accept_worker->schedule([this, self](){
            for(auto& sock : m_socks){
                sock->cancelAll();
                sock->close();
            }
        });
    }

    void TcpServer::handleClient(Socket::ptr client){ //当有客户连接时,相关的回调函数
        SERVER_LOG_INFO(g_logger) << "handleClient: " << *client;
    }

    void TcpServer::startAccept(Socket::ptr sock){
        while(!isStop()){
            //SERVER_LOG_ERROR(g_logger) << "startaccept";
            Socket::ptr client = sock->accept();
            if(client){
                client->setRecvTimeout(m_recvTimeout);
                m_worker->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client));
            }else{
                SERVER_LOG_ERROR(g_logger) << "accept errno = " << errno
                        << " errstr = " << strerror(errno);
            }
        }
    }
}