#include "iomanager_.h"
#include "ethread.h"
#include "fiber.h"
#include "log.h"
#include "scheduler_.h"
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


namespace server{

static Logger::ptr g_logger = SERVER_LOGGER_SYSTEM;

IOManager_::IOManager_(size_t threads_, const std::string& name_)
    : Scheduler_(threads_, name_), m_fdContexts(),
        m_idleFiber(new Fiber_(std::bind_front(&IOManager_::idle, this)))
{
    m_epfd = epoll_create(1024);
    assert(m_epfd > 0);
}

IOManager_::~IOManager_(){
    close(m_epfd);
}

int IOManager_::AddEvent(int fd, Event event, TaskType cb){
    WriteLockGuard wlock(m_mutex);
    FdContext::ptr nfd = m_fdContexts.contains(fd) ? 
        m_fdContexts[fd] : std::make_shared<FdContext>(fd);

    if(m_fdContexts.contains(fd) && (event & nfd->events))
        return 1;
    int op = nfd->events == NONE ? EPOLL_CTL_ADD : EPOLL_CTL_ADD;
    epoll_event evt;
    memset(&evt, 0, sizeof(evt));
    evt.events = EPOLLET | nfd->events | event; 
    evt.data.fd = fd;
    int ret = epoll_ctl(m_epfd, op, fd, &evt);
    if (ret){
        SERVER_LOG_ERROR(g_logger) << "epoll ctl error " << m_epfd << ", "
                                           << op << ", " << fd << ", " << evt.events;
        return 2;
    }

    nfd->events = (Event)(event | nfd->events);
    auto con = event == READ ? nfd->read : nfd->write;
    con.fiber = std::make_shared<Fiber_>(cb); 
    return 0;
}

bool IOManager_::DelEvent(int fd, Event event){
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
    m_fdContexts[fd]->events = new_event;
    // 清除对应Context
    m_fdContexts[fd]->ResetEventContext(event);
    
    return 0;
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
            schedule(exp->GetFunc());
        }

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

        co_yield HOLD;
    }
    co_return TERM;
}

void IOManager_::run(){
    static bool expect = false;

    while (!m_stopping){
        if(!m_idleLock.test_and_set()){
            m_idleFiber->swapIn();
            if(!m_idleFiber->done())
                m_idleLock.clear();
            continue;
        }

        task_type ta;
        if(m_tasks.pop_front(ta)){
            if(ta->done()) continue;
            ++m_working_thread;
            ta->swapIn();
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

void IOManager_::wait(int time){
    if (time < 0)
        while(1);
    sleep(time);
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

Timer_::Timer_(uint64_t ms, TaskType cb, Timer_Manager* manager)
    : m_ms(ms), m_manager(manager)
{
    m_next = Timer_Manager::GetCurTimeStamp() + ms;
    m_cb = Fiber_::ptr(new Fiber_([cb, this]()->CoRet{
        Fiber_::ptr inner(new Fiber_(cb));
        while(1){
            inner->swapIn();
            if(inner->done())
                break;
            refresh();
            m_manager->addTimer(shared_from_this());
        }
    }));
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

Timer_::ptr Timer_Manager::addTimer(uint64_t ms, TaskType cb){
    Timer_::ptr Timer_(new class Timer_(ms, cb, this));
    WriteLockGuard wlock(m_mutex);

    auto it = m_timers.insert(Timer_).first;
    bool tri = it == m_timers.begin();
    wlock.unlock();
    if(tri)
        OnInsertAtFront();
    return Timer_;
}

void Timer_Manager::addTimer(Timer_::ptr Timer_){
    WriteLockGuard wlock(m_mutex);
    m_timers.insert(Timer_);
}

Timer_::ptr Timer_Manager::addConditionTimer(uint64_t ms, TaskType cb, std::weak_ptr<void> weak_cond){
    WriteLockGuard wlock(m_mutex);
    return addTimer(ms, 
            [=]->CoRet{
            if(weak_cond.lock())
                cb().h_.resume();
            } 
    );
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
