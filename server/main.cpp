#include "ethread.h"
#include "fiber.h"
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

void test1();
void test2();
void test3();

auto logger = SERVER_LOGGER_SYSTEM;
int main(){
    SERVER_LOG_INFO(logger) << "main begin-1";
    test3();
    SERVER_LOG_INFO(logger) << "main end-1";
    return 0;
}



void test1(){
    auto log = CreateFileLogger("log.txt");
    SERVER_LOG_INFO(log) << "test begin";
    int count = 0;
    auto fun1 = std::function<void()>(
        [&](){
            int i = 2e4;
            while(i--){
                SERVER_LOG_INFO(log) 
                << "name:" 
                << server::EThread::GetName() << " "
                << "this.name: " 
                << server::EThread::GetThis()->getName() << " "
                << "id: " 
                << syscall(SYS_gettid) << " "
                << "this.id: " 
                << server::EThread::GetThis()->getId();
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

    SERVER_LOG_INFO(log) << "test end " << count;

    cout << counter.get_duration() << endl;
}

void test2(){
    auto log = SERVER_LOGGER_SYSTEM;
    
    Timer counter;
    counter.start_count();
    for(int i = 0; i < 1; ++i){
        SERVER_LOG_INFO(log) << "test";
    }

    counter.end_count();
    cout << counter.get_duration() << endl;

}

void run_in_fiber(){
    SERVER_LOG_INFO(logger) << "begin run in fiber";
    server::Fiber::GetThis()->YieldToHold();
    SERVER_LOG_INFO(logger) << "end run in fiber";
    server::Fiber::GetThis()->YieldToReady();
}

void test3(){
    server::Fiber::GetThis();
    SERVER_LOG_INFO(logger) << "main begin";
    
    server::Fiber::ptr fiber(new server::Fiber(run_in_fiber));
    fiber->swapIn();
    SERVER_LOG_INFO(logger) << "main after swapIn";
    
    fiber->swapIn();
    SERVER_LOG_INFO(logger) << "main end";
    fiber->swapIn();
}

