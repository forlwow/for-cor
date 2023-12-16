#include "scheduler.h"
#include "ethread.h"
#include "fiber.h"
#include "log.h"
#include <cassert>
#include <cstddef>
#include <functional>
#include <sched.h>
#include <string>
#include <thread>

namespace server{

static server::Logger::ptr s_logger = SERVER_LOGGER_SYSTEM;

static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_fiber = nullptr;
extern thread_local int t_thread_id;
extern thread_local std::string t_thread_name;

Scheduler::Scheduler(size_t max_, bool use_caller, const std::string& name)
    : m_name(name)
{
    assert(max_ > 0);
    // max_ = (max_ < std::thread::hardware_concurrency()) ? max_:std::thread::hardware_concurrency();
    if(use_caller){
        server::Fiber::GetThis();
        --max_;
        assert(GetThis() == nullptr);

        t_scheduler = this;
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this)));
        t_fiber = m_rootFiber.get();
        m_rootThread = t_thread_id;
        m_threadIds.push_back(m_rootThread);

    }
    else{
        m_rootThread = -1;
    }

    m_threadCount = max_;
    SERVER_LOG_INFO(s_logger) << "max_thread: " << max_;
}
Scheduler::~Scheduler(){
    if(GetThis() == this)
        t_scheduler = nullptr;
    SERVER_LOG_INFO(s_logger) << "scheduler delete";
}

Scheduler* Scheduler::GetThis(){
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber(){
    return t_fiber;
}

void Scheduler::start(){
    LockGuard lock(m_mutex);
    if(!m_stopping)
        return ;
    m_stopping = false;
    assert(m_threads.empty());

    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i){
        m_threads[i].reset(new EThread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    SERVER_LOG_INFO(s_logger) << "scheduler start ok";
}

void Scheduler::stop(){
    m_autoStop = true;
    if(m_rootFiber
            && m_threadCount == 0
            ){
        SERVER_LOG_INFO(s_logger) << "Sch Stopped";
        m_stopping = true;
        if(stopping())
            return ;
    }
    if(m_rootThread == -1)
        assert(GetThis() != this);
    else
        assert(GetThis() == this);
    for(auto &i: m_threads){
        i->join();
    }
    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i)
        tickle();

    if(m_rootFiber)
        tickle();
    if(stopping())
        return;
}

void Scheduler::run(){
    
    if(t_thread_id != m_rootThread)
        t_fiber = Fiber::GetThis().get();

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while(m_stopping) {
        ft.reset();
        bool tickle_me = false;
        {
            LockGuard lock(m_mutex);
            auto it = m_fibers.begin();
            while(it != m_fibers.end()){
                if(it->threadid != -1 && it->threadid != t_thread_id){
                    ++it;
                    tickle_me = true;
                    continue;
                }
                if(it->fiber && it->fiber->GetState() == server::EXEC){
                    ++it;
                    continue;
                }
                ft = *it;
                m_fibers.erase(it);
                break;
            }
        }
        if(tickle_me){
            tickle();
        }
        if(ft.fiber && ft.fiber->GetState() != TERM){
            ft.fiber->swapIn();
            if(ft.fiber->GetState() == READY){
                schedule(ft.fiber, ft.threadid);
            }
            ft.reset();
        }
        else if(ft.cb){
            if(cb_fiber){
                cb_fiber->reset(ft.cb);
            }
            else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            if(cb_fiber->GetState() == READY){
                schedule(cb_fiber, cb_fiber->GetCurFiberId());
                cb_fiber->reset(nullptr);
            }
        }
        else{
            if(idle_fiber->GetState() == TERM)
                break;
            ++m_idleThreadCount;
            idle_fiber->swapIn();
             
        }
    }
}

void Scheduler::tickle(){
    SERVER_LOG_INFO(s_logger) << "tickled";
}

bool Scheduler::stopping(){
    LockGuard lock(m_mutex);
    return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle(){
    SERVER_LOG_INFO(s_logger) << "idle";
}

}

