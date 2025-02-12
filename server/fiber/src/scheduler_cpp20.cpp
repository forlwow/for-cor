#if __cplusplus >= 202002L
#include "scheduler_cpp20.h"
#include "ethread.h"
#include "range.h"
#include <algorithm>
#include <functional>
#include <string>
#include <thread>
#include <utility>
#include "log.h"

namespace server{

static auto s_log = SERVER_LOGGER_SYSTEM;

extern thread_local const char* t_thread_name;
extern thread_local int t_thread_id;

Scheduler_::Scheduler_(size_t max_, const std::string& name_)
    : m_name(name_)
{
    max_ = (max_ < 0 ? std::thread::hardware_concurrency() : 
            (max_ < std::thread::hardware_concurrency() ?
                max_ : std::thread::hardware_concurrency()));
    m_thread_count = max_; 
}
Scheduler_::~Scheduler_(){
    // TODO: 处理线程无法中断时的处理
    m_stopping = true;
    m_autoStop = true;
    for(auto &td: m_threads){
        td->join();
    }
    SERVER_LOG_INFO(s_log) << "Scheduler delete";
}

void Scheduler_::start(){
    if(!m_stopping)
        return ;
    m_stopping = false;
    for(auto &i : range(m_thread_count)){
        m_threads.emplace_back(new EThread(std::bind_front(
                        &Scheduler_::run, this), 
                        m_name + "_" + std::to_string(i)
                    ));
    }
    SERVER_LOG_INFO(s_log) << "Scheduler start";
}

void Scheduler_::wait_stop(){
    m_autoStop = true;
    for(auto &t: m_threads)
        t->join();
}

void Scheduler_::stop(){
    m_stopping = true;
}

bool Scheduler_::schedule(task_type task_){
    if(m_autoStop)
        return false;
    m_tasks.push_back(std::move(task_));
    return true;
}

void Scheduler_::run(){
    while (!m_stopping){
        task_type ta;
        if(m_tasks.pop_front(ta)){
            if(ta->done()) continue;
            ++m_working_thread;
            ta->swapIn();
            --m_working_thread;
            if(!ta->done())
                m_tasks.push_back(std::move(ta));
        }
        else if(m_autoStop){
            if(!m_working_thread && m_tasks.empty())
                m_stopping = true;
        }
        else{
            std::this_thread::yield();
        }
    }
}

// 等待time秒，小于0则无线阻塞
void Scheduler_::wait(int time){
    if (time < 0)
        while(1);
    sleep(time);
}


} // namespace server

#endif 
