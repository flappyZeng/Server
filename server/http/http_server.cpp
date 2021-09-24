#include"http_server.h"
#include"../log.h"


namespace server{
namespace http{

static server::Logger::ptr g_logger = SERVER_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive , server::IOManager* worker , server::IOManager* accept_worker)
    :TcpServer(worker, accept_worker)
    ,m_keepalive(keepalive){  
        m_dispatch.reset(new ServletDispatch());
    }

void HttpServer::handleClient(Socket::ptr client){
    HttpSession::ptr session(new HttpSession(client));
    do{
        auto req = session->recvRequset();
        if(!req){
            SERVER_LOG_WARN(g_logger) << "recv http request fail, errno = " << errno << " errstr = " << strerror(errno)
                                      << " client : " << *client;
            break;
        }

        //SERVER_LOG_INFO(g_logger) << *req;
        HttpResponse::ptr rsb(new HttpResponse(req->getVersion(), req->isClose() || !m_keepalive));
        m_dispatch->handle(req, rsb, session);

        /*
        rsb->setBody("hello world");
        SERVER_LOG_INFO(g_logger) << *rsb;
        */
        session->sendResponse(rsb);
    }while(m_keepalive);
    session->close();
}
}
}