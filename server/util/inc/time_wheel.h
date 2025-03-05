//
// Created by worker on 25-2-11.
//

#ifndef SERVER_TIME_WHEEL_H
#define SERVER_TIME_WHEEL_H

#include <list>

#include "fiber.h"

namespace server::util {

typedef struct TimePos {
    uint16_t m_ms;
    uint8_t m_s;
    uint8_t m_min;
    uint8_t m_hour;
} TimePos_t;

typedef struct TimeEvent {
    typedef std::shared_ptr<TimeEvent> ptr;

    bool m_circle;          // 是否循环执行
    bool m_cancel = false;  // 是否被取消
    TimePos_t m_step;       // 间隔
    TimePos_t m_next_pos;   // 下一个触发位置
    Fiber::ptr m_cb;
}TimeEvent_t;

// 事件链表 使用头结点
typedef struct TimeEventList {
    typedef std::shared_ptr<TimeEventList> ptr;
    TimeEventList()=default;
    ~TimeEventList()=default;

    // 从目标list中转移尾部元素到自身尾部
    void splice_back(TimeEventList &list) {
        m_events.push_back(list.m_events.back());
        list.m_events.pop_back();
    }
    // 从目标list中转移尾部元素到自身尾部
    void splice_back(TimeEventList *list){
        splice_back(*list);
    }
    TimeEvent_t::ptr pop_back() {
        auto res = m_events.back();
        m_events.pop_back();
        return res;
    }

    bool empty() const { return m_events.empty(); }

    void push_back(const TimeEvent::ptr &event) {
        m_events.push_back(event);
    }

    std::vector<TimeEvent_t::ptr> m_events;
}TimeEventList_t;

constexpr uint16_t kMsPerSec = 1000;
constexpr uint8_t kSecsPerMin = 60;
constexpr uint8_t kMinsPerHour = 60;
constexpr uint8_t kHoursPerDay = 24;

// 默认间隔
constexpr uint16_t kTimeWheelTickMs = 50;
// 最大定时
constexpr uint64_t kTimeWheelMaxMs = kMsPerSec * kSecsPerMin * kMinsPerHour * kHoursPerDay;

class TimeWheel {
public:
    typedef std::shared_ptr<TimeWheel> ptr;
    // typedef void(*func_callback_t)(Fiber_::ptr);
    typedef std::function<void(Fiber_::ptr)> func_callback_t;
    TimeWheel(uint16_t step_ms = kTimeWheelTickMs, uint64_t max_ms = kTimeWheelMaxMs);

    // 添加事件
    void AddEvent(
        Fiber::ptr event,
        uint16_t ms = kTimeWheelTickMs*100, uint8_t s = 0, uint8_t min = 0, uint8_t hour = 0,
        bool circle = true // 是否循环执行
        );
    // 取消事件，在下一轮执行时取消
    // 返回取消是否成功
    static void CancelEvent(const TimeEvent::ptr &event);

    // 将时间时间插入到槽中
    void InsertEvent(TimeEvent::ptr &event);

    uint16_t GetStepMs() const {return m_step_ms_;}

    void SetCallBack(func_callback_t cb) {m_cb = cb;}// 设置处理函数

    void tick();


    void handle_events(TimeEventList_t&);

private:
    func_callback_t m_cb = [](Fiber::ptr cb)->void{cb->swapIn();}; // 回调函数，定时值调用这个函数执行事件的fiber
    const uint16_t m_step_ms_;
    TimePos_t m_cur_timepos_ = {0, 0, 0, 0};

    std::vector<TimeEventList_t> m_slot_ms_;
    std::vector<TimeEventList_t> m_slot_s_;
    std::vector<TimeEventList_t> m_slot_min_;
    std::vector<TimeEventList_t> m_slot_hour_;
};

}


#endif
