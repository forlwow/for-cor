#include "timer.h"
#include "ethread.h"
#include <cstdint>
#include <memory>
#include <sys/socket.h>
#include <vector>

namespace server::util {

bool Timer::Compare::operator()(const Timer::ptr& lp, const Timer::ptr& rp) const {
    if(!lp && !rp)
        return false;
    if(!lp)
        return true;
    if(!rp)
        return false;
    if(lp->m_next < rp->m_next)
        return true;
    if(rp->m_next < lp->m_next)
        return false;
    return lp < rp;
}

Timer::Timer(uint64_t ms, std::function<void()> cb, bool circulate, TimerManager* manager)
    : m_ms(ms), m_circulate(circulate), m_manager(manager)
{
    m_next = TimerManager::GetCurTimeStamp() + ms;
    if(m_circulate){
        m_cb = [cb, this]{
            cb();
            this->refresh();
            if(this->m_manager)
                this->m_manager->addTimer(shared_from_this());
        };
    }
    else {
        m_cb = cb;
    }
}

Timer::Timer(uint64_t next_time)
    :m_next(next_time), m_circulate(false)
{

}

void Timer::refresh(){
    m_next = TimerManager::GetCurTimeStamp() + m_ms;
}

bool Timer::InsertIntoManager(Timer::ptr timer){
    if(timer->m_manager){
        timer->m_manager->addTimer(timer);
        return true;
    }
    return false;
}

TimerManager::TimerManager(){

}

TimerManager::~TimerManager(){

}

void TimerManager::OnInsertAtFront(){

}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool circulate){
    Timer::ptr timer = std::make_shared<Timer>(ms, cb, circulate, this);
    WriteLockGuard wlock(m_mutex);

    auto it = m_timers.insert(timer).first;
    bool tri = it == m_timers.begin();
    wlock.unlock();
    if(tri)
        OnInsertAtFront();
    return timer;
}

void TimerManager::addTimer(Timer::ptr timer){
    WriteLockGuard wlock(m_mutex);
    m_timers.insert(timer);
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool circulate){
    WriteLockGuard wlock(m_mutex);
    return addTimer(ms, 
            [=]{
            if(weak_cond.lock())
                cb();
            }, 
            circulate
    );
}

std::vector<Timer::ptr> TimerManager::GetExpireTimers(){
    std::vector<Timer::ptr> res;
    Timer::ptr ttime(new Timer(GetCurTimeStamp()));
    ReadLockGuard rlock(m_mutex);
    auto iter = m_timers.upper_bound(ttime);
    for(auto it = m_timers.begin(); it != iter;++it){
        res.push_back(*it);
    }
    m_timers.erase(m_timers.begin(), iter);
    return res;
}

uint64_t TimerManager::GetNextTimeDuration(){
    ReadLockGuard rlock(m_mutex);
    auto iter = m_timers.begin();
    if(iter == m_timers.end())
        return UINT64_MAX;
    uint64_t now = GetCurTimeStamp(), next = (*iter)->m_next;
    if(next > now)
        return next - now;
    else
        return 0;
}

void TimerManager::AutoStop(){
    WriteLockGuard wlock(m_mutex);
    for(auto &timer : m_timers){
        timer->m_circulate = false;
        timer->m_manager = nullptr;
    }
}

} // namespace server
