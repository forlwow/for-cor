//
// Created by worker on 25-2-21.
//

#include "test_dashboard.h"

#include <http_server.h>
#include <sys_info.h>

static auto logger = SERVER_LOGGER_SYSTEM;

void serverinfo(server::HttpContext::ptr c) {
    c->SetHeader("Access-Control-Allow-Credentials", "true");
    c->SetHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS,DELETE,PUT,HEAD,PATCH");
    c->SetHeader("Access-Control-Allow-Origin", "*");
    c->SetHeader("Access-Control-Allow-Headers", "*");
    c->JSON(200, {
        {"name", "forlwow"},
        {"version", "ubuntu-20.04"},
        {"kernel_version", "5.4.0"},
        {"system", "x86_64"},
        {"start_time", "2025-07-21"},
        {"run_time", "54h-46m-12s"},
        {"status", "run"}
        }
    );
}

void server_load(server::HttpContext::ptr c) {

}

void test_dashboard_() {
    static server::HttpServer::ptr s = std::make_shared<server::HttpServer>();
    s->GET("/api/server/info", serverinfo);

    s->serverV4("192.168.1.110", 39000);
}

void test_dashboard(){
    // test_dashboard_();
    SERVER_LOG_DEBUG(logger) << std::to_string(server::util::getCpuUse());
}