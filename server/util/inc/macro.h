#ifndef WEBSERVER_MACRO_H
#define WEBSERVER_MACRO_H

#include "macro_define.h"

// 日志模式
#ifndef LOG_MODE
    #define LOG_MODE ASYNC_LOG
#endif
#define ASYNC_LOG_THREADS 1

// 当前日志等级
#ifndef CURRENT_LOG_LEVEL
    #define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#endif


#endif
