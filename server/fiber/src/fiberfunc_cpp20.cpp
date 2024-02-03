#include <chrono>
#include <coroutine>
#include <thread>
#if __cplusplus >= 202002L

#include "fiberfunc_cpp20.h"
#include "iomanager_cpp20.h"

namespace server {

sleep::sleep(int time)
    :m_time(time)
{

}

void sleep::await_suspend(std::coroutine_handle<CoRet::promise_type> handle){
    auto fib = Fiber_::GetThis();
    auto iom = IOManager_::GetIOManager();
    if(!iom && !fib){
        std::this_thread::sleep_for(std::chrono::seconds(m_time));
        handle.resume();
        return ;
    }
    iom->addTimer(m_time * 1000, [fib, iom]{
        iom->schedule(fib);
    }, false);

}

void msleep::await_suspend(std::coroutine_handle<CoRet::promise_type> handle){
    auto fib = Fiber_::GetThis();
    auto iom = IOManager_::GetIOManager();
    if(!iom && !fib){
        std::this_thread::sleep_for(std::chrono::seconds(m_time));
        return ;
    }
    iom->addTimer(m_time, [fib, iom]{
        iom->schedule(fib);
    }, false);

}


} // namespace server

#endif
