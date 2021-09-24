#pragma once 

//一个配置解析类
#include<memory> //智能指针
#include<string.h>
#include<sstream>
#include <boost/lexical_cast.hpp>   //提供安全的类型转换，在转换过程中会抛出异常
#include<yaml-cpp/yaml.h>
#include<functional>
#include"log.h"
#include"thread.h"

//复杂类型转换支持：
#include<vector>
#include<map>
#include<set>
#include<list>
#include<unordered_map>
#include<unordered_set>

namespace server{
    //支持简单类型的转换：
    //F : From_type, T : To_type
    template<class F, class T>
    class LexicalCast{
    public:
        T operator()(const F& v){
            return boost::lexical_cast<T>(v);
        }
    };

    //支持常用类型的模板特例化(模板偏特化)

    //1. vector类型
    // string  -> vector 
    template<class T>
    class LexicalCast<std::string, std::vector<T> >{
    public:
        std::vector<T> operator()(const std::string& v){
            YAML::Node node = YAML::Load(v);  //使用YAML::Load 将string转换为node结构
            typename std::vector<T> vec;      //使用typename的目的是标识模板类型名
            std::stringstream ss;
            for(size_t i = 0; i < node.size(); ++i){
                //因为node的每一个子节点还有可能是一个复合类型， 例如vector < vector<int> >转化而来的node,需要继续解析
                //另外，为了支持递归调用lexical,所以需要将node[i]序列化为string.
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };
    // vector -> vector
    template<class T>
    class LexicalCast<std::vector<T> , std::string>{
    public:
        std::string  operator()(const std::vector<T>& v){
            YAML::Node node;
            for(auto& i : v){
                //Node的push_back方法支持将任意类型转换为yaml格式, T也有可能是一个复杂类型，所以还要递归解析
                //原作者的写法是： node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
                node.push_back(LexicalCast<T, std::string>()(i));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    
    //2.list类型
    // string -> list 
    template<class T>
    class LexicalCast<std::string, std::list<T> >{
    public:
        std::list<T> operator()(const std::string& v){
            YAML::Node node = YAML::Load(v);  //使用YAML::Load 将string转换为node结构
            typename std::list<T> lst;      //使用typename的目的是标识模板类型名
            std::stringstream ss;
            for(size_t i = 0; i < node.size(); ++i){
                ss.str("");
                ss << node[i];
                lst.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return lst;
        }
    };
    // list ->  string 
    template<class T>
    class LexicalCast<std::list<T> , std::string>{
    public:
        std::string  operator()(const std::list<T>& v){
            YAML::Node node;
            for(auto& i : v){
                node.push_back(LexicalCast<T, std::string>()(i));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    //3.set类型：
    //string -> set
    template<class T>
    class LexicalCast<std::string, std::set<T> >{
    public:
        std::set<T> operator()(const std::string& v){
            YAML::Node node = YAML::Load(v);  //使用YAML::Load 将string转换为node结构
            typename std::set<T> lst;      //使用typename的目的是标识模板类型名
            std::stringstream ss;
            for(size_t i = 0; i < node.size(); ++i){
                ss.str("");
                ss << node[i];
                lst.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return lst;
        }
    };
    // set -> string
    template<class T>
    class LexicalCast<std::set<T> , std::string>{
    public:
        std::string  operator()(const std::set<T>& v){
            YAML::Node node;
            for(auto& i : v){
                node.push_back(LexicalCast<T, std::string>()(i));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 4.unordered_set类型
    //  string -> unordered_set
    template<class T>
    class LexicalCast<std::string, std::unordered_set<T> >{
    public:
        std::unordered_set<T> operator()(const std::string& v){
            YAML::Node node = YAML::Load(v);  //使用YAML::Load 将string转换为node结构
            typename std::unordered_set<T> lst;      //使用typename的目的是标识模板类型名
            std::stringstream ss;
            for(size_t i = 0; i < node.size(); ++i){
                ss.str("");
                ss << node[i];
                lst.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return lst;
        }
    };
    
    //  unordered_set -> string
    template<class T>
    class LexicalCast<std::unordered_set<T> , std::string>{
    public:
        std::string  operator()(const std::unordered_set<T>& v){
            YAML::Node node;
            for(auto& i : v){
                node.push_back(LexicalCast<T, std::string>()(i));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    

    // 5.map<string, T>类型
    // string -> map<string, T>
    template<class T>
    class LexicalCast<std::string, std::map<std::string, T> >{
    public:
        std::map<std::string, T> operator()(const std::string& v){
            YAML::Node node = YAML::Load(v);  //使用YAML::Load 将string转换为node结构
            typename std::map<std::string, T> mp;      //使用typename的目的是标识模板类型名
            std::stringstream ss;
            for(auto it = node.begin(); it != node.end(); ++it){
                ss.str("");
                ss << it->second;
                mp.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }
            return mp;
        }
    };
    // map<string, T>  -> string 
    template<class T>
    class LexicalCast<std::map<std::string, T> , std::string>{
    public:
        std::string  operator()(const std::map<std::string, T>& v){
            YAML::Node node;
            for(auto& i : v){
                node[i.first] = LexicalCast<T, std::string>()(i.second);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    
    // 6. unordered_map<int, T>类型
    // string -> unordered_map<int, T>
    template<class T>
    class LexicalCast<std::string, std::unordered_map<int, T> >{
    public:
        std::unordered_map<int, T> operator()(const std::string& v){
            YAML::Node node = YAML::Load(v);  //使用YAML::Load 将string转换为node结构
            typename std::unordered_map<int, T> mp;      //使用typename的目的是标识模板类型名
            std::stringstream ss;
            for(auto it = node.begin(); it != node.end(); ++it){
                ss.str("");
                ss << it->second;
                mp.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(it->second)));
            }
            return mp;
        }
    };
    // string -> unordered_map<string, T>
    template<class T>
    class LexicalCast<std::unordered_map<int, T> , std::string>{
    public:
        std::string  operator()(const std::unordered_map<int, T>& v){
            YAML::Node node;
            for(auto& i : v){
                node[i.first] = LexicalCast<T, std::string>()(i.second);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 7. unordered_map<std::string, T>类型
    // string -> unordered_map<std::string, T>
    template<class T>
    class LexicalCast<std::string, std::unordered_map<std::string, T> >{
    public:
        std::unordered_map<std::string, T> operator()(const std::string& v){
            YAML::Node node = YAML::Load(v);  //使用YAML::Load 将string转换为node结构
            typename std::unordered_map<std::string, T> mp;      //使用typename的目的是标识模板类型名
            std::stringstream ss;
            for(auto it = node.begin(); it != node.end(); ++it){
                ss.str("");
                ss << it->second;
                mp.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(it->second)));
            }
            return mp;
        }
    };
    // string -> unordered_map<string, T>
    template<class T>
    class LexicalCast<std::unordered_map<std::string, T> , std::string>{
    public:
        std::string  operator()(const std::unordered_map<int, T>& v){
            YAML::Node node;
            for(auto& i : v){
                node[i.first] = LexicalCast<T, std::string>()(i.second);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    //如果存在自定义类型，要实现具体的自定义类型的leixcal_cast的偏特化版本
    //实现后即可支持自定义类型。


    class ConfigVarBase{
    public:
        typedef std::shared_ptr<ConfigVarBase>ptr;
        ConfigVarBase(const std::string& name, const std::string& description = "")
            :m_name(name)
            ,m_description(description){
                std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);  //转成小写字母,减少复杂度
            }
        virtual ~ConfigVarBase() {};

        const std::string getName() const {return m_name;};
        const std::string getDescription() const {return m_description;};

        //序列化和反序列化
        //m_val格式序列化为string;
        virtual std::string toString() = 0;
        //string格式反序列化为m_val;
        virtual bool fromString(const std::string& val) = 0;
        virtual std::string getTypeName() const  = 0;
    protected:
        std::string m_name;
        std::string m_description;
    };

    template<class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string> >
    class ConfigVar : public ConfigVarBase{
    public: 
        typedef RWMutex RWMutexType;
        typedef std::shared_ptr<ConfigVar>ptr;
        //日志监听函数
        typedef std::function<void (const T& old_value, const T& new_value)>  on_change_cb;
        ConfigVar(const std::string& name
                , const T& value
                , const std::string& description = "")
            :ConfigVarBase(name, description)
            ,m_val(value){
                //std::cout<<m_val << std::endl;
        }

        std::string toString() override{
            //基于boost库的lexical_cast进行类型转换（原理是使用字节流 + 异常抛出）: T -> string
            try{
                //return boost::lexical_cast<std::string>(m_val);
                //读取m_val需要加读锁
                RWMutexType::ReadLock lock(m_mutex);
                return ToStr()(m_val);
            }
            catch(std::exception& e){
                SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "ConfigVar::toString exception"
                << e.what() << "convert"  << typeid(m_val).name() << " to string";
            }
            return "";
        }

        bool fromString(const std::string& val) override{
            //基于boost库的lexical_cast进行类型转换（原理是使用字节流 + 异常抛出）string ->  T
            try{
                //m_val = boost::lexical_cast<T>(val);
                //写值，需要加写锁（m_val可能是复杂类型)
                RWMutexType::WriteLock lock(m_mutex);
                setValue(FromStr()(val));
                return true;
            }
            catch(std::exception& e){
                SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "ConfigVar::fromString exception"
                << e.what() << "convert string to" << typeid(m_val).name() ;
            }
            return false;
        }

        const T getValue()  { //这里的const因为使用了锁，就不能加上了, 这里可以把锁的类型改称mutable
            RWMutexType::ReadLock lock(m_mutex);
            return m_val;
        }
        void setValue(const T& val) {
            //{}添加局部作用域
            {
                //前面判断加读锁
                RWMutexType::ReadLock lock(m_mutex);
                if(val == m_val){
                    return;
                }
                for(auto& i : m_cbs){
                    i.second(m_val ,val);
                }
            }
            //写入变量加写锁
            RWMutexType::WriteLock lock(m_mutex);
            m_val = val;
        }
        std::string getTypeName() const override {return typeid(T).name();}

        //日志监听器：
        uint64_t addListener(on_change_cb cb){
            static uint64_t m_cb_id = 0;
            RWMutexType::WriteLock lock(m_mutex);
            m_cb_id ++;
            m_cbs[m_cb_id] = cb;
            return m_cb_id;
        }
        on_change_cb getListener(uint64_t key){
            //读锁
            RWMutexType::ReadLock lock(m_mutex);
            auto cb = m_cbs.find(key);
            return cb == m_cbs.end() ? nullptr : cb->second;
        }
        void delListener(uint64_t key){
            //写锁
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.erase(key);
        }
        void clearListener(){
            //写锁
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.clear();
        }
    private:
        T m_val;
        std::map<uint64_t, on_change_cb>m_cbs;
        RWMutexType m_mutex;
    };

    class Config{
    public:
        typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
        typedef RWMutex RWMutexType;
        //下面使用typename是为了避免ConfigVar<T>::ptr被解析成其他的格式，告知编译器这是一个类型名
        //给s_datas添加对应的类型
        template<class T>
        static typename ConfigVar<T>::ptr lookup(const std::string& name, const T& value, const std::string& description = ""){
            //是用下面的方式会有key相同，但是类型不同，也能返回的隐患
            /*
            auto temp = lookup<T>(name);
            //已经存在的话就地返回
            if(temp){
                //日志记录转换器存在
                SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "lookup name : " << name << "exists";
                return temp;
            }
            */
           {
               //读锁
               RWMutexType::ReadLock lock(getMutex());
                auto it = getDatas().find(name);
                if(it != getDatas().end()){
                    auto temp = std::dynamic_pointer_cast<ConfigVar<T> >(it -> second);
                    if(temp){
                        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "lookup name : " << name << "exists";
                        return temp;
                    }
                    else{
                        SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "lookup name(" << name << ") exists but type not match， The need type is "
                            << typeid(T).name() << " but the real type is " << (it->second)->getTypeName();
                        return nullptr;
                    }
                }
                //检查命名规范
                if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._123456789")
                    != std::string::npos){
                        SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "lookup name invalid " << name;
                        throw std::invalid_argument(name);
                }
           }
            //写锁
            RWMutexType::WriteLock lock(getMutex());
            //不存在的话新建一个
            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, value, description));
            getDatas()[name] = v;
            return v;   //这个地方犯了一个错误，忘了返回这个v，居然不报错。。。
        }

        //查找对应的类型转换器
        template<class T>
        static typename ConfigVar<T>::ptr lookup(const std::string& name){
            RWMutexType::ReadLock lock(getMutex());
            getDatas();
            auto it = getDatas().find(name);
            if(it == getDatas().end()){
                //查找的类型不存在
                return nullptr;
            }
            //dynamic_pointer_cast是将智能指针转换为对应类型的智能指针。
            return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
        }

        static ConfigVarBase::ptr lookupBase(const std::string& name);
        static void LoadFromYaml(const YAML::Node& node);

        static void Visit(std::function<void(ConfigVarBase::ptr)>cb);
    private:
        //static ConfigVarMap s_datas;  这个静态成员的初始化顺序可能不确定，因为工程中存在多个静态成员，别的静态成员可能调用这个静态成员，但是可能还没有初始化，改成一个静态方法。
        static ConfigVarMap& getDatas() {
            static ConfigVarMap s_datas;
            return s_datas;
        }
        static RWMutexType& getMutex(){
            static RWMutexType s_mutex;
            return s_mutex;
        }
    };
}