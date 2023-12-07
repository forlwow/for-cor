#include "log.h"
#include <map>
#include <functional>

#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <utility>

namespace server {

// FormatItem派生类 负责格式化不同对象
class MessageFormatItem: public LogFormatter::FormatItem{
public:
    MessageFormatItem(const std::string& = "") {}
    void format(std::ostream &os, Logger::ptr, LogLevel::Level level, const LogEvent::ptr &event) override{
        os << event->getSS();
    }
    inline void format(FILE* file, std::shared_ptr<Logger> logger, const LogEvent::ptr &event) override{
        fputs(event->getSS().data(), file);
    }
};

class FileNameFormatItem: public LogFormatter::FormatItem{
public:
    FileNameFormatItem(const std::string& = "") {}
    void format(std::ostream &os, Logger::ptr, LogLevel::Level level, const LogEvent::ptr &event) override{
        os << event->getFile();
    }
    inline void format(FILE* file, std::shared_ptr<Logger> logger, const LogEvent::ptr &event) override{
        fputs(event->getFile(), file);
    }
};

class LineFormatItem: public LogFormatter::FormatItem{
public:
    LineFormatItem(const std::string& = "") {}
    void format(std::ostream &os, Logger::ptr, LogLevel::Level level, const LogEvent::ptr &event) override{
        os << event->getLine();
    }
    inline void format(FILE* file, std::shared_ptr<Logger> logger, const LogEvent::ptr &event) override{
        fprintf(file, "%d", event->getLine());
    }
};

class LevelFormatItem: public LogFormatter::FormatItem{
public:
    LevelFormatItem(const std::string& = "") {}
    void format(std::ostream &os, Logger::ptr, LogLevel::Level level, const LogEvent::ptr &event) override{
        os << LogLevel::ToString(level);
    }
    inline void format(FILE* file, std::shared_ptr<Logger> logger, const LogEvent::ptr &event) override{
        fputs(LogLevel::ToString(event->getLevel()), file);
    }
};

class ElapseFormatItem: public LogFormatter::FormatItem{
public:
    ElapseFormatItem(const std::string& = "") {}
    void format(std::ostream &os, Logger::ptr, LogLevel::Level level, const LogEvent::ptr &event) override{
        os << event->getElapse();
    }
    inline void format(FILE* file, std::shared_ptr<Logger> logger, const LogEvent::ptr &event) override{
        fprintf(file, "%u", event->getElapse());
    }
};

class NameFormatItem: public LogFormatter::FormatItem{
public:
    NameFormatItem(const std::string& = "") {}
    void format(std::ostream &os, Logger::ptr log, LogLevel::Level level, const LogEvent::ptr &event) override{
        os << log->getName();
    }
    inline void format(FILE* file, std::shared_ptr<Logger> logger, const LogEvent::ptr &event) override{
        fputs(logger->getName().data(), file);
    }
};

class ThreadIdFormatItem: public LogFormatter::FormatItem{
public:
    ThreadIdFormatItem(const std::string& = "") {}
    void format(std::ostream &os, Logger::ptr, LogLevel::Level level, const LogEvent::ptr &event) override{
        os << event->getThreadId();
    }
    inline void format(FILE* file, std::shared_ptr<Logger> logger, const LogEvent::ptr &event) override{
        fprintf(file, "%d", event->getThreadId());
    }
};

class FiberIdFormatItem: public LogFormatter::FormatItem{
public:
    FiberIdFormatItem(const std::string& = "") {}
    void format(std::ostream &os, Logger::ptr, LogLevel::Level level, const LogEvent::ptr &event) override{
        os << event->getFiberId();
    }
    inline void format(FILE* file, std::shared_ptr<Logger> logger, const LogEvent::ptr &event) override{
        fprintf(file, "%u", event->getFiberId());
    }
};

class DateTimeFormatItem: public LogFormatter::FormatItem{
public:
    DateTimeFormatItem(const std::string format="%Y:%m:%d %H:%M:%S")
        :m_format(format)
        {}

