#include "TCP_server.h"

#include <socketfunc_cpp20.h>

#include "log.h"


namespace server
{
    static auto logger = SERVER_LOGGER_SYSTEM;


    TcpServer::TcpServer(int max_thread)
        : m_worker(IOManager_::GetInstance(max_thread)),
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
                SERVER_LOG_ERROR(logger) << "TCP server bind failed " << "errno= " << errno << "addr=[" << i->toString() << "]";
                res = false;
                fail.push_back(i);
                continue;
            }
            if (!sock->listen()){
                SERVER_LOG_ERROR(logger) << "TCP server listen failed " << "errno = " << errno << "addr=[" << i->toString() << "]";
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
        for (Socket::ptr & i: m_sockets) {
            // m_worker->schedule(
            //     std::make_shared<AsyncFiber>(
            //         std::bind_front(&TcpServer::startAccept, shared_from_this(), i)
            //         )
            //     );
            m_worker->schedule(AsyncFiber::CreatePtr(&TcpServer::startAccept, shared_from_this(), i));
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

    Task TcpServer::handleClient(Socket::ptr client){
        SERVER_LOG_INFO(logger) << "TCPServer handleClient : " << client->toString();
        co_return;
    }

    Task TcpServer::startAccept(Socket::ptr sock) {
        auto strongSock = sock;
        while (1) {
            // Socket::ptr client = sock->accept();
            Socket::ptr client = co_await server::accept(strongSock);

            if (!client) {
                if (errno != EAGAIN && errno != 0) {
                    SERVER_LOG_ERROR(logger) << "TCP server accept failed " <<
                        "accept socket: " << sock->getFd() << " " <<
                        "socket error: " << sock->getError() << " " <<
                        "errno=" << errno <<
                            " errstr=" << std::string(strerror(errno));
                }
            }
            else {
                SERVER_LOG_DEBUG(logger) << "TCP server accept: " << client->getFd();
                m_worker->schedule(std::make_shared<AsyncFiber>(&TcpServer::handleClient, this, client));
            }
        }
        SERVER_LOG_WARN(logger) << "TCP server accept close";
        co_return;
    }

    Task TcpEchoServer::handleClient(Socket::ptr client) {
        const int MAXSIZE = 1024;
        char buffer[MAXSIZE];
        auto recver = server::recv(client, buffer, MAXSIZE);
        auto sender = server::send(client, buffer, 0, 0);
        while (1) {
            recver.reset(buffer, MAXSIZE);
            int num = co_await recver;
            // int num = co_await server::recv(client, buffer, MAXSIZE);
            if (num < 0) {
                SERVER_LOG_WARN(logger) << "TCP server recv failed: " << Sock_Result2String(num);
                if (num == SOCK_CLOSE) {
                    break;
                }
                continue;
            }
            sender.reset(buffer, num);
            int wnum = co_await sender;
            if (wnum < 0) {
                SERVER_LOG_WARN(logger) << "TCP server send failed: " << Sock_Result2String(wnum);
                if (wnum == SOCK_CLOSE) {
                    break;
                }
                continue;
            }
            SERVER_LOG_DEBUG(logger) << "TCP server echo success";
        }
    }
}
