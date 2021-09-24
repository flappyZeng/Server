#include"../http/http_parser.h"
#include"../log.h"

static server::Logger::ptr g_logger = SERVER_LOG_NAME("system");


const char test_request_data[] = "CONNECT / HTTP/1.1\r\n"
                           "Host: www.sylar.top\r\n"
                           "Content-length: 10\r\n\r\n"
                           "1234567890";

void test_request(){
    server::http::HttpRequestParser parser;
    std::string temp  = test_request_data;
    size_t rt = parser.execute(&temp[0], temp.size());
    SERVER_LOG_INFO(g_logger) << "execute rt = " << rt   << "\n"
                        << "has_error = " << parser.hasError() << '\n'
                        << "is_finished = " << parser.isFinished() << "\n"
                        << "content-length = " << parser.getContentLength();
    temp.resize(temp.size() - rt);
    SERVER_LOG_INFO(g_logger) << parser.getDate()->toString();
    SERVER_LOG_INFO(g_logger) << temp;
}

const char test_response_data[] = "HTTP/1.1 301 Moved Permanently\r\n"
                                "Server: nginx/1.12.2\r\n"
                                "Date: Wed, 01 Sep 2021 14:03:11 GMT\r\n"
                                "Content-Type: text/html\r\n"
                                "Content-Length: 185\r\n"
                                "Location: http://www.sylar.top/blog/index.php\r\n"
                                "Connection: keep-alive\r\n\r\n"
                                "<html>\r\n"
                                "<head><title>301 Moved Permanently</title></head>\r\n"
                                "<body bgcolor=\"white\">\r\n"
                                "<center><h1>301 Moved Permanently</h1></center>\r\n"
                                "<hr><center>nginx/1.12.2</center>\r\n"
                                "</body>\r\n"
                                "</html>\r\n";

void test_response(){
    server::http::HttpResponseParser parser;
    std::string temp = test_response_data;
    size_t rt = parser.execute(&temp[0], temp.size());
    SERVER_LOG_INFO(g_logger) << "execute rt = " << rt
                            << " has_error = " << parser.hasError()
                            << " is_finished = " << parser.isFinished()
                            << " content-length = " << parser.getContentLength();
    temp.resize(temp.size() - rt);
    SERVER_LOG_INFO(g_logger) << parser.getDate()->toString();
    SERVER_LOG_INFO(g_logger) << temp;
    
}

int main(int argc, char** argv){
    test_request();
    test_response();
    return 0;
}

