#ifndef SERVER_ASYNC_H
#define SERVER_ASYNC_H

#include <atomic>
#include <concepts>
#include <log.h>
#if __cplusplus >= 202002L

#include <coroutine>
#include <memory>
#include <functional>
#include "fiber_cpp20.h"

namespace server{

enum TaskState {
    RUN,            // 运行中
    YIELD,          // 让出执行权
    SUSPEND,        // 暂停执行
    ERR,            // 发生错误
};

typedef void(*errorHandler)(int);

struct Task{
    struct promise_type{
        TaskState m_state = TaskState::SUSPEND;
        int errnum = 0;
        errorHandler handler = nullptr;

        std::suspend_always initial_suspend() const noexcept {return {};}
        std::suspend_never final_suspend() const noexcept {return {};}
        void unhandled_exception() {
            SERVER_LOG_ERROR(SERVER_LOGGER_SYSTEM) << "Task Error: " << errnum;
            if(handler) handler(errnum);
        }
        Task get_return_object() {
            return {std::coroutine_handle<Task::promise_type>::from_promise(*this)};
        }
        std::suspend_always yield_value(TaskState s);
        std::suspend_always yield_value(TaskState s, int err);
        void return_void();
    };

    std::coroutine_handle<promise_type> h_;
    void operator()(){h_.resume();}
    bool done(){return h_.done() || iserror();}
    bool iserror() {return h_.address() == nullptr || h_.promise().errnum != 0;}
}; 


// 协程包装 线程安全
class AsyncFiber: public Fiber_{
public:
    typedef std::shared_ptr<AsyncFiber> ptr;

public:
    AsyncFiber(std::function<Task()> cb): m_cb(cb()) {}
    template<typename Func, typename... Args>
    requires std::invocable<Func, Args...> 
        && std::same_as<std::invoke_result_t<Func, Args...>, Task>
    AsyncFiber(Func &&func, Args &&...args){
        m_cb = std::bind_front(std::forward<Func>(func), std::forward<Args>(args)...)();
    }

    template<typename Func, typename... Args>
    requires std::invocable<Func, Args...> 
        && std::same_as<std::invoke_result_t<Func, Args...>, Task>
    static ptr CreatePtr(Func &&func, Args &&...args){
        return std::make_shared<AsyncFiber>(std::forward<Func>(func), std::forward<Args>(args)...);
    }

    void setErrHandler(errorHandler handler) {m_cb.h_.promise().handler = handler;}

    virtual ~AsyncFiber()=default;

    virtual bool swapIn() override;                               // 切换到当前协程执行
    virtual bool done() override{
        return m_cb.done();
    }

    void setDone(bool done = false) override {}
    
private:
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
    Task m_cb;
};

// 函数包装
// 执行后会将done设置为true，需要自行置位
class FuncFiber: public Fiber_{
public:
    typedef std::shared_ptr<FuncFiber> ptr;

    template<typename Func, typename ...Args>
    requires std::invocable<Func, Args...>
    FuncFiber(Func &&func, Args &&...args){
        m_cb = std::bind_front(std::forward<Func>(func), std::forward<Args>(args)...);
    }

    template<typename Func, typename... Args>
    requires std::invocable<Func, Args...> 
    static ptr CreatePtr(Func &&func, Args &&...args){
        return ptr(new FuncFiber(std::forward<Func>(func), std::forward<Args>(args)...));
    }

    ~FuncFiber()=default;

    virtual bool swapIn() override;
    virtual bool done() override{return m_done;}

    void setDone(bool d = false){m_done = d;}

private:
    bool m_done = false;
    std::function<void()> m_cb;
};


}   // namespace server

#endif // cpp20
#endif // SERVER_ASYNC_H