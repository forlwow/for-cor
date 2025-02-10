#ifndef SERVER_TEST_UTIL_HPP
#define SERVER_TEST_UTIL_HPP

#include "ethread.h"
#include "fqueue.h"
#include "log.h"
#include "threadsafe_deque.h"
#include "timer.h"
#include <cstdlib>
#include <functional>
#include <vector>
#include <range.h>

auto logger = SERVER_LOGGER_SYSTEM;

template<typename T>
bool read(server::util::fpipe<T> &from, std::vector<T> &to){
    T value = T();
    bool res = from.read_noblock(value);
    if(res) {
        to.push_back(value);
    }
    return true;
}

template<typename T>
void write(server::util::fpipe<T> &to, std::vector<T> &from, int nums){
    while (from.size() > 1 && --nums){
        T value = from.back();
        from.pop_back();
        to.write(value, true);
    }
    T value = from.back();
    from.pop_back();
    to.write(value, false);
}

const int vsize = 5e6;
const int writeNums = 10;

int test_fque(){
    Timer timer;
    std::vector<int> v(vsize, 0);
    std::vector<int> to;
    server::util::fpipe<int> pipe;

    for (int i = 0; i < vsize; ++i) {
        v[i] = i;
    }

    auto read_pipe = [&to, &pipe]{
        while (to.size() < vsize){
            read(pipe, to);
        }
    };
    auto write_pipe = [&v, &pipe]{
        while (!v.empty()){
            write(pipe, v, writeNums);
        }
        pipe.notify();
    };

    timer.start_count();
    server::EThread::ptr tread = std::make_shared<server::EThread>(read_pipe, "ft1");
    server::EThread::ptr twrite = std::make_shared<server::EThread>(write_pipe, "ft2");
    tread->join();
    twrite->join();
    timer.end_count();
    SERVER_LOG_DEBUG(logger) << "fipe complete num=" << to.size();
    SERVER_LOG_DEBUG(logger) << "fpipe: " << std::to_string(timer.get_duration());
    return timer.get_duration<std::chrono::milliseconds>().count();
}

int test_deque(){
    Timer timer;
    std::vector<int> v(vsize, 1);
    std::vector<int> to;
    server::threadsafe_deque<int> deque;
    auto read_deque = [&deque, &to]{
        while (to.size() < vsize){
            int data;
            deque.pop_front(data);
            to.push_back(data);
        }
    };
    auto write_deque = [&v, &deque]{
        while (!v.empty()){
            int data = v.back();
            v.pop_back();
            deque.push_back(data);
        }
    };
    timer.start_count();
    server::EThread::ptr tread = std::make_shared<server::EThread>(read_deque, "dt1");
    server::EThread::ptr twrite = std::make_shared<server::EThread>(write_deque, "dt2");
    tread->join();
    twrite->join();
    timer.end_count();
    SERVER_LOG_DEBUG(logger) << "deque complete num=" << to.size();
    SERVER_LOG_DEBUG(logger) << "Deque: " << std::to_string(timer.get_duration());
    return timer.get_duration<std::chrono::milliseconds>().count();
}

void test_que(){
    int sum = 0, n = 100;
    for (int i : range(n)) {
        int res = test_fque();
        // int res = test_deque();
        SERVER_LOG_DEBUG(logger) << "res: " << std::to_string(res) << "ms";
        sum += res;
    }
    SERVER_LOG_DEBUG(logger) << "average res: " << sum / n << "ms";
}

#endif