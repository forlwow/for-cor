#ifndef TIMER_H
#define TIMER_H

#include <chrono>

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

#endif 
