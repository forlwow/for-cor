#include "iomanager.h"
#include "ethread.h"
#include "fiber.h"
#include "log.h"
#include "scheduler.h"
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ios>
#include <memory>
#include <sched.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>


namespace server{

static server::Logger::ptr g_logger = SERVER_LOGGER_SYSTEM;

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(Event event){
    switch (event) {
        case READ:
            return read;
            break;
        case WRITE:
            return write;
            break;
        default:
            assert(0);
    }
}

void IOManager::FdContext::resetContext(EventContext& ctx){
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(Event event){
    assert(events & event);
    events = (Event)(events & ~event);
    auto& ctx = getContext(event);
    if (ctx.cb)
        ctx.scheduler->schedule(ctx.cb);
    else 
        ctx.scheduler->schedule(ctx.fiber);
    ctx.scheduler = nullptr;
    return ;
}

IOManager::IOManager(size_t threads, bool usr_caller, const std::string& name)
    :Scheduler(threads, usr_caller, name)
{
    m_epfd = epoll_create(1024);
    assert(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    assert(!rt);
    SERVER_LOG_INFO(g_logger) << "pipe read:" << m_tickleFds[0] << "  "
                                      << "pipe write:" << m_tickleFds[1];

    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0]; 

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    assert(!rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    assert(!rt);

    contextResize(32);

    start();

}

IOManager::~IOManager(){
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i = 0; i < m_fdContexts.size(); ++i){
        if(m_fdContexts[i])
            delete m_fdContexts[i];
    }
}

void IOManager::contextResize(size_t size){
    m_fdContexts.resize(size);
    for(size_t i = 0; i < m_fdContexts.size(); ++i){
        if(m_fdContexts[i]) continue;
        m_fdContexts[i] = new FdContext;
        m_fdContexts[i]->fd = i;
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb){
    FdContext* fd_ctx = nullptr;
    ReadLockGuard rlock(m_mutex);
    if((int)m_fdContexts.size() > fd){
        fd_ctx = m_fdContexts[fd];
        rlock.unlock();
    }
    else{
        rlock.unlock();
        WriteLockGuard wlock(m_mutex);
        contextResize(fd*1.5);
        fd_ctx = m_fdContexts[fd];
    }
    LockGuard lock2(fd_ctx->mutex);
    // 添加的事件一致 则出错
    if(fd_ctx->events & event){
        SERVER_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                                           << "fd_ctx event=" << fd_ctx->events
                                           << "event=" << event;
        assert(0);
    }
    // epoll 操作类型 为1则修改 其他则向epoll添加
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    memset(&epevent, 0, sizeof(epevent));
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;
    
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        SERVER_LOG_ERROR(g_logger) << "epoll ctl error " << m_epfd << ", "
                                           << op << ", " << fd << ", " << epevent.events;
    }
    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    assert(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetThis();
    if(cb)
        event_ctx.cb.swap(cb);
    else{
        event_ctx.fiber = Fiber_1::GetThis();
        assert(event_ctx.fiber->GetState() == State::EXEC);
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event){
    ReadLockGuard rlock(m_mutex);
    if((int)m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    rlock.unlock();

    LockGuard lock(fd_ctx->mutex);
    if(!(fd_ctx->events & event))
        return false;
    Event new_event = (Event)(fd_ctx->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

    epoll_event epevent;
    memset(&epevent, 0, sizeof(epevent));
    epevent.events = EPOLLET | new_event;
    epevent.data.ptr = fd_ctx;
    
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        SERVER_LOG_ERROR(g_logger) << "epoll ctl error " << m_epfd << ", "
                                           << op << ", " << fd << ", " << epevent.events;
    }

    --m_pendingEventCount;
    fd_ctx->events = new_event;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}

bool IOManager::cancelEvent(int fd, Event event){
    ReadLockGuard rlock(m_mutex);
    if((int)m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    rlock.unlock();

    LockGuard lock(fd_ctx->mutex);
    if(!(fd_ctx->events & event))
        return false;
    Event new_event = (Event)(fd_ctx->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

    epoll_event epevent;
    memset(&epevent, 0, sizeof(epevent));
    epevent.events = EPOLLET | new_event;
    epevent.data.ptr = fd_ctx;
    
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        SERVER_LOG_ERROR(g_logger) << "epoll ctl error " << m_epfd << ", "
                                           << op << ", " << fd << ", " << epevent.events;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd){
    ReadLockGuard rlock(m_mutex);
    if((int)m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    rlock.unlock();

    LockGuard lock(fd_ctx->mutex);
    if(!(fd_ctx->events))
        return false;
    int op = EPOLL_CTL_DEL;

    epoll_event epevent;
    memset(&epevent, 0, sizeof(epevent));
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;
    
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        SERVER_LOG_ERROR(g_logger) << "epoll ctl error " << m_epfd << ", "
                                           << op << ", " << fd << ", " << epevent.events;
    }

    if(fd_ctx->events & READ){
        fd_ctx->triggerEvent(READ);
    }
    if(fd_ctx->events & WRITE){
        fd_ctx->triggerEvent(WRITE);
    }
    --m_pendingEventCount;
    return true;
}

IOManager* IOManager::GetThis(){
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle(){
    if (!m_threadCount)
        return ;
    int rt = write(m_tickleFds[1], "1", 1);
    assert(rt == 1);
}

bool IOManager::stopping(){
    return true;
}

void IOManager::idle(){
    const uint64_t MAX_EVENTS = 256;
    epoll_event* events = new epoll_event[MAX_EVENTS];
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){delete [] ptr;});
    while (true){
        uint64_t next_timeout = 0;
        if(m_stopping) break;
        int rt = 0;
        do{
            static const int MAX_TIMEOUT = 3000;
            rt = epoll_wait(m_epfd, events, MAX_EVENTS, (int)MAX_TIMEOUT);
            SERVER_LOG_INFO(g_logger) << "epoll wait result=" << rt;
            if(rt < 0 && errno == EINTR){}
            else break;
        }while(true);


        for(int i = 0; i < rt; ++i){
            epoll_event& event = events[i];
            if(event.data.fd == m_tickleFds[0]){
                uint8_t dummy[256];
                while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }
            SERVER_LOG_INFO(g_logger) << "epoll accept";
            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            LockGuard lock(fd_ctx->mutex);
            if(event.events & (EPOLLERR | EPOLLHUP))
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            int real_events = NONE;
            if(event.events & EPOLLIN)
                real_events |= READ;
            if(event.events & EPOLLOUT)
                real_events |= WRITE;

            if(fd_ctx->events & real_events == NONE)
                continue;

            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;
            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2)
                assert(0);
            if(real_events & READ){
                fd_ctx->triggerEvent(READ);
            }
            if(real_events & WRITE){
                fd_ctx->triggerEvent(WRITE);
            }
        }
        Fiber_1::ptr cur = Fiber_1::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->YieldToHold();
    }
}


} // namespace server
