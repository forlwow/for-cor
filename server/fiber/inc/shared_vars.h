#ifndef SERVER_SHARED_VARS_H
#define SERVER_SHARED_VARS_H

#include <atomic>

namespace server {

static std::atomic<uint64_t> s_fiber_id = 0;
static std::atomic<uint64_t> s_fiber_count = 0;

}


#endif
