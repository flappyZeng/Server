#include"http.h"
#include<sstream>

namespace server{
namespace http{

    HttpMethod stringToHttpMethod(const std::string& m){
    #define XX(num, name, string) \
        if(strcmp(#string, m.c_str()) == 0){ \
            return HttpMethod::name; \
        } 
        HTTP_METHOD_MAP(XX); 
    #undef XX
        return HttpMethod::INVALID_METHOD;
    }
    HttpMethod charsToHttpMethod(const char* m){
    #define XX(num, name, string) \
        if(strncmp(#name, m, sizeof(#name) - 1) == 0){ \
            return HttpMethod::name; \
        } 
        HTTP_METHOD_MAP(XX); 
    #undef XX
        return HttpMethod::INVALID_METHOD;    
    }

    static const char* s_method_string[] = {
    #define XX(num, name, string) #string,
        HTTP_METHOD_MAP(XX)
    #undef XX
    };
    const char* HttpMehodToString(const HttpMethod& m){
        uint32_t idx = (uint32_t)m;
        if(idx >= (sizeof(s_method_string) / sizeof(s_method_string[0]))) {
            return "<unknown>";
        }
        return s_method_string[idx];
    }
    const char* HttpStatusToString(const HttpStatues& m){
        switch (m){
    #define XX(code, name, msg) \
            case HttpStatues::name: \
                return #msg;
            HTTP_STATUS_MAP(XX)
    #undef XX
            default:
                return "<Invalid>";
        }
    }

    bool CaseInsensitiveLess::operator()(const std::string& lhs, const std::string& rhs) const{
        return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
    }

    HttpRequest::HttpRequest(uint8_t version, bool close)
        :m_method(HttpMethod::GET)
        ,m_version(version)
        ,m_close(close)
        ,m_path("/"){
    }


    std::string HttpRequest::getHeader(const std::string& key, const std::string& def ) const{
        auto it = m_headers.find(key);
        return it == m_headers.end() ? def : it -> second;
    }
    std::string HttpRequest::getParam(const std::string& key, const std::string& def ) const{
        auto it = m_params.find(key);
        return it == m_params.end() ? def : it -> second;
    }
    std::string HttpRequest::getCookie(const std::string& key, const std::string& def ) const{
        auto it = m_cookies.find(key);
        return it == m_cookies.end() ? def : it -> second;
    }

    void HttpRequest::SetHeader(const std::string& key, const std::string& val){
        m_headers[key] = val;
    }
    void HttpRequest::SetParam(const std::string& key, const std::string& val){
        m_params[key] = val;
    }
    void HttpRequest::SetCookie(const std::string& key, const std::string& val){
        m_cookies[key] = val;
    }

    void HttpRequest::delHeader(const std::string& key){
        m_headers.erase(key);
    }
    void HttpRequest::delParam(const std::string& key){
        m_params.erase(key);
    }
    void HttpRequest::delCookie(const std::string& key){
        m_cookies.erase(key);
    }

    //判断对面的key是否存在，存在的话将key指针赋值给对应的vSal
    bool HttpRequest::hasHeader(const std::string& key, std::string* val ){
        auto it = m_headers.find(key);
        if(it == m_headers.end()){
            return false;
        }
        if(val){
            *val = it -> second;
        }
        return true;
    }
    bool HttpRequest::hasParam(const std::string& key, std::string* val){
        auto it = m_params.find(key);
        if(it == m_params.end()){
            return false;
        }
        if(val){
            *val = it -> second;
        }
        return true;

    }
    bool HttpRequest::hasCookie(const std::string& key, std::string* val){
        auto it = m_cookies.find(key);
        if(it == m_cookies.end()){
            return false;
        }
        if(val){
            *val = it -> second;
        }
        return true;
    }

    std::ostream& HttpRequest::dump(std::ostream& os) const{
        // GET /uri HTTP/1.1
        // host : www.xxx.com
        //
        //
        os << HttpMehodToString(m_method) << " "
           << m_path
           << ((m_query.empty()) ? "": "?")
           << m_query
           << ((m_fragment.empty()) ? "" : "#")
           << m_fragment
           << " HTTP/"
           << ((uint32_t)(m_version >> 4))
           <<"."
           << ((uint32_t)(m_version & 0x0f))
           << "\r\n";
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
        for(auto head : m_headers){
            if(strcasecmp(head.first.c_str(), "connection") == 0){
                continue;
            }
            os << head.first << ": " << head.second << "\r\n";
        }
        if(!m_body.empty()){
            os << "content-length: " << m_body.size() << "\r\n\r\n"
               << m_body;
        }
        os << "\r\n";
        return os;
    }

    std::string HttpRequest::toString() const{
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    //HttpResponce
    HttpResponse::HttpResponse(uint8_t version, bool close )
        : m_status(HttpStatues::OK)
        , m_version(version)
        , m_close(close){
    }

    std::string HttpResponse::getHeader(const std::string& key, const std::string& def ) const{
        auto it = m_headers.find(key);
        if(it == m_headers.end()){
            return def;
        }
        return it -> second;
    }

    void HttpResponse::setHeader(const std::string& key, const std::string& val){
        m_headers[key] = val;
    }

    void HttpResponse::delHeader(const std::string& key){
        m_headers.erase(key);
    }

    std::ostream& HttpResponse::dump(std::ostream& os) const{
        // HTTP/1.1 200 OK
        // 

        os << "HTTP/"
           << ((uint32_t)(m_version >> 4))
           << "."
           << ((uint32_t)(m_version & 0x0f))
           << " "
           << (uint32_t)m_status
           << " "
           << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
           << "\r\n";
        
        for(auto & header : m_headers){
            if(strcasecmp(header.first.c_str(), "connection") == 0){
                continue;
            }
            os << header.first << ": " << header.second << "\r\n";
        }
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";

        if(!m_body.empty()){
            os << "content-length: " << m_body.size() << "\r\n\r\n"
                << m_body;
        }else{
            os << "\r\n";
        }

        return os;
    }

    std::string HttpResponse::toString() const{
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    std::ostream& operator<<(std::ostream& os, const HttpRequest& req){
        return req.dump(os);
    }
    std::ostream& operator<<(std::ostream& os, const HttpResponse& rsb){
        return rsb.dump(os);
    }

}
}