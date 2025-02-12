#ifndef SERVER_TEST_MAIN_H
#define SERVER_TEST_MAIN_H

#include <cstddef>
#include <iostream>
#include "atomic_ptr.h"
#include "fqueue.h"
#include "test_log.h"
#include "test_util.h"
#include "timer.h"
#include "util.h"
#include <atomic>
#include <unistd.h>
#include "range.h"
#include "time_wheel.h"

int test_main(){
    std::cout << "Test Version 1.0.0" << std::endl;
    test_timewheel();
    return 0;
}

#endif