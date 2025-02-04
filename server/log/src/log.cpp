#include "log.h"
#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/node/parse.h"
#include "yaml-cpp/yaml.h"
#include <iostream>
#include <map>
#include <functional>

#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <utility>

namespace server {



const char* TabFormatString = "  ";

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
        os << LogLevel::LevelColor.find(level)->second << LogLevel::ToString(level) << LogLevel::LevelColorEnd;
    }
    inline void format(FILE* file, std::shared_ptr<Logger> logger, const LogEvent::ptr &event) override{
        // fputs(LogLevel::ToString(event->getLevel()), file);
        if(file == stdout) 
            fprintf(file, "%s%-5s%s", LogLevel::LevelColor.find(event->getLevel())->second, LogLevel::ToString(event->getLevel()), LogLevel::LevelColorEnd);
        else
            fprintf(file, "%-5s", LogLevel::ToString(event->getLevel()));
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
        fprintf(file, "%-2d", event->getThreadId());
    }
};

class ThreadNameFormatItem: public LogFormatter::FormatItem{
public:
    ThreadNameFormatItem(const std::string& = "") {}
    void format(std::ostream &os, Logger::ptr, LogLevel::Level level, const LogEvent::ptr &event) override{
        os << event->getThreadName();
    }
    inline void format(FILE* file, std::shared_ptr<Logger> logger, const LogEvent::ptr &event) override{
        fprintf(file, "%-13s", event->getThreadName());
    }
};

class FiberIdFormatItem: public LogFormatter::FormatItem{
public:
    FiberIdFormatItem(const std::string& = "") {}
    void format(std::ostream &os, Logger::ptr, LogLevel::Level level, const LogEvent::ptr &event) override{
        os << event->getFiberId();
    }
    inline void format(FILE* file, std::shared_ptr<Logger> logger, const LogEvent::ptr &event) override{
        fprintf(file, "%-3u", event->getFiberId());
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
        os << TabFormatString;
    }
    inline void format(FILE* file, Logger::ptr, const LogEvent::ptr &event) override{
        fputs(TabFormatString, file);
    }
};

typedef std::map<std::string, std::function<LogFormatter::FormatItem::ptr(const std::string &str)>> format_map_t;

class SingletonMap{
public:
    static format_map_t& GetInstance()
    {
        static format_map_t instance = { 
    #define XX(str, C) \
        {#str, [](const std::string &fmt){return LogFormatter::FormatItem::ptr(new C(fmt));}}
        XX(m, MessageFormatItem),       // m：消息
        XX(p, LevelFormatItem),         // p：日志级别
        XX(r, ElapseFormatItem),        // r：累计毫秒数
        XX(c, NameFormatItem),          // c：日志名称
        XX(t, ThreadIdFormatItem),      // t：线程id
        XX(N, ThreadNameFormatItem),    // t：线程id
        XX(n, NewLineFormatItem),       // n：换行
        XX(d, DateTimeFormatItem),      // d：时间
        XX(f, FileNameFormatItem),      // f：文件名
        XX(l, LineFormatItem),          // l：行号
        XX(T, TabLineFormatItem),       // T：Tab
        XX(F, FiberIdFormatItem)        // F：协程id
    #undef XX
        };
        return instance;
    }

    SingletonMap(format_map_t&&) = delete;
    SingletonMap(const format_map_t&) = delete;
    void operator= (const format_map_t&) = delete;

protected:
    SingletonMap() = default;
    virtual ~SingletonMap() = default;
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
Logger::Logger(const std::string &name)
    : m_name(name) 
{

}

void Logger::log(const server::LogEvent::ptr &event) {
    for (auto &i: m_appenders){
        i->log(shared_from_this(), event);
    }
}

void Logger::addAppender(server::LogAppender::ptr appender) {
    LockGuard lock(m_mutex);
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
        // 转译 %%
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
            auto s_format_items = SingletonMap::GetInstance();
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


LogManager::LogManager()
{
    if (int res = initFromYaml("log.yml")){
        std::cout << "log file error "<< res << std::endl;
        init();
    }
    else
        std::cout << "log file OK" << std::endl;
}

Logger::ptr LogManager::getLogger(const std::string &name){
    LockGuard lock(m_mutex);
    auto it = m_loggers.find(name);
    if (it != m_loggers.end())
        return it->second;
    Logger::ptr logger(new Logger(name));
    m_loggers[name] = logger;
    return logger;
}

// 通过Log.yaml初始化失败的默认初始化
void LogManager::init(){
    server::Logger::ptr log = std::make_shared<server::Logger>("system");
    server::LogAppender::ptr app(new server::StdoutLogAppender);
    server::LogFormatter::ptr fmt(new server::LogFormatter(""));
    app->setFormatter(fmt);
    app->setLevel(LogLevel::INFO);
    log->addAppender(app);

    m_loggers["system"] = log;
}

int LogManager::initFromYaml(const std::string& file_name){
    try{
        auto s_format_items = SingletonMap::GetInstance();
        auto logs = YAML::LoadFile(file_name); 
        for(auto logitem: logs){
            std::string logname = logitem.first.as<std::string>();
            if (logname.empty())
                return 1;
            Logger::ptr log = std::make_shared<Logger>(logname);
            for(auto appenditem: logitem.second){
                LogAppender::ptr appender;
                std::string mode = appenditem["out"].as<std::string>();
                std::string fmtpat = appenditem["fmt"].as<std::string>();
                std::string level = appenditem["level"].as<std::string>();
                // 只要有一个没有就失败
                if (mode.empty() || fmtpat.empty() || level.empty())
                    return 2;
                // 创建Appender
                // TODO: 添加更多输出
                if(mode == "std"){
                    appender.reset(new StdoutLogAppender);
                }
                else if(mode == "file"){
                    auto outname = appenditem["filename"].as<std::string>();
                    if (outname.empty())
                        return 3;
                    appender.reset(new FileLogAppender(outname));
                }
                else{
                    return 4;
                }
                // 设置等级
                auto lv = LogLevel::FromString(level);
                if (lv == LogLevel::UNKNOW)
                    return 5;
                appender->setLevel(lv);
                // 创建格式化器
                LogFormatter::ptr fmter(new LogFormatter(fmtpat));
                appender->setFormatter(fmter);

                log->addAppender(appender);
            }
            m_loggers[logname] = log;
        }
    }
    catch(YAML::Exception error){
        std::cout << error.msg << std::endl;
        return 6;
    }
    return 0;
}

}
