#ifndef SERVER_IOMANAGER_H
#define SERVER_IOMANAGER_H

#include "ethread.h"
#include "fiber.h"
#include "scheduler.h"
#include "timer.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <sys/epoll.h>
#include <sys/select.h>
#include <vector>

namespace server{

class IOManager: public Scheduler, public TimerManager{
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;
    
    enum Event{
        NONE = 0x0,
        READ = 0x1,     // EPOLLIN
        WRITE = 0x4     // EPOLLOUT
    };

private:
    struct FdContext{
        struct EventContext{
            Scheduler* scheduler = nullptr;     // 事件执行的调度器
            Fiber_1::ptr fiber;                 // 事件协程
            std::function<void()> cb;           // 事件的回调函数
        };

        int fd;
        Event events = NONE;
        EventContext read;
        EventContext write;
        Mutex mutex;
        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);
    };

public:
    IOManager(size_t threads = 1, bool usr_caller = true, const std::string& name = "");
    ~IOManager() override;

    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    bool cancelEvent(int fd, Event event);
    bool cancelAll(int fd);

    static IOManager* GetThis();

protected:
    void contextResize(size_t size);

    void tickle() override;
    bool stopping() override;
    void idle() override;

private:
    int m_epfd = 0;                                     // epoll描述符
    int m_tickleFds[2];                                 // pipe描述符 0写1读

    std::atomic<size_t> m_pendingEventCount = 0;        // 当前等待执行的事件数量
    RWMutexType m_mutex;                                // 读写锁
    std::vector<FdContext*> m_fdContexts;   // socket容器
};


}

#endif // SERVER_IOMANAGER_H
