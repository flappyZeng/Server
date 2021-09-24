#pragma once 

#include<functional>
#include<memory>
#include<string>
#include<vector>
#include<unordered_map>


#include"http.h"
#include"httpsession.h"
#include"../thread.h"


namespace server{
namespace http{

class Servlet{
public:
    typedef std::shared_ptr<Servlet> ptr;

    Servlet(const std::string& name)
        : m_name(name){  
    }

    virtual ~Servlet() {}

    virtual int32_t handle(server::http::HttpRequest::ptr request
                 , server::http::HttpResponse::ptr response
                 , server::http::HttpSession::ptr session) = 0;
    
    const std::string& getName() const {return m_name;}
    
protected:
    std::string m_name;
};

class FunctionServlet : public Servlet{
public:
    typedef std::shared_ptr<FunctionServlet> ptr;
    typedef std::function<int32_t (server::http::HttpRequest::ptr request
                 , server::http::HttpResponse::ptr response
                 , server::http::HttpSession::ptr session)>callback;

    FunctionServlet(callback cb);
    int32_t handle(server::http::HttpRequest::ptr request
                 , server::http::HttpResponse::ptr response
                 , server::http::HttpSession::ptr session) override;
private:
    callback m_cb;
};

class ServletDispatch : public Servlet{
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef server::RWMutex RWMutexType;
    
    ServletDispatch();

    int32_t handle(server::http::HttpRequest::ptr request
                 , server::http::HttpResponse::ptr response
                 , server::http::HttpSession::ptr session) override;

    void addServlet(const std::string& uri, Servlet::ptr slt);
    void addServlet(const std::string& uri, FunctionServlet::callback cb);
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    Servlet::ptr getServlet(const std::string& uri);
    Servlet::ptr getGlobServlet(const std::string& uri);
    Servlet::ptr getMatchedServlet(const std::string& uri);

    Servlet::ptr getDefault() const {return m_default;}
    void setDefault(Servlet::ptr v) { m_default = v;}


private:
    RWMutexType m_mutex;
    //url  --> servlet 精确匹配 优先级高
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    //uri  --> servlet 模糊匹配
    std::vector< std::pair<std::string , Servlet::ptr > > m_globs;
    //默认serlvet
    Servlet::ptr m_default;
};

class NotFoundServlet : public Servlet{
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;
    NotFoundServlet(const std::string& name);
    int32_t handle(server::http::HttpRequest::ptr request
                 , server::http::HttpResponse::ptr response
                 , server::http::HttpSession::ptr session) override;
private:
    std::string m_content;
    std::string m_name;
};
}
}