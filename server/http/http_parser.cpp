#include"http_parser.h"
#include"log.h"
#include"config.h"
#include<string.h>


namespace server{
namespace http{

    static server::Logger::ptr g_logger = SERVER_LOG_NAME("system");
    //请求消息缓存大小
    static server::ConfigVar<uint64_t>::ptr g_http_request_buffer_size = 
                server::Config::lookup("http.request_buffer_size", uint64_t(4 * 1024), "http request buffer size");
    
    //消息体最大长度
    static server::ConfigVar<uint64_t>::ptr g_http_request_max_body_size = 
                server::Config::lookup("http.request_max_body_size", uint64_t(64 * 1024 * 1024), "http request max body size");
        
    //请求消息缓存大小
    static server::ConfigVar<uint64_t>::ptr g_http_response_buffer_size = 
                server::Config::lookup("http.response_buffer_size", uint64_t(4 * 1024), "http response buffer size");
    
    //消息体最大长度
    static server::ConfigVar<uint64_t>::ptr g_http_response_max_body_size = 
                server::Config::lookup("http.response_max_body_size", uint64_t(64 * 1024 * 1024), "http response max body size");

    static uint64_t s_http_request_buffer_size = 0;
    static uint64_t s_http_request_max_body_size = 0;

    static uint64_t s_http_response_buffer_size = 0;
    static uint64_t s_http_response_max_body_size = 0;

    struct _RequestSizeIniter{
        _RequestSizeIniter(){
            s_http_request_buffer_size = g_http_request_buffer_size->getValue();
            s_http_request_max_body_size = g_http_request_max_body_size->getValue();
            s_http_response_buffer_size = g_http_response_buffer_size->getValue();
            s_http_response_max_body_size = g_http_response_max_body_size->getValue();

            g_http_request_buffer_size->addListener([](const uint64_t& old_value, const uint64_t& new_value){
                s_http_request_buffer_size = new_value;
            });
            g_http_request_max_body_size->addListener([](const uint64_t& old_value, const uint64_t& new_value){
                s_http_request_max_body_size = new_value;
            });
            g_http_response_buffer_size->addListener([](const uint64_t& old_value, const uint64_t& new_value){
                s_http_response_buffer_size = new_value;
            });
            g_http_response_max_body_size->addListener([](const uint64_t& old_value, const uint64_t& new_value){
                s_http_response_max_body_size = new_value;
            });
        }
    };

    static _RequestSizeIniter _init;

    //request相关的回调函数
    void on_request_method(void *data, const char *at, size_t length){
        HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
        HttpMethod m = charsToHttpMethod(at);

        //方法出错
        if(m == HttpMethod::INVALID_METHOD){
            SERVER_LOG_WARN(g_logger) << "invalid http request method: " << std::string(at, length);
            parser->setError(1000);
            return;
        }
        parser->getDate()->setMethod(m);
    }
    void on_request_uri(void *data, const char *at, size_t length){

    }
    void on_request_fragment(void *data, const char *at, size_t length){
        HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
        parser->getDate()->setFragment(std::string(at, length));
    }

    void on_request_path(void *data, const char *at, size_t length){
        HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
        parser->getDate()->setPath(std::string(at, length));
    }

    void on_request_query(void *data, const char *at, size_t length){
        HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
        parser->getDate()->setQuery(std::string(at, length));
    }

    void on_request_version(void *data, const char *at, size_t length){   
        HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
        uint8_t v = 0;
        if(strncmp(at, "HTTP/1.1", length) == 0){
            v = 0x11;
        }else if(strncmp(at, "HTTP/1.0", length) == 0){
            v = 0x10;
        }else{
            SERVER_LOG_WARN(g_logger) << "invalid http request version: " << std::string(at, length);
            parser->setError(1001);
            return;
        }
        parser->getDate()->setVersion(v);

    }

    void on_request_header_done(void *data, const char *at, size_t length){

    }

    void on_request_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen){
        HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
        if(flen == 0){
            SERVER_LOG_WARN(g_logger) << "invalid http request field length = 0";
            //parser->setError(1002);
            return;
        }
        parser->getDate()->SetHeader(std::string(field, flen), std::string(value, vlen));
    }

