#ifndef SERVER_TEST_MAIN_H
#define SERVER_TEST_MAIN_H

#include <iostream>
#include "test_log.h"

int test_main(){
    std::cout << "Test Version 1.0.0" << std::endl;
    test_log();
    return 0;
}

#endif