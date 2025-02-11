//
// Created by worker on 25-2-11.
//

#ifndef SERVER_TIME_WHEEL_H
#define SERVER_TIME_WHEEL_H

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

    bool m_circle;
    bool m_cancel = false;
    TimePos_t m_pos;
    Fiber::ptr m_cb;
    TimeEvent::ptr next;
}TimeEvent_t;

// 事件链表 使用头结点
typedef struct TimeEventList {
    TimeEvent_t::ptr m_dummy = nullptr;
    TimeEvent_t::ptr m_tail = m_dummy;
    TimeEvent_t::ptr m_cur = m_dummy;

    // 重设当前遍历节点为头结点
    void reset(){m_cur = m_dummy;}
    const TimeEvent::ptr &end() {return m_dummy;}
    // 返回下一个节点,不存在返回dummy节点
    TimeEvent_t::ptr next() {
        if (m_cur->next) {
            m_cur = m_cur->next;
            return m_cur;
        }
        else {
            return m_dummy;
        }
    }
    void push_back(const TimeEvent::ptr &event) {
        m_tail->next = event;
        m_tail = m_tail->next;
    }

}TimeEventList_t;

constexpr uint16_t kTimeWheelTickMs = 50;
constexpr uint64_t kTimeWheelMaxMs = 24 * 60 * 60 * 1000;

class TimeWheel {
public:
    typedef std::shared_ptr<TimeWheel> ptr;
    typedef void(*func_callback_t)(Fiber::ptr);
    TimeWheel(uint16_t step_ms, uint64_t max_ms = kTimeWheelMaxMs);

    // 添加事件
    void AddEvent(
        const TimeEvent::ptr &event,
        bool circle = false,
        uint16_t ms = kTimeWheelTickMs, uint8_t s = 0, uint8_t min = 0, uint8_t hour = 0
        );
    // 取消事件，在下一轮执行时取消
    // 返回取消是否成功
    bool CancelEvent(const TimeEvent::ptr &event);

private:
    void tick();

private:
    func_callback_t m_cb = [](Fiber::ptr cb)->void{cb->swapIn();}; // 回调函数，定时值调用这个函数执行事件的fiber
    uint16_t m_step_ms_;
    TimePos_t m_cur_timepos_;

    std::vector<TimeEventList_t> m_slot_ms_;
    std::vector<TimeEventList_t> m_slot_s_;
    std::vector<TimeEventList_t> m_slot_min_;
    std::vector<TimeEventList_t> m_slot_hour_;
};

}


#endif
