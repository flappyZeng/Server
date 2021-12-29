#pragma once

#include"http.h"
#include"http11_parser.h"
#include"httpclient_parser.h"


namespace server{
namespace http{

class HttpRequestParser{
public:
    typedef std::shared_ptr<HttpRequestParser>ptr;
    HttpRequestParser();

    int isFinished();
    int hasError() ;
    size_t execute(char* data, size_t len);

    HttpRequest::ptr getDate() const {return m_request;}
    void setError(int v) {m_error = v;}

    uint64_t getContentLength();

public:
    static uint64_t GetHttpRequsetBufferSize();
    static uint64_t GetHttpRequestMaxBodySize();
private:
    http_parser m_parser;
    HttpRequest::ptr m_request;
    //1000:Invalid Method
    //1001:Invalid Version
    //1002: Invalid Field Length 
    int m_error;
};

class HttpResponseParser{
public:
    typedef std::shared_ptr<HttpResponseParser>ptr;
    HttpResponseParser();

    int isFinished() ;
    int hasError() ;
    size_t execute(char* data, size_t len, bool chunked);

    HttpResponse::ptr getDate() const {return m_response;}
    void setError(int v) {m_error = v;}

    const httpclient_parser& getParser() const{
        return  m_parser;
    }
    
    uint64_t getContentLength();
public:
    static uint64_t GetHttpResponseBufferSize();
    static uint64_t GetHttpResponseMaxBodySize();

private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_response;
    //1000:Invalid Method
    //1001:Invalid Version
    //1002: Invalid Field Length 
    int m_error;
};

}
}