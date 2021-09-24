#pragma once 

// http server , accept   --> Session
// http client , connect  --> Connection

#include"http.h"
#include"../socketstream.h"
#include"../socket.h"

//客户端专供

namespace server{
namespace http{
class HttpConnetction : public SocketStream {
public:
    typedef std::shared_ptr<HttpConnetction>ptr;

    HttpConnetction(Socket::ptr sock, bool owner = true);
    
    int sendRequest(HttpRequest::ptr req);
    HttpResponse::ptr recvResponse();

};
}
}