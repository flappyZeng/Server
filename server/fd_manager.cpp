#include <sys/stat.h>  //提供fstat函数，判断当前传入的fd是文件描述符还是socket
#include"fd_manager.h"
#include"hook.h"

namespace server{
    FdCtx::FdCtx(int fd)
    :m_isInit(false)
    ,m_isSocket(false)
    ,m_sysNonblock(false)
    ,m_userNonblock(false)
    ,m_isClosed(false)
    ,m_fd(fd)
    ,m_recvTimeout(-1)
    ,m_sendTimeout(-1){
        init();
    }
    FdCtx::~FdCtx(){

    }

    bool FdCtx::init(){
        if(m_isInit){
            return true;
        }
        m_recvTimeout = -1;
        m_sendTimeout = -1;

        struct stat fd_stat;
        if(-1 == fstat(m_fd, &fd_stat)){  //返回-1表示出错
            m_isInit = false;
            m_isSocket = false;
        }else{
            m_isInit = true;
            m_isSocket = S_ISSOCK(fd_stat.st_mode);
        }
    
        if(m_isSocket){
            int flags = fcntl_f(m_fd, F_GETFL, 0);
            if(!(flags & O_NONBLOCK)){
                fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);  //不是的话，要让他非阻塞
            }
            m_sysNonblock = true;
        }else{
            m_sysNonblock = false;
        }
        m_userNonblock = false;
        m_isClosed = false;
        return m_isInit;

    }
    bool FdCtx::close(){

    }

    void FdCtx::setTimeout(int type, uint64_t v){
        if(type == SO_RCVTIMEO){
            m_recvTimeout = v;
        }else{
            m_sendTimeout = v;
        }
    }
    uint64_t FdCtx::getTimeout(int type){
        if(type == SO_RCVTIMEO){
            return m_recvTimeout;
        }else{
            return m_sendTimeout;
        }
    }

    FdManager::FdManager(){
        m_datas.resize(64);
    }
    FdCtx::ptr FdManager::get(int fd, bool auto_create){
        RWMutexType::ReadLock lock(m_mutex);
        if(m_datas.size() <= fd){
            if(auto_create = false){
                return nullptr;
            }

        } else{
            if(m_datas[fd] || !auto_create){
                return m_datas[fd];
            }
        }
        lock.unLock();

        RWMutexType::WriteLock rlock(m_mutex);
        FdCtx::ptr ctx(new FdCtx(fd));
        if(m_datas.size() <= fd){
            m_datas.resize(fd * 2);
        }
        m_datas[fd] = ctx;
        return ctx;
    }
    void FdManager::del(int fd){
        RWMutexType::WriteLock lock(m_mutex);
        if(m_datas.size() <= fd){
            return;
        }
        m_datas[fd].reset();
    }
}