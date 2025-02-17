//
// Created by worker on 25-2-15.
//
#include "async_log_pool.h"
#include "util.h"
#include <cstring>
#include <init_error.h>
#include <iostream>
#include <sys/timerfd.h>

namespace server::log
{

// 北京时区
const std::chrono::time_zone *beijing_tz = std::chrono::locate_zone("Asia/Shanghai");
const auto beijing_td = std::chrono::hours(8);  // 北京时差

const auto local_td = beijing_td;   // 本地时差

AsyncLogPool::AsyncLogPool(int max_)
    : m_time_wheel(new util::TimeWheel(50))
{
    std::cout << "AsyncLogPool::AsyncLogPool()" << std::endl;
    max_ = (max_ < 0 ? std::thread::hardware_concurrency() :
        (max_ < std::thread::hardware_concurrency() ?
            max_ : std::thread::hardware_concurrency()));
    m_thread_count = max_;


    char time_tmp[m_buf_size] = "20191231235958";
    memcpy(m_time_buf[0], time_tmp, sizeof(time_tmp));
    memcpy(m_time_buf[1], time_tmp, sizeof(time_tmp));

    SyncTime();
    m_time_wheel->SetCallBack(std::bind_front(&AsyncLogPool::schedule, this));
    // 同步与增加时间缓冲, 每秒调用一次
    m_time_wheel->AddEvent(
        FuncFiber::CreatePtr(
        [this](){
            static int i = 0;
            // 60秒同步一次时间
            i %= 60;
            if (i) {
                // std::cout << "Tick time now=" << GetTime() << std::endl;
                this->TickTime();
            }
            else {
                // std::cout << "Sync time now=" << GetTime() << std::endl;
                this->SyncTime();
            }
            ++i;
        }),
        0, 1, 0, 0, true
        );
}

AsyncLogPool::~AsyncLogPool() {
    m_delete = true;
    m_stopping = true;
    close(m_timerfd);
    while (!m_threads.empty()) {
        m_threads.pop_back();
    }
    while (!m_tasks.empty()) {
        log_task_type_t task;
        m_tasks.pop_front(task);
        task->swapIn();
    }
}

AsyncLogPool::ptr AsyncLogPool::GetInstance(int max_threads) {
    // std::cout << "Async GetInstance" << std::endl;
    static AsyncLogPool::ptr ap;
    static std::once_flag flag;
    std::call_once(flag, [&]{
        ap = std::shared_ptr<AsyncLogPool>(new AsyncLogPool(max_threads), Deleter());
    });
    [[unlikely]]if (ap->is_deleting()) return nullptr;
    return ap;
}

void AsyncLogPool::start() {
    if (!m_stopping) return;
    m_stopping = false;
    for (int i : range(m_thread_count)) {
        m_threads.push_back(
            std::make_shared<EThread>(
                std::bind_front(&AsyncLogPool::run, this),
                m_name + "_" + std::to_string(i)
                ));
    }

    InitTimerfd();
    handler = std::make_shared<EThread>(std::bind_front(&AsyncLogPool::Handler, this), "AsyncLogPoolHandler");
    // 等待线程构造
    for (auto &i : m_threads) {
        while (!i->getStartFlag()){std::this_thread::yield();}
    }
    while (!handler->getStartFlag()){std::this_thread::yield();}
}

void AsyncLogPool::run() {
    while (!m_stopping){
        log_task_type_t ta;
        if(m_tasks.pop_front(ta)){
            ta->swapIn();
        }
        else{
            std::this_thread::yield();
        }
    }
}


void AsyncLogPool::InitTimerfd(){
    m_timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (m_timerfd == -1) {
        std::cerr << "AsyncLogPool timerfd_create failed" << std::endl;
        init_error(ASYNCLOGPOOL_INIT_ERR);
    }
    struct itimerspec timevalue;
    memset(&timevalue, 0, sizeof(timevalue));
    // 首次触发
    timevalue.it_value.tv_sec = 0;
    timevalue.it_value.tv_nsec = 50 * 1000 * 1000;
    // 触发间隔
    timevalue.it_interval.tv_sec = 0;
    timevalue.it_interval.tv_nsec = m_time_wheel->GetStepMs() * 1000 * 1000;
    if (timerfd_settime(m_timerfd, 0, &timevalue, nullptr) == -1) {
        init_error(ASYNCLOGPOOL_TIMERFD_INIT_ERR);
    }
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

bool AsyncLogPool::schedule(log_task_type_t t) {
    if (m_stopping) return false;
    m_tasks.push_back(t);
    return true;
}

void AsyncLogPool::wait(int time) {
    if (time < 0) {
        while (true) std::this_thread::yield();
    }
    std::this_thread::sleep_for(secs_t(time));
}

void AsyncLogPool::wait_stop(int time) {
    wait(time);
    m_stopping = true;
}

void AsyncLogPool::Handler() {
    uint16_t i = 0;
    while (!m_stopping) {
        uint64_t expire;
        ssize_t s = read(m_timerfd, &expire, sizeof(expire));
        // 未到期
        if (s == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // std::cout << "yield" << std::endl;
                std::this_thread::yield();
            }
            else {
                exit((-1));
            }
        }
        // 到期
        else if (s == sizeof(expire)) {
            // std::cout << "Tick " << i++ << std::endl;
            m_time_wheel->tick();
        }
    }
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
