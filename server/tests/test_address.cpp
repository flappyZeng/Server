#include"../address.h"
#include"../log.h"

server::Logger::ptr g_logger = SERVER_LOG_ROOT();

void test(){
    std::vector<server::Address::ptr>addrs;

    SERVER_LOG_INFO(g_logger) << "begin";
    bool v = server::Address::Lookup(addrs, "www.sylar.top");
    SERVER_LOG_INFO(g_logger) << "end";
    if(!v){
        SERVER_LOG_ERROR(g_logger) << "lookup fail";
        return ;
    }
    for(size_t i = 0; i < addrs.size(); ++i){
        SERVER_LOG_INFO(g_logger) << i << "-" << addrs[i]->toString();
    }
}

void test_iface(){
    std::multimap<std::string, std::pair<server::Address::ptr, uint32_t> >results;
    bool v = server::Address::GetInterfaceAddresses(results);
    if(!v){
        SERVER_LOG_INFO(g_logger) << "Address::GetInterfaceAddresses fail";
        return ;
    }
    for(auto& i : results){
        SERVER_LOG_INFO(g_logger) << i.first << "-" << i.second.first->toString() << "-" << i.second.second;
    }
}

void test_ipv4(){
    auto addr = server::IPAddress::Create("196.168.0.3");
    if(addr){
        SERVER_LOG_INFO(g_logger) << addr->toString();
    }

}
int main(int argc, char** argv){
    test_ipv4();
    return 0;
}