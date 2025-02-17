#include "test_log.h"

#include <async_log_pool.h>

#include "log.h"
#include "timer.h"
#include <vector>
#include <iostream>

auto system_logger = SERVER_LOGGER_SYSTEM;
auto file_logger = SERVER_LOGGER("file");
auto std_logger = SERVER_LOGGER("std");
std::vector<server::Logger::ptr> loggers = {system_logger, file_logger, std_logger};

const char msg[] = "test msg %s %f %l";

void test_output(){
    for(auto log: loggers){
        SERVER_LOG_DEBUG(log) << log->getName();
        SERVER_LOG_INFO(log) << log->getName();
        SERVER_LOG_WARN(log) << log->getName();
        SERVER_LOG_ERROR(log) << log->getName();
        SERVER_LOG_FATAL(log) << log->getName();
    }
}

void test_performance(){
    Timer timer;
    timer.start_count();
    for(int i = 0; i < 1e2; ++i){
        SERVER_LOG_INFO(system_logger);
    }
    timer.end_count();
    std::cout << std::to_string(timer.get_duration()) << std::endl;
}

void test_async_log() {
    SERVER_LOG_DEBUG(system_logger) << "0";
    auto asy = server::log::AsyncLogPool::GetInstance();
    asy->start();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    SERVER_LOG_DEBUG(system_logger) << "1";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    SERVER_LOG_DEBUG(system_logger) << "2";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    SERVER_LOG_DEBUG(system_logger) << "3";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    SERVER_LOG_DEBUG(system_logger) << "4";
    asy->wait_stop(1);
}
