#if __cplusplus < 202002L
#include "enums.h"
#include "ethread.h"
#include "fiber.h"
#include "iomanager.h"
#include "range.h"
#include "timer.h"
#include "scheduler.h"
#include "sys/socket.h"
#include "sys/types.h"
#include "arpa/inet.h"
#include "arpa/telnet.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <log.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <string>

#include <timer.h>
#include <vector>
using namespace std;

auto slog = SERVER_LOGGER_SYSTEM;

void fib_func(){
    auto fib = server::Fiber::GetThis();
    SERVER_LOG_INFO(slog) << "before yield 1";
    //fib->YieldToHold();
    //SERVER_LOG_INFO(slog) << "before yield 2";
    //fib->YieldToReady();
    //SERVER_LOG_INFO(slog) << "after yield 2";
}


int main(){

}

#endif
