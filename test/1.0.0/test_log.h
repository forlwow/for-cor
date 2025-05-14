#ifndef SERVER_TEST_LOG_H
#define SERVER_TEST_LOG_H


void test_output();
void test_performance();        // 同步日志性能测试

void test_async_log();

inline void test_log(){
    test_async_log();
}

void test_async_log_performance();

#endif