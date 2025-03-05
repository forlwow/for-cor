#ifndef SERVER_SCHEDULER_H
#define SERVER_SCHEDULER_H

#if __cplusplus >= 202002L
#include "scheduler_cpp20.h"
namespace server {
typedef Scheduler_ Scheduler;
}
#else
#include "scheduler_cpp17.h"
namespace server {

}

#endif


#endif
