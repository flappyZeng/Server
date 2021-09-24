#include"../http/http.h"
#include"../log.h"

void test_request(){
    server::http::HttpRequest::ptr req(new server::http::HttpRequest());
    req->SetHeader("host", "www.sylar.top");
    req->setBody("hello sylar");

    req->dump(std::cout) << std::endl;
}

void test_responce(){
    server::http::HttpResponse::ptr res(new server::http::HttpResponse());
    res->setHeader("X-X", "server");
    res->setBody("hello sylar");
    res->setStatus(server::http::HttpStatues(400));
    res->setClose(false);
    res->dump(std::cout) << std::endl;
}

int main(int argc, char** argv){
    test_request();
    test_responce();
    return 0;
}