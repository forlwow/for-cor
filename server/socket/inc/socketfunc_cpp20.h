#ifndef SERVER_SOCKETFUNC_20_H
#define SERVER_SOCKETFUNC_20_H

#include "address.h"
#include "log.h"
#include "socket.h"
#include <algorithm>
#include <bits/fs_fwd.h>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#if __cplusplus >= 202002L
#include <coroutine>
#include <fiber_cpp20.h>
#include <iomanager_cpp20.h>

namespace server {


enum SOCK_RESULT{
    SOCK_REMAIN_DATA = -10,
    SOCK_CLOSE,
    SOCK_OTHER,
    SOCK_EAGAIN,
    SOCK_EWOULDBLOCK,
    SOCK_SUCCESS = 0,
};

struct connect{
    connect(const Socket::ptr& sock, const Address::ptr& add)
    {
        m_sock = sock;
        m_address = add;
    }
    bool await_ready() {
        return false;
    }
    void await_suspend(std::coroutine_handle<CoRet::promise_type> handle){
        auto fib = std::dynamic_pointer_cast<Fiber_2>(Fiber_::GetThis());
        auto iom = IOManager_::GetIOManager();
        if(!iom || !fib){
            throw std::logic_error("fib|iomanager not found");
            return;
        }
        int res = m_sock->connect(m_address);
        if(res == EINPROGRESS){
            int res = iom->AddEvent(m_sock->getFd(), IOManager_::WRITE, fib);
            printf("res: %d\n", res);
        }
        return;
    }
    int await_resume() {
        int res = m_sock->getError();
        if(!res)
            m_sock->setConnected(true);
        return res;
    }
    Socket::ptr m_sock;
    Address::ptr m_address;
};

// return SOCK_RESULT
// 0: success
// >0: send num
// <0: error
struct send{
    send(const Socket::ptr& sock, std::string_view str_, int len, int flag = 0)
    {
        m_sock = sock;
        m_str = str_;
        m_len = len;
        m_flag = flag;
    }

    ~send(){
    }

    bool await_ready() {
        return false;
    }
    void await_suspend(std::coroutine_handle<CoRet::promise_type> handle){
        auto fib = std::dynamic_pointer_cast<Fiber_2>(Fiber_::GetThis());
        auto iom = IOManager_::GetIOManager();
        if(!iom || !fib){
            throw std::logic_error("fib|iomanager not found");
            return;
        }
        res = m_sock->send(m_str.data(), m_len, m_flag);
        if(res == - 1 && (errno == EAGAIN || errno == EWOULDBLOCK)){
            res = SOCK_EAGAIN;
            iom->AddEvent(m_sock->getFd(), IOManager_::WRITE, fib);
        }
        else if (res == 0){
            res = SOCK_CLOSE;
            handle.resume();
        }
        else if(res > 0){
            m_str = m_str.substr(res);
            m_len = std::min(m_len, (int)m_str.length());
            res = SOCK_REMAIN_DATA;
            if (m_str.empty()) {
                res = SOCK_SUCCESS;
            }
            handle.resume();
        }
        else {
            res = SOCK_OTHER;
        }


    }
    int await_resume() {
        return res;
    }
    int m_len;
    int m_flag;
    int res = 0;
    std::string_view m_str;
    Socket::ptr m_sock;
};

// return SOCK_RESULT
// 0: success
// >0: recv num
// <0: error
struct recv{
    recv(const Socket::ptr& sock, void *buf, size_t len, int flag = 0)
    {
        m_sock = sock;
        m_buf = buf;
        m_len = len;
        m_flag = flag;
    }

    ~recv(){
    }

    bool await_ready() {
        return false;
    }
    void await_suspend(std::coroutine_handle<CoRet::promise_type> handle){
        auto fib = std::dynamic_pointer_cast<Fiber_2>(Fiber_::GetThis());
        auto iom = IOManager_::GetIOManager();
        if(!iom || !fib){
            throw std::logic_error("fib|iomanager not found");
            return;
        }
        res = m_sock->recv(m_buf, m_len, m_flag);
        if(res == - 1 && (errno == EAGAIN || errno == EWOULDBLOCK)){
            res = SOCK_EAGAIN;
            iom->AddEvent(m_sock->getFd(), IOManager_::READ, fib);
        }
        else if (res == 0){
            res = SOCK_CLOSE;
            handle.resume();
        }
        else if(res > 0){
            handle.resume();
        }
        else {
            res = SOCK_OTHER;
        }


    }
    int await_resume() {
        return res;
    }
    size_t m_len;
    int res = 0;
    int m_flag;
    void* m_buf;
    Socket::ptr m_sock;
};
} // namespace server

#endif

#endif // SERVER_SOCKETFUNC_20_H
