//
// Created by worker on 25-2-21.
//

#include "test_dashboard.h"

#include <http_server.h>

void test_dashboard_() {
    static server::HttpServer::ptr s = std::make_shared<server::HttpServer>();
    s->GET("/api/server/info", [](server::HttpContext::ptr c){
        c->JSON(200, {
            {"name", "forlwow"},
            {"version", "ubuntu-20.04"},
            {"kernal_version", "5.4.0"},
            {"system", "x86_64"},
            {"start_time", ""},
            {"run_time", ""}
        });
    });

    s->serverV4("192.168.1.100", 39002);
}

void test_dashboard(){

}