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


class Timer_Manager;
class Timer_: public std::enable_shared_from_this<server::Timer_>{
friend class Timer_Manager;
public:
    typedef std::shared_ptr<Timer_> ptr;
    typedef Fiber_::ptr TaskType;
    Fiber_::ptr GetFunc(){return m_cb;}
    inline void refresh();
    bool isCirculate(){return m_circulate;}
    static bool InsertIntoManager(Timer_::ptr Timer_);

    ~Timer_()=default;
private:
    Timer_(uint64_t ms, TaskType cb, Timer_Manager* manager = nullptr, bool cur = true);
    Timer_(uint64_t next_time);
    struct Compare{
        bool operator()(const Timer_::ptr& lp, const Timer_::ptr& rp) const;
    };

private:
    uint64_t m_ms;              // 周期
    uint64_t m_next;            // 执行的时间点
    bool m_circulate;             // 是否周期执行 只对含有void()的协程有效
    Timer_Manager* m_manager;    // 
    TaskType m_cb; // 
};


class Timer_Manager{
friend class Timer_;
public:
    typedef std::shared_ptr<Timer_Manager> ptr;
    typedef RWMutex RWMutexType;
    typedef std::function<void()> TaskType1;
    typedef std::function<CoRet()> TaskType2;

    Timer_Manager();
    virtual ~Timer_Manager();
    static uint64_t GetCurTimeStamp(){
        auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();
        if(now <= 0)
            return UINT64_MAX;
        else
            return (uint64_t)now;
    }

    Timer_::ptr addTimer(uint64_t ms, TaskType1, bool circulate);
    Timer_::ptr addTimer(uint64_t ms, TaskType2, bool drop);
    void addTimer(Timer_::ptr Timer_);
    std::vector<Timer_::ptr> GetExpireTimers();          // 获取已经到期的Timer_
    uint64_t GetNextTimeDuration();                     // 获取下个定时器触发的剩余时间
protected:
    void OnInsertAtFront();

    void AutoStop();

private:
    RWMutexType m_mutex;
    std::set<Timer_::ptr, Timer_::Compare> m_timers;
};


// 由于每个Fiber对象可能会在短时间内被多次调度
// 因此使用Fiber2
class IOManager_: public Scheduler_, public Timer_Manager{
public:
    typedef std::shared_ptr<IOManager_> ptr;
    typedef RWMutex RWMutexType;
    typedef std::function<CoRet()> TaskType;

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
            EventContext(TaskType task): fiber(new Fiber_(task)){}
            void reset() {scheduler = nullptr, fiber.reset();}
        };
        FdContext()=delete;
        FdContext(int fd_):fd(fd_){}
        ~FdContext(){close(fd);}
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
        }
        // 只在一个线程中操作 不需要锁
    };
public:
    IOManager_(size_t threads_ = 1, const std::string& name_ = "Sche");
    ~IOManager_() override;

    int AddEvent(int fd, Event event, TaskType cb, bool drop = false);
    bool DelEvent(int fd, Event event);
    bool CancelEvent(int fd, Event event);

    static IOManager_* GetIOManager(){return dynamic_cast<IOManager_*>(Scheduler_::GetScheduler());}

protected:
    bool stopping() override;
    CoRet idle() override;
    void run() override;

    // 将schedule修改为protected禁止外界访问
    // using Scheduler_::schedule;

private:
    int m_epfd = -1;
    RWMutexType m_mutex;
    std::unordered_map<int, FdContext::ptr> m_fdContexts;

    Fiber_::ptr m_idleFiber;

};
 
} // namespace server

#endif // IOMANAGER_20_H
#endif
