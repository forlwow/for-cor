#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <sys/types.h>
#include "ethread.h"
#include <stdio.h>
#include <sys/ucontext.h>
#include <vector>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

class Timer{
public:
    Timer()
        : start_time(), end_time()
    {}
    ~Timer(){}
    void start_count(){
        start_time = std::chrono::high_resolution_clock::now();
    }
    void end_count(){
        end_time = std::chrono::high_resolution_clock::now();
    }

    template<typename T = std::chrono::milliseconds>
    auto get_duration(){
        return std::chrono::duration_cast<T>(
                end_time - start_time);
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> end_time;

};

namespace server {

class TimerManager;
class Timer: public std::enable_shared_from_this<server::Timer>{
friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;
    std::function<void()> GetFunc(){return m_cb;}
    inline bool IsCirculate(){return m_circulate;}
    inline void refresh();
    static bool InsertIntoManager(Timer::ptr timer);

    ~Timer()=default;
private:
    Timer(uint64_t ms, std::function<void()> cb, bool circulate = false, 
            TimerManager* manager = nullptr);
    Timer(uint64_t next_time);
    struct Compare{
        bool operator()(const Timer::ptr& lp, const Timer::ptr& rp) const;
    };

private:
    uint64_t m_ms;              // 周期
    uint64_t m_next;            // 执行的时间点
    bool m_circulate;           // 是否循环定时器
    TimerManager* m_manager;    // 
    std::function<void()> m_cb; // 回调
};


class TimerManager{
friend class Timer;
public:
    typedef std::shared_ptr<TimerManager> ptr;
    typedef RWMutex RWMutexType;

    TimerManager();
    virtual ~TimerManager();
    static uint64_t GetCurTimeStamp(){
        auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();
        if(now <= 0)
            return UINT64_MAX;
        else
            return (uint64_t)now;
    }

    Timer::ptr addTimer(uint64_t ms, std::function<void()>, bool circulate = false);
    void addTimer(Timer::ptr timer);
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()>, std::weak_ptr<void> weak_cond, bool circulate);
    std::vector<Timer::ptr> GetExpireTimers();          // 获取已经到期的timer
    uint64_t GetNextTimeDuration();                     // 获取下个定时器触发的剩余时间
protected:
    void OnInsertAtFront();

    void AutoStop();

private:
    RWMutexType m_mutex;
    std::set<Timer::ptr, Timer::Compare> m_timers;
};

} // namespace server

#endif 
