#if __cplusplus >= 202002L
#include "fiber_cpp20.h"
#include "log.h"
#include "shared_vars.h"
#include <cstdint>
#include <functional>
#include <sys/types.h>
#include <tuple>
#include <utility>

namespace server{

thread_local Fiber_::ptr t_fiber_ = nullptr;            // 当前执行的协程

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
    return {};
}

std::suspend_always CoRet::promise_type::yield_value(State s) {
    if(m_cbBeforeYield)
        m_cbBeforeYield();
    m_state = s;
    return {};
}

void CoRet::promise_type::return_value(State s){
    if(m_cbBeforeReturn)
        m_cbBeforeReturn();
    if(m_done)
        *m_done = true;
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

    // SERVER_LOG_INFO(g_logger) << "fiber  destroy id:" << m_id;
}

void Fiber_::reset(std::function<CoRet()> cb){
    m_cb = cb();
    m_done = false;
    m_cb.h_.promise().m_done = &m_done;
}

bool Fiber_::swapIn(){
    if(!m_done){
        t_fiber_ = shared_from_this();
        m_cb();
        t_fiber_.reset();
        return true;
    }
    return false;
}

bool Fiber_::done(){
    return m_done;
}

Fiber_::ptr Fiber_::GetThis(){
    return t_fiber_;
}

uint64_t Fiber_::GetCurFiberId(){
    if(t_fiber_){
        return t_fiber_->getId();
    }
    return 0;
}


Fiber_2::Fiber_2(co_fun cb, bool drop_)
    : m_cb(cb()), m_drop(drop_)
{
    std::get<COROUTINE>(m_cb).h_.promise().m_done = &m_done;
}

Fiber_2::Fiber_2(void_fun cb)
    : m_cb(cb)
{

}

Fiber_2::Fiber_2(std::function<CoRet()> cb, bool drop_)
    : m_cb(cb()), m_drop(drop_)
{
    std::get<COROUTINE>(m_cb).h_.promise().m_done = &m_done;
}

Fiber_2::Fiber_2(std::function<void()> cb)
    : m_cb(cb)
{

}


Fiber_2::~Fiber_2(){
    
}

bool Fiber_2::swapIn(){
    if(m_cb.index() == COROUTINE){
        if(m_cbBeforeSwapIn)
            m_cbBeforeSwapIn();
        if(m_flag.test_and_set())   // 已被其他线程调用
            return false;
        if(!m_done){
            t_fiber_ = shared_from_this();
            std::get<COROUTINE>(m_cb)();
            m_flag.clear();
            t_fiber_.reset();
            return true;
        }
        return false;
    }
    else if (m_cb.index() == FUNCTION){
        if(m_cbBeforeSwapIn)
            m_cbBeforeSwapIn();
        t_fiber_ = shared_from_this();
        std::get<FUNCTION>(m_cb)();
        t_fiber_.reset();
        return true;
    }
    else
        return false;
}

void Fiber_2::setCbBeforeReturn(std::function<void()> cb){
    if(m_cb.index() == COROUTINE)
        std::get<COROUTINE>(m_cb).h_.promise().m_cbBeforeReturn = cb;
}

void Fiber_2::setCbBeforeSwapIn(std::function<void()> cb){
    m_cbBeforeSwapIn = cb;
}

void Fiber_2::setCbBeforeYield(std::function<void()> cb){
    if(m_cb.index() == COROUTINE)
        std::get<COROUTINE>(m_cb).h_.promise().m_cbBeforeYield = cb;
}

} // namespace server


#endif

