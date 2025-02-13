#include "test_util.h"

#include <async.h>
#include <iomanager_cpp20.h>
#include <time_wheel.h>

#include "log.h"


auto logger = SERVER_LOGGER_SYSTEM;

int test_fque() {
    Timer timer;
    std::vector<int> v(vsize, 0);
    std::vector<int> to;
    server::util::fpipe<int> pipe;

    for (int i = 0; i < vsize; ++i) {
        v[i] = i;
    }

    auto read_pipe = [&to, &pipe]{
        while (to.size() < vsize){
            read(pipe, to);
        }
    };
    auto write_pipe = [&v, &pipe]{
        while (!v.empty()){
            write(pipe, v, writeNums);
        }
        pipe.notify();
    };

    timer.start_count();
    server::EThread::ptr tread = std::make_shared<server::EThread>(read_pipe, "ft1");
    server::EThread::ptr twrite = std::make_shared<server::EThread>(write_pipe, "ft2");
    tread->join();
    twrite->join();
    timer.end_count();
    SERVER_LOG_DEBUG(logger) << "fipe complete num=" << to.size();
    SERVER_LOG_DEBUG(logger) << "fpipe: " << std::to_string(timer.get_duration());
    return timer.get_duration<std::chrono::milliseconds>().count();
}

int test_deque(){
    Timer timer;
    std::vector<int> v(vsize, 1);
    std::vector<int> to;
    server::threadsafe_deque<int> deque;
    auto read_deque = [&deque, &to]{
        while (to.size() < vsize){
            int data;
            deque.pop_front(data);
            to.push_back(data);
        }
    };
    auto write_deque = [&v, &deque]{
        while (!v.empty()){
            int data = v.back();
            v.pop_back();
            deque.push_back(data);
        }
    };
    timer.start_count();
    server::EThread::ptr tread = std::make_shared<server::EThread>(read_deque, "dt1");
    server::EThread::ptr twrite = std::make_shared<server::EThread>(write_deque, "dt2");
    tread->join();
    twrite->join();
    timer.end_count();
    SERVER_LOG_DEBUG(logger) << "deque complete num=" << to.size();
    SERVER_LOG_DEBUG(logger) << "Deque: " << std::to_string(timer.get_duration());
    return timer.get_duration<std::chrono::milliseconds>().count();
}

void test_que(){
    int sum = 0, n = 100;
    for (int i[[maybe_unused]] : range(n)) {
        int res = test_fque();
        // int res = test_deque();
        SERVER_LOG_DEBUG(logger) << "res: " << std::to_string(res) << "ms";
        sum += res;
    }
    SERVER_LOG_DEBUG(logger) << "average res: " << sum / n << "ms";
}

auto test_timewheel_func = [](){
    SERVER_LOG_DEBUG(logger) << "time func";
};

void tick(server::util::TimeWheel::ptr tw) {
    SERVER_LOG_DEBUG(logger) << "tick";
    tw->tick();
}

int test_timewheel() {
    auto io = server::IOManager_::GetInstance(5);
    // server::util::TimeWheel::ptr timewheel = std::make_shared<server::util::TimeWheel>();
    // timewheel->AddEvent(
    //     std::make_shared<server::FuncFiber>(test_timewheel_func),
    //     500, 1, 0, 0
    //     );
    // io->addTimer(timewheel->GetStepMs(), true, std::bind_front(::tick, timewheel));
    io->addTimer(4000, true,
        []{
            SERVER_LOG_DEBUG(logger) << "test_timewheel";
        }
        );
    io->start();
    io->wait();
    return 0;
}
