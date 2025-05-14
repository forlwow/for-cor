//
// Created by worker on 25-2-21.
//

#include "test_dashboard.h"

#include <http_server.h>
#include <sys_info.h>
#include "configuration.h"
#include "file_reader.h"
#include "socketfunc_cpp20.h"
#include "Json/json.hpp"
#include "format"
#include "timer.h"

static auto logger = SERVER_LOGGER_SYSTEM;
static auto file_logger = SERVER_LOGGER("file");
static std::string IP = server::util::Config::GetInstance()->ReadString("IP");
static uint16_t PORT = server::util::Config::GetInstance()->Read<uint16_t>("PORT");
static std::string CARD = server::util::Config::GetInstance()->Read<std::string>("NET_CARD");

static const std::string EmptyResult = "No Result";
static std::string CmdResult = EmptyResult;

server::Task RunSysCmd(const std::string s) {
    // SERVER_LOG_INFO(logger) << "RunSysCmd: " << s;
    CmdResult = co_await server::ExecAwaitable(s);
    // SERVER_LOG_INFO(logger) << "CmdResult: " << CmdResult;
    co_return;
}

void get_server_log(server::HttpContext::ptr c) {
    auto j = nlohmann::json::parse(c->m_request->GetBody());
    if (!j.contains("file") || !j.contains("startLine") || !j.contains("count")) {
        // 字段缺失，返回空命令
        c->JSON(400, {{"msg" , "load error"}});
        return;
    }
    std::string file = j["file"].get<std::string>();
    int start = j.at("startLine").get<int>();
    int count = j.at("count").get<int>();
    std::vector<std::string> res;
    if (start < 0) {
        res = server::util::FileReader::readLastLines(file, count);
    }
    else {
        res = server::util::FileReader::readLines(file, start, count > 10 ? 10 : count);
    }
    if (res.empty()) {
        c->TEXT(204, "Empty log");
    }
    else {
        std::string s;
        for (auto &i : res) {
            s.append(i);
            s.push_back('\n');
        }
        auto size = server::util::FileReader::getFileSize(file);
        auto lines = server::util::FileReader::getFileLineCount(file);
        c->JSON(200, {
            {"data", s},
            {"size", std::to_string(size)},
            {"lines", std::to_string(lines)},
            }
            );
    }
}

void write_server_log(server::HttpContext::ptr c) {
    auto j = nlohmann::json::parse(c->m_request->GetBody());
    if (!j.contains("level") || !j.contains("message") || !j.contains("count")) {
        // 字段缺失，返回空命令
        c->JSON(400, {{"msg" , "load error"}});
        return;
    }
    int count = j.at("count").get<int>();
    std::string level = j.at("level").get<std::string>();
    std::string message = j.at("message").get<std::string>();
    for (int i = 0; i < count; i++) {
        if (level == "debug") {
            SERVER_LOG_DEBUG(logger) << message;
        }
        else if (level == "info") {
            SERVER_LOG_INFO(logger) << message;
        }
        else if (level == "warn") {
            SERVER_LOG_WARN(logger) << message;
        }
        else if (level == "error") {
            SERVER_LOG_ERROR(logger) << message;
        }
        else {
            c->JSON(400, {{"msg" , "load error"}});
            return;
        }
    }
    c->JSON(200, {{"msg" , "OK"}});
}

void flush_server_log(server::HttpContext::ptr c) {
    const std::string s = c->m_request->GetBody().data();
    auto l = SERVER_LOGGER(s);
    if (!l) {
        c->JSON(400, {{"msg" , "load error"}});
        return;
    }
    l->flush();
    c->JSON(200, {{"msg" , "OK"}});
}

std::atomic_uint64_t time_counter = 0;

void start_server_log_load(server::HttpContext::ptr c) {
    auto j = nlohmann::json::parse(c->m_request->GetBody());
    if (!j.contains("count")) {
        c->JSON(400, {{"msg" , "load error"}});
        return;
    }
    int count = j.at("count").get<int>();
    server::IOManager_::GetInstance()->schedule(server::FuncFiber::CreatePtr(
        [count]()
        {
            Timer timer;
            timer.start_count();
            for (int i = 0; i < count; i++) {
                SERVER_LOG_INFO(file_logger);
            }
            timer.end_count();
            time_counter.store(timer.get_duration().count());
        }
        ));
    c->JSON(200, {{"msg" , "OK"}});
}

void get_server_log_load(server::HttpContext::ptr c) {
    c->TEXT(200, std::to_string(time_counter.load()) + "ms");
}

void serverinfo(server::HttpContext::ptr c) {
    c->JSON(200, {
        {"name", "forlwow"},
        {"version", "ubuntu-20.04"},
        {"kernel_version", "5.4.0"},
        {"system", "x86_64"},
        {"start_time", "2025-04-21"},
        {"run_time", "54h-46m-12s"},
        {"status", "run"}
        }
    );
}

void server_load(server::HttpContext::ptr c) {
    auto cpu = server::util::getCpuUse();
    auto m = server::util::getMemUsage();
    server::util::DiskInfo d;
    server::util::getDiskInfo_(d, "/");
    server::util::NetInfo netinfo;
    server::util::getNetInfo_(netinfo, CARD);
    // server::util::getNetInfo_(netinfo, "eth0");
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

void get_server_load(server::HttpContext::ptr c) {
    c->TEXT(200, CmdResult);
}

void start_server_load(server::HttpContext::ptr c) {
    auto j = nlohmann::json::parse(c->m_request->GetBody());
    if (!j.contains("duration") || !j.contains("concurrency") || !j.contains("target")) {
        // 字段缺失，返回空命令
        c->JSON(400, {{"msg" , "load error"}});
        return;
    }
    int threads = j.at("concurrency").get<int>();
    int duration = j.at("duration").get<int>();
    std::string target = j.at("target").get<std::string>();
    std::string cmd = std::format("wrk -t4 -c{} -d{}s {}", threads, duration, target);

    server::IOManager_::GetInstance()->schedule(server::AsyncFiber::CreatePtr(RunSysCmd, cmd));
    c->JSON(200,
        {
            {"msg", "OK"},
        }
        );
}

void test_dashboard_() {
    static server::HttpServer::ptr s = std::make_shared<server::HttpServer>();
    s->SetDefaultHeader("Access-Control-Allow-Credentials", "true");
    s->SetDefaultHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS,DELETE,PUT,HEAD,PATCH");
    s->SetDefaultHeader("Access-Control-Allow-Origin", "*");
    s->SetDefaultHeader("Access-Control-Allow-Headers", "*");

    s->GET("/api/server/info", serverinfo);
    s->GET("/api/server/load", server_load);
    s->POST("/api/server/loadtest", start_server_load);
    s->GET("/api/server/loadtest", get_server_load);
    s->GET("/ping", [](server::HttpContext::ptr c) {
        c->JSON(200, {{"msg", "pong"}});
    });
    s->POST("/api/server/log/read", get_server_log);
    s->POST("/api/server/log/write", write_server_log);
    s->POST("/api/server/log/flush", flush_server_log);

    s->POST("/api/server/log/load", start_server_log_load);
    s->GET("/api/server/log/load/get", get_server_log_load);

    s->serverV4(IP, PORT);
}

void test_dashboard(){
    test_dashboard_();
}
