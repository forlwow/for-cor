#ifndef SERVER_TEST_H
#define SERVER_TEST_H
#if __cplusplus >= 202002L
#include "address.h"
#include "async.h"
#include "http.h"
#include "socket.h"
#include "socketfunc_cpp20.h"
#include <cerrno>
#include <cstddef>
#include "enums.h"
#include "ethread.h"
#include "range.h"
#include "timer.h"
#include "sys/socket.h"
#include "sys/types.h"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <log.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <vector>
#include "fiberfunc_cpp20.h"
#include "test_http.h"
#include "test_fiber.h"
#include "test_log.h"
using namespace std;

#define in :

auto logger = SERVER_LOGGER_SYSTEM;
auto slog = SERVER_LOGGER("system");
server::CoRet fiber_timer_cir();

int main(){
    server::test::httpv2_response_test();
}




#endif

#endif
