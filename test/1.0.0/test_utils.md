# 工具模块测试
## fpipe测试

**简介**

fpipe是基于ypipe(ZMQ)进行包装的无锁队列，仅支持一读一写。使用conditional_var进行调度。

使用write(value, incomplete)写入，incomplete表示写入是否完成，在写入最后一个数据时需设置为false，否则会导致读线程阻塞。

notify表示唤醒写进程，建议写入完成时调用，否则读线程可能会阻塞。

read_noblock读出数据，失败返回false。
read_block读出数据，失败则阻塞。

**测试方法**
- 创建两个vector(v, to)，和一个pipe，从v中读取数据到pipe中，再从pipe写入to中。
- 使用一读一写两个线程
- 使用Timer计时，从创建线程开始计时，线程结束后计时结束
- 测试100次取平均
- 测试1:
  - 数据类型：int
  - 数据量：$ 5 \times 10 ^6 $ (5e6)
  - 读取方法：read_block
  - 每次写入量：1
  - **结果**：931ms
- 测试2:
  - 数据类型：int
  - 数据量：$ 5 \times 10 ^6 $ (5e6)
  - 读取方法：read_block
  - 每次写入量：10
  - **结果**：893ms
- 测试3:
  - 数据类型：int
  - 数据量：$ 5 \times 10 ^6 $ (5e6)
  - 读取方法：read_block
  - 每次写入量：100
  - **结果**：994ms
- 测试4:
  - 数据类型：int
  - 数据量：$ 5 \times 10 ^6 $ (5e6)
  - 读取方法：*read_noblock*
  - 每次写入量：10
  - **结果**：1393ms


## 线程安全双端队列测试

**简介**

线程安全双端队列，使用锁实现，支持多读多写
方法：push_back(),push_front(),pop_back(),pop_front()

**测试方法**
- 创建两个vector(v, to)，和一个deque，从v中读取数据到deque中，再从deque写入to中。
- 使用一读一写两个线程
- 使用Timer计时，从创建线程开始计时，线程结束后计时结束
- 测试100次取平均
- 测试1:
  - 数据类型：int
  - 数据量：$ 5 \times 10 ^6 $ (5e6)
  - 每次写入量：1
    - write：push_back()
    - read：pop_front()
  - **结果**：3165ms

测试代码
```c++
auto logger = SERVER_LOGGER_SYSTEM;

template<typename T>
bool read(server::util::fpipe<T> &from, std::vector<T> &to){
    T value = T();
    from.read_block(value);
    // if(res) {
    to.push_back(value);
    // }
    return true;
}

template<typename T>
void write(server::util::fpipe<T> &to, std::vector<T> &from, int nums){
    while (from.size() > 1 && --nums){
        T value = from.back();
        from.pop_back();
        to.write(value, true);
    }
    T value = from.back();
    from.pop_back();
    to.write(value, false);
}

const int vsize = 5e6;
const int writeNums = 10;

int test_fque(){
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
    for (int i : range(n)) {
        // int res = test_fque();
           int res = test_deque();
        SERVER_LOG_DEBUG(logger) << "res: " << std::to_string(res) << "ms";
        sum += res;
    }
    SERVER_LOG_DEBUG(logger) << "average res: " << sum / n << "ms";
}

```

