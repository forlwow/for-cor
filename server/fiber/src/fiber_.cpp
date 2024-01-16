#include "fiber_.h"
#include "log.h"
#include "shared_vars.h"
#include <cstdint>
#include <functional>
#include <sys/types.h>

namespace server{

thread_local Fiber_* t_fiber_ = nullptr;            // 当前执行的协程
thread_local Fiber_::ptr t_threadIber_ = nullptr;   // 主协程

auto g_logger = SERVER_LOGGER_SYSTEM;

// c++20

std::suspend_always CoRet::promise_type::initial_suspend() const noexcept{
    return {};
}

std::suspend_never CoRet::promise_type::final_suspend() const noexcept{
    return {};
}

void CoRet::promise_type::unhandled_exception() {

}

CoRet CoRet::promise_type::get_return_object(){
    return {std::coroutine_handle<promise_type>::from_promise(*this)};
}

std::suspend_always CoRet::promise_type::yield_void() {
    if(m_cbBeforeYield)
        m_cbBeforeYield();
    t_fiber_ = t_threadIber_.get();
    return {};
}

std::suspend_always CoRet::promise_type::yield_value(State s) {
    if(m_cbBeforeYield)
        m_cbBeforeYield();
    t_fiber_ = t_threadIber_.get();
    m_state = s;
    return {};
}

void CoRet::promise_type::return_value(State s){
    if(m_cbBeforeReturn)
        m_cbBeforeReturn();
    if(m_done)
        *m_done = true;
    t_fiber_ = t_threadIber_.get();
    m_state = s;
}


Fiber_::Fiber_(){
    ++s_fiber_count;
}

Fiber_::Fiber_(std::function<CoRet()> cb)
    :m_id(++s_fiber_id), m_cb(cb())
{
    m_cb.h_.promise().m_done = &m_done;
    ++s_fiber_count;
}

Fiber_::~Fiber_(){
    --s_fiber_count;    

    SERVER_LOG_INFO(g_logger) << "fiber  destroy id:" << m_id;
}

void Fiber_::reset(std::function<CoRet()> cb){
    m_cb = cb();
    m_done = false;
    m_cb.h_.promise().m_done = &m_done;
}

bool Fiber_::swapIn(){
    if(!m_done){
        t_fiber_ = this;
        m_cb();
        return true;
    }
    return false;
}

bool Fiber_::done(){
    return m_done;
}

uint64_t Fiber_::GetCurFiberId(){
    if(t_fiber_){
        return t_fiber_->getId();
    }
    return 0;
}


Fiber_2::Fiber_2(std::function<CoRet()> cb, bool drop_)
    : Fiber_(cb), m_drop(drop_)
{

}

Fiber_2::~Fiber_2(){
    
}

bool Fiber_2::swapIn(){
    if(m_cbBeforeSwapIn)
        m_cbBeforeSwapIn();
    if(m_flag.test_and_set())   // 已被其他线程调用
        return false;
    if(!m_done){
        t_fiber_ = this;
        m_cb();
        m_flag.clear();
        return true;
    }
    return false;
}

void Fiber_2::setCbBeforeReturn(std::function<void()> cb){
    m_cb.h_.promise().m_cbBeforeReturn = cb;
}

void Fiber_2::setCbBeforeSwapIn(std::function<void()> cb){
    m_cbBeforeSwapIn = cb;
}

void Fiber_2::setCbBeforeYield(std::function<void()> cb){
    m_cb.h_.promise().m_cbBeforeYield = cb;
}

} // namespace server



