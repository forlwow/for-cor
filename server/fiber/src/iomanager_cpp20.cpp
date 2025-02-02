#include "async.h"
#include "enums.h"
#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#if __cplusplus >= 202002L
#include "iomanager_cpp20.h"
#include "ethread.h"
#include "async.h"
#include "fiber.h"
#include "log.h"
#include "scheduler_cpp20.h"
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <functional>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <memory>
#include <utility>


namespace server{

static Logger::ptr g_logger = SERVER_LOGGER_SYSTEM;

IOManager_::IOManager_(size_t threads_, const std::string& name_)
    : Scheduler_(threads_, name_), m_fdContexts()
{
    m_epfd = epoll_create(1024);
    if (m_epfd == -1) {
        SERVER_LOG_FATAL(g_logger) << "IOManager Create Failed" << BacktraceToString(10, 0);
        exit(-1);
    }
}

IOManager_::~IOManager_(){
    close(m_epfd);
}

void IOManager_::start(){
    Scheduler_::start();
    handlerThread = EThread::ptr(new EThread(std::bind_front(&IOManager_::idle, this), "Handler"));
    if(!addInterrupt()){
        SERVER_LOG_ERROR(g_logger) << "Failed to add Interrupt";
    }
}

int IOManager_::AddEvent(int fd, Event event, Fiber_::ptr cb){
    WriteLockGuard wlock(m_mutex);

    if(fcntl(fd, F_GETFD) == -1){
        SERVER_LOG_ERROR(g_logger) << "sock err: " << std::string(strerror(errno));
        return -1;
    }

    // 是否已经含有该描述符 没有则新建
    FdContext::ptr nfd = m_fdContexts.contains(fd) ? 
        m_fdContexts[fd] : std::make_shared<FdContext>(fd);

    // 如果已经有对应事件则返回错误
    // if(m_fdContexts.contains(fd) && (event & nfd->events))
    //     return 1;

    // 判断操作符
    int op = nfd->events == NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    epoll_event evt;
    memset(&evt, 0, sizeof(evt));
    evt.events = (uint32_t)EPOLLET | nfd->events | event; 
    evt.data.fd = fd;
    int ret = epoll_ctl(m_epfd, op, fd, &evt);
    if (ret){
        SERVER_LOG_ERROR(g_logger) << "epoll ctl error " 
            << "epoll fd:" << m_epfd << ", "
            << "operator:" << op << ", " 
            << "fd:"<< fd << ", " 
             << "epoll_event:" << (evt.events & WRITE) << "-" << (evt.events & READ) << ", "
             << "errno:" << errno << ", "
             << "errstr:" << std::string(strerror(errno));
        return 2;
    }

    nfd->events = (Event)(event | nfd->events);
    // 拿到对应事件的处理对象
    auto &con = event == READ ? nfd->read : nfd->write;
    con.fiber = std::move(cb);
    con.scheduler = this;
    if(!m_fdContexts.contains(fd)){
        m_fdContexts[fd] = std::move(nfd);
    }

    return 0;
}

bool IOManager_::DelEvent(int fd, Event event){
    WriteLockGuard wlock(m_mutex);
    if(!m_fdContexts.contains(fd))
        return false;
    auto tarfd = m_fdContexts[fd];
    if(!(tarfd->events & event))
        return false;
    // 获取新的event
    Event new_event = (Event)(tarfd->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

    epoll_event epevent;
    memset(&epevent, 0, sizeof(epevent));
    epevent.events = (int)(EPOLLET) | new_event;
    epevent.data.fd = fd;
    int ret = epoll_ctl(m_epfd, op, fd, &epevent);
    if(ret){
        SERVER_LOG_ERROR(g_logger) << "epoll ctl error " << m_epfd << ", "
                                           << op << ", " << fd << ", " << epevent.events;
        return false;
    }
    // 更改原event
    m_fdContexts[fd]->events = new_event;
    // 清除对应Context
    m_fdContexts[fd]->ResetEventContext(event);
    
    return true;
}

bool IOManager_::DelFd(int fd){
    int res = epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, NULL);
    if(res < 0 && errno != EBADF){
        SERVER_LOG_ERROR(g_logger) << "epoll ctl error " << m_epfd << ", " << fd 
                                            << ", errno: " << errno << ", error:"
                                            << std::string(strerror(errno));
        return false;
    }
    WriteLockGuard wlock(m_mutex);
    if(m_fdContexts.contains(fd)){
        m_fdContexts.erase(m_fdContexts.find(fd));
    }
    return true;
}

bool IOManager_::CancelEvent(int fd, Event event){
    WriteLockGuard wlock(m_mutex);
    if(!m_fdContexts.contains(fd))
        return 1;
    auto tarfd = m_fdContexts[fd];
    if(!(tarfd->events & event))
        return 2;
    // 获取新的event
    Event new_event = (Event)(tarfd->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

    epoll_event epevent;
    memset(&epevent, 0, sizeof(epevent));
    epevent.events = (int)(EPOLLET) | new_event;
    epevent.data.fd = fd;
    int ret = epoll_ctl(m_epfd, op, fd, &epevent);
    if(ret){
        SERVER_LOG_ERROR(g_logger) << "epoll ctl error " << m_epfd << ", "
                                           << op << ", " << fd << ", " << epevent.events;
        return 2;
    }
    // 更改原event
    tarfd->events = new_event;
    // 触发事件
    tarfd->TriggerEvent(event);
    // 清除对应Context
    tarfd->ResetEventContext(event);
    
    return 0;
}

IOManager_::ptr IOManager_::GetInstance(size_t threads_, const std::string& name_) {
    static IOManager_::ptr m_manager(new IOManager_(threads_, name_), Deleter());
    return m_manager;
}

bool IOManager_::stopping(){
    return true;
}

void IOManager_::idle(){
    const uint64_t MAX_EVENTS = 256;
    epoll_event* events = new epoll_event[MAX_EVENTS];
    std::shared_ptr<epoll_event> shared_events(
           events, 
           [](epoll_event* ptr){delete [] ptr;}
    );
    uint64_t next_time = UINT64_MAX;
    while (!m_stopping) {
        int ret;
        const uint64_t MAX_TIMEOUT = 3000;
        next_time = GetNextTimeDuration();
        next_time = next_time > MAX_TIMEOUT ? MAX_TIMEOUT : next_time;
        do{
            ret = epoll_wait(m_epfd, events, MAX_EVENTS, next_time);
            if(ret == -1 && errno == EINTR){}
            else {
                break;
            }
        }while(true);

        for(auto exp : GetExpireTimers()){
            if(exp->GetFunc() && !exp->GetFunc()->done()){
                schedule(exp->GetFunc());
            }
        }
        {
            ReadLockGuard rlock(m_mutex);
            for(int i = 0; i < ret; ++i){
                epoll_event cur_evt = events[i];
                SERVER_LOG_INFO(g_logger) << "epoll accept fd:" << cur_evt.data.fd 
                                                << " write:" << (bool)(cur_evt.events & EPOLLOUT)
                                                << " read:" << (bool)(cur_evt.events & EPOLLIN);
                auto cur_fdcont = m_fdContexts[cur_evt.data.fd];
                assert(cur_fdcont);
                // 描述符出错或已关闭
                if((cur_evt.events & EPOLLHUP) || (cur_evt.events & EPOLLERR)){
                    m_fdContexts.erase(m_fdContexts.find(cur_evt.data.fd));
                    continue;
                }
                if(cur_evt.events & EPOLLIN)
                    cur_fdcont->TriggerEvent(READ);
                if(cur_evt.events & EPOLLOUT)
                    cur_fdcont->TriggerEvent(WRITE);
            }
        }

        std::this_thread::yield();
    }
    return;
}

void IOManager_::run(){
    while (!m_stopping){
        task_type ta;
        if(m_tasks.pop_front(ta)){
            if(ta->done()) continue;
            ++m_working_thread;
            if(!ta->swapIn()){

            }
            --m_working_thread;
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

bool IOManager_::addInterrupt(){
    if(pipe(m_interruptFd) == -1){
        return false;
    }
    AddEvent(m_interruptFd[0], READ, FuncFiber::CreatePtr([fd = m_interruptFd[0]]{
        char bytes;
        while (read(fd, &bytes, 1) != 0);
    }));
    return true;
}

bool IOManager_::interruptEpoll(){
    if(m_interruptFd[1] == -1){
        return false;
    }
    write(m_interruptFd[1], "1", 1);
    return true;
}

} // namespace server
  //
#endif
