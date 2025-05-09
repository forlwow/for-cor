//
// Created by worker on 25-5-9.
//

#include "test_static_web.h"
#include "http_server.h"
#include "configuration.h"
#include "file_reader.h"
#include "log.h"


static auto IP = server::util::Config::GetInstance()->ReadString("IP");
static auto PORT = server::util::Config::GetInstance()->ReadInt("PORT");

// /template/
// ├── index.html         # 博客首页
// ├── about.html         # 关于我们
// ├── contact.html       # 联系方式
// ├── post1.html         # 模拟博文页面1
// ├── post2.html         # 模拟博文页面2

static std::string indexPage = server::util::FileReader::readFile("./template/index.html");
static std::string about = server::util::FileReader::readFile("./template/about.html");
static std::string contact = server::util::FileReader::readFile("./template/contact.html");
static std::string post1 = server::util::FileReader::readFile("./template/post1.html");
static std::string post2 = server::util::FileReader::readFile("./template/post2.html");


void IndexHandler(server::HttpContext::ptr c) {
    c->SetHeader("Access-Control-Allow-Credentials", "true");
    c->SetHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS,DELETE,PUT,HEAD,PATCH");
    c->SetHeader("Access-Control-Allow-Origin", "*");
    c->SetHeader("Access-Control-Allow-Headers", "*");
    c->HTML(200, indexPage);
}
void AboutHandler(server::HttpContext::ptr c) {
    c->HTML(200, about);
}
void ContactHandler(server::HttpContext::ptr c) {
    c->HTML(200, contact);
}
void Post1Handler(server::HttpContext::ptr c) {
    c->HTML(200, post1);
}
void Post2Handler(server::HttpContext::ptr c) {
    c->HTML(200, post2);
}

void test_static_web(){
    server::HttpServer::ptr s = std::make_shared<server::HttpServer>(5);
    s->GET("/index", IndexHandler);
    s->GET("/about", AboutHandler);
    s->GET("/contact", ContactHandler);
    s->GET("/post1", Post1Handler);
    s->GET("/post2", Post2Handler);
    s->serverV4(IP, PORT);
}