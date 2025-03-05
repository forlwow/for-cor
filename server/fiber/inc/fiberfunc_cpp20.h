#include "async.h"
#include <chrono>
#include <concepts>
#include <coroutine>
#include <cstdint>
#if __cplusplus >= 202002L
#ifndef SERVER_FIBERFUNC_20_H
#define SERVER_FIBERFUNC_20_H

#include "fiber_cpp20.h"
#include "iomanager_cpp20.h"
#include "concept.h"

namespace server{

struct sleep{
    sleep(int time);
    bool await_ready() {
        return false;
    }

    template<FiberPromise T>
    void await_suspend(T handle){
    auto fib = Fiber_::GetThis().lock();
    auto iom = IOManager_::GetIOManager();
    if(!iom && !fib){
        std::this_thread::sleep_for(std::chrono::seconds(m_time));
        handle.resume();
        return ;
    }
    iom->addTimer(m_time * 1000, false, [fib, iom]{
        iom->schedule(fib);
    });

}
    void await_resume() {}
    int m_time = 0;
};

struct msleep{
    msleep(uint64_t time): m_time(time){
        
    }
    bool await_ready() {
        return false;
    }

    template<FiberPromise T>
    void await_suspend(T handle){
    auto fib = Fiber_::GetThis().lock();
    auto iom = IOManager_::GetIOManager();
    if(!iom && !fib){
        std::this_thread::sleep_for(std::chrono::milliseconds(m_time));
        return ;
    }
    iom->addTimer(m_time, false, [fib, iom]{
        iom->schedule(fib);
    });

}
    void await_resume() {}
    uint64_t m_time = 0;
};


} // namespace server


#endif //SERVER_FIBERFUNC_20_H

#endif
