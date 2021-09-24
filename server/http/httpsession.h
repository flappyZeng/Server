#pragma once 

// http server , accept   --> Session
// http client , connect  --> Connection

#include"http.h"
#include"../socketstream.h"
#include"../socket.h"

//服务端专供

namespace server{
namespace http{
class HttpSession : public SocketStream {
public:
    typedef std::shared_ptr<HttpSession>ptr;

    HttpSession(Socket::ptr sock, bool owner = true);
    
    HttpRequest::ptr recvRequset();
    int sendResponse(HttpResponse::ptr rsp);

};
}
}