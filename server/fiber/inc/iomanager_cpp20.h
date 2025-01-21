#include "async.h"
#include "scheduler.h"
#include <ctime>
#include <sys/types.h>
#include <utility>
#if __cplusplus >= 202002L
#ifndef IOMANAGER_20_H
#define IOMANAGER_20_H

#include "ethread.h"
#include "fiber_cpp20.h"
#include "scheduler_cpp20.h"
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_map>
#include <set>


namespace server {

struct TimeEvent{
    typedef std::shared_ptr<TimeEvent> ptr;
    static TimeEvent::ptr GetNow(){
        timeval cur;
        gettimeofday(&cur, NULL);
        return std::make_shared<TimeEvent>(cur);
    }
    TimeEvent(timeval t): m_until(t){}

    TimeEvent(int m_ms, bool is_cir, Fiber_::ptr cb)
        : m_cycle_ms(m_ms), m_iscirculate(is_cir), m_cb(cb)
    {
        gettimeofday(&m_until, NULL);
        m_until.tv_sec += m_ms / 1000;
        m_until.tv_usec += (m_ms % 1000) * 1000;
    }
    void refresh(){
        gettimeofday(&m_until, NULL);
        m_until.tv_sec += m_cycle_ms / 1000;
        m_until.tv_usec += (m_cycle_ms % 1000) * 100;
    }

    int getLeftTime(){
        timeval cur;
        gettimeofday(&cur, NULL);
        int res = ((m_until.tv_sec - cur.tv_sec) * 1000 ) + ((m_until.tv_usec - cur.tv_usec) / 1e3);
        return res;
    }

    void trigger(){
        if(!m_cb->done())
            m_cb->swapIn();
    }

    bool operator<(const TimeEvent& that){
        if (getSec() < that.getSec()){
            return true;
        }
        else if(getSec() > that.getSec()){
            return false;
        }
        else{
            if(getUSec() < that.getUSec()){
                return true;
            }
            else{
                return false;
            }
        }
    }
    
    struct TimerCompareLess{
        bool operator() (const TimeEvent::ptr& l, const TimeEvent::ptr& r) const {
            if (l->getSec() < r->getSec()){
                return true;
            }
            else if(l->getSec() > r->getSec()){
                return false;
            }
            else{
                if(l->getUSec() < r->getUSec()){
                    return true;
                }
                else{
                    return false;
                }
            }
        }
    };

    Fiber_::ptr GetFunc() {return m_cb;}
    const __time_t getSec() const {return m_until.tv_sec;}
    const __suseconds_t getUSec() const {return m_until.tv_usec;}

    bool m_iscirculate = false;
    uint32_t m_cycle_ms = 0;
    timeval m_until;
    Fiber_::ptr m_cb = nullptr;
};


// 定时器管理器
// 不提供线程安全
class TimeManager{
public:
    TimeManager()=default;
    void emplaceTimer(int m_ms, bool is_cir, Fiber_::ptr cb){
        m_times.emplace(new TimeEvent(m_ms, is_cir, cb));
    }
    void addTimer(TimeEvent::ptr timer){
        m_times.insert(timer);
    }
    void addTimer(int m_ms, bool is_cir, std::function<void()> cb){
        Fiber_::ptr fib = FuncFiber::ptr(new FuncFiber(cb));
        auto te = TimeEvent::ptr(new TimeEvent(m_ms, is_cir, fib));
        m_times.insert(te);
    }

    // 获取已经过期的定时器
    std::vector<TimeEvent::ptr> GetExpireTimers(){
        std::vector<TimeEvent::ptr> res;
        auto curtime = TimeEvent::GetNow();
        auto iter = m_times.upper_bound(curtime);
        for(auto it = m_times.begin(); it != iter;++it){
            res.push_back(*it);
            if ((*it)->m_iscirculate){
                m_times.insert(*it);
            }
        }
        m_times.erase(m_times.begin(), iter);
        return res;
    }

    // 获取最近过期剩余时间
    int GetNextTimeDuration(){
        if(m_times.empty()){
            return -1;
        }
        return (*m_times.begin())->getLeftTime();
    }

private:
    std::set<TimeEvent::ptr, TimeEvent::TimerCompareLess> m_times;
};


// 由于每个Fiber对象可能会在短时间内被多次调度
// 因此使用Fiber2
class IOManager_: public Scheduler_, public TimeManager{
public:
    typedef std::shared_ptr<IOManager_> ptr;
    typedef RWMutex RWMutexType;

    enum Event{
        NONE    = 0b000,
        READ    = EPOLLIN,
        WRITE   = EPOLLOUT
    };

private:
    struct FdContext{
        typedef std::shared_ptr<FdContext> ptr;
        struct EventContext{
            // fiber可能会被多线程调用
            Scheduler_* scheduler = nullptr;
            Fiber_::ptr fiber;
            EventContext()=default;
            void reset() {scheduler = nullptr, fiber.reset();}
            // void reset() {scheduler = nullptr, fibers.clear();}
        };
        FdContext()=delete;
        FdContext(int fd_):fd(fd_){}
        ~FdContext(){}
        int fd = -1;
        Event events = NONE;
        EventContext read;
        EventContext write;
        EventContext& GetEventContext(Event event) {
            switch (event) {
                case READ:
                    return read;
                case WRITE:
                    return write;
                default:
                    assert(0);
            }
        }
        void ResetEventContext(Event event){
            GetEventContext(event).reset();
        }
        void TriggerEvent(Event event){
            EventContext& evt = GetEventContext(event);
            evt.scheduler->schedule(evt.fiber);
            // events = (Event)(events & ~event);
        }
        // 只在一个线程中操作 不需要锁
    };
public:
    IOManager_(size_t threads_ = 1, const std::string& name_ = "Schedule");
    ~IOManager_() override;

    int AddEvent(int fd, Event event, Fiber_::ptr cb);
    bool DelEvent(int fd, Event event);
    bool DelFd(int fd);
    bool CancelEvent(int fd, Event event);

    static IOManager_* GetIOManager(){return dynamic_cast<IOManager_*>(Scheduler_::GetScheduler());}

    void start() override;

    void addTimer(int m_ms, bool is_cir, std::function<void()> cb){
        TimeManager::addTimer(m_ms, is_cir, std::move(cb));
        if (m_ms < 3000){
            interruptEpoll();
        }
    }

    void addTimer(TimeEvent::ptr timer){
        TimeManager::addTimer(std::move(timer));
        if (timer->m_cycle_ms < 3000){
            interruptEpoll();
        }
    }

protected:
    bool stopping() override;
    void idle() override;
    void run() override;

    bool addInterrupt();
    bool interruptEpoll();

private:
    int m_epfd = -1;
    int m_interruptFd[2] = {-1, -1};
    RWMutexType m_mutex;
    std::unordered_map<int, FdContext::ptr> m_fdContexts;
    EThread::ptr handlerThread;      // IO处理线程
};
 
} // namespace server

#endif // IOMANAGER_20_H
#endif
