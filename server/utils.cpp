#include<execinfo.h>
#include<sys/time.h>
#include"utils.h"
#include"log.h"


namespace server{

    static server::Logger::ptr g_logger = SERVER_LOG_NAME("system");
    //获取线程id
    pid_t GetThreadId(){
        return syscall(SYS_gettid);
    }

    void BackTrace(std::vector<std::string>& bt, int size, int skip){
        void **array = (void**)malloc((sizeof(void*)) * size);
        size_t s = ::backtrace(array, size);
        char** strings = backtrace_symbols(array, s);

        if(strings == nullptr){
            SERVER_LOG_ERROR(g_logger) << "backtrace_symbols error";
        }
        for(size_t i = skip; i < s; ++i){
            bt.push_back(strings[i]);
        }
        free(array);
        free(strings);
    }
    std::string BackTrace(int size, int skip, const std::string& prefex){
        std::vector<std::string>bt;
        BackTrace(bt, size, skip);
        std::stringstream ss;
        for(std::string& str : bt){
            ss << prefex << str << std::endl;
        }
        return ss.str();
    }

    uint64_t GetCurrentMs(){
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
    }

    uint64_t GetCurrentUs(){
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return tv.tv_sec * 1000 * 1000ul+ tv.tv_usec;
    }

   
}