#include"fd_manager.h"
#include"hook.h"
#include"log.h"
#include"macro.h"
#include"socket.h"

#include<sys/socket.h>
#include <sys/types.h> 
#include<netinet/tcp.h>  //提供TCP_NODELAY

namespace server{

    static server::Logger::ptr g_logger = SERVER_LOG_NAME("system");
    void Socket::initSock(){
        int val = 1;
        setOption(SOL_SOCKET, SO_REUSEADDR, val);  //允许端口复用
        if(m_type == SOCK_STREAM){
            setOption(IPPROTO_TCP,  TCP_NODELAY, val); //TCP_NODELAY，就意味着禁用了Nagle算法，允许小包的发送
        }
    }

    void Socket::newSock(){
        m_sock = socket(m_family, m_type, m_protocol);
        if(SERVER_LICKLY(m_sock != -1)){
            initSock();
        }
        else{
            SERVER_LOG_ERROR(g_logger) << "socket(" << m_family << ", " << m_type << ", "<< m_protocol << ")"
                            << " errno = " << errno << " errstr = " << strerror(errno);
        }
    }

    
    Socket::ptr Socket::CreateTCP(server::Address::ptr address){
        Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
        return sock;
    }   

    Socket::ptr Socket::CreateUDP(server::Address::ptr address){
        Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateTCPSocket(){
        Socket::ptr sock(new Socket(IPv4, TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDPSocket(){
        Socket::ptr sock(new Socket(IPv4, UDP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateTCPSocket6(){
        Socket::ptr sock(new Socket(IPv6, TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUDPSocket6(){
        Socket::ptr sock(new Socket(IPv6, UDP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUnixTCPSocket(){
        Socket::ptr sock(new Socket(UNIX, TCP, 0));
        return sock;
    }

    Socket::ptr Socket::CreateUnixUDPSocket(){
        Socket::ptr sock(new Socket(UNIX, UDP, 0));
        return sock;
    }


    Socket::Socket(int family, int type, int protocol)
        :m_sock(-1)
        ,m_family(family)
        ,m_type(type)
        ,m_protocol(protocol)
        ,m_isConnected(false){

    }   
    Socket::~Socket(){
        close();
    }

    int64_t Socket::getSendTimeout(){
        server::FdCtx::ptr ctx = server::FdMgr::getInstance()->get(m_sock);
        if(ctx){
            return ctx->getTimeout(SO_SNDTIMEO);
        }
        return -1;
    }
    void Socket::setSendTimeout(int64_t v){
        timeval tv{int(v/1000), int(v % 1000 * 1000)};
        setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
    }

    int64_t Socket::getRecvTimeout(){
        server::FdCtx::ptr ctx = server::FdMgr::getInstance()->get(m_sock);
        if(ctx){
            return ctx->getTimeout(SO_RCVTIMEO);
        }
        return -1;
    }
    void Socket::setRecvTimeout(int64_t v){
        timeval tv{int(v/1000), int(v % 1000 * 1000)};
        setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
    }

    bool Socket::getOption(int level, int option, void* result, socklen_t* len){  //获取socket的option信息
        int rt =  getsockopt(m_sock, level, option, result, (socklen_t*)len);
        if(rt){
            SERVER_LOG_DEBUG(g_logger) << "getOption(" << m_sock << ", " << level  << ", " << option << ",...)"
                            << " errno = " << errno << " errstr = " << strerror(errno); 
            return false;
        }
        return true;
    }

    bool Socket::setOption(int level, int option, const void* value, socklen_t len){
        int rt = setsockopt(m_sock, level, option, value, (socklen_t)len);
        if(rt){
            SERVER_LOG_DEBUG(g_logger) << "setOption(" << m_sock << ", " << level  << ", " << option << ",...)"
                            << " errno = " << errno << " errstr = " << strerror(errno); 
            return false;
        }
        return true;
    }
   
    Socket::ptr Socket::accept(){
        Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
        int newsock = ::accept(m_sock, nullptr, nullptr);
        if(-1 == newsock){
            SERVER_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno = " << errno << " errstr = " << strerror(errno);
            return nullptr;
        }
        if(sock->init(newsock)){
            //SERVER_LOG_ERROR(g_logger) << "accept(inited) ";
            return sock;
        }
        return nullptr;
    }

    
    bool Socket::init(int sock){
        server::FdCtx::ptr ctx = server::FdMgr::getInstance() -> get(sock);
        if(ctx && ctx->isSocket() && !ctx->isClose()){
            m_sock = sock;
            m_isConnected = true;
            initSock();
            getLocalAddress();
            getRemoteAddress();
        }
        return true;
    }
    bool Socket::bind(const Address::ptr addr){
        if(!isValid()){
            newSock();
            if(SERVER_UNLICKLY(!isValid())){
                return false;
            }
        }

        if(addr->getFamily() != m_family){
            SERVER_LOG_ERROR(g_logger) << "bind sock.family(" << m_family << ") addr.family(" 
                        << addr->getFamily() << ") not equal, addr = " << addr->toString();
            return false;
        }

        if(::bind(m_sock, addr->getAddr(), addr->getAddrlen())){
            SERVER_LOG_ERROR(g_logger) << "bind error errno = " << errno << " errstr = " << strerror(errno) << "address: " << addr->toString();
        }
        getLocalAddress();

        return true;
    }

    bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms){
        if(!isValid()){
            newSock();
            if(SERVER_UNLICKLY(!isValid())){
                return false;
            }
        }

        if(addr->getFamily() != m_family){
            SERVER_LOG_ERROR(g_logger) << "connect sock.family(" << m_family << ") addr.family(" 
                        << addr->getFamily() << ") not equal, addr = " << addr->toString();
            return false;
        }

        if((uint64_t)-1 == timeout_ms){
            if(::connect(m_sock, addr->getAddr(), addr->getAddrlen())){
                SERVER_LOG_ERROR(g_logger) << "sock = " << m_sock << " connect(" << addr->toString() << ") error errno = " << errno
                            << " errstr = " << strerror(errno);
                close();  //连接失败的话就要释放句柄
                return false;
            }
        }else{
            if(::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrlen(), timeout_ms)){
                SERVER_LOG_ERROR(g_logger) << "sock = " << m_sock << " connect(" << addr->toString() << ") error errno = " << errno
                            << " errstr = " << strerror(errno);
                close();  //连接失败的话就要释放句柄
                return false;
            }
        }
        m_isConnected = true;
        getRemoteAddress();
        getLocalAddress();
        return true;
    }
    bool Socket::listen(int backlog ){
        if(!isValid()){
            SERVER_LOG_ERROR(g_logger) << "listen error sock = -1";
            return false;
        }

        if(::listen(m_sock, backlog)){
            SERVER_LOG_ERROR(g_logger) << "listen sock = " << m_sock << " errno = " << errno << " errstr = " << strerror(errno);
            return false;
        }
        return true;
    }

    bool Socket::close(){
        if(!m_isConnected && -1 == m_sock){
            return true;
        }
        m_isConnected = false;
        if(-1 != m_sock){
            ::close(m_sock);
            m_sock = -1;
        }
        return false;  //这里为什么要return false;??
    }

    int Socket::send(const void* buffer, size_t length, int flag){
        if(isConnected()){
            return ::send(m_sock, buffer, length, flag);
        }
        return -1;

    }

    int Socket::send(const iovec* buffer, size_t length, int flag){
        //基于sendmsg实现
        if(isConnected()){
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffer;
            msg.msg_iovlen = length;
            return ::sendmsg(m_sock, &msg, flag);
        }
        return -1;
    }

    int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags){
        if(isConnected()){
            return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrlen());
        }
        return -1;
    }
    int Socket::sendTo(const iovec* buffer, size_t length, const Address::ptr to, int flags ){
        if(isConnected()){
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffer;
            msg.msg_iovlen = length;
            msg.msg_name = to->getAddr();
            msg.msg_namelen = to->getAddrlen();

            return ::sendmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    int Socket::recv(void* buffer, size_t length, int flags){
        if(isConnected()){
            SERVER_LOG_INFO(g_logger) << "read from buffer";
            return ::recv(m_sock, buffer, length, flags);
        }
        return -1;
    }
    int Socket::recv(iovec* buffer, size_t length, int flags){
        if(isConnected()){
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffer;
            msg.msg_iovlen = length;
            return ::recvmsg(m_sock, &msg, flags);
        }
        return -1;
    }
    int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags){
        if(isConnected()){
            socklen_t len = from->getAddrlen();
            return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
        }
        return -1;
    }
    int Socket::recvFrom(iovec* buffer, size_t length, Address::ptr from, int flags){
        if(isConnected()){
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffer;
            msg.msg_iovlen = length;
            msg.msg_name = from->getAddr();
            msg.msg_namelen = from->getAddrlen();

            return ::recvmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    //获取连接远端的地址
    Address::ptr Socket::getRemoteAddress(){
        if(m_RemoteAddress){
            return m_RemoteAddress;
        }
        Address::ptr result;
        switch (m_family)
        {
            case AF_INET:
                result.reset(new IPv4Address());
                break;
            case AF_INET6:
                result.reset(new IPv6Address());
                break;
            case AF_UNIX:
                result.reset(new UnixAddress());
            default:
                result.reset(new UnknownAddress(m_family));
                break;
        }
        socklen_t addrlen = result->getAddrlen();
        if(getpeername(m_sock, result->getAddr(), &addrlen)){   //获取连接段的ip地址并存入到result中
            SERVER_LOG_ERROR(g_logger) << "getpeername error sock = " << m_sock 
                        << " errno  = " << errno << " errstr = " << strerror(errno);
            return Address::ptr(new UnknownAddress(m_family)); 
        }
        if(m_family == AF_UNIX){
            //Unix地址的地址长度需要重设
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrlen(addrlen);
        }
        m_RemoteAddress = result;
        return m_RemoteAddress;

    }
    //获取本地的地址
    Address::ptr Socket::getLocalAddress(){
        if(m_localAddress){
            return m_localAddress;
        }
        Address::ptr result;
        switch (m_family)
        {
            case AF_INET:
                result.reset(new IPv4Address());
                break;
            case AF_INET6:
                result.reset(new IPv6Address());
                break;
            case AF_UNIX:
                result.reset(new UnixAddress());
                break;
            default:
                result.reset(new UnknownAddress(m_family));
                break;
        }
        socklen_t addrlen = result->getAddrlen();
        if(getsockname(m_sock, result->getAddr(), &addrlen)){   //获取连接段的ip地址并存入到result中
            SERVER_LOG_ERROR(g_logger) << "getsockname error sock = " << m_sock 
                        << " errno  = " << errno << " errstr = " << strerror(errno);
            return Address::ptr(new UnknownAddress(m_family)); 
        }
        if(m_family == AF_UNIX){
            //Unix地址的地址长度需要重设
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrlen(addrlen);
        }
        m_localAddress = result;
        return m_localAddress;
    }


    bool Socket::isValid() const{
        return m_sock != -1;
    }

    int Socket::getError(){
        int error = 0;
        socklen_t len = sizeof(error);
        if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len)){  //返回socket存在的错误
            return -1;
        }
        return error;
    }

    std::ostream& Socket::dump(std::ostream& os) const{ //格式化输出socket信息
        os << "[Socket sock = " << m_sock
           << " is_connected = " <<m_isConnected
           << " family = " << m_family
           << " type = " << m_type
           << " protocol = " << m_protocol;
        if(m_localAddress){
            os << " local_address =" << m_localAddress->toString();
        }
        if(m_RemoteAddress){
            os << " Remote_address = " << m_RemoteAddress->toString();
        }
        os << "]";
        return os;
    }
    
    bool Socket::cancelRead(){
        return IOManager::GetThis()->cancelEvent(m_sock, server::IOManager::READ);
    }

    bool Socket::cancelWrite(){
        return IOManager::GetThis()->cancelEvent(m_sock, server::IOManager::WRITE);
    }
    bool Socket::cancelAccept(){
        return IOManager::GetThis()->cancelEvent(m_sock, server::IOManager::READ);
    }
    bool Socket::cancelAll(){
        return IOManager::GetThis()->cancelAll(m_sock);
    }

    std::ostream& operator<<(std::ostream& os , const Socket& sock){
        return sock.dump(os);
    }
}
