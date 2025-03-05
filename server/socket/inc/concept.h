#ifndef SERVER_CONCEPT_H
#define SERVER_CONCEPT_H

#include "fiber_cpp20.h"
#include "async.h"

namespace server {

template<typename T>
concept FiberPromise = requires {requires std::same_as<std::coroutine_handle<CoRet::promise_type>, T>
                            || std::same_as<std::coroutine_handle<Task::promise_type>, T>;};


}

#endif