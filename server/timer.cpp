#include"timer.h"
#include"utils.h"
#include"log.h"
#include<algorithm>

namespace server{
    static server::Logger::ptr g_logger =  SERVER_LOG_ROOT();
    bool Timer::Comparator::operator()(const Timer::ptr lhs, const Timer::ptr rhs) const{
        if(!lhs && !rhs){
            return false;
        }
        if(!lhs){
            return true;
        }
        if(!rhs){
            return false;
        }
        if(lhs->m_next < rhs->m_next){
            return true;
        }
        if(lhs->m_next > rhs->m_next){
            return false;
        }
        return lhs.get() < rhs.get();
    }

    Timer::Timer(uint64_t ms, std::function<void()> cb, bool recuring, TimerManager* manager)
        : m_ms(ms)
        , m_cb(cb)
        , m_recurring(recuring)
        , m_manager(manager){
        m_next = GetCurrentMs() + m_ms;
    }

    Timer::Timer(uint64_t next)
        : m_next(next){
    }

    bool Timer::cancel(){
        TimerManager::MutexType::WriteLock lockr(m_manager->m_mutex);
        if(m_cb){
            m_cb = nullptr;
            m_manager->m_timers.erase(shared_from_this());
            return true;
        }
        return false;
    }

    bool Timer::refresh(){
        //SERVER_LOG_INFO(g_logger) << "enter " << m_manager->m_timers.size();
        TimerManager::MutexType::WriteLock lockr(m_manager->m_mutex);
        //SERVER_LOG_INFO(g_logger) << "enter 2" ;
        if(!m_cb){
            return false;
        }
        //SERVER_LOG_INFO(g_logger) << "enter 3" ;
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end()){
            return false;
        }   
        //SERVER_LOG_INFO(g_logger) << "enter 4" ;
        //先删除再更新再插入的原因是因为set基于m_next排序，如果不这样子会导致内部顺序混乱
        m_manager->m_timers.erase(it);
        //SERVER_LOG_INFO(g_logger) << "enter 5" ;
        m_next = GetCurrentMs() + m_ms;
        //SERVER_LOG_INFO(g_logger) << "enter 5.5" ;
        try{
            m_manager->m_timers.insert(shared_from_this());
        }catch(std::exception &e){
            SERVER_LOG_INFO(g_logger) << e.what();
        }
        //SERVER_LOG_INFO(g_logger) << "enter 6" ;
        //SERVER_LOG_INFO(g_logger) << "m_timer.size = " << m_manager->m_timers.size();
        return true;
    }

    bool Timer::reset(uint64_t ms, bool from_now){
        //时间没变，直接返回
        if(ms == m_ms && !from_now){
            return true;  
        }
        TimerManager::MutexType::WriteLock lockr(m_manager->m_mutex);
        if(!m_cb){
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end()){
            return false;
        }

        //先删除再更新再插入的原因是因为set基于m_next排序，如果不这样子会导致内部顺序混乱
        m_manager->m_timers.erase(it);
        
        uint64_t start = 0;
        if(from_now){
            start = GetCurrentMs();
        }else{
            start = m_next - m_ms;
        }
        m_ms = ms;
        m_next = start + m_ms;
        m_manager->addTimer(shared_from_this(), lockr);  //防止新的定时器插入到set首位
        SERVER_LOG_INFO(g_logger) << "m_timer.size = " << m_manager->m_timers.size();
        return true;
    }   

    TimerManager::TimerManager(){
        m_previousTime = GetCurrentMs();
    }

    TimerManager::~TimerManager(){

    }

    void TimerManager::addTimer(Timer::ptr val, TimerManager::MutexType::WriteLock& lock){
        auto it = m_timers.insert(val).first;  //insert返回pair<iterator, bool> 表示是否成功以及对应的迭代器位置
        bool at_front = (it == m_timers.begin()) && !m_tickled;  //判断是否插入到最前方的位置（成为最早触发的定时器）
        if(at_front){  //当前定时器下第一次提醒插入到前面
            m_tickled = true;
        }
        lock.unLock();

        if(at_front){
            onTimerInsertedAtFront();  //如果插入到最前方的位置，提醒进程内部更新定时器的ddl；
        }
    }
    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recuring){
        //SERVER_LOG_INFO(g_logger) << "add timer, ms =  " << ms;
        MutexType::WriteLock lock(m_mutex);
        Timer::ptr timer(new Timer(ms, cb, recuring, this));
        addTimer(timer, lock);
        return timer;  //允许管理器取消定时器
    }

    static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()>cb){
        std::shared_ptr<void> temp = weak_cond.lock();
        if(temp){
            cb();
        }
    }

    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond,bool recuring){
        return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recuring);
    }

    uint64_t TimerManager::getNextTimer(){
        MutexType::ReadLock lock(m_mutex);
        //更新定时器时
        m_tickled = false; //获取下一个定时器的时候要更新tickle状态
        if(m_timers.empty()){ //不含有下一个计时器
            return ~0ul;  //最大的数值
        }
        const Timer::ptr& timer = *m_timers.begin();
        uint64_t now = GetCurrentMs();

        if(timer->m_next <= now){
            //超时了；
            return 0;
        }
        return timer->m_next - now;
    }

    void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs){

        uint64_t now_ms = GetCurrentMs();
        std::vector<Timer::ptr>expires;
        {
            MutexType::ReadLock lockr(m_mutex);
            if(m_timers.empty()){
                return;
            }
        }
        MutexType::WriteLock lockw(m_mutex);
        //SERVER_LOG_INFO(g_logger) << "nowtimer message : " << GetCurrentMs();
        if(m_timers.empty()){
            return;
        }
        Timer::ptr now_timer(new Timer(GetCurrentMs()));
        auto it = m_timers.lower_bound(now_timer);
        //SERVER_LOG_INFO(g_logger) << "is end : " << (it == m_timers.end());
        while(it != m_timers.end() && (*it)->m_next == now_ms){
            ++it;
        }

        expires.insert(expires.begin(), m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);

        for(auto& timer : expires){
            cbs.push_back(timer->m_cb);
            //如果是循环计时器，就重新加上
            if(timer->m_recurring){
                timer->m_next = timer->m_ms + GetCurrentMs();
                m_timers.insert(timer);
            }else{
                timer->m_cb = nullptr;
            }
        }

    }

    bool TimerManager::hasTimer(){
        MutexType::ReadLock lock(m_mutex);

        return !m_timers.empty();
    }

    bool TimerManager::detectClockRollover(uint64_t now_ms){
        bool rollover = false;
        if(now_ms < m_previousTime && 
                now_ms < (m_previousTime  - 60 * 60 * 1000)){
            rollover = true;
        }
        m_previousTime = now_ms;
        return rollover;
    }
}