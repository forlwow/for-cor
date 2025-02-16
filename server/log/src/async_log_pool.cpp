//
// Created by worker on 25-2-15.
//
#include "async_log_pool.h"
#include "util.h"
#include <cstring>

namespace server::log
{

// 北京时区
const std::chrono::time_zone *beijing_tz = std::chrono::locate_zone("Asia/Shanghai");
const auto beijing_td = std::chrono::hours(8);  // 北京时差

const auto local_td = beijing_td;   // 本地时差

AsyncLogPool::AsyncLogPool(int max_threads)
    : Scheduler_(max_threads, "LogScheduler")
{
    char time_tmp[m_buf_size] = "20191231235958";
    memcpy(m_time_buf[0], time_tmp, sizeof(time_tmp));
    memcpy(m_time_buf[1], time_tmp, sizeof(time_tmp));
}

void AsyncLogPool::SyncTime() {
    auto now = std::chrono::system_clock::now() + local_td;
    auto today = std::chrono::floor<days_t>(now);
    std::chrono::year_month_day ymd(today);
    int year = static_cast<int>(ymd.year());
    unsigned month = static_cast<unsigned>(ymd.month());
    unsigned day = static_cast<unsigned>(ymd.day());

    auto thms = now - today;
    std::chrono::hh_mm_ss hms{duration_cast<secs_t>(thms)};
    auto hour = hms.hours().count();
    auto minute = hms.minutes().count();
    auto second = hms.seconds().count();

    bool flag = !m_buf_flag;
    char buf[m_buf_size];
    memcpy(buf, m_time_buf[!flag], m_buf_size);
    SetYear(year, buf);
    SetMonth(month, buf);
    SetDay(day, buf);
    SetHour(hour, buf);
    SetMinute(minute, buf);
    SetSecond(second, buf);
    memcpy(m_time_buf[flag], buf, m_buf_size);
    m_buf_flag.store(flag);
}


void AsyncLogPool::TickTime() {
    int num = !m_buf_flag;
    char buf[m_buf_size];
    memcpy(buf, m_time_buf[!num], m_buf_size);
    // 时分秒
    // 秒
    bool res = TickTime_(buf, 12, 13, 60, 1);
    if (res) {
        // 分
        res = TickTime_(buf, 10, 11, 60, res);
        if (res) {
            // 时
            res = TickTime_(buf, 8, 9, 24, res);
            if (res) {
                // 年月日
                // 判断月份中的天数
                int month = GetMonth(); // 当前月份
                int day = month == 2 ? month + IsLeapYear(GetYear()) : map_month_day_int.at(month); // 当前月份的天数
                res = TickTime_(buf, 6, 7, day+1, res, 1);
                if (res) {
                    res = TickTime_(buf, 4, 5, 13, res, 1);
                    if (res) {
                        TickTime_(buf, 2, 3, 10000, res, 1);
                    }
                }
            }
        }
    }
    memcpy(m_time_buf[num], buf, m_buf_size);
    m_buf_flag.store(num);
}

bool AsyncLogPool::TickTime_(char str[], int start, int end, int cycle, int8_t cin, int8_t acc) {
    bool res = false;
    int cur = 0;
    int c = start;
    while (c <= end) {
        cur *= 10;
        cur += str[c] - '0';
        ++c;
    }
    cur += cin;

    if (cur >= cycle) {
        cur += acc;
        res = true;
    }
    cur %= cycle;
    c = end;
    while (start <= c) {
        str[c] = cur % 10 + '0';
        cur /= 10;
        --c;
    }
    return res;
}


}
