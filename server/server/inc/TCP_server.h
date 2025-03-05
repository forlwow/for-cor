#ifndef SERVER_TCP_SERVER_H
#define SERVER_TCP_SERVER_H
#include <iomanager.h>
#include <memory>
#include <vector>
#include "socket.h"

namespace server {

class TcpServer : public std::enable_shared_from_this<TcpServer>
{
public:
    typedef std::shared_ptr<TcpServer> ptr;
    TcpServer(int max_thread = 2);
    virtual ~TcpServer();


    virtual bool bind(Address::ptr address);
    // 绑定多个套接字 绑定失败的地址放入第二个参数中
    virtual bool bind(const std::vector<Address::ptr>& addresses, std::vector<Address::ptr>&);
    virtual bool start();
    virtual void stop();

    uint64_t getReadTimeout() const {return m_readTimeOut;}
    std::string getName() const {return m_name;}
    void setReadTimeout(uint64_t timeout) {m_readTimeOut = timeout;};
    void setName(const std::string& name) {m_name = name;}

    bool isStop() const { return m_isStop; }

protected:
    virtual Task handleClient(Socket::ptr client);
    virtual Task startAccept(Socket::ptr sock);

private:
    std::vector<Socket::ptr> m_sockets;
    IOManager::ptr m_worker;
    uint64_t m_readTimeOut;
    std::string m_name;
    bool m_isStop;
};


class TcpEchoServer : public TcpServer {
public:
    TcpEchoServer() = default;
    ~TcpEchoServer() override = default;
protected:
    Task handleClient(Socket::ptr client) override;

};



}


#endif

