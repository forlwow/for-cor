#ifndef SERVER_SOCKET_H
#define SERVER_SOCKET_H

#include <bits/types/struct_iovec.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <address.h>
#include <sys/socket.h>

namespace server{

class Socket: public std::enable_shared_from_this<Socket>{
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    Socket(int family, int type, int protocol = 0);
    ~Socket();
    Socket(const Socket&)=delete;
    Socket& operator=(const Socket&)=delete;
    Socket(const Socket&&)=default;
    Socket& operator=(const Socket&&)=default;

    bool init(int sock);

    int64_t getSendTimeOut();
    bool setSendTimeOut(int64_t v);

    int64_t getRecvTimeOut();
    bool setRecvTimeOut(int64_t v);

    bool getOption(int level, int option, void *result, size_t *len);
    template<typename T>
    bool getOption(int level, int option, T *result){
        return getOption(level, option, result, sizeof(T));
    }
    bool setOption(int level, int option, const void *result, size_t len);
    template<typename T>
    bool setOption(int level, int option, const T *result){
        return setOption(level, option, result, sizeof(T));
    }

    Socket::ptr accept();
    bool bind(const Address::ptr);
    bool connect(const Address::ptr);

    bool listen(int backlog = SOMAXCONN);

    void close();


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

    int getFamily() const {return m_family;};
    int getType() const {return m_type;};
    int getProtocol() const {return m_protocol;};

    bool isConnected() const {return m_isConnected;};
    bool isVaild() const;
    int getError();

    bool cancelRead();
    bool cancelWrite();
    bool cancelAccept();
    bool cancelAll();

private:
    void initSock();
    void newSock();
private:
    int m_sock;
    int m_family;
    int m_type;
    int m_protocol;
    bool m_isConnected;

    Address::ptr m_localAddress;
    Address::ptr m_remoteAddress;
};


} // namespace server

#endif // SERVER_SOCKET_H
