#include "ethread.h"
#include "fiber.h"
#include "scheduler.h"
#include <functional>
#include <log.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <unistd.h>
#include <string>

#include <timer.h>

using namespace std;

auto logger = SERVER_LOGGER_SYSTEM;
void test1();
void test2();
void test3();
void fib_func(){
    auto fib = server::Fiber::GetThis();
    SERVER_LOG_INFO(logger) << "before yield 1";
    fib->YieldToHold();
    SERVER_LOG_INFO(logger) << "before yield 2";
    fib->YieldToReady();
    SERVER_LOG_INFO(logger) << "after yield 2";
}

int main(){
    SERVER_LOG_INFO(logger) << "main begin-1";
    test2();
    SERVER_LOG_INFO(logger) << "main end-1";
    return 0;
}



void test1(){
    SERVER_LOG_INFO(logger) << "test begin";
    int count = 0;
    auto fun1 = std::function<void()>(
        [&](){
            int i = 4;
            while(i--){
                SERVER_LOG_INFO(logger) 
                << "name:" 
                << server::EThread::GetName() << " "
                << "id: " 
                << syscall(SYS_gettid) << " "
                ;
            }
            }
        );
    Timer counter;
    counter.start_count();

    std::vector<server::EThread::ptr> thp;
    for (int i = 0; i < 5; ++i){
        thp.emplace_back(new server::EThread(fun1, "name_" + std::to_string(i)));
    }
    for (auto &item: thp){
        item->join();
    }
    counter.end_count();

    SERVER_LOG_INFO(logger) << "test end " << count;

    cout << counter.get_duration() << endl;
}

void test2(){
    server::Fiber::GetThis();
    server::Scheduler sc(2);
    server::Fiber::ptr fib(new server::Fiber(fib_func));
    // auto fib = fib_func;
    sc.start();
    sc.schedule(fib);
    sc.stop();

}

server::CoRet run_in_fiber(){
    SERVER_LOG_INFO(logger) << "begin run in fiber";
    co_yield server::HOLD;
    SERVER_LOG_INFO(logger) << "end run in fiber";
    co_return server::READY;
}

void test3(){
    server::Fiber::GetThis();
    SERVER_LOG_INFO(logger) << "main begin";
    
    server::Fiber::ptr fiber(new server::Fiber(run_in_fiber));
    fiber->swapIn();
    SERVER_LOG_INFO(logger) << "main after swapIn";
    
    fiber->swapIn();
    SERVER_LOG_INFO(logger) << "main end";
}

