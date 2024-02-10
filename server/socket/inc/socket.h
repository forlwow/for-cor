#ifndef SERVER_SOCKET_H
#define SERVER_SOCKET_H

#include <bits/types/struct_iovec.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <address.h>
#include <ostream>
#include <sys/socket.h>
#include <unistd.h>

namespace server{

class Socket: public std::enable_shared_from_this<Socket>{
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    Socket(int family, int type, int protocol = 0, void(*)(int) = nullptr);
    ~Socket();
    Socket(const Socket&)=delete;
    Socket& operator=(const Socket&)=delete;
    Socket(Socket&&)=default;
    Socket& operator=(Socket&&)=default;

    bool init(int sock);

    int64_t getSendTimeOut();
    bool setSendTimeOut(int64_t v);

    int64_t getRecvTimeOut();
    bool setRecvTimeOut(int64_t v);

    bool getOption(int level, int option, void *result, socklen_t *len);
    template<typename T>
    bool getOption(int level, int option, T &result){
        socklen_t len = sizeof(T);
        return getOption(level, option, &result, &len);
    }
    bool setOption(int level, int option, const void *result, socklen_t len);
    template<typename T>
    bool setOption(int level, int option, const T *result){
        return setOption(level, option, result, sizeof(T));
    }

    Socket::ptr accept();
    bool bind(const Address::ptr);
    int connect(const Address::ptr);

    bool listen(int backlog = SOMAXCONN);

    void close();

    std::ostream& dump(std::ostream&);

    int send(const void* buffer, size_t length, int flags = 0);
    int send(iovec* buffer, size_t length, int flags = 0);
    int sendTo(const void* buffer, size_t length, const Address::ptr, int flags = 0);
    int sendTo(iovec* buffer, size_t length, const Address::ptr ,int flags = 0);

    int recv(void* buffer, size_t length, int flags = 0);
    int recv(iovec* buffer, size_t length, int flags = 0);
    int recvFrom(void* buffer, size_t length, const Address::ptr, int flags = 0);
    int recvFrom(iovec* buffer, size_t length, const Address::ptr ,int flags = 0);

    Address::ptr getRemoteAddress();
    Address::ptr refreshRemoteAddress();
    Address::ptr getLocalAddress();

    int getFd() const {return m_sock;}
    int getFamily() const {return m_family;}
    int getType() const {return m_type;}
    int getProtocol() const {return m_protocol;}

    bool isConnected() const {return m_isConnected;}
    void setConnected(bool connect) {m_isConnected = connect;}
    bool isVaild() const;
    int getError();
    auto getHandler() const {return m_err_handler;}

private:
    void initSock();
    void newSock();
private:
    int m_sock;
    int m_family;
    int m_type;
    int m_protocol;
    bool m_isConnected;

    void(*m_err_handler)(int);

    Address::ptr m_localAddress;
    Address::ptr m_remoteAddress;
};


} // namespace server

#endif // SERVER_SOCKET_H
