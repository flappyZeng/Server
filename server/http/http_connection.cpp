#include"http_connection.h"
#include"http_parser.h"
#include"../log.h"

namespace server{
namespace http{

    static server::Logger::ptr g_logger = SERVER_LOG_ROOT();

    HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
        :SocketStream(sock, owner){
    }

    HttpResponse::ptr HttpConnection::recvResponse(){
        HttpResponseParser::ptr parser(new HttpResponseParser());
        uint64_t buff_size = HttpResponseParser::GetHttpResponseBufferSize();
        //uint64_t buff_size = 150;  //buffer太小就会报错，不支持分段
        std::shared_ptr<char> buffer( new char[buff_size], [](char* ptr){
            delete[] ptr;
        });
        //SERVER_LOG_ERROR(g_logger) << "buffer size: " << buff_size;
        char* data = buffer.get();
        int offset = 0;
        //SERVER_LOG_ERROR(g_logger) << "start parse";
        do{
            int len = read(data + offset, buff_size - offset);
            if(len <= 0){
                SERVER_LOG_ERROR(g_logger) << "read error";
                return nullptr;
            }
            len += offset;
            size_t nparse = parser->execute(data, len, true);  //parser对date进行解析
            if(parser->hasError()){ //报文解析出错
                return nullptr;
            }
            offset  = len - nparse;
            if(offset == buff_size){  //缓存区满了
                return nullptr;
            }
            if(parser->isFinished()){
                break;
            }

        }while(true);

        auto& client_parser = parser->getParser();
        if(client_parser.chunked){
            std::string body;
            int len = offset;
            do{
                do{
                    int rt = read(data + len, buff_size - len);
                    if(rt <= 0){
                        return nullptr;
                    }
                    len += rt;
                    data[len] = '\0';
                    size_t n_parse =  parser->execute(data, len, true);
                    if(parser->hasError()){
                        SERVER_LOG_ERROR(g_logger) << "parser error";
                        return nullptr;
                    }
                    len -= n_parse;
                    if(len == (int)buff_size){   //这是解析不了，缓冲区满了,注意等号==和=
                        return nullptr;
                    }
                }while(!parser->isFinished());
                len -= 2;  //每一个content 后面有一个\r\n;
                //SERVER_LOG_ERROR(g_logger) << "content_len = " << client_parser.content_len;
                if(client_parser.content_len <= len){
                    body.append(data, client_parser.content_len);
                    memmove(data, data + client_parser.content_len, len - client_parser.content_len);
                    len -= client_parser.content_len;
                } else{
                    body.append(data, len);
                    int left = client_parser.content_len - len;
                    while(left > 0){
                        int rt = read(data, left > buff_size ? buff_size : left);
                        if(rt <= 0){
                            return nullptr;
                        }
                        body.append(data, rt);
                        left -= rt;
                    }
                    len  = 0;
                }
            }while(!client_parser.chunks_done);
            parser->getDate()->setBody(body);
        }
        else{
            uint64_t length = parser->getContentLength();
            if(length > 0){
                std::string body;
                body.resize(length);

                int len = 0;
                if(length >= offset){
                    memcpy(&body[0], data, offset);
                    len = offset;
                }else{
                    memcpy(&body[0], data, length);
                    len = length;
                }
                length -= offset;
                if(length > 0){
                    if(readFixSize(&body[len], length) <= 0){
                        SERVER_LOG_ERROR(g_logger) << "read error";
                        return nullptr;
                    }
                }
                parser->getDate()->setBody(body);
            }
        }
        return parser->getDate();
    }

    int HttpConnection::sendRequest(HttpRequest::ptr req){
        std::stringstream ss;
        ss << *req;
        std::string data = ss.str();
        return writeFixSize(data.c_str(), data.size());
    }

    HttpResult::ptr HttpConnection::DoGet(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body){                    
        Uri::ptr uri = Uri::Create(url);
        SERVER_LOG_INFO(g_logger) << uri->toString();
        if(!uri){
            return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL,  nullptr, "invalid url: " + url);
        }
        return DoGet(uri, timeout_ms, headers, body);
    }

    HttpResult::ptr HttpConnection::DoGet(Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body){
        return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
    }

    HttpResult::ptr HttpConnection::DoPost(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body){
        Uri::ptr uri = Uri::Create(url);
        if(!uri){
            return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL,  nullptr, "invalid url: " + url);
        }
        return DoPost(uri, timeout_ms, headers, body);
    }

    HttpResult::ptr HttpConnection::DoPost(Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body){
        return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
    }

    HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                            , const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body){
        Uri::ptr uri = Uri::Create(url);
        if(!uri){
            return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL,  nullptr, "invalid url: " + url);
        }
        return DoRequest(method, uri, timeout_ms, headers, body);                        
    }

    HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                            , Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers
                            , const std::string& body){
        HttpRequest::ptr req = std::make_shared<HttpRequest>();
        req->setPath(uri->getPath());
        req->setMethod(method);
        bool has_host = false;
        for(auto& i : headers){
            if(strcasecmp(i.first.c_str(), "connection") == 0){
                if(strcasecmp(i.second.c_str(), "keep-alive") == 0){
                    req->setClose(false);
                }
                continue;
            }
            if(!has_host && strcasecmp(i.first.c_str(), "host") == 0){
                has_host = !i.second.empty();
            }
            req->SetHeader(i.first, i.second);
        }
        if(!has_host){
            req->SetHeader("Host", uri->getHost());
        }
        req->setBody(body);
        return DoRequest(req, uri, timeout_ms);

    }

    HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req
                            , Uri::ptr uri
                            , uint64_t timeout_ms){
        Address::ptr addr = uri->createAddress();
        if(!addr){
            return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST,  nullptr, "invalid host: " + uri->getHost());
        }
        Socket::ptr sock = Socket::CreateTCP(addr);
        if(!sock){
            return std::make_shared<HttpResult>((int)HttpResult::Error::CREATE_TCP_FAILED,  nullptr, "create tcp failed: " + addr->toString());
        }
        if(!sock->connect(addr)){
            return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAILED,  nullptr, "connect failed: " + addr->toString());
        }
        sock->setRecvTimeout(timeout_ms);
        HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
        int rt = conn->sendRequest(req);
        if(rt == 0){
            return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSED_BY_PEER,  nullptr, "send request closed by peer : " + addr->toString());
        }
        if(rt < 0){
            return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR,  nullptr, 
                            "send socket error  errno = : " + std::to_string(errno) +  " errstr= " + std::string(strerror(errno)));
        }
        auto rsb = conn -> recvResponse();
        if(!rsb){
            return std::make_shared<HttpResult>((int)HttpResult::Error::REQUEST_TIMEOUT,  nullptr, 
                            "receive response timeout: " + addr->toString() + " timeout_ms: " + std::to_string(timeout_ms));
        }
        return std::make_shared<HttpResult>((int)HttpResult::Error::Ok, rsb, "OK");
    }
}
}