    void format(std::ostream &os, Logger::ptr, LogLevel::Level level, const LogEvent::ptr &event) override{
        // os << event->getTime();
        os << std::put_time(std::localtime(event->getTimePtr()), m_format.data());
    }
    inline void format(FILE* file, std::shared_ptr<Logger> logger, const LogEvent::ptr &event) override{
        char buffer[30];
        int res = strftime(buffer, 30, m_format.data(), std::localtime(event->getTimePtr()));
        fwrite(buffer, res, 1, file);
    }

private:
    std::string m_format;
};

class StringFormatItem: public LogFormatter::FormatItem{
public:
    StringFormatItem(const std::string &str): FormatItem(str), m_string(str) {}
    void format(std::ostream &os, Logger::ptr, LogLevel::Level level, const LogEvent::ptr &event) override{
        os << m_string;
    }
    inline void format(FILE* file, Logger::ptr, const LogEvent::ptr &event) override{
        fputs(m_string.data(), file);
    }
private:
    std::string m_string;
};

class NewLineFormatItem: public LogFormatter::FormatItem{
public:
    NewLineFormatItem(const std::string& = "") {}
    void format(std::ostream &os, Logger::ptr, LogLevel::Level level, const LogEvent::ptr &event) override{
        os << "\n";
    }
    inline void format(FILE* file, Logger::ptr, const LogEvent::ptr &event) override{
        fputs("\n", file);
    }
};

class TabLineFormatItem: public LogFormatter::FormatItem{
public:
    TabLineFormatItem(const std::string& = "") {} 
    void format(std::ostream &os, Logger::ptr, LogLevel::Level level, const LogEvent::ptr &event) override{
        os << "  ";
    }
    inline void format(FILE* file, Logger::ptr, const LogEvent::ptr &event) override{
        fputs("  ", file);
    }
};

static std::map<std::string, std::function<LogFormatter::FormatItem::ptr(const std::string &str)>> s_format_items = {
    #define XX(str, C) \
        {#str, [](const std::string &fmt){return LogFormatter::FormatItem::ptr(new C(fmt));}}
        XX(m, MessageFormatItem),       // m：消息
        XX(p, LevelFormatItem),         // p：日志级别
        XX(r, ElapseFormatItem),        // r：累计毫秒数
        XX(c, NameFormatItem),          // c：日志名称
        XX(t, ThreadIdFormatItem),      // t：线程id
        XX(n, NewLineFormatItem),       // n：换行
        XX(d, DateTimeFormatItem),      // d：时间
        XX(f, FileNameFormatItem),      // f：文件名
        XX(l, LineFormatItem),          // l：行号
        XX(T, TabLineFormatItem),       // T：Tab
        XX(F, FiberIdFormatItem)        // F：协程id
    #undef XX
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
    ,const char* file, int32_t line, uint32_t elapse
    ,uint32_t thread_id, uint32_t fiber_id, uint64_t time
    ,const char* thread_name)
    :m_file(file), m_line(line), m_elapse(elapse),
    m_thread_id(thread_id), m_fiber_id(fiber_id), m_time(time), m_threadName(thread_name),
    m_logger(logger), m_level(level)
{

}


// LogEventWrap
LogEventWrap::LogEventWrap(LogEvent::ptr e)
    :m_event(e)
{

}

LogEventWrap::~LogEventWrap(){
    m_event->getLogger()->log(m_event);
}


// Logger
Logger::Logger(const std::string &name, const std::string &fmt)
    : m_name(name), 
    m_formatter(new LogFormatter(fmt))
{

}

void Logger::log(const server::LogEvent::ptr &event) {
    for (auto &i: m_appenders){
        i->log(shared_from_this(), event);
    }
}

void Logger::addAppender(server::LogAppender::ptr appender) {
    LockGuard lock(m_mutex);
    if (!appender->getFormatter()){
        appender->setFormatter(m_formatter);
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(server::LogAppender::ptr appender) {
    LockGuard lock(m_mutex);
    for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it){
        if(*it == appender){
            m_appenders.erase(it);
            break;
        }
    }
}

// LogAppender
void LogAppender::setFormatter(LogFormatter::ptr val) {
    LockGuard lock(m_mutex);
    m_formatter = val;
}

LogFormatter::ptr LogAppender::getFormatter() const {
    LockGuard lock(m_mutex);
    return m_formatter;
}

void LogAppender::setLevel(LogLevel::Level level){
    LockGuard lock(m_mutex);
    m_level = level;
}

LogLevel::Level LogAppender::getLevel() const{
    return m_level;
}

// FileLogAppender
FileLogAppender::FileLogAppender(const std::string &filename)
    : m_filename(filename)
{
    m_file = fopen(filename.data(), "w");
}

void FileLogAppender::log(Logger::ptr logger, const LogEvent::ptr &event) {
    if (event->getLevel() < m_level) return;
    LockGuard lock(m_mutex);
    if(!m_formatter->format(m_file, logger, event)){
        reopen();
        std::cout << "error" << std::endl;
    }
}

inline bool FileLogAppender::reopen() {
    // TODO: 频繁判断
    fclose(m_file);
    m_file = fopen("./log.txt", "w");
    if (m_file){
        return true;
    }
    return false;
}

void StdoutLogAppender::log(Logger::ptr logger, const LogEvent::ptr &event) {
    if (event->getLevel() < m_level) return;
    LockGuard lock(m_mutex);
    m_formatter->format(stdout, logger, event);
}


// LogFormatter
LogFormatter::LogFormatter(const std::string &pattern)
    : m_pattern(pattern)
{
    init();
}

void LogFormatter::init() {
    // 格式化 %xxx %xxx{xxx} %%
    // str formatter type
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string nstr;
    for (size_t i = 0; i < m_pattern.size(); ++i){
        if (m_pattern[i] != '%'){
            nstr.append(1 ,m_pattern[i]);
            continue;
        }

        if ((i+1) < m_pattern.size() && m_pattern[i+1] == '%'){
            nstr.append(1, '%');
            continue;
        }

        size_t n = i + 1;
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;

        while (n < m_pattern.size()) {
            if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}')){
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if (fmt_status == 0){
                if (m_pattern[n] == '{'){
                    str = m_pattern.substr(i+1, n-i-1);
                    fmt_status = 1;
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            }
            else if (fmt_status == 1){
                if (m_pattern[n] == '}'){
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if (n == m_pattern.size() && str.empty())
                str = m_pattern.substr(i+1);
        }
        switch (fmt_status){
            case 0:
                if (!nstr.empty()){
                    vec.emplace_back(nstr, "", 0);
                    nstr.clear();
                }
                vec.emplace_back(str, fmt, 1);
                i = n - 1;
                break;
            case 1:
                std::cout << "pattern parse error" << m_pattern << " - " << m_pattern.substr(i) << std::endl;
                vec.emplace_back("<<pattern_error>>", fmt, 0);
                break;
        }
    }
    if (!nstr.empty())
        vec.emplace_back(nstr, "", 0);

    for(auto& i : vec){
        if (std::get<2>(i) == 0){
            m_items.emplace_back(new StringFormatItem(std::get<0>(i)));
        }
        else{
            auto tstr = std::get<0>(i);
            auto it = s_format_items.find(tstr);
            if (it == s_format_items.end()){
                m_items.emplace_back(new StringFormatItem("<<error format %" + tstr + ">>"));
            }
            else{
                m_items.emplace_back(it->second(std::get<1>(i)));
            }
        }
    }
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    std::stringstream ss;
    for (auto &i : m_items){
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event){
    for (auto &i: m_items){
        i->format(ofs, logger, level, event);
    }
    return ofs;
}

std::FILE* LogFormatter::format(FILE *file, std::shared_ptr<Logger> logger, LogEvent::ptr event){
    for (auto &i: m_items){
        i->format(file, logger, event); 
    }
    return file;
}


LogManger::LogManger()
    :m_root(std::make_shared<Logger>())
{
    m_root->addAppender(std::make_shared<StdoutLogAppender>());
    m_loggers[m_root->getName()] = m_root;
    init();
}

Logger::ptr LogManger::getLogger(const std::string &name){
    auto it = m_loggers.find(name);
    if (it != m_loggers.end())
        return it->second;
    return {};
}

void LogManger::init(){

}

}
