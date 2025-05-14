//
// Created by lwow on 25-5-13.
//

#ifndef TEST_WEB_FRONT_H
#define TEST_WEB_FRONT_H


#include "http_server.h"
#include "configuration.h"
#include "file_reader.h"

static auto rootPage = server::util::FileReader::readFile("./template/benchmark.html");

static auto IP = server::util::Config::GetInstance()->ReadString("IP");
static auto PORT = server::util::Config::GetInstance()->ReadInt("PORT");

void page(server::HttpContext::ptr c) {
    c->HTML(200, rootPage);
}

void test_web_front() {
    auto s = std::make_shared<server::HttpServer>(2);
    s->GET("/", page);
    s->serverV4(IP, PORT);
}


#endif //TEST_WEB_FRONT_H
