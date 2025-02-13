#ifndef SERVER_TEST_MAIN_H
#define SERVER_TEST_MAIN_H

#include <iostream>
#include "test_util.h"

int test_main(){
    std::cout << "Test Version 1.0.0" << std::endl;
    test_timewheel();
    return 0;
}

#endif