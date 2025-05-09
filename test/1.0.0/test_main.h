#ifndef SERVER_TEST_MAIN_H
#define SERVER_TEST_MAIN_H


#include <iostream>

#include "test_log.h"
#include "test_util.h"
#include "log.h"
#include "test_dashboard.h"
#include "test_performance_web.h"
#include "test_tcpserver.h"
#include "test_iomanager.h"
#include "test_fiber.h"
#include "test_static_web.h"

int test_main(){
    auto logger = SERVER_LOGGER_SYSTEM;
    // std::cout << "Test Version 1.0.0" << std::endl;
    SERVER_LOG_DEBUG(logger) << "Test Version 1.0.0";
    // test_fiber();
    // test_iomanager();
    // server::test::test_http_performance();  // 测试http服务器性能
    // test_performance_web();
    // test_async_log_performance();    // 测试异步日志
    test_performance();  // 测试日志
    // test_dashboard();    // 测试动态网页
    // test_static_web();  // 测试静态网页
    SERVER_LOG_DEBUG(logger) << "Test END";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}

#endif