#include "test_log.h"
#include "log.h"
#include <vector>

auto system_logger = SERVER_LOGGER_SYSTEM;
auto file_logger = SERVER_LOGGER("file");
std::vector<server::Logger::ptr> loggers = {system_logger, file_logger};

const char msg[] = "test msg %s %f %l";

void test_output(){
    for(auto log: loggers){
        SERVER_LOG_DEBUG(log) << msg;
        SERVER_LOG_INFO(log) << msg;
        SERVER_LOG_WARN(log) << msg;
        SERVER_LOG_ERROR(log) << msg;
        SERVER_LOG_FATAL(log) << msg;
    }
}