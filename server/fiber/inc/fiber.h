#ifndef SERVER_FIBER_H
#define SERVER_FIBER_H

#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <ucontext.h>
#include <memory>
#include <functional>


namespace server{

class Fiber_1;
class Fiber_;

typedef Fiber_1 Fiber;

enum State{
    INIT,       // 初始化状态
    HOLD,       // 暂停挂起状态
    EXEC,       // 结束状态
    TERM,       // 结束状态
    READY,      // 可执行状态
    EXCEPT      // 出错状态
};

class Fiber_1: public std::enable_shared_from_this<Fiber_1>{
public:
    typedef std::shared_ptr<Fiber_1> ptr;

public:
    Fiber_1(std::function<void()> cb, size_t stacksize=0);
    ~Fiber_1();

    void reset(std::function<void()> cb);       // 重置协程函数和状态[INIT, TERM]
    void swapIn();                              // 切换到当前协程执行
    void swapOut();                             // 把当前协程切换到后台

    uint64_t getId(){return m_id;}
    State GetState(){return m_state;}
public:
    static void SetThis(Fiber_1 *f);            // 设置当前协程
    static Fiber_1::ptr GetThis();              // 返回当前执行的协程
    static void YieldToReady();                 // 协程切换到后台，并设置为Ready状态
    static void YieldToHold();                  // 协程切换到后台，并设置为Hold状态
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

// c++20 协程

class Fiber_;

struct CoRet{
    struct promise_type{
        State m_state = State::INIT;
        std::suspend_always initial_suspend() const noexcept;
        std::suspend_never final_suspend() const noexcept;
        void unhandled_exception();
        CoRet get_return_object();
        std::suspend_always yield_void();
        std::suspend_always yield_value(State s);
        void return_value(State s);

    };
    std::coroutine_handle<promise_type> h_;
    void operator()(){h_.resume();}
    bool done(){return h_.done();}
};

class Fiber_: public std::enable_shared_from_this<Fiber_>{
public:
    typedef std::shared_ptr<Fiber_> ptr;


public:
    Fiber_(std::function<CoRet()> cb);
    ~Fiber_();

    void reset(std::function<CoRet()> cb);       // 重置协程函数和状态[INIT, TERM]
    void swapIn();                               // 切换到当前协程执行
    bool done();
    uint64_t getId(){return m_id;}
public:
    static void SetThis(Fiber_ *f){}             // 设置当前协程
    static Fiber_::ptr GetThis(){return {};}     // 返回当前执行的协程
    static uint64_t TotalFibers(){}              // 返回总协程数
    static uint64_t GetCurFiberId();             // 获取当前协程号
    static void MainFunc(){}
private:
    Fiber_();

private:
    uint64_t m_id = 0;

    CoRet m_cb;

};

}

#endif // SERVER_FIBER_H
