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
        SERVER_LOG_ERROR(g_logger) << "buffer size: " << buff_size;
        char* data = buffer.get();
        int offset = 0;
        SERVER_LOG_ERROR(g_logger) << "start parse";
        do{
            SERVER_LOG_ERROR(g_logger) << "start read";
            int len = read(data + offset, buff_size - offset);
            if(len <= 0){
                SERVER_LOG_ERROR(g_logger) << "read error";
                return nullptr;
            }
            SERVER_LOG_ERROR(g_logger) << "end parse";
            len += offset;
            size_t nparse = parser->execute(data, len);  //parser对date进行解析
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
        SERVER_LOG_ERROR(g_logger) << "end parse";
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