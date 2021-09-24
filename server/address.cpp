#include"address.h"
#include"log.h"
#include"private_endian.h"

#include <arpa/inet.h>
#include<ifaddrs.h>  //提供getifaddr方法，获取网卡信息。
#include <netdb.h>
#include<sstream>

namespace server{

    static server::Logger::ptr g_logger = SERVER_LOG_ROOT();

    template<class T>
    uint32_t CountBytes(T val){
        uint32_t result = 0;
        for(; val; ){
            result ++;
            val &= (val - 1);
        }
        return result;
    }

    template<class T>
    static T createMask(uint32_t bit){
        return (1 << (sizeof(T) * 8 - bit)) - 1;
    }
    //Address

    int Address::getFamily() const{
        return getAddr()->sa_family;
    }
    Address::ptr Address::Create(const sockaddr* address, socklen_t addrlen){
        if(address == nullptr){
            return nullptr;
        }
        Address::ptr result;
        switch (address->sa_family)
        {
            case AF_INET:
                result.reset(new IPv4Address(*(const sockaddr_in*)address));
                break;
            case AF_INET6:
                result.reset(new IPv6Address(*(const sockaddr_in6*)address));
                break;
            default:
                result.reset(new UnknownAddress(*address));
                break;
        }
        return result;
    }

