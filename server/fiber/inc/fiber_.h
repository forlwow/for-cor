#ifndef SERVER_FIBER__H
#define SERVER_FIBER__H

#include <coroutine>
#include <memory>
#include <functional>
#include <atomic>
#include <variant>
#include "enums.h"

namespace server {

// c++20 协程

class Fiber_;

struct CoRet{
    struct promise_type{
        State m_state = State::INIT;
        bool *m_done = nullptr;
        std::function<void()> m_cbBeforeYield = nullptr;
        std::function<void()> m_cbBeforeReturn = nullptr;

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
    bool done(){return h_.promise().m_done;}
};

class Fiber_{
public:
    typedef std::shared_ptr<Fiber_> ptr;

public:
    Fiber_(std::function<CoRet()> cb);
    Fiber_();
    virtual ~Fiber_();

    virtual void reset(std::function<CoRet()> cb);       // 重置协程函数和状态[INIT, TERM]
    virtual bool swapIn();                               // 切换到当前协程执行
    virtual bool done();
    uint64_t getId(){return m_id;}
public:
    static Fiber_::ptr GetThis(){return {};}     // 返回当前执行的协程
    static uint64_t TotalFibers(){}              // 返回总协程数
    static uint64_t GetCurFiberId();             // 获取当前协程号

protected:
    uint64_t m_id = 0;
    bool m_done = false;
private:
    CoRet m_cb;
};

// 线程安全的协程
// 可以同时传入普通函数和协程
// 使用普通函数时不保证函数内部的线程安全
class Fiber_2 :public Fiber_{
public:
    typedef std::shared_ptr<Fiber_2> ptr;
    enum {
        FUNCTION = 0,
        COROUTINE,
    };

public:
    Fiber_2(std::function<CoRet()> cb, bool drop_ = false);
    Fiber_2(std::function<void()> cb);
    ~Fiber_2() override;

    bool swapIn() override;                               // 切换到当前协程执行
    bool isDrop() const {return m_drop; }

    void setCbBeforeYield(std::function<void()>);
    void setCbBeforeSwapIn(std::function<void()>);
    void setCbBeforeReturn(std::function<void()>);
private:
    Fiber_2();
    using Fiber_::reset;
private:
    std::variant<std::function<void()>, CoRet> m_cb;
    bool m_drop = false;                        // 当协程被多进程操作时是否要丢弃
    std::function<void()> m_cbBeforeSwapIn;
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT;


};


} // namespace server

#endif SERVER_FIBER__H
