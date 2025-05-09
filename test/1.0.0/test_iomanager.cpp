//
// Created by worker on 25-4-27.
//

#include "test_iomanager.h"
#include "iomanager.h"
#include "log.h"

static auto logger = SERVER_LOGGER_SYSTEM;

server::Task test_task() {
    SERVER_LOG_DEBUG(logger) << "1";
    co_yield server::YIELD;
    SERVER_LOG_DEBUG(logger) << "1";
    co_yield server::YIELD;
    SERVER_LOG_DEBUG(logger) << "1";
    co_yield server::YIELD;
    SERVER_LOG_DEBUG(logger) << "1";
    co_return;
}

void test_iomanager() {
    auto iom = server::IOManager_::GetInstance(5);
    iom->start();

    for (int i = 0; i < 1e6; i++) {
        iom->schedule(server::AsyncFiber::CreatePtr(test_task));
    }

    iom->wait();
}
