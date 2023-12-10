#ifndef SERVER_FIBER_H
#define SERVER_FIBER_H

#include <cstddef>
#include <cstdint>
#include <sys/ucontext.h>
#include <ucontext.h>
#include <memory>
#include <functional>


namespace server{

class Fiber: public std::enable_shared_from_this<Fiber>{
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State{
        INIT,       // 初始化状态
        HOLD,       // 暂停挂起状态
        EXEC,       // 结束状态
        TERM,       // 结束状态
        READY,      // 可执行状态
        EXCEPT      // 出错状态
    };

public:
    Fiber(std::function<void()> cb, size_t stacksize=0);
    ~Fiber();

    void reset(std::function<void()> cb);       // 重置协程函数和状态[INIT, TERM]
    void swapIn();                              // 切换到当前协程执行
    void swapOut();                             // 把当前协程切换到后台

    uint64_t getId(){return m_id;}
public:
    static void SetThis(Fiber *f);              // 设置当前协程
    static Fiber::ptr GetThis();                // 返回当前执行的协程
    static void YieldToReady();                 // 协程切换到后台，并设置为Ready状态
    static void YieldToHold();                  // 协程切换到后台，并设置为Hold状态
    static uint64_t TotalFibers();              // 返回总协程数
    static uint64_t GetCurFiberId();            // 获取当前协程号
    static void MainFunc();
private:
    Fiber();

private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;
    void* m_stack = nullptr;

    std::function<void()> m_cb;
};


}

#endif // SERVER_FIBER_H
