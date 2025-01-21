#include "shared_vars.h"
#include <type_traits>
#include <utility>
#if __cplusplus >= 202002L // 判断标准

#ifndef SERVER_FIBER_20_H
#define SERVER_FIBER_20_H

#include <coroutine>
#include <memory>
#include <functional>
#include <atomic>
#include <variant>
#include <sys/epoll.h>
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

template<typename T>
concept FunctionPointer = std::is_function_v<std::remove_pointer_t<std::decay_t<T>>>;

class Fiber_: public std::enable_shared_from_this<Fiber_>{
public:
    typedef std::shared_ptr<Fiber_> ptr;

public:
    virtual ~Fiber_();

    virtual bool swapIn() = 0;                               // 切换到当前协程执行
    virtual bool done() = 0;
public:
    static std::weak_ptr<Fiber_> GetThis();                                   // 返回当前执行的协程
    static void SetThis(std::weak_ptr<Fiber_>);                                   // 设置当前执行的协程
    static uint64_t TotalFibers(){return s_fiber_count;}            // 返回总协程数
    static uint64_t GetCurFiberId();                                // 获取当前协程号

};

// TODO: 可拆成多个Fiber
// 线程安全的协程
// 可以同时传入普通函数和协程
// 使用普通函数时不保证函数内部的线程安全
class Fiber_2 :public Fiber_{
public:
    typedef std::shared_ptr<Fiber_2> ptr;
    typedef CoRet(*co_fun)();
    typedef void(*void_fun)();
    enum {
        FUNCTION = 0,
        COROUTINE,
    };

public:
    Fiber_2(co_fun cb, bool drop_ = false);
    explicit Fiber_2(std::function<CoRet()> cb, bool drop_ = false);
    Fiber_2(void_fun cb);
    explicit Fiber_2(std::function<void()> cb);
    ~Fiber_2() override;

    bool swapIn() override;                               // 切换到当前协程执行
    bool done() {return m_done;}
    bool isDrop() const {return m_drop; }

    void setCbBeforeYield(std::function<void()>);
    void setCbBeforeSwapIn(std::function<void()>);
    void setCbBeforeReturn(std::function<void()>);
private:
    Fiber_2();
private:
    bool m_done;
    bool m_drop = false;                                // 当协程被多进程操作时是否要丢弃
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
    std::variant<std::function<void()>, CoRet> m_cb;
    std::function<void()> m_cbBeforeSwapIn;
};

class FiberIO: public Fiber_2{
public:
    FiberIO(co_fun cb, bool drop_ = false):Fiber_2(cb, drop_){}
    explicit FiberIO(std::function<CoRet()> cb, bool drop_ = false):Fiber_2(cb, drop_){}
    FiberIO(void_fun cb):Fiber_2(cb){}
    explicit FiberIO(std::function<void()> cb):Fiber_2(cb){}
    bool isRead(){
        return events & EPOLLIN;
    }
    bool isWrite(){ return events & EPOLLOUT;}
    void setRead() {events |= EPOLLIN;}
    void setWrite() {events |= EPOLLOUT;}
    void resetRead() {events &= ~EPOLLIN;}
    void resetWrite() {events &= ~EPOLLOUT;}


private:
    int events;
};


} // namespace server

#endif // SERVER_FIBER_20_H


#endif
