#ifndef SERVER_TEST_MAIN_H
#define SERVER_TEST_MAIN_H


#include <iostream>
#include "test_web_front.h"

#include "test_log.h"
#include "log.h"
#include "test_dashboard.h"
#include "test_static_web.h"

int test_main(){
    auto logger = SERVER_LOGGER_SYSTEM;
    // std::cout << "Test Version 1.0.1" << std::endl;
    SERVER_LOG_INFO(logger) << "Test Version 1.0.1";
    // test_http_performance();  // 测试http服务器性能
    // test_async_log_performance();    // 测试异步日志
    // test_performance();  // 同步日志性能测试
    test_dashboard();    // 测试动态网页
    // test_static_web();  // 测试静态网页
    // test_web_front();
    SERVER_LOG_INFO(logger) << "Test END";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}

#endif