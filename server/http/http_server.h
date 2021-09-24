#pragma once 

#include"tcp_server.h"
#include"httpsession.h"
#include"servlet.h"

namespace server{
namespace http{

class HttpServer : public TcpServer{
public:
    typedef std::shared_ptr<HttpServer>ptr;
    HttpServer(bool keepalive = false
                , server::IOManager* worker  = server::IOManager::GetThis()
                , server::IOManager* accept_worker = server::IOManager::GetThis());

    ServletDispatch::ptr getServletDispatch() const {return m_dispatch;}
    void setDispatch(ServletDispatch::ptr v) {m_dispatch = v;}
    
protected:
    virtual void handleClient(Socket::ptr client) override;

private:
    bool m_keepalive;
    ServletDispatch::ptr m_dispatch;
};

}
}