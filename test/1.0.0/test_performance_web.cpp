//
// Created by worker on 25-4-19.
//
#include "test_performance_web.h"
#include "http_server.h"
#include "configuration.h"

static auto logger = SERVER_LOGGER_SYSTEM;
static std::string IP = server::util::Config::GetInstance()->ReadString("IP");
static uint16_t PORT = server::util::Config::GetInstance()->Read<uint16_t>("PORT");

void test_fun(server::HttpContext::ptr c) {
    c->SetHeader("Access-Control-Allow-Credentials", "true");
    c->SetHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS,DELETE,PUT,HEAD,PATCH");
    c->SetHeader("Access-Control-Allow-Origin", "*");
    c->SetHeader("Access-Control-Allow-Headers", "*");
    c->HTML(200, "Hello world!");
}


void test_performance_web() {
    static server::HttpServer::ptr s = std::make_shared<server::HttpServer>();
    s->GET("/performance_web", test_fun);
    s->serverV4(IP, PORT);
}

