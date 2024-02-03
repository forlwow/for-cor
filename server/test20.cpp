#if __cplusplus >= 202002L
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
#include "fiberfunc_cpp20.h"
using namespace std;

#define in :

auto logger = SERVER_LOGGER_SYSTEM;
auto slog = SERVER_LOGGER("system");
server::CoRet fiber_timer_cir();
void test1();
void test2();
void test3();
void test4();
void test5();

server::CoRet run_in_fiber(){
    SERVER_LOG_INFO(logger) << "enter fiber";
    co_await server::sleep(5);
    SERVER_LOG_INFO(logger) << "begin run in fiber";
    co_yield server::HOLD;
    SERVER_LOG_INFO(logger) << "before end run in fiber";
    co_yield server::HOLD;
    SERVER_LOG_INFO(logger) << "end run in fiber";
    co_return server::TERM;
}

server::CoRet count_in_fiber(){
    uint i = 0;
    while (i < 10) {
        SERVER_LOG_INFO(logger) << "count:" << i++;
        co_yield server::HOLD;
    }
    co_return server::TERM;
}

atomic<int> g_count;
server::CoRet test_Fiber(){
    for(auto i in range(20)){
        co_yield server::HOLD;
    }
    ++g_count;
    co_return server::TERM;
}




void test1(){
}

void test2(){
    server::Fiber_::ptr task(new server::Fiber_(run_in_fiber));
    for(auto i in range(10)){
        SERVER_LOG_DEBUG(logger) << task->done();
        if(!task->done()){
            task->swapIn();
        }
    }
}

void test3(){
}


void test4(){
    server::Scheduler_ sc(5);
    server::Fiber_2::ptr fib2(new server::Fiber_2(test_Fiber, false));
    Timer timer;
    for(auto i in range(5000)){
        server::Fiber_::ptr fib(new server::Fiber_(test_Fiber));
        sc.schedule(fib);
    }
    timer.start_count();
    sc.start();
    sc.wait_stop();
    timer.end_count();
    SERVER_LOG_INFO(logger) << timer.get_duration();
    SERVER_LOG_INFO(logger) << g_count.load();
}

server::CoRet fiber_timer_cir(){
    while(1){
        SERVER_LOG_DEBUG(logger) << "circulate timer trigger";
        co_yield server::HOLD;
    }
}
void p(){SERVER_LOG_INFO(logger) << "test";}

void test5(){
    server::IOManager_ iom(2);
    server::Fiber_2::ptr fib2(new server::Fiber_2(run_in_fiber)); 

    iom.schedule(fib2);
    for(auto i in range(10)){
        server::Fiber_2::ptr fib(new server::Fiber_2(count_in_fiber)); 
        iom.schedule(fib);
    }
    iom.start();
    iom.wait(100);
    // iom.wait_stop();
}


#endif
