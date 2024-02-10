#include "address.h"
#include "socket.h"
#include "socketfunc_cpp20.h"
#include <cerrno>
#include <cstddef>
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


server::CoRet test_Fiber2(int i_){
    for(auto i in range(i_)){
        SERVER_LOG_DEBUG(logger) << "test2 swapIn=" << i;
        co_yield server::HOLD;
    }
    ++g_count;
    co_return server::TERM;
}

server::CoRet test_sock(){
    auto sock = server::Socket::ptr(new server::Socket(AF_INET, SOCK_STREAM));
    auto address = server::IPv4Address::CreateAddress("192.168.2.18", 9999);
    SERVER_LOG_DEBUG(logger) << "start connect";
    int res = co_await server::connect(sock, address);
    SERVER_LOG_DEBUG(logger) << res;
    if(res)
        co_return server::TERM;
    char* buff = "write example";
    SERVER_LOG_DEBUG(logger) << "start write";
    int left = strlen(buff);
    auto writer = server::send(sock, buff, 11);
    while(1){
        int res = co_await writer;
        if(res == server::SOCK_SUCCESS){
            SERVER_LOG_DEBUG(logger) << "write success";
            break;
        }
        else if(res == server::SOCK_REMAIN_DATA || res == server::SOCK_EAGAIN){
            continue;
        }
        else {
            SERVER_LOG_DEBUG(logger) << "write failed";
            break;
        }
    }
    char buf[10];
    std::string recvData;
    auto recver = server::recv(sock, buf, 10);
    while (1) {
        int res = co_await recver;
        if(res > 0){
            recvData.append(buf, res);
            SERVER_LOG_DEBUG(logger) << "recved:" << std::string(buf, res);
            if (recvData.back() == '\0') {
                SERVER_LOG_DEBUG(logger) << "recv success";
                break;
            }
        }
        else {
            if(res == server::SOCK_EAGAIN){
                continue;
            }
            else{
                SERVER_LOG_DEBUG(logger) << "recv fail";
                break;
            }
        }

    }

    co_return server::TERM;
}

int main(){
    test1();
}

void test1(){
    server::IOManager_ iom(3);
    iom.start();
    auto task = server::Fiber_2::ptr(new server::Fiber_2(test_sock));
    iom.schedule(task);

    iom.wait();
}

void test2(){
    server::Fiber_::ptr task(new server::Fiber_(test_Fiber2, 10));
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
    SERVER_LOG_INFO(logger) << timer.get_duration().count();
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
