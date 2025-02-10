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

int test_main(){
    std::cout << "Test Version 1.0.0" << std::endl;
    test_que();
    // std::shared_ptr<int> p(new int);
    // std::shared_ptr<int> tmp;
    // server::util::fpipe<std::shared_ptr<int>, 1> pipe;
    //
    // SERVER_LOG_DEBUG(logger) << "count1:" << p.use_count();
    // pipe.write(p, false);
    // SERVER_LOG_DEBUG(logger) << "count2:" << p.use_count();
    // pipe.write(p, false);
    // SERVER_LOG_DEBUG(logger) << "count3:" << p.use_count();
    // pipe.read_block(tmp);
    // SERVER_LOG_DEBUG(logger) << "count4:" << p.use_count();
    // pipe.read_block(tmp);
    // SERVER_LOG_DEBUG(logger) << "count5:" << p.use_count();
    return 0;
}

#endif