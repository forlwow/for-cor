#include "address.h"
#include "async.h"
#include "http.h"
#include "http_parser.h"
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
#include <signal.h>
#include <timer.h>
#include <vector>
#include "fiberfunc_cpp20.h"
using namespace std;

#define in :

auto logger = SERVER_LOGGER_SYSTEM;
auto slog = SERVER_LOGGER("system");
server::CoRet fiber_timer_cir();

char response_data[] = 
    "HTTP/1.1 200 OK"
    "Date: Tue, 27 Feb 2024 15:15:02 GMT"
    "Server: Apache"
    "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT"
    "ETag: \"51-47cf7e6ee8400\""
    "Accept-Ranges: bytes"
    "Content-Length: 81"
    "Cache-Control: max-age=86400"
    "Expires: Wed, 28 Feb 2024 15:15:02 GMT"
    "Connection: Keep-Alive"
    "Content-Type: text/html"
    ;
char request_data[] = 
    "GET / HTTP/1.1\r\n"
    "User-Agent: Wget/1.20.3 (linux-gnu)\r\n"
    "Accept: */*\r\n"
    "Accept-Encoding: identity\r\n"
    "Host: www.baidu.com\r\n"
    "Connection: Keep-Alive\r\n"
    "Content-Length: 10\r\n"
    "\r\n"
    "1234567890" 
    ;

void test1();
void test2();
void test3();
void test4();
void test5();

void http_test(){
    server::http::HttpRequestParser parser;
    string tmp(request_data);
    auto s = parser.execute(&(request_data[0]), tmp.size());
    SERVER_LOG_DEBUG(logger) << "execute rt=" << s << 
    " error=" << parser.GetErrstr() << " is finished=" << parser.isFinished();
    SERVER_LOG_DEBUG(logger) << "data: \n" << parser.GetData()->toString();
    SERVER_LOG_DEBUG(logger) << "body: \n" << parser.GetData()->GetBody();
}

server::Task run_in_fiber(){
    SERVER_LOG_INFO(logger) << "enter fiber";
    co_await server::msleep(2000);
    SERVER_LOG_INFO(logger) << "begin run in fiber";
    co_yield server::YIELD;
    SERVER_LOG_INFO(logger) << "before end run in fiber";
    co_yield server::SUSPEND;
    SERVER_LOG_INFO(logger) << "end run in fiber";
    co_return ;
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
        --i;
        co_yield server::HOLD;
    }
    ++g_count;
    co_return server::TERM;
}


server::Task test_Fiber2(int i_){
    for(auto i in range(i_)){
        SERVER_LOG_DEBUG(logger) << "test2 swapIn=" << i;
        co_yield server::SUSPEND;
    }
    ++g_count;
    co_return ;
}

server::Task test_sock(){
    auto sock = server::Socket::ptr(new server::Socket(AF_INET, SOCK_STREAM));
    auto address = server::IPv4Address::CreateAddressPtr("192.168.1.10", 9999);
    SERVER_LOG_DEBUG(logger) << "start connect";
    int res = co_await server::connect(sock, address);
    SERVER_LOG_DEBUG(logger) << res;
    if(res)
        co_return ;
    const char* buff = "write example";
    SERVER_LOG_DEBUG(logger) << "start write";
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

    co_return;
}

server::Task test_sock2(){
    auto sock = server::Socket::ptr(new server::Socket(AF_INET, SOCK_STREAM));
    auto address = server::IPv4Address::CreateAddressPtr("192.168.1.110", 39001);
    sock->bind(address);
    sock->listen();
    auto res_sock = co_await server::accept(sock);
    if (!res_sock){
        co_return;
    }
    SERVER_LOG_DEBUG(logger) << "accept sock:" << res_sock->getFd();
    const char* buff = "write example";
    auto writer = server::send(res_sock, buff, 11);
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
    co_return;
}

int main(){
    http_test();
    auto file_log = SERVER_LOGGER("file");
    SERVER_LOG_DEBUG(file_log) << "aaa";
}

void test1(){
    server::IOManager_ iom(2);
    auto Fib = server::AsyncFiber::CreatePtr(test_sock2);
    iom.start();
    iom.schedule(Fib);
    iom.wait();
} 

void test2(){
    auto func = server::FuncFiber::ptr(new server::FuncFiber([]{
        printf("ok\n");
    }));
    func->swapIn();
}

void test3(){
    cout << server::http::HttpStatus2String(server::http::HTTP_STATUS_INVALID) << endl;
}


void test4(){
}

server::CoRet fiber_timer_cir(){

}
void p(){SERVER_LOG_INFO(logger) << "test";}

void test5(){

}


#endif
