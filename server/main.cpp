#include "enums.h"
#include "ethread.h"
#include "fiber_.h"
#include "iomanager_.h"
#include "fiber.h"
#include "iomanager.h"
#include "range.h"
#include "scheduler.h"
#include "scheduler_.h"
#include "sys/socket.h"
#include "sys/types.h"
#include "arpa/inet.h"
#include "arpa/telnet.h"
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

#define in :

using namespace std;

auto logger = SERVER_LOGGER_SYSTEM;
auto slog = SERVER_LOGGER("system");
server::CoRet fiber_timer_cir();
void test1();
void test2();
void test3();
void test4();
void test5();

void fib_func(){
    auto fib = server::Fiber::GetThis();
    SERVER_LOG_INFO(slog) << "before yield 1";
    //fib->YieldToHold();
    //SERVER_LOG_INFO(slog) << "before yield 2";
    //fib->YieldToReady();
    //SERVER_LOG_INFO(slog) << "after yield 2";
}
server::CoRet run_in_fiber(){
    SERVER_LOG_INFO(logger) << "enter fiber";
    sleep(9);
    SERVER_LOG_INFO(logger) << "begin run in fiber";
    co_yield server::HOLD;
    SERVER_LOG_INFO(logger) << "before end run in fiber";
    co_yield server::HOLD;
    SERVER_LOG_INFO(logger) << "before end run in fiber";
    co_yield server::HOLD;
    SERVER_LOG_INFO(logger) << "before end run in fiber";
    co_yield server::HOLD;
    SERVER_LOG_INFO(logger) << "end run in fiber";
    co_return server::TERM;
}

server::CoRet count_in_fiber(){
    uint i = 0;
    while (1) {
        SERVER_LOG_INFO(logger) << "count:" << i++;
        co_yield server::HOLD;
    }
}


int main(){
    test5();
    return 0;
}



void test1(){
    server::IOManager iom(2);
    iom.schedule(make_shared<server::Fiber_1>(fib_func));

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8080);
    // int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    if(!bind(sock, (const sockaddr*)&addr, sizeof(addr))){

    iom.addEvent(sock, server::IOManager::WRITE, [](){
            SERVER_LOG_INFO(logger) << "connect write";
            });
    iom.addEvent(sock, server::IOManager::READ, [](){
            SERVER_LOG_INFO(logger) << "connect read";
            });
    }
    else{
        SERVER_LOG_INFO(logger) << "bind error";
    }
    iom.start();
    iom.wait();
    //iom.stop();
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
    server::Fiber::GetThis();
    SERVER_LOG_INFO(logger) << "main begin";
    
    server::Fiber_::ptr fiber(new server::Fiber_(run_in_fiber));
    SERVER_LOG_INFO(logger) << "done/" << fiber->done();
    fiber->swapIn();
    SERVER_LOG_INFO(logger) << "done/" << fiber->done();
    SERVER_LOG_INFO(logger) << "main after swapIn";
    
    fiber->swapIn();
    SERVER_LOG_INFO(logger) << "done/" << fiber->done();
    fiber->swapIn();
    SERVER_LOG_INFO(logger) << "done/" << fiber->done();
    SERVER_LOG_INFO(logger) << "main end";
    // while (!fiber->done()) {
    //     fiber->swapIn();
    // }
}

void test4(){
    server::IOManager_ iom(2);
    // iom.schedule(make_shared<server::Fiber_>(run_in_fiber));

    iom.addTimer(1000, fiber_timer_cir);
    iom.addTimer(5000, fiber_timer_cir);
    iom.start();
    iom.wait();
    iom.stop();
}

server::CoRet fiber_timer_cir(){
    while(1){
        SERVER_LOG_DEBUG(logger) << "circulate timer trigger";
        co_yield server::HOLD;
    }
}
void p(){SERVER_LOG_INFO(logger) << "test";}

void test5(){
    server::IOManager_ iom(5);
    server::Fiber_2::ptr fib(new server::Fiber_2(run_in_fiber, false)); 
    server::Fiber_2::ptr fib2(new server::Fiber_2(p)); 
    iom.start();
    iom.addTimer(5000, run_in_fiber, true);
    iom.addTimer(1000, count_in_fiber, false);
    iom.wait(100);
}
