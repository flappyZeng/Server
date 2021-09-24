#include"../http/http_server.h"
#include"../log.h"

static server::Logger::ptr g_logger = SERVER_LOG_ROOT();

void run(){
    server::http::HttpServer::ptr hserver(new server::http::HttpServer());
    server::Address::ptr addr = server::Address::LookupAnyIPAddress("0.0.0.0:8020");
    SERVER_LOG_INFO(g_logger) << *addr;
    while(!hserver->bind(addr)){
        sleep(2);
    }
    auto sd = hserver->getServletDispatch();
    sd->addServlet("/server/xx", [](server::http::HttpRequest::ptr req, server::http::HttpResponse::ptr rsb, server::http::HttpSession::ptr sess) {
        rsb->setBody(req->toString());
        SERVER_LOG_INFO(g_logger) << *rsb;
        return 0;
    });

    sd->addGlobServlet("/server/*", [](server::http::HttpRequest::ptr req, server::http::HttpResponse::ptr rsb, server::http::HttpSession::ptr sess) {
        rsb->setBody("Glob: \r\n" + req->toString());
        SERVER_LOG_INFO(g_logger) << *rsb;
        return 0;
    });
    hserver->start();
}

int main(int argc, char** argv){
    server::IOManager iom(2);
    iom.schedule(&run);
    return 0;                                                                                                                                                                                                                                                                                                                 
}