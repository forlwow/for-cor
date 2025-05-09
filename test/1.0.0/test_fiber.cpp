//
// Created by worker on 25-4-23.
//

#include "test_fiber.h"

#include <fiberfunc_cpp20.h>
#include <iomanager.h>
#include <timer.h>

#include "fiber.h"
#include "iomanager_cpp20.h"

using server::Task;

static auto logger = SERVER_LOGGER_SYSTEM;
static unsigned long long times1 = 0;
static unsigned long long times2 = 0;

int thread_pool_size = 5;
long num_tasks = 1e6;

Task test_fib(std::string_view name) {
    Timer timer;
    timer.start_count();
    for (int i = 0; i < 10; i++) {
        co_yield server::YIELD;
    }
    timer.end_count();
    times1 += timer.get_duration<std::chrono::microseconds>().count();
    co_return;
}

void test_thread(std::string_view name) {
    Timer timer;
    timer.start_count();
    for (int i = 0; i < 10; i++) {
        std::this_thread::yield();
    }
    timer.end_count();
    times2 += timer.get_duration<std::chrono::microseconds>().count();
    return;
}

void test_th() {
    ThreadPool pool(thread_pool_size);
    std::vector<std::future<void>> futures;
    futures.reserve(num_tasks);

    Timer total_timer;

    for (int i = 0; i < num_tasks; ++i) {
        futures.emplace_back(pool.submit([i]() {
            test_thread("thread_" + std::to_string(i));
        }));
    }

    for (auto& fut : futures) {
        fut.get();
    }

    // total_timer.stop();
}

void test_fiber() {
    auto iom = server::IOManager::GetInstance(5);
    iom->start();

    for (int i = 0; i < 1e6; i++) {
        iom->schedule(server::AsyncFiber::CreatePtr(test_fib, "a"));
    }

    iom->wait_stop(0);
    std::cout << "times: " << times1 << " single: " << times1/1e6 <<  std::endl;
}
