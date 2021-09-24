#include"../log.h"
#include"../config.h"
#include<yaml-cpp/yaml.h>
#include<iostream>



void print_yaml(const YAML::Node& root, int level){

    if(root.IsMap()){
        for(auto it  = root.begin(); it != root.end(); ++it){
            if(it->second.IsScalar()){
                std::cout << std::string(3* level,  ' ') << it->first <<": " << it->second <<  std::endl;
            }
            else
                // std::cout << std::string(3* level, ' ') << "parse : " << it->first << std::endl;
                print_yaml(it -> second, level + 1);
        }
    }
    else if(root.IsScalar()){
        std::cout <<std::string(3*level, ' ') <<  root.Scalar() << std::endl;;
    }
    else if (root.IsSequence()){
        for(size_t i = 0; i < root.size(); i++){
            print_yaml(root[i], level + 1);
        }
    }
    else if(root.IsNull()){
        std::cout <<std::string(3*level, ' ') << " null" << std::endl;
    }

}

void test_yaml(){
    YAML::Node root = YAML::LoadFile("/home/adminz/桌面/project/HighPerformanceServer/server/bin/conf/log.yml");
    //SERVER_LOG_INFO(SERVER_LOG_ROOT()) << root;
    print_yaml(root,1);
}


void test_config(){
    server::ConfigVar<int>::ptr g_int_value_config  = server::Config::lookup("system.port", int(8080), "systemport");
    server::ConfigVar<float>::ptr g_intf_value_config  = server::Config::lookup("system.port", float(8080), "systemport");  //key相同，但是类型不同,这个应该报错的
    server::ConfigVar<float>::ptr g_float_value_config  = server::Config::lookup("system.value", float(8080.11), "systemvalue");
    server::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config = server::Config::lookup("system.vec", std::vector<int>{1, 2}, "system vec");
    server::ConfigVar<std::list<int> >::ptr g_int_list_value_config = server::Config::lookup("system.list", std::list<int>{1, 2}, "system list");
    server::ConfigVar<std::set<int> >::ptr g_int_set_value_config = server::Config::lookup("system.set", std::set<int>{1, 2}, "system set");
    server::ConfigVar<std::unordered_set<int> >::ptr g_int_unorder_set_value_config = server::Config::lookup("system.unset", std::unordered_set<int>{1, 2}, "system unordered_set");
    server::ConfigVar<std::map<std::string, int> >::ptr g_string_int_map_value_config = server::Config::lookup("system.map", std::map<std::string, int>{{"a", 1}, {"b", 2}}, "system map");
    server::ConfigVar<std::vector<std::vector<int> > >::ptr g_int_vec_vec_value_config = server::Config::lookup("system.vvec", std::vector<std::vector<int> >{{1, 2}, {3, 4}}, "system vvec");

    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << g_int_value_config->getValue();
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << g_float_value_config->toString();
    //SERVER_LOG_INFO(SERVER_LOG_ROOT()) << g_intf_value_config->toString();
    
//定义一个宏方便测试：
#define XX(g_val, name, prefix) \
    { \
        auto& v = g_val->getValue(); \
        for(auto& i : v) { \
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << #prefix " " #name ": " << i; \
        } \
    }
    /*SERVER_LOG_INFO(SERVER_LOG_ROOT()) << #prefix " " #name " yaml: " << g_val->toString(); \*/
    XX(g_int_vec_value_config, vector, before);
    XX(g_int_list_value_config, list, before);
    XX(g_int_set_value_config, set, before);
    XX(g_int_unorder_set_value_config, unordered_set, before);

    auto v = g_string_int_map_value_config->getValue();
    for(auto i : v){
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "test map before:" << i.first << " " << i.second;  
    }

    YAML::Node root = YAML::LoadFile("/home/adminz/桌面/project/HighPerformanceServer/server/bin/conf/test.yml");
    server::Config::LoadFromYaml(root);

    XX(g_int_vec_value_config, vector, after);
    XX(g_int_list_value_config, list, after);
    XX(g_int_set_value_config, set, after);
    XX(g_int_unorder_set_value_config, unordered_set, after);

    v = g_string_int_map_value_config->getValue();
    for(auto i : v){
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "test map after:" << i.first << " " << i.second;  
    }
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << g_int_value_config->getValue();
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << g_float_value_config->toString();

}

void test_listener(){
    server::ConfigVar<int>::ptr g_int_value_config = server::Config::lookup("system.int", int(10), "system vec");
    g_int_value_config->addListener([](const int a, const int b){
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "g_int_value_config changed:  before:" << a << " after: " << b;
    });
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "before: " << g_int_value_config->getValue();

    YAML::Node root = YAML::LoadFile("/home/adminz/桌面/project/HighPerformanceServer/server/bin/conf/test.yml");
    server::Config::LoadFromYaml(root);

    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "after: " << g_int_value_config->getValue();

}

void test_log(){
    server::Logger::ptr system_log = SERVER_LOG_NAME("system");
    //之前没有配置system，所有会使用root的logger，也就是打印到桌面
    SERVER_LOG_INFO(system_log) << "hello system";
    //std::cout << server::loggerMgr::getInstance()->toYamlString() << std::endl;;
    YAML::Node root = YAML::LoadFile("/home/adminz/桌面/project/HighPerformanceServer/server/bin/conf/logs.yml");
    server::Config::LoadFromYaml(root);
    std::cout << root << std::endl;
    //server::Config::LoadFromYaml(root);
    std::cout << root << std::endl;
    std::cout << std::endl;
    std::cout << server::loggerMgr::getInstance()->toYamlString() << std::endl;
    SERVER_LOG_ERROR(system_log) << "hello system";
    
    system_log->setFormatter("%d -- [%p] -- %m%n");
    SERVER_LOG_ERROR(system_log) << "hello system";
}
int main(){
    //test_config();
    //test_listener();
    test_log();

    server::Config::Visit([](server::ConfigVarBase::ptr var){
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "name: " << var->getName()
                    << " \ndescription: " << var->getDescription()
                    << " \ntypename : "  << var->getTypeName()
                    << " \nvalue:\n " << var->toString();
    });
    return 0;
}
