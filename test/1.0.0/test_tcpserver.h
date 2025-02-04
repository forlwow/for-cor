//
// Created by worker on 25-1-25.
//

#ifndef SERVER_TEST_TCP_SERVER_H
#define SERVER_TEST_TCP_SERVER_H
#include <http_server.h>
#include "log.h"

namespace server{

    Logger::ptr logger = SERVER_LOGGER_SYSTEM;
namespace test
{
    extern char response[];

    void usercallback1(HttpContext::ptr c) {
        c->HTML(http::HTTP_STATUS_OK, "1");
    }
    void usercallback2(HttpContext::ptr c) {
        c->HTML(http::HTTP_STATUS_OK, "2");
    }
    void usercallback3(HttpContext::ptr c) {
        c->HTML(http::HTTP_STATUS_OK, "3");
    }


    void test_router() {
        Router::callback cb = [](HttpContext::ptr){
            SERVER_LOG_DEBUG(logger) << "Callback";
        };
        Router::callback testHandler = cb;
        Router::ptr router = std::make_shared<Router>();
        router->AddRoute("GET", "/user/profile", testHandler);
        router->AddRoute("POST", "/user/login", testHandler);
        router->AddRoute("POST", "/user/login/", testHandler);
        router->AddRoute("POST", "/user//login/", testHandler);

        auto cb1 = router->GetCallback("GET", "/user/profile");
        if (cb1) cb1(nullptr);

        auto cb2 = router->GetCallback("GET", "/user/404");
        if (cb2 == router->ncb()) SERVER_LOG_DEBUG(logger) << "Route not found";
    }

    void run() {
        Address::ptr addr = IPv4Address::CreateAddressPtr("127.0.0.1", 8999);
        Address::ptr addr2 = IPv4Address::CreateAddressPtr("127.0.0.1", 9000);
        std::vector<Address::ptr> addrs = {addr, addr2};
        std::vector<Address::ptr> fails;

        static TcpServer::ptr server = std::make_shared<TcpServer>();
        if (!server->bind(addrs, fails )) {
            SERVER_LOG_ERROR(logger) << "bind failed";
            return;
        }
        server->start();
    }

    void runEcho() {
        Address::ptr addr = IPv4Address::CreateAddressPtr("127.0.0.1", 8999);
        Address::ptr addr2 = IPv4Address::CreateAddressPtr("127.0.0.1", 9000);
        std::vector<Address::ptr> addrs = {addr, addr2};
        std::vector<Address::ptr> fails;

        static TcpEchoServer::ptr server = std::make_shared<TcpEchoServer>();
        if (!server->bind(addrs, fails )) {
            SERVER_LOG_ERROR(logger) << "bind failed";
            return;
        }
        server->start();
    }

    void runHttp() {
        Address::ptr addr = IPv4Address::CreateAddressPtr("192.168.1.110", 39001);
        std::vector<Address::ptr> addrs = {addr};
        std::vector<Address::ptr> fails;

        static HttpServer::ptr server = std::make_shared<HttpServer>();
        if (!server->bind(addrs, fails )) {
            SERVER_LOG_ERROR(logger) << "bind failed";
            return;
        }
        server->start();
    }

    void runHttpRouter() {
        Address::ptr addr = IPv4Address::CreateAddressPtr("192.168.1.110", 39001);
        std::vector<Address::ptr> addrs = {addr};
        std::vector<Address::ptr> fails;

        static HttpServer::ptr server = std::make_shared<HttpServer>();
        server->GET("/1", usercallback1);
        server->GET("/2", usercallback2);
        server->GET("/3", usercallback3);
        if (!server->bind(addrs, fails )) {
            SERVER_LOG_ERROR(logger) << "bind failed";
            return;
        }
        server->start();
    }

    void test_tcpserver() {
        IOManager::ptr iomanager = IOManager_::GetInstance(5);
        auto task = std::make_shared<FuncFiber>(runHttpRouter);
        iomanager->schedule(task);

        iomanager->start();
        iomanager->wait();
    }

    void test_httpserver() {
        auto server = std::make_shared<HttpServer>(5);
        server->GET("/1", usercallback1);
        server->GET("/2", usercallback2);
        server->GET("/3", usercallback3);
        server->serverV4("192.168.1.110", 39001);
    }

}
}

#endif //TEST_TCPSERVER_H