    bool Address::Lookup(std::vector<Address::ptr> & results, const std::string& host, int family , int type, int protocol ){
        addrinfo hints, *result, *next;
        memset(&hints, 0, sizeof(hints));

        hints.ai_family = family;   //控制是IPv4还是IPv6,如果是AF_UNSPEC，则都可以
        hints.ai_socktype = type;   //控制是TCP/UDP；
        hints.ai_protocol = protocol;  //上层协议

        std::string node;
        const char* service = nullptr;


        //检查是不是ipv6的地址，[xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx]:xxxx;
        if(!host.empty() && '[' == host[0]){
            const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
            if(endipv6){
                //就是ipv6地址 ,check out of range
                if(*(endipv6 + 1) == ':'){
                    service = endipv6 + 2;
                }
                node = host.substr(1, endipv6 - host.c_str() - 1);
            }
        }
        //上述地址]后面假如还有:代表有相应的服务需求
        if(node.empty()){
            service = (const char*) memchr(host.c_str(), ':', host.size());
            if(service){
                if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)){
                    node = host.substr(0, service - host.c_str());
                    ++ service;
                }
            }
        }
        if(node.empty()){
            node = host;
        }

        int error = getaddrinfo(node.c_str(), service, &hints, &result);
        if(error){
            SERVER_LOG_ERROR(g_logger) << "Address::lookup getaddr(" << host 
                << family << ", " << type  << ") error = " << error << " errstr = " << strerror(errno);   
        }
        next = result;
        while(next){
            results.push_back(Create(next->ai_addr, next->ai_addrlen));
            next = next ->ai_next;
        }
        freeaddrinfo(result);
        return true;
    }

    Address::ptr Address::LookupAny(const std::string& host, int family , int type, int protocol){
        std::vector<Address::ptr>results;
        bool result = Lookup(results, host, family, type, protocol);
        if(result){
            return results[0];
        }
        return nullptr;
    }

    Address::ptr Address::LookupAnyIPAddress(const std::string& host, int family, int type, int protocol){
        std::vector<Address::ptr>results;
        bool result = Lookup(results, host, family, type, protocol);
        if(result){
            for(Address::ptr address : results){
                IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(address);
                if(v){
                    return v;
                }
            }
        }
        return nullptr;
    }

    //根据网卡信息返回ip地址
    bool Address::GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t> >& result, int family ){
        ifaddrs *next, *results;
        if(getifaddrs(&results) != 0){
            //返回大于0表示出错
            SERVER_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses getifaddr" << "errno = " << errno << " errstr = " << strerror(errno);
            return false;
        }
        try{
            for(next = results; next ; next = next ->ifa_next){
                Address::ptr addr;
                uint32_t prefix_len = ~0u;
                if(family != AF_UNSPEC && family != next->ifa_addr->sa_family){
                    continue;
                }
                switch (next->ifa_addr->sa_family)
                {
                    case AF_INET:
                        {
                            addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                            uint32_t netmask = ((sockaddr_in*) next->ifa_netmask)->sin_addr.s_addr;
                            prefix_len = server::CountBytes<uint32_t>(netmask);

                        }
                        break;
                    case AF_INET6:
                        {
                            addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                            in6_addr& netmask = ((sockaddr_in6*) next->ifa_netmask)->sin6_addr;
                            prefix_len = 0;
                            for(int i = 0; i < 16; ++i){
                                uint8_t& val = netmask.__in6_u.__u6_addr8[i];
                                prefix_len += server::CountBytes<uint8_t>(val);
                            }

                        }
                        break;
                    default:
                        break;
                }
                if(addr){
                    result.insert(std::make_pair(next->ifa_name, std::make_pair(addr, prefix_len)));
                }

            }
        }
        catch(...){
            SERVER_LOG_ERROR(g_logger) << "Address::GetInterfaceAddress exception";
            freeifaddrs(results);
            return  false;
        }
        freeifaddrs(results);
        return true;
    }
    bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >& result, const std::string& iface, int family ){
        if(iface.empty() || iface == "*"){  //“*”表示任意网卡
            if(family == AF_INET || family == AF_UNSPEC){
                result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
            }
            if(family == AF_INET6){
                result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
            }
            return true;
        }
        std::multimap<std::string, std::pair<Address::ptr, uint32_t> >results;
        if(!GetInterfaceAddresses(results, family)){  //找不到
            return false;
        }

        auto its = results.equal_range(iface);  //返回一个pair，表示等于目标值的迭代器区间
        for(;its.first != its.second; ++its.first){
            result.push_back(its.first->second);
        }
        return true;
    }

    std::string Address::toString() const{
        std::stringstream ss;
        insert(ss);
        return ss.str();
    }


    bool Address::operator<(const Address& rhs) const{
        socklen_t minlen = std::min(getAddrlen(), rhs.getAddrlen());
        int result = memcmp(getAddr(), rhs.getAddr(), minlen);
        if(result < 0){
            return true;
        }else if(result > 0){
            return false;
        }else if(getAddrlen() < rhs.getAddrlen()){
            return true;
        }
        return false;
    }

    bool Address::operator==(const Address& rhs) const{
        return getAddrlen() == rhs.getAddrlen()
            && memcmp(getAddr(), rhs.getAddr(), getAddrlen());
    }

    bool Address::operator!=(const Address& rhs) const{
        return !(*this == rhs);
    }

    //IPAddress

    IPAddress::ptr IPAddress::Create(const char* address, uint16_t port){
        //基于getaddrinfo，该方法可以将合法的字符串转换任意的ip地址（IPv4 或者 IPv6)
        addrinfo hints, *results;
        memset(&hints, 0, sizeof(hints));

        //hints.ai_flags = AI_NUMERICHOST; //限定使用数字作为输入address
        hints.ai_family = AF_UNSPEC;

        int error = getaddrinfo(address, NULL, &hints, &results);
        if(error){
            //返回非零值，表示出错
            SERVER_LOG_ERROR(g_logger) << "IPAddress::Create(" << address << "," << port << ") error = "
                            << error << "  errno = " << errno << " errstr = " << strerror(errno);
        }
        try{
            IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
            if(result){
                result->setPort(port);
            }
            freeaddrinfo(results);
            return result;
            
        }catch(...){
            freeaddrinfo(results);
            return nullptr;
        }
    }

    //IPv4

    IPv4Address::IPv4Address(uint32_t address, uint16_t port){
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = byteswapOnLittleEndian(port);
        m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
        
    }

    IPv4Address::IPv4Address(const sockaddr_in& address){
        m_addr = address;
    }
    
    // ”192.168.1.1“  --> 11000000 10111100 00000001 00000001
    IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port){
        IPv4Address::ptr rt(new IPv4Address());
        rt->m_addr.sin_port = port;
        rt->m_addr.sin_family = AF_INET;
        int result = inet_pton(AF_INET, address, &rt->m_addr.sin_family);
        if(result <= 0){
            //表示转换出错
            SERVER_LOG_ERROR(g_logger) << "IPv4Address::Create(" << address << ","
                                << port << ") rt = " << result << " errno = " << errno
                                << " errstr = " << strerror(errno);
            return nullptr;     
        }
        return rt;
    }

    const sockaddr* IPv4Address::getAddr() const {
        return (sockaddr*)&m_addr;
    }
    sockaddr* IPv4Address::getAddr(){
        return (sockaddr*)&m_addr;
    }
    socklen_t IPv4Address::getAddrlen() const {
        return sizeof(m_addr);
    }
    std::ostream& IPv4Address::insert(std::ostream& os) const {
        uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
        os << ((addr >> 24) & 0xff) << "."
           << ((addr >> 16) & 0xff) << "."
           << ((addr >> 8) & 0xff) << "."
           << (addr & 0xff);
        
        os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
        return os;
    }

    //获取ip对应的广播地址
    IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
        if(prefix_len > 32){
            return nullptr;
        }
        sockaddr_in baddr(m_addr);
        baddr.sin_addr.s_addr |= byteswapOnLittleEndian(createMask<uint32_t>(prefix_len));
        return IPv4Address::ptr(new IPv4Address(baddr));
    }

    //获取ip对应的子网地址
    IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len){
        if(prefix_len > 32){
            return nullptr;
        }
        sockaddr_in baddr(m_addr);
        //这里不应该要把前面的置为1么
        baddr.sin_addr.s_addr &= ~byteswapOnLittleEndian(createMask<uint32_t>(prefix_len));
        return IPv4Address::ptr(new IPv4Address(baddr));
    }
    //获取ip对应的子网掩码
    IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
        sockaddr_in subnet;
        memset(&subnet, 0, sizeof(subnet));
        subnet.sin_family = AF_INET;
        subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(createMask<uint64_t>(prefix_len));
        return IPv4Address::ptr(new IPv4Address(subnet));
    }

    //获取端口号
    uint16_t IPv4Address::getPort() const {
        return byteswapOnLittleEndian(m_addr.sin_port);
    }
    //设置端口号
    void IPv4Address::setPort(uint16_t v) {
        m_addr.sin_port = byteswapOnLittleEndian(v);
    }

    //IPv6
    
    IPv6Address::IPv6Address(const sockaddr_in6& address){
        m_addr = address;
    }

    IPv6Address::IPv6Address(const char*  address, uint16_t port){
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
        m_addr.sin6_port = byteswapOnLittleEndian(port);
        memcpy(&m_addr.sin6_addr.__in6_u.__u6_addr16, address, 16);
    }

    IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port){
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
        m_addr.sin6_port = byteswapOnLittleEndian(port);
        memcpy(&m_addr.sin6_addr.__in6_u.__u6_addr16, address, 16);
    }

    IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port){
        IPv6Address::ptr rt(new IPv6Address());
        rt-> m_addr.sin6_port = port;
        rt-> m_addr.sin6_family = AF_INET6;
        int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_family);
        if(result <= 0){
            //表示转换出错
            SERVER_LOG_ERROR(g_logger) << "IPv64Address::Create(" << address << ","
                                << port << ") rt = " << result << " errno = " << errno
                                << " errstr = " << strerror(errno);
            return nullptr;     
        }
        return rt;
    }

    const sockaddr* IPv6Address::getAddr() const {
        return (sockaddr*)&m_addr;
    }

    sockaddr* IPv6Address::getAddr(){
        return (sockaddr*)&m_addr;
    }
    socklen_t IPv6Address::getAddrlen() const {
        return sizeof(m_addr);
    }
    std::ostream& IPv6Address::insert(std::ostream& os) const {
        os << "[";
        const uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr16;
        bool used_zeros = false;
        for(size_t i = 0; i < 8;  ++i){
            if(addr[i] == 0 && !used_zeros){
                continue;
            }
            if(i && addr[i - 1] == 0 && !used_zeros){
                os << ":";
                used_zeros = true;
            }
            //正常情况下要补上“：”
            if(i){
                os << ":";
            }
            os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
        }
        if(!used_zeros && addr[7] == 0){
            os << "::";
        }
        os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
        return os;
    }

    //获取ip对应的广播地址
    IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.__in6_u.__u6_addr16[prefix_len / 8] |= createMask<uint8_t>(prefix_len % 8);
        for(int i = prefix_len / 8 + 1; i < 16; ++i){
            baddr.sin6_addr.__in6_u.__u6_addr16[i] = 0xff;
        }
        return IPv6Address::ptr(new IPv6Address(baddr));
    }

    //获取ip对应的子网地址
    IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len){
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.__in6_u.__u6_addr16[prefix_len / 8] &= ~createMask<uint8_t>(prefix_len % 8);
        for(int i = prefix_len / 8 + 1; i < 16; ++i){
            baddr.sin6_addr.__in6_u.__u6_addr16[i] = 0;
        }
        return IPv6Address::ptr(new IPv6Address(baddr));
    }
    //获取ip对应的子网掩码
    IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
        sockaddr_in6 subnet;
        memset(&subnet, 0, sizeof(subnet));
        subnet.sin6_family = AF_INET6;
        subnet.sin6_addr.__in6_u.__u6_addr16[prefix_len / 8] = ~createMask<uint8_t>(prefix_len % 8);
        for(int i = 0; i < prefix_len / 8; ++i){
            subnet.sin6_addr.__in6_u.__u6_addr16[i] = 0xff;
        }
        return IPv6Address::ptr(new IPv6Address(subnet));
    }
 
    //获取端口号
    uint16_t IPv6Address::getPort() const {
        return byteswapOnLittleEndian(m_addr.sin6_port);
    }
    //设置端口号
    void IPv6Address::setPort(uint16_t v) {
        m_addr.sin6_port = byteswapOnLittleEndian(v);
    }

    //UnixAddress

    static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

    UnixAddress::UnixAddress(){
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_len = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
    }

    UnixAddress::UnixAddress(const std::string& path){
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_len = path.size() + 1;
        
        if(!path.empty() && '\0' == path[0]){
            --m_len;
        }
        if(m_len > sizeof(m_addr.sun_path)){
            throw std::logic_error("path too long");
        }
        mempcpy(m_addr.sun_path, path.c_str(), m_len);

        m_len += offsetof(sockaddr_un, sun_path);
    }
    
    const sockaddr* UnixAddress::getAddr() const {
        return (sockaddr*)& m_addr;
    }

    sockaddr* UnixAddress::getAddr(){
        return (sockaddr*)& m_addr;
    }
    socklen_t UnixAddress::getAddrlen() const{
        return m_len;
    }


    std::ostream& UnixAddress::insert(std::ostream& os) const {
        if(m_len > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0'){
                return os << "\\0" << std::string(m_addr.sun_path + 1, m_len - offsetof(sockaddr_un, sun_path) - 1);
            }
        return os << m_addr.sun_path;
    }

    //UnKnown

    UnknownAddress::UnknownAddress(int family){
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sa_family = byteswapOnLittleEndian(family);
    }

    UnknownAddress::UnknownAddress(const sockaddr& address){
        m_addr = address;
    }

    const sockaddr* UnknownAddress::getAddr() const {
        return (sockaddr*)&m_addr;
    }
    sockaddr* UnknownAddress::getAddr() {
        return (sockaddr*)&m_addr;
    }

    socklen_t UnknownAddress::getAddrlen() const{
        return sizeof(m_addr);
    }


    std::ostream& UnknownAddress::insert(std::ostream& os) const {
        return os << "[ UnKnownAddress family = " << byteswapOnLittleEndian(m_addr.sa_family) <<" ]";
    }

    std::ostream& operator<<(std::ostream& os, const Address& addr){
        return addr.insert(os);
    }
}