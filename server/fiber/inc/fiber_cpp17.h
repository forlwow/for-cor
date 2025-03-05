#if __cplusplus < 202002L

#ifndef SERVER_FIBER_17_H
#define SERVER_FIBER_17_H

#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <ucontext.h>
#include <memory>
#include <functional>
#include "enums.h"


namespace server{

class Fiber_1;

typedef Fiber_1 Fiber;

class Fiber_1: public std::enable_shared_from_this<Fiber_1>{
public:
    typedef std::shared_ptr<Fiber_1> ptr;

public:
    Fiber_1(std::function<void()> cb, size_t stacksize=0);
    ~Fiber_1();

    void reset(std::function<void()> cb);       // 重置协程函数和状态[INIT, TERM]
    void reset_thread();                        // 当切换线程时需要调用
    bool set_return_to(Fiber_1::ptr to_);
    void swapIn();                              // 切换到当前协程执行
    void swapOut();                             // 把当前协程切换到后台

    uint64_t getId(){return m_id;}
    State GetState(){return m_state;}
public:
    static void SetThis(Fiber_1 *f);            // 设置当前协程
    static Fiber_1::ptr GetThis();              // 返回当前执行的协程
    static void YieldToReady();                 // 协程切换到后台，并设置为Ready状态
    static void YieldToHold();                  // 协程切换到后台，并设置为Hold状态
    static void YieldReturn();                  // 结束当前协程
    static uint64_t TotalFibers();              // 返回总协程数
    static uint64_t GetCurFiberId();            // 获取当前协程号
    static void MainFunc();
private:
    Fiber_1();

private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;
    void* m_stack = nullptr;

    std::function<void()> m_cb;
};

}

#endif // SERVER_FIBER_17_H

#endif
