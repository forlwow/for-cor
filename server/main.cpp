#include "ethread.h"
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

using namespace std;

auto logger = SERVER_LOGGER_SYSTEM;
auto slog = SERVER_LOGGER("system");
server::CoRet fiber_timer_cir();
void test1();
void test2();
void test3();
void test4();
void fib_func(){
    auto fib = server::Fiber::GetThis();
    SERVER_LOG_INFO(slog) << "before yield 1";
    //fib->YieldToHold();
    //SERVER_LOG_INFO(slog) << "before yield 2";
    //fib->YieldToReady();
    //SERVER_LOG_INFO(slog) << "after yield 2";
}
server::CoRet run_in_fiber(){
    SERVER_LOG_INFO(logger) << "begin run in fiber";
    co_yield server::HOLD;
    SERVER_LOG_INFO(logger) << "before end run in fiber";
    co_return server::TERM;
    SERVER_LOG_INFO(logger) << "end run in fiber";
}


int main(){
    test4();
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
    Timer counter;
    server::Fiber::GetThis();
    server::Scheduler sc(4);
    server::Scheduler_ sc_(4);
    for(auto i : range<int>(2e0)){
        //server::Fiber_::ptr fib(new server::Fiber_(run_in_fiber));
        //sc_.schedule(fib);
        auto fib = make_shared<server::Fiber_1>(fib_func);
        sc.schedule(fib);
    }
    counter.start_count();
    sc.start();
    sc.stop();
    //sc_.start();
    //sc_.wait_stop();
    counter.end_count();
    cout << counter.get_duration() << endl;
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
