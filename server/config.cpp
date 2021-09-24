#include"config.h"

namespace server{
    //类的静态成员变量一定要初始化，私有的静态成员变量也可以在类外初始化。
    //Config::ConfigVarMap Config::s_datas;

    ConfigVarBase::ptr Config::lookupBase(const std::string& name){
        RWMutexType::ReadLock lock(getMutex());
        auto s_datas = getDatas();
        auto it = s_datas.find(name);
        return it == s_datas.end() ? nullptr : it -> second;
    }
    static void ListAllMembers(const std::string& prefix,const YAML::Node& node, 
                              std::list<std::pair<std::string,const YAML::Node> >& output){
        //prefix名称校验
        if(prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._123456789")
                != std::string::npos){
                    SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "lookup name invalid " << prefix << " " << node;
                    return ;
        }
        output.push_back(std::make_pair(prefix, node));
        //底层搜索
        if(node.IsMap()){
            for(auto it = node.begin(); it != node.end(); it++){
                ListAllMembers(prefix.empty() ? it->first.Scalar():(prefix + "." + it->first.Scalar()), it->second, output);
            }
        }
    }
    void Config::LoadFromYaml(const YAML::Node& node){
        std::list<std::pair<std::string,const YAML::Node> >all_nodes;
        ListAllMembers("", node, all_nodes);
        auto s_datas = Config::getDatas();
        for(auto& i : all_nodes){
            std::string key = i.first;
            if(key.empty())
                continue;
            //全部替换成标准的小写字母
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            
            //在配置变量名中查找
            ConfigVarBase::ptr val = lookupBase(key);
            if(val){
                if(i.second.IsScalar()){
                    val->fromString(i.second.Scalar());
                }
                else{
                    //这一段暂时应该有问题 (没有问题)
                    std::stringstream ss;
                    ss << i.second;
                    val->fromString(ss.str());
                }
            }
        }
    }

    void Config::Visit(std::function<void(ConfigVarBase::ptr)>cb){
        RWMutexType::ReadLock lock(getMutex());
        auto& m = getDatas();
        for(auto it = m.begin(); it != m.end(); it++){
            cb(it->second);
        }
    }
}