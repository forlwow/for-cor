#if __cplusplus >= 202002L
#ifndef SERVER_SCHEDULER_20_H
#define SERVER_SCHEDULER_20_H

#include "ethread.h"
#include "threadsafe_deque.h"
#include "async.h"
#include "enums.h"
#include "fiber_cpp20.h"
#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

namespace server{
typedef Fiber_::ptr task_type;

class Scheduler_{
public:
    typedef std::shared_ptr<Scheduler_> ptr;
    Scheduler_(size_t max_ = 1, const std::string& name_ = "Sche");
    virtual ~Scheduler_();
    
    virtual void start();
    virtual void wait_stop();
    virtual void stop();
    virtual void wait(int time = -1);

    // 传入的参数为Fiber对象，或函数对象
    bool schedule(task_type task_);
    using submit = decltype(&Scheduler_::schedule);

    static Scheduler_* GetScheduler();
protected:
    virtual void run();
    virtual void idle(){}
    virtual bool stopping(){return true;}
protected:
    bool m_stopping = true;                 // 当前状态 停止或运行
    bool m_autoStop = false;                // 是否正在自动停止
    std::string m_name;                     // 调度器名
    int m_thread_count;
    std::atomic_int m_working_thread = 0;       // 正在工作的线程的数量 
    std::vector<EThread::ptr> m_threads;          // 所有线程
    threadsafe_deque<task_type> m_tasks;          // 工作队列 

};

}

#endif // SERVER_SCHEDULER_20_H
#endif 
