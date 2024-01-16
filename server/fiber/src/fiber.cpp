#include "fiber.h"
#include "shared_vars.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <ucontext.h>

#include "log.h"

namespace server{

thread_local Fiber_1* t_fiber_1 = nullptr;          // 当前执行的协程
thread_local Fiber_1::ptr t_threadIber_1 = nullptr; // 主协程

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
    
    SERVER_LOG_INFO2(SERVER_LOGGER_SYSTEM) << "Fiber destroy id:" << m_id;
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

void Fiber_1::reset_thread(){
    m_ctx.uc_link = &t_threadIber_1->m_ctx;
}

bool Fiber_1::set_return_to(Fiber_1::ptr to_){
    if(!to_)
        return false;
    m_ctx.uc_link = &to_->m_ctx;
    return true;
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

void Fiber_1::YieldReturn(){
    Fiber_1::ptr cur = GetThis();
    cur->m_state = TERM;
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
        SERVER_LOG_ERROR2(SERVER_LOGGER_SYSTEM) << "fiber main err";
    }
        // cur.reset();
        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->swapOut();
}



} // namespace server
