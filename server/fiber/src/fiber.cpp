#include "fiber.h"
#include <algorithm>
#include <cassert>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <atomic>
#include <functional>
#include <ucontext.h>

#include "log.h"

namespace server{

static std::atomic<uint64_t> s_fiber_id = 0;        // 协程号 跨线程
static std::atomic<uint64_t> s_fiber_count = 0;     // 协程数量 跨线程
thread_local Fiber_1* t_fiber_1 = nullptr;              // 当前执行的协程
thread_local Fiber_* t_fiber_ = nullptr;              // 当前执行的协程
thread_local Fiber_::ptr t_threadIber_ = nullptr;     // 主协程
thread_local Fiber_1::ptr t_threadIber_1 = nullptr;     // 主协程

struct MallocStackAllocator{
public:
    static void* Alloc(size_t size){
        return malloc(size);
    }
    static void Dealloc(void* vp, size_t size){
        return free(vp);
    }

};

typedef MallocStackAllocator StackAllocator;

Fiber_1::Fiber_1(){
    m_state = EXEC;
    SetThis(this);
    if(getcontext(&m_ctx)){
        abort();
    }
    ++s_fiber_count;
}

Fiber_1::Fiber_1(std::function<void()> cb, size_t stacksize)
    :m_id(++s_fiber_id), m_cb(cb)
{
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize: 1024 * 1024;

    // 分配栈大小
    m_stack = StackAllocator::Alloc(m_stacksize);
    if(getcontext(&m_ctx)){
        abort();
    }
    m_ctx.uc_link = &t_threadIber_1->m_ctx;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    
    makecontext(&m_ctx, &Fiber_1::MainFunc, 0);
}

Fiber_1::~Fiber_1(){
    --s_fiber_count;
    // 有栈则为子协程
    if(m_stack){
        assert(m_state == TERM || 
                m_state == INIT || 
                m_state == EXCEPT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    }
    // 没有则为主协程
    else{
        assert(!m_cb);
        assert(m_state ==EXEC);
        Fiber_1* cur = t_fiber_1;

        if(cur == this){
            SetThis(nullptr);
        }
    }
    
    SERVER_LOG_INFO(SERVER_LOGGER_SYSTEM) << "Fiber destroy id:" << m_id;
}

void Fiber_1::reset(std::function<void()> cb){
    assert(m_stack);
    assert(m_state == TERM || m_state == INIT || m_state == EXCEPT);
    m_cb = cb;

    if(getcontext(&m_ctx)){
        assert(0);
    }
    m_ctx.uc_link = &t_threadIber_1->m_ctx;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    
    makecontext(&m_ctx, &Fiber_1::MainFunc, 0);
    m_state = INIT;
}
void Fiber_1::swapIn(){
    assert(m_state != EXEC);
    SetThis(this);
    m_state = EXEC;
    if(swapcontext(&t_threadIber_1->m_ctx, &m_ctx)){
        assert(0);
    }

}
void Fiber_1::swapOut(){
    SetThis(t_threadIber_1.get()) ;
    if(swapcontext(&m_ctx, &t_threadIber_1->m_ctx))
        assert(0);
}

void Fiber_1::SetThis(Fiber_1 *f){
    t_fiber_1 = f;
}

Fiber_1::ptr Fiber_1::GetThis(){
    if(t_fiber_1){
        return t_fiber_1->shared_from_this();
    }
    Fiber_1::ptr main_fiber(new Fiber_1);
    assert(t_fiber_1 == main_fiber.get());
    t_threadIber_1 = main_fiber;
    return t_fiber_1->shared_from_this();
}

void Fiber_1::YieldToReady(){
    Fiber_1::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}

void Fiber_1::YieldToHold(){
    Fiber_1::ptr cur = GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
}

uint64_t Fiber_1::TotalFibers(){
    return s_fiber_count;
}

uint64_t Fiber_1::GetCurFiberId(){
    if(t_fiber_1)
        return t_fiber_1->getId();
    else
        return 0;
}

void Fiber_1::MainFunc(){
    Fiber_1::ptr cur = GetThis();
    try{
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }
    catch(...){
        cur->m_state = EXCEPT;
        SERVER_LOG_ERROR(SERVER_LOGGER_SYSTEM) << "fiber main err";
    }
    cur.reset();
}


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
    t_fiber_ = t_threadIber_.get();
    return {};
}

std::suspend_always CoRet::promise_type::yield_value(State s) {
    t_fiber_ = t_threadIber_.get();
    m_state = s;
    return {};
}

void CoRet::promise_type::return_value(State s){
    t_fiber_ = t_threadIber_.get();
    m_state = s;
}


Fiber_::Fiber_(){
    SetThis(this);
    ++s_fiber_count;
}

Fiber_::Fiber_(std::function<CoRet()> cb)
    :m_id(++s_fiber_id), m_cb(cb())
{
    ++s_fiber_count;
}

Fiber_::~Fiber_(){
    --s_fiber_count;    
    SERVER_LOG_INFO(SERVER_LOGGER_SYSTEM) << "fiber destroy id:" << m_id;
}

void Fiber_::reset(std::function<CoRet()> cb){
    m_cb = cb();
}

void Fiber_::swapIn(){
    if(!m_cb.done()){
        t_fiber_ = this;
        m_cb();
    }
}

uint64_t Fiber_::GetCurFiberId(){
    if(t_fiber_){
        return t_fiber_->getId();
    }
    return 0;
}


} // namespace server
