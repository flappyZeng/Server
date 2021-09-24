#pragma once

#include<iostream>
#include<memory>
#include<map>
#include<netinet/in.h>
#include<string>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/un.h>
#include<vector>

namespace server{

class IPAddress;

class Address{
public:
    typedef std::shared_ptr<Address>ptr;
    virtual ~Address() {}

    //返回地址类型： IPV4, IPV6, UnixAddress
    int getFamily() const;
    static Address::ptr Create(const sockaddr* address, socklen_t addrlen);

    static bool Lookup(std::vector<Address::ptr> & results, const std::string& host, 
            int family = AF_UNSPEC, int type = 0, int protocol = 0);
    static Address::ptr LookupAny(const std::string& host, int family = AF_UNSPEC, int type = 0, int protocol = 0);
    static Address::ptr LookupAnyIPAddress(const std::string& host, int family = AF_UNSPEC, int type = 0, int protocol = 0);

    //根据网卡信息返回ip地址
    static bool GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t> >& result, int family = AF_UNSPEC);
    static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >& result, const std::string& iface, int family = AF_UNSPEC);

    virtual const sockaddr* getAddr() const = 0;
    virtual sockaddr* getAddr() = 0;
    virtual socklen_t getAddrlen() const = 0;
    virtual std::ostream& insert(std::ostream& os) const = 0;
    virtual void setPort(uint16_t v) = 0;
    virtual uint16_t getPort() const = 0;
    std::string toString() const;

    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;

};

class IPAddress : public Address{
public:
    typedef std::shared_ptr<IPAddress>ptr;

    //域名（baidu.com)   -->   ipv4 or ipv6
    static IPAddress::ptr Create(const char* address, uint16_t port = 0);

    //获取ip对应的广播地址
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
    //获取ip对应的子网地址
    virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
    //获取ip对应的子网掩码
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

    //获取端口号
    virtual uint16_t getPort() const = 0;
    //设置端口号
    virtual void setPort(uint16_t v) = 0;
};

class IPv4Address : public IPAddress{
public:
    typedef std::shared_ptr<IPv4Address>ptr;
    IPv4Address(const sockaddr_in& address);
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);
                                                                                                                                                                                                          
    static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrlen() const override;
    std::ostream& insert(std::ostream& os) const override;

    //获取ip对应的广播地址
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    //获取ip对应的子网地址
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;
    //获取ip对应的子网掩码
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    //获取端口号
    uint16_t getPort() const override;
    //设置端口号
    void setPort(uint16_t v) override;

private:
    sockaddr_in m_addr;
};

class IPv6Address : public IPAddress{
public:
    typedef std::shared_ptr<IPv6Address>ptr;
    IPv6Address(const sockaddr_in6& address);
    IPv6Address(const char* address = INADDR_ANY, uint16_t port = 0);
    IPv6Address(const uint8_t address[16], uint16_t port = 0);

    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrlen() const override;
    std::ostream& insert(std::ostream& os) const override;

    //获取ip对应的广播地址
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    //获取ip对应的子网地址
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;
    //获取ip对应的子网掩码
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    //获取端口号
    uint16_t getPort() const override;
    //设置端口号
    void setPort(uint16_t v) override;

private:
    sockaddr_in6 m_addr;
};

class UnixAddress : public Address{
public:
    typedef std::shared_ptr<UnixAddress>ptr;
    UnixAddress();
    UnixAddress(const std::string& path);
 
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrlen() const override;
    void setAddrlen(uint32_t length){
        m_len = length;
    }
    std::ostream& insert(std::ostream& os) const override;
    void setPort(uint16_t v) override{
    }
    uint16_t getPort() const override{
        return -1;
    }

private:
    sockaddr_un m_addr;
    socklen_t m_len;
};

class UnknownAddress : public Address{
public:
    typedef std::shared_ptr<UnknownAddress>ptr;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& address);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrlen() const override;
    void setPort(uint16_t v)  override{
    }
    uint16_t getPort() const override{
        return -1;
    }

    std::ostream& insert(std::ostream& os) const override;

private:
    sockaddr m_addr;
};

std::ostream& operator<<(std::ostream& os, const Address& addr);

}