#include "time_wheel.h"
#include "log.h"

namespace server::util{

auto logger = SERVER_LOGGER_SYSTEM;

TimeWheel::TimeWheel(uint16_t step_ms, uint64_t max_ms)
    : m_step_ms_(step_ms), m_cur_timepos_{0, 0, 0, 0}
{
    if (max_ms > kTimeWheelMaxMs) {
        SERVER_LOG_ERROR(logger) << "TimeWheel init error: max_ms" << "max_ms = " << max_ms;
        max_ms = kTimeWheelMaxMs;
    }
    // 毫秒的槽数量为 1000毫秒 / tick间隔
    m_slot_ms_ = std::vector<TimeEventList_t::ptr>(1000 / m_step_ms_);
    max_ms /= kMsPerSec;    // 换成秒
    if (!max_ms) return;
    // 大于0初始化秒的时间槽
    m_slot_s_ = std::vector<TimeEventList_t::ptr>(kSecsPerMin);
    max_ms /= kSecsPerMin;  // 换成分钟
    if (!max_ms) return;
    m_slot_min_ = std::vector<TimeEventList_t::ptr>(kMinsPerHour);
    max_ms /= kMinsPerHour; // 换成小时
    if (!max_ms) return;
    m_slot_hour_ = std::vector<TimeEventList_t::ptr>(kHoursPerDay);
}

void TimeWheel::AddEvent(Fiber::ptr event, bool circle, uint16_t ms, uint8_t s, uint8_t min,
    uint8_t hour, bool cancel) {
    TimePos_t tstep = {ms, s, min, hour};
    // 不能超 不能全为0
    if (ms >= kMsPerSec || s >= kSecsPerMin || min >= kMinsPerHour || hour >= kHoursPerDay || !(ms | s | min | hour)) {
        SERVER_LOG_ERROR(logger) << "TimeWheel AddEvent failed error: time overflow "
        << " ms=" << ms << " s=" << s << " min=" << min << " hour=" << hour;
        return ;
    }
    TimeEvent_t::ptr eve = std::make_shared<TimeEvent_t>(circle, cancel, tstep, {0, 0, 0, 0}, event);

    InsertEvent(eve);
}

void TimeWheel::CancelEvent(const TimeEvent::ptr& event) {
    event->m_cancel = true;
}

void TimeWheel::InsertEvent(TimeEvent::ptr& event) {
    TimePos_t target_time = m_cur_timepos_;
    TimePos_t tmp = event->m_step;
    // 进位
    target_time.m_ms += tmp.m_ms;
    if (target_time.m_ms > kMsPerSec) {
        target_time.m_ms %= kMsPerSec;
        ++target_time.m_s;
    }
    target_time.m_s += tmp.m_s;
    if (target_time.m_s > kSecsPerMin) {
        target_time.m_s %= kSecsPerMin;
        ++target_time.m_min;
    }
    target_time.m_min += tmp.m_min;
    if (target_time.m_min > kMinsPerHour) {
        target_time.m_min %= kMinsPerHour;
        ++target_time.m_hour;
    }
    // hour最大不用进位
    target_time.m_hour += tmp.m_hour;
    if (target_time.m_hour > kHoursPerDay) {
        SERVER_LOG_WARN(logger) << "TimeWheel InsertEvent hour overflow";
        target_time.m_hour %= kHoursPerDay;
    }

    event->m_next_pos = target_time;
    if (target_time.m_hour != tmp.m_hour) {
        m_slot_hour_[target_time.m_hour]->push_back(event);
        return;
    }
    if (target_time.m_min != tmp.m_min) {
        m_slot_min_[target_time.m_min]->push_back(event);
        return;
    }
    if (target_time.m_s != tmp.m_s) {
        m_slot_s_[target_time.m_s]->push_back(event);
        return;
    }
    m_slot_ms_[target_time.m_ms]->push_back(event);
}

void TimeWheel::tick() {
    TimeEventList_t::ptr tmp;
    m_cur_timepos_.m_ms = (++m_cur_timepos_.m_ms) % kMsPerSec;
    handle_events(m_slot_ms_[m_cur_timepos_.m_ms]);
    // 秒
    m_cur_timepos_.m_s = (++m_cur_timepos_.m_s) % kSecsPerMin;
    tmp = m_slot_s_[m_cur_timepos_.m_s];    // 要转移内容的链表
    while (!tmp->empty()) {
        // ms time需要整除步长
        auto time = tmp->m_events.front()->m_next_pos.m_ms;
        m_slot_ms_[time / m_step_ms_]->splice_front(tmp);
    }
    // 分
    m_cur_timepos_.m_min = (++m_cur_timepos_.m_min) % kMinsPerHour;
    tmp = m_slot_min_[m_cur_timepos_.m_min];    // 要转移内容的链表
    while (!tmp->empty()) {
        auto time = tmp->m_events.front()->m_next_pos.m_s;
        m_slot_s_[time]->splice_front(tmp);
    }
    // 时
    m_cur_timepos_.m_hour = (++m_cur_timepos_.m_hour) % kHoursPerDay;
    tmp = m_slot_hour_[m_cur_timepos_.m_hour];    // 要转移内容的链表
    while (!tmp->empty()) {
        auto time = tmp->m_events.front()->m_next_pos.m_hour;
        m_slot_min_[time]->splice_front(tmp);
    }
}

void TimeWheel::handle_events(TimeEventList_t::ptr events) {
    while (!events->m_events.empty()) {
        TimeEvent_t::ptr event = events->m_events.front();
        events->m_events.pop_front();
        if (event->m_cancel) continue;
        m_cb(event->m_cb);
        if (event->m_circle) InsertEvent(event);
    }
}


}