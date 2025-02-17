//
// Created by worker on 25-2-15.
//

#ifndef SERVER_ASYNC_LOG_POOL_H
#define SERVER_ASYNC_LOG_POOL_H

#include "scheduler_cpp20.h"
#include <chrono>
#include <atomic>
#include <macro.h>
#include <range.h>
#include <time_wheel.h>

namespace server::log
{
// 月份对应的天数
// 从1开始
const std::unordered_map<int, int> map_month_day_int = {
    {1, 31},
    {2, 28},
    {3, 31},
    {4, 30},
    {5, 31},
    {6, 30},
    {7, 31},
    {8, 31},
    {9, 30},
    {10, 31},
    {11, 30},
    {12, 31},
};

const std::unordered_map<std::string, int> map_month_day_str = {
    {"1", 31},
    {"2", 28},
    {"3", 31},
    {"4", 30},
    {"5", 31},
    {"6", 30},
    {"7", 31},
    {"8", 31},
    {"9", 30},
    {"10", 31},
    {"11", 30},
    {"12", 31},
};

class FiberLog: public Fiber_ {
    // typedef void(*callback_t)();
    typedef std::function<void()> callback_t;
public:
    typedef std::shared_ptr<Fiber_> ptr;
    FiberLog() = default;
    FiberLog(callback_t cb) :m_cb(cb) {}
    ~FiberLog() override = default;

    template<typename Func, typename ...Args>
    requires std::invocable<Func, Args...>
    FiberLog(Func &&func, Args &&...args){
        m_cb = std::bind_front(std::forward<Func>(func), std::forward<Args>(args)...);
    }

    template<typename Func, typename... Args>
    requires std::invocable<Func, Args...>
    static ptr CreatePtr(Func &&func, Args &&...args){
        return ptr(new FiberLog(std::forward<Func>(func), std::forward<Args>(args)...));
    }

    bool swapIn() override {
        if (m_done) {
            return false;
        }
        m_cb();
        m_done = true;
        return true;
    }
    void operator()() {swapIn();}
    bool done() override {return m_done;}
    void setDone(bool done = false) override {m_done = done;};
private:
    bool m_done = false;
    callback_t m_cb;
};

class AsyncLogPool{
    typedef std::shared_ptr<AsyncLogPool> ptr;
    // typedef void(*log_task_type_t)();
    typedef FiberLog::ptr log_task_type_t;
    // timepoint ms s min hour day week month year
    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> ms_tp_t;
    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> secs_tp_t;
    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes> minutes_tp_t;
    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::hours> hours_tp_t;
    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::days> days_tp_t;
    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::weeks> weeks_tp_t;
    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::months> months_tp_t;
    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::years> years_tp_t;
    // time ms s min hour day week month year
    typedef std::chrono::milliseconds ms_t;
    typedef std::chrono::seconds secs_t;
    typedef std::chrono::minutes minutes_t;
    typedef std::chrono::hours hours_t;
    typedef std::chrono::days days_t;
    typedef std::chrono::weeks weeks_t;
    typedef std::chrono::months months_t;
    typedef std::chrono::years years_t;
private:
    AsyncLogPool(int max_threads = 2);
    ~AsyncLogPool();
    AsyncLogPool& operator=(const AsyncLogPool& rhs) = delete;
    AsyncLogPool(const AsyncLogPool& rhs) = delete;
    AsyncLogPool& operator=(AsyncLogPool&& rhs) = delete;
    AsyncLogPool(AsyncLogPool&& rhs) = delete;
public:
    static AsyncLogPool::ptr GetInstance(int max_threads = ASYNC_LOG_THREADS);


    void start();
protected:
    void run();

public:

    // 与系统时间同步
    void SyncTime();

    // 更新时间字符串缓冲区增加一秒，只允许单线程进入
    void TickTime();

    // 调度任务
    bool schedule(log_task_type_t);

    // 等待time秒 -1则无限阻塞
    void wait(int time = -1);
    // 等待time秒后停止
    void wait_stop(int time = -1);

    bool is_stopping() {return m_stopping;}
    bool is_deleting() {return m_delete;}

    std::string_view GetTime(){return {m_time_buf[m_buf_flag]};}

    int GetYear() const {
        uint8_t num = m_buf_flag;
        return GetTimeFrombuf(0, 3, num);
    }
    int GetMonth() const {
        uint8_t num = m_buf_flag;
        return GetTimeFrombuf(4, 5, num);
    }
    int GetDay() const {
        uint8_t num = m_buf_flag;
        return GetTimeFrombuf(6, 7, num);
    }
    int GetHour() const {
        uint8_t num = m_buf_flag;
        return GetTimeFrombuf(8, 9, num);
    }
    int GetMinute() const {
        uint8_t num = m_buf_flag;
        return GetTimeFrombuf(10, 11, num);
    }
    int GetSecond() const {
        uint8_t num = m_buf_flag;
        return GetTimeFrombuf(12, 13, num);
    }

    void SetYear(int16_t year, char *buf) {
        SetTimeBuf(0, 3, year, buf);
    }
    void SetMonth(int8_t month, char *buf) {
        SetTimeBuf(4, 5, month, buf);
    }
    void SetDay(int8_t day, char *buf) {
        SetTimeBuf(6, 7, day, buf);
    }
    void SetHour(int8_t hour, char *buf) {
        SetTimeBuf(8, 9, hour, buf);
    }
    void SetMinute(int8_t minute, char *buf) {
        SetTimeBuf(10, 11, minute, buf);
    }
    void SetSecond(int8_t second, char *buf) {
        SetTimeBuf(12, 13, second, buf);
    }

    struct Deleter {
        void operator()(AsyncLogPool* pool) {
            delete pool;
        }
    };

private:
    // 更新一个时间字符串 传入字符串与其周期修改范围[start, end]
    // cycle: 循环周期, cin增加步长, acc超出周期增加的步长
    // 返回是否超过上限
    inline bool TickTime_(char str[], int start, int end, int cycle, int8_t cin = 0, int8_t acc = 0);

    // 设置时间缓冲的时间 范围[start, end]
    void SetTimeBuf(int8_t start, int8_t end, int16_t time, char *buf) {
        int i = end;
        while (i >= start) {
            buf[i] = time % 10 + '0';
            time /= 10;
            --i;
        }
    }

    int GetTimeFrombuf(int8_t start, int8_t end, int8_t buf_num) const {
        int res = 0;
        while (start <= end) {
            res *= 10;
            res += m_time_buf[buf_num][start] - '0';
            ++start;
        }
        return res;
    }

private:
    // 处理定时器线程
    void Handler();
    void InitTimerfd();
    static constexpr uint8_t m_buf_size = 15;   // 时间缓冲长度

    std::atomic_bool m_buf_flag = false;
    std::atomic_bool m_stopping = true;            // 是否停止
    std::atomic_bool m_delete = false;              // 是否正在析构
    char m_time_buf[2][m_buf_size];
    int m_timerfd;
    int m_thread_count;         // 任务线程数量
    EThread::ptr handler;       // 处理时间的线程
    util::TimeWheel::ptr m_time_wheel;
    std::vector<EThread::ptr> m_threads;
    threadsafe_deque<log_task_type_t> m_tasks;
    std::string m_name;

};

}

#endif
