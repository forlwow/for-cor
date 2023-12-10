#include "fiber.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <atomic>
#include <ucontext.h>

#include "log.h"

namespace server{

static std::atomic<uint64_t> s_fiber_id = 0;        // 协程号 跨线程
static std::atomic<uint64_t> s_fiber_count = 0;     // 协程数量 跨线程
thread_local Fiber* t_fiber = nullptr;              // 当前执行的协程
thread_local Fiber::ptr t_threadIber = nullptr;     // 主协程

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

Fiber::Fiber(){
    m_state = EXEC;
    SetThis(this);
    if(getcontext(&m_ctx)){
        abort();
    }
    ++s_fiber_count;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize)
    :m_id(++s_fiber_id), m_cb(cb)
{
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize: 1024 * 1024;

    // 分配栈大小
    m_stack = StackAllocator::Alloc(m_stacksize);
    if(getcontext(&m_ctx)){
        abort();
    }
    m_ctx.uc_link = &t_threadIber->m_ctx;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
}

Fiber::~Fiber(){
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
        Fiber* cur = t_fiber;

        if(cur == this){
            SetThis(nullptr);
        }
    }
    
    SERVER_LOG_INFO(SERVER_LOGGER_SYSTEM) << "Fiber destroy id:" << m_id;
}

void Fiber::reset(std::function<void()> cb){
    assert(m_stack);
    assert(m_state == TERM || m_state == INIT || m_state == EXCEPT);
    m_cb = cb;

    if(getcontext(&m_ctx)){
        assert(0);
    }
    m_ctx.uc_link = &t_threadIber->m_ctx;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}
void Fiber::swapIn(){
    assert(m_state != EXEC);
    SetThis(this);
    m_state = EXEC;
    if(swapcontext(&t_threadIber->m_ctx, &m_ctx)){
        assert(0);
    }

}
void Fiber::swapOut(){
    SetThis(t_threadIber.get()) ;
    if(swapcontext(&m_ctx, &t_threadIber->m_ctx))
        assert(0);
}

void Fiber::SetThis(Fiber *f){
    t_fiber = f;
}

Fiber::ptr Fiber::GetThis(){
    if(t_fiber){
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    assert(t_fiber == main_fiber.get());
    t_threadIber = main_fiber;
    return t_fiber->shared_from_this();
}

void Fiber::YieldToReady(){
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}

void Fiber::YieldToHold(){
    Fiber::ptr cur = GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
}

uint64_t Fiber::TotalFibers(){
    return s_fiber_count;
}

uint64_t Fiber::GetCurFiberId(){
    if(t_fiber)
        return t_fiber->getId();
    else
        return 0;
}

void Fiber::MainFunc(){
    Fiber::ptr cur = GetThis();
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

} // namespace server
