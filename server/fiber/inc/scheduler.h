#ifndef SERVER_SCHEDULER_H
#define SERVER_SCHEDULER_H

#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <vector>
#include "fiber.h"
#include "ethread.h"

namespace server {

class Scheduler{
public:
    typedef std::shared_ptr<Scheduler> ptr;
    
    Scheduler(size_t max_ = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    static Scheduler* GetThis();
    static Fiber* GetMainFiber();

    virtual void start();
    virtual void stop();
    virtual void wait();

    template<typename T>
    void schedule(T fc, int thread = -2){
        // thread = -1：主协程
        // thread = -2：未指定对应线程
        bool need_tickle = false;
        {
            LockGuard lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        if(need_tickle){
            tickle();
        }
    }

    template<typename T>
    void schedule(T begin, T end){
        bool need_tickle = false;
        {
            LockGuard lock(m_mutex);
            while (begin != end){
                need_tickle = scheduleNoLock(&*(begin++)) || need_tickle;
            }
        }
        if(need_tickle){
            tickle();
        }
    }

private:
    template<typename T>
    bool scheduleNoLock(T fc, int thread){
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb){
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }

protected:
    void run();             // 调度方法
    virtual void tickle();
    virtual bool stopping();
    virtual void idle();        // 协程空闲时执行的方法

private:
    struct FiberAndThread{
        Fiber::ptr fiber;
        std::function<void()> cb;
        int threadid;
        FiberAndThread(): threadid(-1){}
        FiberAndThread(Fiber::ptr f, int thr)
            :fiber(f), threadid(thr) {}
        FiberAndThread(std::function<void()> f, int thr)
            :cb(f), threadid(thr) {}
        void reset(){
            fiber = nullptr;
            cb = nullptr;
            threadid = -1;
        }
    };

private:
    Mutex m_mutex;
    std::vector<EThread::ptr> m_threads;
    std::list<FiberAndThread> m_fibers;
    Fiber::ptr m_rootFiber;
    std::string m_name = "Scheduler";
    
protected:
    std::vector<int> m_threadIds;
    size_t m_threadCount = 0;
    size_t m_activeThreadCount = 0;
    size_t m_idleThreadCount = 0;
    int m_rootThread = 0; 
    bool m_stopping = true;
    bool m_autoStop = false;
};


}

#endif
