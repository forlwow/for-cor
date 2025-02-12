#ifndef SERVER_LOG_H
#define SERVER_LOG_H

#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <map>
#include <list>
#include <memory>
#include <cstdio>
#include <unordered_map>
#include <ethread.h>
#include <singleton.h>
#include "fiber.h"

#if __cplusplus >= 202002L
// TODO: 添加多线程
#define SERVER_LOG_LEVEL(logger, level)  \
    server::LogEventWrap(server::LogEvent::ptr(\
        new server::LogEvent(logger, level, \
        __FILE__, __LINE__, \
        0, server::t_thread_id, server::Fiber::GetCurFiberId(), \
        time(0), server::t_thread_name\
        )))
#else

#define SERVER_LOG_LEVEL2(logger, level)  \
    server::LogEventWrap(server::LogEvent::ptr(\
        new server::LogEvent(logger, level, \
        __FILE__, __LINE__, \
        0, server::t_thread_id, server::Fiber::GetCurFiberId(), \
        time(0), server::t_thread_name\
        )))
#endif

namespace server{
#define SERVER_LOG_LEVEL(logger, level)  \
    server::LogEventWrap(server::LogEvent::ptr(\
        new server::LogEvent(logger, level, \
        __FILE__, __LINE__, \
        0, server::t_thread_id, server::Fiber::GetCurFiberId(), \
        time(0), server::t_thread_name\
        )))

extern thread_local int t_thread_id;
extern thread_local const char* t_thread_name;

// 日志等级
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_ERROR 3
#define LOG_LEVEL_FATAL 4
#define LOG_LEVEL_OFF 5

// 当前日志等级 默认为INFO
#ifndef CURRENT_LOG_LEVEL
    #define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#endif

// DEBUG
#if CURRENT_LOG_LEVEL <= LOG_LEVEL_DEBUG
    #define SERVER_LOG_DEBUG(logger) SERVER_LOG_LEVEL(logger, server::LogLevel::DEBUG)
#else
    #define SERVER_LOG_DEBUG(logger) while (false) server::NullStream()
#endif

// INFO
#if CURRENT_LOG_LEVEL <= LOG_LEVEL_INFO
    #define SERVER_LOG_INFO(logger) SERVER_LOG_LEVEL(logger, server::LogLevel::INFO)
#else
    #define SERVER_LOG_INFO(logger) while (false) server::NullStream()
#endif

// WARN
#if CURRENT_LOG_LEVEL <= LOG_LEVEL_WARN
    #define SERVER_LOG_WARN(logger) SERVER_LOG_LEVEL(logger, server::LogLevel::WARN)
#else
    #define SERVER_LOG_WARN(logger) while (false) server::NullStream()
#endif

// ERROR
#if CURRENT_LOG_LEVEL <= LOG_LEVEL_ERROR
    #define SERVER_LOG_ERROR(logger) SERVER_LOG_LEVEL(logger, server::LogLevel::ERROR)
#else
    #define SERVER_LOG_ERROR(logger) while (false) server::NullStream()
#endif

// TODO 异常退出时也会输出
// FATAL
#if CURRENT_LOG_LEVEL <= LOG_LEVEL_FATAL
    #define SERVER_LOG_FATAL(logger) SERVER_LOG_LEVEL(logger, server::LogLevel::FATAL)
#else
    #define SERVER_LOG_FATAL(logger) while (false) server::NullStream()
#endif

// TODO: 支持级别定义
#define SERVER_LOG_DEBUG2(logger) SERVER_LOG_LEVEL2(logger, server::LogLevel::DEBUG)
#define SERVER_LOG_INFO2(logger) SERVER_LOG_LEVEL2(logger, server::LogLevel::INFO)
#define SERVER_LOG_WARN2(logger) SERVER_LOG_LEVEL2(logger, server::LogLevel::WARN)
#define SERVER_LOG_ERROR2(logger) SERVER_LOG_LEVEL2(logger, server::LogLevel::ERROR)
#define SERVER_LOG_FATAL2(logger) SERVER_LOG_LEVEL2(logger, server::LogLevel::FATAL)

// 系统日志
#define SERVER_LOGGER_SYSTEM server::LogManager::GetInstance()->getLogger("system")
#define SERVER_LOGGER(name) server::LogManager::GetInstance()->getLogger(name)

class Logger;


// 日志级别
class LogLevel{
public:
    enum Level{
        UNKNOW = 0,
        DEBUG = 1,
        INFO,
        WARN,
        ERROR,
        FATAL,
        OFF
    };

    inline static const std::unordered_map<Level, const char*> LevelColor = {
        {DEBUG, "\e[34m"},
        {INFO, "\e[32m"},
        {WARN, "\e[33m"},
        {ERROR, "\e[95m"},
        {FATAL, "\e[91m"},
        {OFF, "\e[39m"},
        {UNKNOW, "\e[36m"},
    };
    inline static const char* LevelColorEnd = "\e[0m";

    inline static const char* ToString(LogLevel::Level level) {
        switch (level){
        #define XX(name) \
            case LogLevel::name: \
                return #name; \
                break;
            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);
            XX(OFF);
        #undef XX
            default:
                return "UNKNOW";
        }
    }
   
