#include "address.h"
#include "log.h"
#include <algorithm>
#include <arpa/inet.h>
#include <bits/fs_fwd.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <netinet/in.h>
#include <ostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netdb.h>
#include <system_error>
#include <unistd.h>


namespace server {

static Logger::ptr s_log = SERVER_LOGGER_SYSTEM;

int Address::getFamily() const {
    return getAddr()->sa_family;
}

bool Address::host2Address(std::vector<ptr>& v, const std::string& host, const std::string& service, int family, int type, int protocol){
    struct addrinfo hints, *result, *p;
    int status;

    // 设置查询条件
    std::memset(&hints, 0, sizeof hints);
    hints.ai_family = family;     // 支持 IPv4 或 IPv6
    hints.ai_socktype = type; // 流套接字
    hints.ai_protocol = protocol;

    // 获取地址信息
    if ((status = getaddrinfo(host.data(), service.data(), &hints, &result)) != 0) {
        SERVER_LOG_ERROR(s_log) << "getaddrinfo: " << gai_strerror(status);
        freeaddrinfo(result);
        return false;
    }

    // 遍历结果链表
    switch (family) {
        case AF_INET:
            for (p = result; p != NULL; p = p->ai_next) {
                v.emplace_back(new IPv4Address(*(sockaddr_in*)(p->ai_addr)));
            }
            break;
        case AF_INET6:
            for (p = result; p != NULL; p = p->ai_next) {
                v.emplace_back(new IPv6Address(*(sockaddr_in6*)(p->ai_addr)));
            }
            break;
    }
    freeaddrinfo(result);
    return true;
}

std::ostream& Address::insert(std::ostream& os) const {

}

std::string Address::toString() const {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

bool Address::operator<(const Address& r) const{
    socklen_t minlen = std::min(getAddrLen(), r.getAddrLen());
    int result = memcmp(getAddr(), r.getAddr(), minlen);
    if(result < 0)
        return true;
    else if (result > 0)
        return false;
    else if (getAddrLen() < r.getAddrLen())
        return true;
    return false;
}

bool Address::operator==(const Address& r) const{
    return getAddrLen() == r.getAddrLen() && 
        memcmp(getAddr(), r.getAddr(), getAddrLen()) == 0;
}

IPv4Address::ptr IPv4Address::CreateAddress(const char *address, uint16_t port){
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_port = port;
    addr.sin_family = AF_INET;
    int result = inet_pton(AF_INET, address, &addr.sin_addr);
    if(result <= 0){
        SERVER_LOG_ERROR(s_log) << "IPv4Address Create Error:IP-" << address 
            << "-Port-" << port;
        return {};
    }
    return IPv4Address::ptr(new IPv4Address(addr));
}

IPv4Address::IPv4Address(const sockaddr_in& sock)
    : m_addr(sock)
{
}

IPv4Address::IPv4Address(uint32_t address, uint32_t port){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    m_addr.sin_addr.s_addr = htonl(address);
}

const sockaddr* IPv4Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t IPv4Address::getAddrLen() const {
    return  sizeof(m_addr);
}

std::ostream& IPv4Address::insert(std::ostream& os) const {
    os << inet_ntoa(m_addr.sin_addr)
        << ":" << ntohs(m_addr.sin_port);
    return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
    if(prefix_len > 32)
        return {};
    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr |= htonl(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) {
    if(prefix_len > 32)
        return {};
    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr &= htonl(~CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr = ~htonl(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}

uint32_t IPv4Address::getPort() const {
    return ntohs(m_addr.sin_port);
}

void IPv4Address::setPort(uint32_t port) {
    m_addr.sin_port = htons(port);
}

IPv6Address::ptr IPv6Address::CreateAddress(const char *address, uint16_t port){
    sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_port = port;
    addr.sin6_family = AF_INET;
    int result = inet_pton(AF_INET6, address, &addr.sin6_addr);
    if(result <= 0){
        SERVER_LOG_ERROR(s_log) << "IPv6Address Create Error:IP-" << address 
            << "-Port-" << port;
        return {};
    }
    return IPv6Address::ptr(new IPv6Address(addr));
}

IPv6Address::IPv6Address(const sockaddr_in6& addr)
    : m_addr(addr)
{}

IPv6Address::IPv6Address(const char* address, uint32_t port){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = htons(port);
    inet_pton(AF_INET6, address, &(m_addr.sin6_addr));
}

const sockaddr* IPv6Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t IPv6Address::getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream& IPv6Address::insert(std::ostream& os) const {
    char ip[INET6_ADDRSTRLEN];
    const char* res = inet_ntop(AF_INET6, &(m_addr.sin6_addr), ip, INET6_ADDRSTRLEN);
    if(res)
        os << "[" << ip << "]:" << ntohs(m_addr.sin6_port);
    return os;
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i){
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len / 8] = ~CreateMask<uint8_t>(prefix_len % 8);

}

uint32_t IPv6Address::getPort() const {
    return ntohs(m_addr.sin6_port);
}

void IPv6Address::setPort(uint32_t port) {
    m_addr.sin6_port = htons(port);
}

static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

UnixAddress::UnixAddress(){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress(const std::string& path){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = path.size() + 1;
    if(!path.empty() && path[0] == '\0')
        --m_length;
    if(m_length > sizeof(m_addr.sun_path)){
        throw std::logic_error("path too long");
    }
    memcpy(&m_addr.sun_path, path.c_str(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);
}

const sockaddr* UnixAddress::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t UnixAddress::getAddrLen() const {
    return m_length;
}

std::ostream& UnixAddress::insert(std::ostream& os) const {
    if(m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0')
        return os << '\\0' 
            << std::string(m_addr.sun_path + 1, m_length - offsetof(sockaddr_un, sun_path) - 1);
    return os << m_addr.sun_path;
}

UnknowAddress::UnknowAddress(int family){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

const sockaddr* UnknowAddress::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t UnknowAddress::getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream& UnknowAddress::insert(std::ostream& os) const {
    return os << "[UnknowAddress] family=" << m_addr.sa_family;
}

} // namespace server
