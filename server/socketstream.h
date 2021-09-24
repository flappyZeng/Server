#pragma once

#include"stream.h"
#include"socket.h"

namespace server{
class SocketStream : public Stream {
public:
    typedef std::shared_ptr<SocketStream>ptr;

    SocketStream(Socket::ptr sock, bool owner = true);
    ~SocketStream();

    virtual int read(void* buffer, size_t length) override;
    virtual int read(ByteArray::ptr ba, size_t length) override;
    virtual int write(const void* buffer, size_t length) override;
    virtual int write(ByteArray::ptr ba, size_t length) override; 
    virtual void close() override;

    bool isConnected() const;
  
private:
    Socket::ptr m_socket;
    bool m_onwer;  //如果owner为true,那么socketStream析构时会将socket析沟;
};
}