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
    c->SetHeader("Access-Control-Allow-Credentials", "true");
    c->SetHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS,DELETE,PUT,HEAD,PATCH");
    c->SetHeader("Access-Control-Allow-Origin", "*");
    c->SetHeader("Access-Control-Allow-Headers", "*");
    auto cpu = server::util::getCpuUse();
    auto m = server::util::getMemUsage();
    server::util::DiskInfo d;
    server::util::getDiskInfo_(d, "/");
    server::util::NetInfo netinfo;
    server::util::getNetInfo_(netinfo, "eth0");
    c->JSON(200, {
        {"cpu", std::to_string(cpu)},
        {"mem", std::to_string(m)},
        {"disk-total", std::to_string(d.total)},
        {"disk-free", std::to_string(d.free)},
        {"net-w", std::to_string(netinfo.wbytes)},
        {"net-r", std::to_string(netinfo.rbytes)},
    }
    );
}

void test_dashboard_() {
    static server::HttpServer::ptr s = std::make_shared<server::HttpServer>();
    s->GET("/api/server/info", serverinfo);
    s->GET("/api/server/load", server_load);

    s->serverV4("10.120.115.120", 39000);
}

void test_dashboard(){
    test_dashboard_();
}
