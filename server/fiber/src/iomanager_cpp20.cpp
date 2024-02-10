#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <cstddef>
#include <string>
#include <sys/socket.h>
#if __cplusplus >= 202002L
#include "iomanager_cpp20.h"
#include "ethread.h"
#include "fiber.h"
#include "log.h"
#include "scheduler_cpp20.h"
#include "timer.h"
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <memory>
#include <utility>


namespace server{

static Logger::ptr g_logger = SERVER_LOGGER_SYSTEM;

IOManager_::IOManager_(size_t threads_, const std::string& name_)
    : Scheduler_(threads_, name_), m_fdContexts(),
        m_idleFiber(new Fiber_2(std::bind_front(&IOManager_::idle, this), false))
{
    m_epfd = epoll_create(1024);
    assert(m_epfd > 0);
}

IOManager_::~IOManager_(){
    close(m_epfd);
}

int IOManager_::AddEvent(int fd, Event event, TaskType cb, bool drop){
    AddEvent(fd, event, Fiber_2::ptr(new Fiber_2(cb, drop)));
}

int IOManager_::AddEvent(int fd, Event event, Fiber_2::ptr cb){
    WriteLockGuard wlock(m_mutex);
        // 获取socket上的错误状态
    int error = 0;
    socklen_t errlen = sizeof(error);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &errlen) < 0){
        SERVER_LOG_ERROR(g_logger) << "sock err:" << std::string(strerror(errno));
        return -1;
    }

    // 是否已经含有该描述符 没有则新建
    FdContext::ptr nfd = m_fdContexts.contains(fd) ? 
        m_fdContexts[fd] : std::make_shared<FdContext>(fd);

    // 如果已经有对应事件则返回错误
    if(m_fdContexts.contains(fd) && (event & nfd->events))
        return 1;

    // 判断操作符
    int op = nfd->events == NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    epoll_event evt;
    memset(&evt, 0, sizeof(evt));
    evt.events = EPOLLET | nfd->events | event; 
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
    epevent.events = EPOLLET | new_event;
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
    epevent.events = EPOLLET | new_event;
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

bool IOManager_::stopping(){
    return true;
}

CoRet IOManager_::idle(){
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
            if(!exp->GetFunc()->done()){
                schedule(exp->GetFunc());
                if(exp->isCirculate()){
                    exp->refresh();
                    addTimer(exp);
                }
            }
        }
        {
            ReadLockGuard rlock(m_mutex);
            for(int i = 0; i < ret; ++i){
                epoll_event cur_evt = events[i];
                SERVER_LOG_INFO(g_logger) << "epoll accept fd:" << cur_evt.data.fd;
                auto cur_fdcont = m_fdContexts[cur_evt.data.fd];
                assert(cur_fdcont);
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

        co_yield HOLD;
    }
    co_return TERM;
}

void IOManager_::run(){
    static bool expect = false;

    while (!m_stopping){
        if(m_idleFiber->swapIn()){
            std::this_thread::yield();
            continue;
        }

        task_type ta;
        if(m_tasks.pop_front(ta)){
            if(ta->done()) continue;
            ++m_working_thread;
            auto tb = std::dynamic_pointer_cast<Fiber_2>(ta);
            if(!tb->swapIn() && !tb->isDrop()){
                schedule(ta);
            }
            else{
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

bool Timer_::Compare::operator()(const Timer_::ptr& lp, const Timer_::ptr& rp) const {
    if(!lp && !rp)
        return false;
    if(!lp)
        return true;
    if(!rp)
        return false;
    if(lp->m_next < rp->m_next)
        return true;
    if(rp->m_next < lp->m_next)
        return false;
    return lp < rp;
}

Timer_::Timer_(uint64_t ms, TaskType cb, Timer_Manager* manager, bool cir)
    : m_ms(ms), m_manager(manager), m_circulate(cir)
{
    m_next = Timer_Manager::GetCurTimeStamp() + ms;
    m_cb = cb;
}

Timer_::Timer_(uint64_t next_time)
    :m_next(next_time)
{

}

void Timer_::refresh(){
    m_next = Timer_Manager::GetCurTimeStamp() + m_ms;
}

bool Timer_::InsertIntoManager(Timer_::ptr Timer_){
    if(Timer_->m_manager){
        Timer_->m_manager->addTimer(Timer_);
        return true;
    }
    return false;
}

Timer_Manager::Timer_Manager(){

}

Timer_Manager::~Timer_Manager(){

}

void Timer_Manager::OnInsertAtFront(){

}

Timer_::ptr Timer_Manager::addTimer(uint64_t ms, TaskType1 cb, bool circulate){
    Fiber_2::ptr fib(new Fiber_2(cb));
    Timer_::ptr timer(new Timer_(ms, fib, this, circulate));
    addTimer(timer);
    return timer;
}
   
Timer_::ptr Timer_Manager::addTimer(uint64_t ms, TaskType2 cb, bool drop){
    Fiber_2::ptr fib(new Fiber_2(cb, drop));
    Timer_::ptr timer(new Timer_(ms, fib, this));
    addTimer(timer);
    return timer;
}

void Timer_Manager::addTimer(Timer_::ptr Timer_){
    WriteLockGuard wlock(m_mutex);
    m_timers.insert(Timer_);
}

std::vector<Timer_::ptr> Timer_Manager::GetExpireTimers(){
    std::vector<Timer_::ptr> res;
    Timer_::ptr ttime(new Timer_(GetCurTimeStamp()));
    ReadLockGuard rlock(m_mutex);
    auto iter = m_timers.upper_bound(ttime);
    for(auto it = m_timers.begin(); it != iter;++it){
        res.push_back(*it);
    }
    m_timers.erase(m_timers.begin(), iter);
    return res;
}

uint64_t Timer_Manager::GetNextTimeDuration(){
    ReadLockGuard rlock(m_mutex);
    auto iter = m_timers.begin();
    if(iter == m_timers.end())
        return UINT64_MAX;
    uint64_t now = GetCurTimeStamp(), next = (*iter)->m_next;
    if(next > now)
        return next - now;
    else
        return 0;
}

void Timer_Manager::AutoStop(){
    WriteLockGuard wlock(m_mutex);
    for(auto &Timer_ : m_timers){
        Timer_->m_manager = nullptr;
    }
}


} // namespace server
  //
#endif
