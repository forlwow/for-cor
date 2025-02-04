#ifndef SERVER_TEST_H
#define SERVER_TEST_H
#if __cplusplus >= 202002L
#include "address.h"
#include "socketfunc_cpp20.h"
#include <cerrno>
#include <cstddef>
#include "ethread.h"
#include "timer.h"
#include "sys/socket.h"
#include "sys/types.h"
#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <log.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <csignal>
#include <vector>
#include "fiberfunc_cpp20.h"

#define in :

#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG

int test_cpp20(){
    return 0;
}




#endif

#endif
