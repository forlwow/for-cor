#ifndef SERVER_TEST_FIBER_H
#define SERVER_TEST_FIBER_H
#include "fiber_cpp20.h"
#include "log.h"
#include <atomic>
#define in :

namespace server
{
namespace test
{
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

std::atomic<int> g_count;
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
}
}

#endif