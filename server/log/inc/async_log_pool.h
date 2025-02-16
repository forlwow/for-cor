//
// Created by worker on 25-2-15.
//

#ifndef SERVER_ASYNC_LOG_POOL_H
#define SERVER_ASYNC_LOG_POOL_H

#include "scheduler_cpp20.h"
#include <chrono>
#include <atomic>
#include <range.h>

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


class AsyncLogPool:public Scheduler_ {
    typedef std::shared_ptr<AsyncLogPool> ptr;
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
public:
    AsyncLogPool(int max_threads = 2);
    virtual ~AsyncLogPool() = default;

    // 与系统时间同步
    void SyncTime();

    // 更新时间字符串缓冲区增加一秒，只允许单线程进入
    void TickTime();

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
    std::atomic_bool m_buf_flag = false;
    static constexpr uint8_t m_buf_size = 15;   // 时间缓冲长度
    char m_time_buf[2][m_buf_size];

};
}

#endif
