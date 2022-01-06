#include<iostream>
#include"../http/http_connection.h"
#include"../log.h"
#include"../iomanager.h"


static server::Logger::ptr g_logger = SERVER_LOG_ROOT();

void run(){
    /*
    server::Address::ptr addr = server::Address::LookupAnyIPAddress("www.sylar.top:80");
    if(!addr){
        SERVER_LOG_ERROR(g_logger) << "get addr error";
        return;
    }
    server::Socket::ptr sock = server::Socket::CreateTCP(addr);
    bool rt = sock->connect(addr);
    if(!rt){
        SERVER_LOG_INFO(g_logger) << "connect " << *addr << " failed";
        return;
    }
    
    server::http::HttpConnection::ptr conn(new server::http::HttpConnection(sock));
    server::http::HttpRequest::ptr req(new server::http::HttpRequest());
    req->setPath("/blog/");
    req->SetHeader("host", "www.sylar.top");
    SERVER_LOG_INFO(g_logger) << "req: " << std::endl << *req;

    conn->sendRequest(req);

    SERVER_LOG_INFO(g_logger) << "send request over";
    auto rsb = conn->recvResponse();
    if(!rsb){
        SERVER_LOG_INFO(g_logger) << "recieve response failed";
        return;
    }
    SERVER_LOG_INFO(g_logger) << "rsb: " << std::endl << *rsb;
    */
    SERVER_LOG_INFO(g_logger) << "-----------------------------------------------------------------------------";

    auto rh = server::http::HttpConnection::DoGet("http://www.baidu.com", 3000);
    if(!rh){
        SERVER_LOG_INFO(g_logger) << "recieve response failed";
        return;
    }
    SERVER_LOG_INFO(g_logger) << "result = " << rh->result << "\n"
                               << "error = " << rh->error << "\n"
                               << "rsb = " << rh->response->toString();
    
}
int main(int argc, char** argv){
    server::IOManager iom(2);
    iom.schedule(run);
    return 0;
}