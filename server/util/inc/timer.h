#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include "ethread.h"

class Timer{
public:
    Timer()
        : start_time(), end_time()
    {}
    ~Timer(){}
    void start_count(){
        start_time = std::chrono::high_resolution_clock::now();
    }
    void end_count(){
        end_time = std::chrono::high_resolution_clock::now();
    }

    template<typename T = std::chrono::milliseconds>
    auto get_duration(){
        return std::chrono::duration_cast<T>(
                end_time - start_time);
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> end_time;

};

namespace server {

class TimerManager;
class Timer: public std::enable_shared_from_this<server::Timer>{
friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;

private:
    Timer(uint64_t ms, std::function<void()> cb, bool circulate = false, 
            TimerManager* manager = nullptr);
};

} // namespace server

#endif 
