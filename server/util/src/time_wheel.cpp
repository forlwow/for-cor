#include "time_wheel.h"


namespace server::util{

TimeWheel::TimeWheel(uint16_t step_ms, uint64_t max_ms) {
}

void TimeWheel::AddEvent(const TimeEvent::ptr& event, bool circle, uint16_t ms, uint8_t s, uint8_t min,
    uint8_t hour) {

}

bool TimeWheel::CancelEvent(const TimeEvent::ptr& event) {
  return false;
}

void TimeWheel::tick() {
}

}