    inline static LogLevel::Level FromString(const std::string& str){
        #define XX(level, v) \
            if(str == #v)  \
                return LogLevel::level;
            XX(DEBUG, debug);
            XX(DEBUG, DEBUG);
            XX(INFO, info);
            XX(INFO, INFO);
            XX(WARN, warn);
            XX(WARN, WARN);
            XX(ERROR, error);
            XX(ERROR, ERROR);
            XX(FATAL, fatal);
            XX(FATAL, FATAL);
            XX(OFF, OFF);
            XX(OFF, off);
            return LogLevel::UNKNOW;
        #undef XX
    }
};

// 日志事件
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
        ,const char* file, int32_t line, uint32_t elapse
        ,uint32_t thread_id, uint32_t fiber_id, uint64_t time
        ,const char* thread_name = "default");

    auto getFile() const {return m_file;}
    auto getLine() const {return m_line;}
    auto getElapse() const {return m_elapse;}
    auto getThreadId() const {return m_thread_id;}
    auto getFiberId() const {return m_fiber_id;}
    auto getTime() const {return m_time;}
    auto getTimePtr() const {return &m_time;}
    auto getThreadName() const {return m_threadName;}
    auto& getSS() {return m_ss;}
    auto getLogger() const {return m_logger;}
    auto getLevel() const {return m_level;}
private:
    int32_t m_line = 0;             // 行号
    uint32_t m_elapse = 0;          // 程序启动开始到现在的毫秒数
    uint32_t m_thread_id = 0;       // 线程号
    uint32_t m_fiber_id = 0;        // 协程号
    time_t m_time;                  // 时间戳
    const char* m_file = nullptr;   // 文件名
    const char* m_threadName;       // 线程名称
    std::string m_ss;               // 日志内容流
    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;
     
};

// 事件包装类
class LogEventWrap{
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    auto& getSS() {return m_event->getSS();}
    auto getEvent() {return m_event;}

    inline LogEventWrap& operator<<(const std::string& str){
        m_event->getSS().append(str);
        return *this;
    }
    inline LogEventWrap& operator<<(std::string_view str){
        m_event->getSS().append(str);
        return *this;
    }
    inline LogEventWrap& operator<<(const char* c){
        m_event->getSS().append(c);
        return *this;
    }
    LogEventWrap& operator<<(const std::unordered_map<std::string, std::string> &map){
        m_event->getSS().append("{ ");
        for (auto iter : map)
        {
            m_event->getSS().append(iter.first + ": " + iter.second + ", ");
        }
        m_event->getSS().append(" }");
        return *this;
    }
    LogEventWrap& operator<<(const std::map<std::string, std::string> &map){
        m_event->getSS().append("{ ");
        for (auto iter : map)
        {
            m_event->getSS().append(iter.first + ": " + iter.second + ", ");
        }
        m_event->getSS().append(" }");
        return *this;
    }
    template<typename T>
    LogEventWrap& operator<<(T arg){
        m_event->getSS().append(std::to_string(arg));
        return *this;
    }

    template<typename T>
    auto &operator()(T) {return this;}

private:
    LogEvent::ptr m_event;
};


// 日志格式器
class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string &pattern);

    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    FILE* format(FILE* file, std::shared_ptr<Logger> logger, LogEvent::ptr event);
public:
    class FormatItem{
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        FormatItem(const std::string &fmt = "") {}
        virtual ~FormatItem() = default;
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, const LogEvent::ptr &event) = 0;
        virtual void format(FILE* file, std::shared_ptr<Logger> logger, const LogEvent::ptr &event) = 0;
    };

    int init();

private:
    std::string m_pattern = "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n";
    std::vector<FormatItem::ptr> m_items;
};

// 日志输出地
class LogAppender{
public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender() = default;

    virtual void log(std::shared_ptr<Logger>, const LogEvent::ptr &event) = 0;

    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter() const;
    void setLevel(LogLevel::Level level);
    LogLevel::Level getLevel() const;
protected:
    LogLevel::Level m_level = LogLevel::INFO;
    LogFormatter::ptr m_formatter;
   
    mutable Mutex m_mutex;
};

// 日志输出器
class Logger: public std::enable_shared_from_this<Logger>{
public:
    typedef std::shared_ptr<Logger> ptr;

    Logger(const std::string &name = "undef");
    void log(const LogEvent::ptr &event);
    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    auto getName() const {return m_name;}
private:
    std::string m_name;                 // 日志名称
    std::list<LogAppender::ptr>  m_appenders;     // Appender集合
    
    mutable Mutex m_mutex;
};

// 输出到控制台的Appender
class StdoutLogAppender: public LogAppender{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(Logger::ptr logger, const LogEvent::ptr &event) override;
};

// 输出到文件的Appender
class FileLogAppender: public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string &filename);
    void log(Logger::ptr logger, const LogEvent::ptr &event) override;
    ~FileLogAppender() override {fclose(m_file);}

    bool reopen();      // 重新打开文件，打开成功返回true
private:
    std::string m_filename;
    std::ofstream m_filestream;
    FILE *m_file;
    char buffer[1024];
};

class LogManager: public Singleton<LogManager>{
public:
    LogManager();
    Logger::ptr getLogger(const std::string &name);
    void init();    // 默认初始化
    int initFromYaml(const std::string& file_name); //通过配置文件初始化

private:
    std::map<std::string, Logger::ptr> m_loggers;        // 日志起容器
    Mutex m_mutex;
};

} // namespace server

#endif //SERVER_LOG_H
