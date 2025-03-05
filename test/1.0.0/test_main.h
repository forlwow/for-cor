#ifndef SERVER_TEST_MAIN_H
#define SERVER_TEST_MAIN_H


#include <iostream>

#include "test_log.h"
#include "test_util.h"
#include "log.h"
#include "test_dashboard.h"


int test_main(){
    auto logger = SERVER_LOGGER_SYSTEM;
    // std::cout << "Test Version 1.0.0" << std::endl;
    SERVER_LOG_DEBUG(logger) << "Test Version 1.0.0";
    test_dashboard();
    SERVER_LOG_DEBUG(logger) << "Test END";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}

#endif