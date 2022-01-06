#pragma once 

// http server , accept   --> Session
// http client , connect  --> Connection

#include"http.h"
#include"../socketstream.h"
#include"../socket.h"
#include"../uri.h"

//客户端专供

namespace server{
namespace http{

struct HttpResult{
    typedef std::shared_ptr<HttpResult> ptr;

    enum class Error{
        Ok = 0,
        INVALID_URL = 1,
        INVALID_HOST = 2,
        CREATE_TCP_FAILED = 3,
        CONNECT_FAILED = 4,
        SEND_CLOSED_BY_PEER = 5,
        SEND_SOCKET_ERROR = 6,
        REQUEST_TIMEOUT = 7,
    };
    HttpResult(int _result, HttpResponse::ptr _response, const std::string& _error)
        :result(_result)
        ,response(_response)
        ,error(_error){
    }
    int result;
    HttpResponse::ptr response;
    std::string error;

};
class HttpConnection : public SocketStream {
public:
    typedef std::shared_ptr<HttpConnection>ptr;

    static HttpResult::ptr DoGet(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    static HttpResult::ptr DoGet(Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    static HttpResult::ptr DoPost(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    static HttpResult::ptr DoPost(Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpMethod method
                            , const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpMethod method
                            , Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpRequest::ptr req
                            , Uri::ptr uri
                            , uint64_t timeout_ms);
                        

    HttpConnection(Socket::ptr sock, bool owner = true);
    
    int sendRequest(HttpRequest::ptr req);
    HttpResponse::ptr recvResponse();

};

}
}