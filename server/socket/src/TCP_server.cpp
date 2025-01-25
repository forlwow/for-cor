#include "TCP_server.h"

#include <socketfunc_cpp20.h>

#include "log.h"

auto logger = SERVER_LOGGER_SYSTEM;

namespace server
{
    TcpServer::TcpServer(IOManager* worker)
        : m_worker(worker),
        m_readTimeOut(1000),
        m_name("server/1.0.0"),
        m_isStop(true)
    {

    }

    TcpServer::~TcpServer()
    {
        for (auto &i: m_sockets) {
            i->close();
        }
        m_sockets.clear();
    }

    bool TcpServer::bind(Address::ptr address)
    {
        std::vector<Address::ptr> v;
        v.push_back(address);
        return bind(v, v);
    }

    bool TcpServer::bind(const std::vector<Address::ptr>& addresses, std::vector<Address::ptr> &fail){
        bool res = true;
        for (auto &i: addresses){
            Socket::ptr sock = Socket::CreateTCP(i);
            if (!sock->bind(i)){
                SERVER_LOG_ERROR(logger) << "TCP server bind failed" << "errno= " << errno << "addr=[" << i->toString() << "]";
                res = false;
                fail.push_back(i);
                continue;
            }
            if (!sock->listen()){
                SERVER_LOG_ERROR(logger) << "TCP server listen failed" << "errno = " << errno << "addr=[" << i->toString() << "]";
                res = false;
                fail.push_back(i);
                continue;
            }
            m_sockets.push_back(sock);
            SERVER_LOG_DEBUG(logger) << "TCP server bind success: " << i->toString();
        }
        return res;
    }

    bool TcpServer::start()
    {
        if (!m_isStop) {
            return true;
        }
        m_isStop = false;
        for (auto &i: m_sockets) {
            m_worker->schedule(std::make_shared<AsyncFiber>(std::bind_front(&TcpServer::startAccept, shared_from_this(), i)));
        }
        return true;
    }

    void TcpServer::stop(){
        auto self = shared_from_this();
        for (auto &i: m_sockets) {
            m_worker->schedule(std::make_shared<FuncFiber>([this, self](){
                for (auto &sock : m_sockets) {
                    sock->close();
                }
                m_sockets.clear();
            }));
        }
    }

    void TcpServer::handleClient(Socket::ptr client){
        SERVER_LOG_INFO(logger) << "TCPServer handleClient : " << client->toString();
    }

    Task TcpServer::startAccept(Socket::ptr sock) {
        while (!m_isStop) {
            // Socket::ptr client = sock->accept();
            Socket::ptr client = co_await server::accept(sock);

            if (client == nullptr) {
                SERVER_LOG_ERROR(logger) << "TCP server accept failed " << "errno=" << errno << " errstr=" << std::string(strerror(errno));
            }
            else {
                std::function<void()> fun = std::bind_front(&TcpServer::handleClient, this, client);
                m_worker->schedule(std::make_shared<FuncFiber>(fun));

            }
        }
    }

}
