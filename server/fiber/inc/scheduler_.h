#ifndef SERVER_SCHEDULER__H
#define SERVER_SCHEDULER__H

#include "ethread.h"
#include "fiber.h"
#include "threadsafe_deque.h"
#include <cstddef>
#include <memory>
#include <vector>

namespace server{
typedef Fiber_ task_type;

class Scheduler_{
public:
    typedef std::shared_ptr<Scheduler_> ptr;
    Scheduler_(size_t max_ = 1, const std::string& name_ = "");
    ~Scheduler_();
    
    virtual void start();
    virtual void stop();

    // 传入的参数为Fiber对象，或函数对象
    void schedule(task_type task_);
    using submit = decltype(&Scheduler_::schedule);
protected:
    virtual void run();

private:
    std::string m_name;                     // 调度器名
    std::vector<EThread::ptr> threads;      // 所有线程
    threadsafe_deque<task_type> tasks;      // 工作队列 

    bool m_stopping = true;                 // 当前状态 停止或运行
    bool m_autoStop = false;                // 是否正在自动停止
};

}

#endif // SERVER_SCHEDULER__H
