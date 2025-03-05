#ifndef SERVER_ADDRESS_H
#define SERVER_ADDRESS_H

#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <ostream>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>

namespace server{

template<typename T>
static T CreateMask(uint32_t bits){
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}

class Address{
public:
    typedef std::shared_ptr<Address> ptr;
    virtual ~Address()=default;

    static bool host2Address(std::vector<ptr>&, const std::string& host, const std::string& service, int family, int type, int protocol = 0);

    int getFamily() const;
    virtual const sockaddr* getAddr() const=0;
    virtual sockaddr* getAddr() =0;
    virtual socklen_t getAddrLen() const=0;

    // 刷出地址信息
    virtual std::ostream& insert(std::ostream&) const;
    std::string toString() const;

    bool operator<(const Address&) const;
    bool operator==(const Address&) const;
    bool operator!=(const Address& r) const {return !operator==(r);}


};


class IPAddress: public Address{
public:
    typedef std::shared_ptr<IPAddress> ptr;

    // 输入网络号长度 返回广播地址
    virtual IPAddress::ptr broadcastAddress(uint32_t)=0;
    // 输入网络号长度 返回网络地址
    virtual IPAddress::ptr networkAddress(uint32_t)=0;
    // 输入网络号长度 返回子网掩码
    virtual IPAddress::ptr subnetMask(uint32_t)=0;
    
    virtual uint32_t getPort() const=0;
    virtual void setPort(uint32_t)=0;
};

class IPv4Address: public IPAddress{
public:
    typedef std::shared_ptr<IPv4Address> ptr;
    static IPv4Address CreateAddress(const char* address, uint16_t port = 0);
    static ptr CreateAddressPtr(const char* address, uint16_t port = 0);
    IPv4Address(const sockaddr_in&);
    IPv4Address(uint32_t address = INADDR_ANY, uint32_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override{return (sockaddr*)&m_addr;}
    socklen_t getAddrLen() const override;

    std::ostream& insert(std::ostream&) const override;

    IPAddress::ptr broadcastAddress(uint32_t) override;
    IPAddress::ptr networkAddress(uint32_t) override;
    IPAddress::ptr subnetMask(uint32_t) override;
    
    uint32_t getPort() const override;
    void setPort(uint32_t) override;
private:
    sockaddr_in m_addr;
};

class IPv6Address: public IPAddress{
public:
    typedef std::shared_ptr<IPv6Address> ptr;
    static ptr CreateAddress(const char* address, uint16_t port = 0);
    IPv6Address(const char* address = "::1", uint32_t port = 0);
    IPv6Address(const sockaddr_in6&);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override{return (sockaddr*)&m_addr;}
    socklen_t getAddrLen() const override;

    std::ostream& insert(std::ostream&) const override;

    IPAddress::ptr broadcastAddress(uint32_t) override;
    IPAddress::ptr networkAddress(uint32_t) override;
    IPAddress::ptr subnetMask(uint32_t) override;
    
    uint32_t getPort() const override;
    void setPort(uint32_t) override;
private:
    sockaddr_in6 m_addr;
};

class UnixAddress: public Address{
public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const std::string&);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override{return (sockaddr*)&m_addr;}
    socklen_t getAddrLen() const override;

    void setAddrLen(socklen_t len){m_length = len;}

    std::ostream& insert(std::ostream&) const override;

private:
    struct sockaddr_un m_addr;
    socklen_t m_length;
};

class UnknowAddress: public Address{
public:
    typedef std::shared_ptr<UnknowAddress> ptr;
    UnknowAddress(int family = AF_INET);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override{return (sockaddr*)&m_addr;}
    socklen_t getAddrLen() const override;

    std::ostream& insert(std::ostream&) const override;
private:
    sockaddr m_addr;
};



} // namespace server

#endif // SERVER_ADDRESS_H
