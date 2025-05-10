#ifndef SERVER_SOCKETFUNC_20_H
#define SERVER_SOCKETFUNC_20_H
#include <cstdint>
#if __cplusplus >= 202002L

#include "address.h"
#include "log.h"
#include "socket.h"
#include "concept.h"
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
#include <coroutine>
#include <fiber_cpp20.h>
#include <iomanager_cpp20.h>

namespace server {

static auto s_log = SERVER_LOGGER_SYSTEM;

enum SOCK_RESULT{
    SOCK_REMAIN_DATA = -10,
    SOCK_CLOSE,
    SOCK_OTHER,
    SOCK_EAGAIN,
    SOCK_EWOULDBLOCK,
    SOCK_SUCCESS = 0,
};

inline std::string Sock_Result2String(int res) {
    switch (res) {
        case SOCK_REMAIN_DATA: return "remain data";
        case SOCK_CLOSE: return "close socket";
        case SOCK_OTHER: return "other socket";
        case SOCK_EAGAIN: return "eagain socket";
        case SOCK_EWOULDBLOCK: return "ewould block socket";
        case SOCK_SUCCESS: return "success";
        default: {
            return "recv: " + std::to_string(res);
        }
    }
}

struct connect{
    connect(const Socket::ptr& sock, const Address::ptr& add)
    {
        m_sock = sock;
        m_address = add;
    }
    ~connect(){
        if(m_isAddEvent){
            IOManager_::GetIOManager()->DelEvent(m_sock->getFd(), IOManager_::WRITE);
        }
    }
    bool await_ready() {
        return false;
    }
    template<FiberPromise T>
    void await_suspend(T handle){
        // auto fib = std::dynamic_pointer_cast<FiberIO>(Fiber_::GetThis().lock());
        auto fib = Fiber_::GetThis().lock();
        auto iom = IOManager_::GetIOManager();
        if(!iom || !fib){
            throw std::logic_error("fib|iomanager not found");
            return;
        }
        int res = m_sock->connect(m_address);
        if(res == EINPROGRESS){
            int res = iom->AddEvent(m_sock->getFd(), IOManager_::WRITE, fib);
            m_isAddEvent = true;
            if(res)
                handle.resume();
        }
        return;
    }
    int await_resume() {
        int res = m_sock->getError();
        if(!res)
            m_sock->setConnected(true);
        return res;
    }
    bool m_isAddEvent = false;
    Socket::ptr m_sock;
    Address::ptr m_address;
};

// return SOCK_RESULT
// 0: success
// >0: send num
// <0: error
struct send{
    send(const Socket::ptr& sock, std::string_view str_ = {}, int len = 0, int flag = 0)
    {
        m_sock = sock;
        m_str = str_;
        m_len = len;
        m_flag = flag;
    }

    ~send(){
        if(m_isAddEvent){
            IOManager_::GetIOManager()->DelEvent(m_sock->getFd(), IOManager_::WRITE);
        }
        // SERVER_LOG_DEBUG(s_log) << "sender destory";
    }

    bool await_ready() {
        return false;
    }
    template<FiberPromise T>
    void await_suspend(T handle){
        auto fib = Fiber_::GetThis().lock();
        // auto fib = std::dynamic_pointer_cast<FiberIO>(Fiber_::GetThis().lock());
        auto iom = IOManager_::GetIOManager();
        if(!iom || !fib){
            SERVER_LOG_ERROR(s_log) << ("fib|iomanager not found");
            return;
        }
        res = m_sock->send(m_str.data(), m_len, m_flag);
        if(res == - 1 && (errno == EAGAIN || errno == EWOULDBLOCK)){
            res = SOCK_EAGAIN;
            int tr = iom->AddEvent(m_sock->getFd(), IOManager_::WRITE, fib);
            m_isAddEvent = true;
            if(tr)
                handle.resume();
        }
        else if (res == 0){
            res = SOCK_CLOSE;
            handle.resume();
        }
        else if(res > 0){
            // m_str = m_str.substr(res);
            // m_len = std::min(m_len, (int)m_str.length());
            // res = SOCK_REMAIN_DATA;
            // if (m_str.empty()) {
            //     res = SOCK_SUCCESS;
            // }
            if (res == m_str.size()) {
                res = SOCK_SUCCESS;
            }
            else {
                m_str = m_str.substr(m_len);
                m_len = m_str.size() - res;
                res = SOCK_REMAIN_DATA;
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

    void reset(std::string_view buf, size_t len) {
        m_isAddEvent = false;
        m_len = len;
        m_str = buf;
    }

    bool m_isAddEvent = false;
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
        if(m_isAddEvent){
            IOManager_::GetIOManager()->DelEvent(m_sock->getFd(), IOManager_::READ);
        }
        // SERVER_LOG_DEBUG(s_log) << "recv destroy";
    }

    bool await_ready() {
        return false;
    }
    template<FiberPromise T>
    void await_suspend(T handle){
        auto fib = Fiber_::GetThis().lock();
        auto iom = IOManager_::GetIOManager();
        if(!iom || !fib){
            throw std::logic_error("fib|iomanager not found");
            return;
        }
        res = m_sock->recv(m_buf, m_len, m_flag);
        if(res == - 1 && (errno == EAGAIN || errno == EWOULDBLOCK)){
            res = SOCK_EAGAIN;
            int res = iom->AddEvent(m_sock->getFd(), IOManager_::READ, fib);
            if(res)
                handle.resume();
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

    void reset(void *buf, size_t len) {
        m_isAddEvent = false;
        m_len = len;
        m_buf = buf;
    }

    bool m_isAddEvent = false;
    size_t m_len;
    int res = 0;
    int m_flag;
    void* m_buf;
    Socket::ptr m_sock;
};


struct accept{
    accept(const Socket::ptr& sock)
    {
        m_sock = sock;
    }

    ~accept(){
        if(m_isAddEvent){
            IOManager_::GetIOManager()->DelEvent(m_sock->getFd(), IOManager_::READ);
        }
        // SERVER_LOG_DEBUG(s_log) << "accepter destory";
    }

    bool await_ready() {
        m_res = m_sock->accept();
        if (m_res) return true;
        return false;
    }
    template<FiberPromise T>
    void await_suspend(T handle){
        auto fib = Fiber_::GetThis().lock();
        auto iom = IOManager_::GetIOManager();
        if(!iom || !fib){
            throw std::logic_error("fib|iomanager not found");
            return;
        }
        iom->AddEvent(m_sock->getFd(), IOManager_::READ, fib);
    }
    Socket::ptr await_resume() {
        return m_res;
    }

    bool m_isAddEvent = false;
    Socket::ptr m_sock;
    Socket::ptr m_res;
};

} // namespace server

#endif

#endif // SERVER_SOCKETFUNC_20_H
