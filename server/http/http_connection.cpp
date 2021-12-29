#include"http_connection.h"
#include"http_parser.h"
#include"../log.h"

namespace server{
namespace http{

    static server::Logger::ptr g_logger = SERVER_LOG_ROOT();

    HttpConnetction::HttpConnetction(Socket::ptr sock, bool owner)
        :SocketStream(sock, owner){
    }

    HttpResponse::ptr HttpConnetction::recvResponse(){
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
            //SERVER_LOG_ERROR(g_logger) << "start read";
            int len = read(data + offset, buff_size - offset);
            SERVER_LOG_ERROR(g_logger) << "read len = " << len;
            if(len <= 0){
                SERVER_LOG_ERROR(g_logger) << "read error";
                return nullptr;
            }
            //SERVER_LOG_ERROR(g_logger) << "end parse";
            len += offset;
            size_t nparse = parser->execute(data, len, true);  //parser对date进行解析
            if(parser->hasError()){ //报文解析出错
                SERVER_LOG_ERROR(g_logger) << "parser error";
                return nullptr;
            }
            offset  = len - nparse;
            if(offset == buff_size){  //缓存区满了
                SERVER_LOG_ERROR(g_logger) << "buffer error";
                return nullptr;
            }
            if(parser->isFinished()){
                break;
            }

        }while(true);
        //SERVER_LOG_ERROR(g_logger) << "end parse";

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
                    size_t n_parse =  parser->execute(data, len, true);
                    if(parser->hasError()){
                        return nullptr;
                    }
                    len -= n_parse;
                    if(len = (int)buff_size){
                        return nullptr;
                    }
                }while(!parser->isFinished());
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
            SERVER_LOG_ERROR(g_logger) << "return parse";
        }
        return parser->getDate();
    }

    int HttpConnetction::sendRequest(HttpRequest::ptr req){
        std::stringstream ss;
        ss << *req;
        std::string data = ss.str();
        return writeFixSize(data.c_str(), data.size());
    }
}
}