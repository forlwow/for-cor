#include "socket.h"
#include "address.h"
#include "log.h"
#include <asm-generic/socket.h>
#include <bits/types/error_t.h>
#include <bits/types/struct_timeval.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <unistd.h>

namespace server {

static Logger::ptr s_log = SERVER_LOGGER_SYSTEM;

Socket::Socket(int family, int type, int protocol)
    :m_sock(-1), m_family(family), m_type(type), m_protocol(protocol),
    m_isConnected(false)
{

}

Socket::~Socket(){
    if(m_isConnected)
        close();
}

int64_t Socket::getSendTimeOut(){
    timeval tv;
    if(getOption(SOL_SOCKET, SO_SNDTIMEO, &tv) == -1)
        return -1;
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

bool Socket::setSendTimeOut(int64_t v){
    struct timeval tv;
    tv.tv_sec = v / 1000;
    tv.tv_usec = v % 1000 * 1000;
    return setOption(SOL_SOCKET, SO_SNDTIMEO, &tv);
}

int64_t Socket::getRecvTimeOut(){
    timeval tv;
    if(getOption(SOL_SOCKET, SO_RCVTIMEO, &tv) == -1)
        return -1;
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

bool Socket::setRecvTimeOut(int64_t v){
    struct timeval tv;
    tv.tv_sec = v / 1000;
    tv.tv_usec = v % 1000 * 1000;
    return setOption(SOL_SOCKET, SO_RCVTIMEO, &tv);
}

bool Socket::getOption(int level, int option, void *result, size_t *len){
    int res = getsockopt(m_sock, level, option, result, (socklen_t*)len);

    if(res){
        SERVER_LOG_DEBUG(s_log) << "getSockOpt error sock=" << m_sock
            << " level=" << level << " option=" << option
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::setOption(int level, int option, const void *result, size_t len){
    int res = setsockopt(m_sock, level, option, result, (socklen_t)len);

    if(res){
        SERVER_LOG_DEBUG(s_log) << "setSockOpt error sock=" << m_sock
            << " level=" << level << " option=" << option
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}


Socket::ptr Socket::accept(){
    int newsock = ::accept(m_sock, nullptr, nullptr);
    if(newsock != -1){
        SERVER_LOG_DEBUG(s_log) << "accept error sock=" << m_sock
            << " errno=" << errno << " errstr=" << strerror(errno);
        return {};
    }
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    if(sock->init(newsock))
        return sock;
    return {};

}

bool Socket::init(int newsock){
    struct stat fileStat;
    if(fstat(newsock,  &fileStat) == -1){
        return false;
    }
    if(S_ISSOCK(fileStat.st_mode)){
        return false;
    }
    m_sock = newsock;
    m_isConnected = true;
    initSock();
    return true;

}

bool Socket::bind(const Address::ptr addr){
    if(!isVaild()){
        newSock();
        if(!isVaild())
            return false;
    }
    if(addr->getFamily() != m_family){
        SERVER_LOG_ERROR(s_log) 
            << "bind sock error: family different addr.family=" << addr->getFamily()
            << "sock.family=" << m_family;
        return false;
    }
    if(::bind(m_sock, addr->getAddr(), addr->getAddrLen())){
        SERVER_LOG_ERROR(s_log) 
            << "bind sock error: bind error errno=" << errno
            << "errstr=" << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::connect(const Address::ptr addr){
    if(!isVaild()){
        newSock();
        if(!isVaild())
            return false;
    }
    if(addr->getFamily() != m_family){
        SERVER_LOG_ERROR(s_log) 
            << "connect sock error: family different addr.family=" << addr->getFamily()
            << "sock.family=" << m_family;
        return false;
    }
    if(::connect(m_sock, addr->getAddr(), addr->getAddrLen())){
        SERVER_LOG_ERROR(s_log) 
            << "connect sock error: bind error errno=" << errno
            << "errstr=" << strerror(errno);
        return false;
    }
    m_isConnected = true;
    return true;
}

bool Socket::listen(int backlog){
    if(!isVaild()){
        return false;
    }
    if(::listen(m_sock, backlog)){
        return false;
    }
    return true;
}

void Socket::close(){
    if(!m_isConnected && m_sock == -1)
        return ;
    if(m_sock != -1){
        ::close(m_sock);
        m_sock = -1;
    }
    m_isConnected = false;
    return ;
}

int Socket::send(const void* buffer, size_t length, int flags){
    if(!isConnected()){
        return -1;
    }
    return ::send(m_sock, buffer, length, flags);
}

int Socket::send(iovec* buffer, size_t length, int flags){
    if(!isConnected())
        return -1;
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = buffer;
    msg.msg_iovlen = length;
    return ::sendmsg(m_sock, &msg, flags);
}

int Socket::sendTo(const void* buffer, size_t length, const Address::ptr toAddr, int flags){
    return ::sendto(m_sock, buffer, length, flags, toAddr->getAddr(), toAddr->getAddrLen());
}

int Socket::sendTo(iovec* buffer, size_t length, const Address::ptr to,int flags){
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = buffer;
    msg.msg_iovlen = length;
    msg.msg_name = (void*)to->getAddr();
    msg.msg_namelen = to->getAddrLen();
    return ::sendmsg(m_sock, &msg, flags);
}

int Socket::recv(void* buffer, size_t length, int flags){
    if(!isConnected())
        return -1;
    return ::recv(m_sock, buffer, length, flags);
}

int Socket::recv(iovec* buffer, size_t length, int flags){
    if(!isConnected())
        return -1;
    msghdr msg;
    msg.msg_iov = buffer;
    msg.msg_iovlen = length;
    return ::recvmsg(m_sock, &msg, flags);
}

int Socket::recvFrom(void* buffer, size_t length, const Address::ptr from, int flags){
    socklen_t len = from->getAddrLen();
    return ::recvfrom(m_sock, buffer, length, flags, (sockaddr*)from->getAddr(), &len);
}

int Socket::recvFrom(iovec* buffer, size_t length, const Address::ptr from,int flag){
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = buffer;
    msg.msg_iovlen = length;
    msg.msg_name = (void*)from->getAddr();
    msg.msg_namelen = from->getAddrLen();
    return ::recvmsg(m_sock, &msg, flag);
}

Address::ptr Socket::getRemoteAddress(){
    if(m_remoteAddress)
        return m_remoteAddress;
    return {};
}

Address::ptr Socket::refreshRemoteAddress(){
    Address::ptr result;
    switch (m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknowAddress());
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if(getpeername(m_sock, result->getAddr(), &addrlen)){
        result.reset(new UnknowAddress);
        return result;
    }
    if(m_family == AF_UNIX){
        UnixAddress::ptr unaddr = std::dynamic_pointer_cast<UnixAddress>(result);
        unaddr->setAddrLen(addrlen);
    }
    m_remoteAddress = result;
    return result;
}

Address::ptr Socket::getLocalAddress(){
    Address::ptr result;
    switch (m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknowAddress());
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if(getsockname(m_sock, result->getAddr(), &addrlen)){
        result.reset(new UnknowAddress);
        return result;
    }
    if(m_family == AF_UNIX){
        UnixAddress::ptr unaddr = std::dynamic_pointer_cast<UnixAddress>(result);
        unaddr->setAddrLen(addrlen);
    }
    m_localAddress = result;
    return result;
}

bool Socket::isVaild() const{
   return m_sock != -1; 
}

int Socket::getError(){

}

bool Socket::cancelRead(){

}

bool Socket::cancelWrite(){

}

bool Socket::cancelAccept(){

}

bool Socket::cancelAll(){

}

void Socket::initSock(){
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, &val);
    if(m_type == SOCK_STREAM){
        setOption(IPPROTO_TCP, TCP_NODELAY, &val);
    }
}

void Socket::newSock(){
    m_sock = socket(m_family, m_type, m_protocol);
    if(m_sock != -1){
        initSock();    
    }
    else{
        SERVER_LOG_DEBUG(s_log) << "getSockOpt error sock=" << m_sock
            << " errno=" << errno << " errstr=" << strerror(errno);
    }
}


} // namespace server