    //HttpRequestParser
    HttpRequestParser::HttpRequestParser()
        :m_error(0)
        ,m_request(new HttpRequest()){
        http_parser_init(&m_parser);
        //设置相关的回调函数
        m_parser.request_method = on_request_method;
        m_parser.request_uri = on_request_uri;
        m_parser.fragment = on_request_fragment;
        m_parser.request_path = on_request_path;
        m_parser.query_string = on_request_query;
        m_parser.http_version = on_request_version;
        m_parser.header_done = on_request_header_done;
        m_parser.http_field = on_request_http_field;
        m_parser.data = this;
    }


    int HttpRequestParser::isFinished() {
        return http_parser_is_finished(&m_parser);
    }
    int HttpRequestParser::hasError() {
        return m_error || http_parser_has_error(&m_parser);
    }
    size_t HttpRequestParser::execute(char* data, size_t len){
        size_t rt = http_parser_execute(&m_parser, data, len, 0);  //返回值表示解析的内容的长度
        memmove(data, data + rt, len - rt);  //将内存前移,覆盖重写
        return rt;
    }

    uint64_t HttpRequestParser::getContentLength(){

        return m_request->getHeaderAs<uint64_t>("content-length", 0);
    }
    
    uint64_t HttpRequestParser::GetHttpRequsetBufferSize(){
        return s_http_request_buffer_size;
    }

    uint64_t HttpRequestParser::GetHttpRequestMaxBodySize(){
        return s_http_request_max_body_size;
    }
    //response相关的回调函数
    void on_response_reason(void *data, const char *at, size_t length){
        HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
        parser->getDate()->setReason(std::string(at, length));
    }
    void on_response_status(void *data, const char *at, size_t length){
        HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
        HttpStatues status = (HttpStatues)(atoi(at));
        parser->getDate()->setStatus(status);
        
    }
    void on_response_chunk(void *data, const char *at, size_t length){

    }
    void on_response_version(void *data, const char *at, size_t length){
        HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
        uint8_t v = 0;
        if(strncmp(at, "HTTP/1.1", length) == 0){
            v = 0x11;
        }else if(strncmp(at, "HTTP/1.0", length) == 0){
            v = 0x10;
        }else{
            SERVER_LOG_WARN(g_logger) << "invalid http response version: " << std::string(at, length);
            parser->setError(1001);
            return;
        }
        parser->getDate()->setVersion(v);
    }
    void on_response_header_done(void *data, const char *at, size_t length){

    }
    void on_response_last_chunk(void *data, const char *at, size_t length){

    }
    void on_response_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen){
        HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
        if(flen == 0){
            SERVER_LOG_WARN(g_logger) << "invalid http response field length = 0";
            //parser->setError(1002);
            return;
        }
        parser->getDate()->setHeader(std::string(field, flen), std::string(value, vlen));
    }

    //HttpResponseParser
    HttpResponseParser::HttpResponseParser()
        :m_error(0)
        ,m_response(new HttpResponse()){
        httpclient_parser_init(&m_parser);
        m_parser.reason_phrase = on_response_reason;
        m_parser.status_code = on_response_status;
        m_parser.chunk_size = on_response_chunk;
        m_parser.http_version = on_response_version;
        m_parser.header_done = on_response_header_done;
        m_parser.last_chunk = on_response_last_chunk;
        m_parser.http_field = on_response_http_field;
        m_parser.data = this;
    }

    int HttpResponseParser::isFinished() {
        return httpclient_parser_is_finished(&m_parser);
    }
    int HttpResponseParser::hasError() {
        return m_error || httpclient_parser_has_error(&m_parser);
    }
    size_t HttpResponseParser::execute(char* data, size_t len, bool chunked){
        if(chunked){
            httpclient_parser_init(&m_parser);
        }
        size_t rt = httpclient_parser_execute(&m_parser, data, len, 0);  //返回值表示解析的内容的长度
        memmove(data, data + rt, len - rt);  //将内存前移,覆盖重写
        //SERVER_LOG_INFO(g_logger) << "parser len = "  << rt;
        return rt;
    }

    uint64_t HttpResponseParser::getContentLength(){
        return m_response->getHeaderAs<uint64_t>("content-length", 0);

    }

    uint64_t HttpResponseParser::GetHttpResponseBufferSize(){
        return s_http_response_buffer_size;
    }

    uint64_t HttpResponseParser::GetHttpResponseMaxBodySize(){
        return s_http_response_max_body_size;
    }
}
}