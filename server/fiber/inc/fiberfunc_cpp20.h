#include <cstdint>
#if __cplusplus >= 202002L
#ifndef SERVER_FIBERFUNC_20_H
#define SERVER_FIBERFUNC_20_H

#include "fiber_cpp20.h"

namespace server{

struct sleep{
    sleep(int time);
    bool await_ready() {
        return false;
    }
    void await_suspend(std::coroutine_handle<CoRet::promise_type> handle); 
    void await_resume() {}
    int m_time = 0;
};

struct msleep{
    msleep(uint64_t time);
    bool await_ready() {
        return false;
    }
    void await_suspend(std::coroutine_handle<CoRet::promise_type> handle); 
    void await_resume() {}
    int m_time = 0;
};


} // namespace server


#endif //SERVER_FIBERFUNC_20_H

#endif
