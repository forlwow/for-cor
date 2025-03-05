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

int test_fque();
int test_deque();
void test_que();

int test_timewheel();

#endif