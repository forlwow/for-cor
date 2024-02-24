#if __cplusplus >= 202002L
#include <memory>
#include "async.h"
#include "iomanager_cpp20.h"

namespace server{

extern thread_local std::weak_ptr<Fiber_> t_fiber_;

std::suspend_always Task::promise_type::yield_value(TaskState s){
    m_state = s;
    switch (s) {
        case YIELD:
            IOManager_::GetIOManager()->schedule(Fiber_::GetThis().lock());
            break;
        case SUSPEND:
            break;
        case ERR:
            handler(errnum);
            break;
        default:
            break;
    }
    return {};
}

std::suspend_always Task::promise_type::yield_value(TaskState s, int err){
    handler(err);
    return {};
}

void Task::promise_type::return_void(){

}


bool AsyncFiber::swapIn(){
    if (m_flag.test_and_set()){
        return false;
    }
    t_fiber_ = weak_from_this();
    m_cb();
    t_fiber_.reset();
    m_flag.clear();
    return true;
}



bool FuncFiber::swapIn(){
    t_fiber_ = weak_from_this();
    m_cb();
    t_fiber_.reset();
    return true;
}


}; // namepspace server


#endif