#pragma once

#include<memory>
#include<functional>

#include"address.h"
#include"iomanager.h"
#include"socket.h"

namespace server{

class TcpServer : public std::enable_shared_from_this<TcpServer> {
public:
    typedef std::shared_ptr<TcpServer>ptr;
    
    TcpServer(IOManager* worker = IOManager::GetThis(), IOManager* accept_worker = IOManager::GetThis());
    virtual ~TcpServer(); //可能有些协议需要在tcp基础上实现,如websocket;

    virtual bool bind(Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails);


    //启动accept
    virtual bool start();
    virtual void stop();

    uint64_t getReadTimeout() const {return m_recvTimeout;}
    std::string getName() const { return m_name;}

    void setName(const std::string& v) { m_name = v;}

    bool isStop() const {return m_stop;}

protected:
    virtual void handleClient(Socket::ptr client);  //当有客户连接时,相关的回调函数
    virtual void startAccept(Socket::ptr sock);
private:
    std::vector<Socket::ptr> m_socks;
    IOManager* m_worker;   //用于sock的调度
    IOManager* m_accept_worker;  //用于accept的调度
    uint64_t m_recvTimeout;  //读超时,防止恶意请求,防止无效句柄被占用,及时释放.
    std::string m_name;
    bool m_stop;
}; 
}