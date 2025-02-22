# 日志测试
## 输出测试

**测试方法**
- 创建三个输出器
  - 系统(system)
    - 输出：标准输出
    - 级别：DEBUG
    - 格式：2025-02-04 18:49:58  1450220  Main Thread    [FATAL]  [system]:18
  - 文件(file)
    - 输出：文件(log.txt)
    - 级别：INFO
    - 格式：2025-02-04 19:19:29  1450618  Main Thread    0    [FATAL]  [file]  /home/worker/webserver/test/1.0.0/test_log.cpp:18
  - 标准输出(std)
    - 输出：控制台
    - 级别：WARN
    - 格式：同文件输出
- 额外输入格式化字符，观察是否输出错误
- 依次输出
- 配置文件：log.yml

**测试代码**
```c++
auto system_logger = SERVER_LOGGER_SYSTEM;
auto file_logger = SERVER_LOGGER("file");
auto std_logger = SERVER_LOGGER("std");
std::vector<server::Logger::ptr> loggers = {system_logger, file_logger, std_logger};

const char msg[] = "test msg %s %f %l";

void test_output(){
    for(auto log: loggers){
        SERVER_LOG_DEBUG(log) << log->getName();
        SERVER_LOG_INFO(log) << log->getName();
        SERVER_LOG_WARN(log) << log->getName();
        SERVER_LOG_ERROR(log) << log->getName();
        SERVER_LOG_FATAL(log) << log->getName();
    }
}
```

**测试结果**

控制台输出正确
```bash
2025-02-04 19:23:27  1450816  Main Thread    [DEBUG]  [system]:14  system
2025-02-04 19:23:27  1450816  Main Thread    [INFO ]  [system]:15  system
2025-02-04 19:23:27  1450816  Main Thread    [WARN ]  [system]:16  system
2025-02-04 19:23:27  1450816  Main Thread    [ERROR]  [system]:17  system
2025-02-04 19:23:27  1450816  Main Thread    [FATAL]  [system]:18  system
2025-02-04 19:23:27  1450816  Main Thread    0    [WARN ]  [std]  /home/worker/webserver/test/1.0.0/test_log.cpp:16  std
2025-02-04 19:23:27  1450816  Main Thread    0    [ERROR]  [std]  /home/worker/webserver/test/1.0.0/test_log.cpp:17  std
2025-02-04 19:23:27  1450816  Main Thread    0    [FATAL]  [std]  /home/worker/webserver/test/1.0.0/test_log.cpp:18  std
```

文件输出正确
```txt
2025-02-04 19:23:27  1450816  Main Thread    0    [INFO ]  [file]  /home/worker/webserver/test/1.0.0/test_log.cpp:15  file
2025-02-04 19:23:27  1450816  Main Thread    0    [WARN ]  [file]  /home/worker/webserver/test/1.0.0/test_log.cpp:16  file
2025-02-04 19:23:27  1450816  Main Thread    0    [ERROR]  [file]  /home/worker/webserver/test/1.0.0/test_log.cpp:17  file
2025-02-04 19:23:27  1450816  Main Thread    0    [FATAL]  [file]  /home/worker/webserver/test/1.0.0/test_log.cpp:18  file
```

## 性能测试
由于输出到终端的性能瓶颈在终端上，因此只测试文件输出

_还未加入添加多线程与异步功能，因此是单线程 同步输出_

**编译参数**
> CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -w -g2 -ggdb -ldl"

- 计时器：Timer(utils库中，chrono实现)
- 输出数量: 1e6条数据
- 第一次输出内容：
  - > %d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
  - > 2025-02-04 19:51:37  1453358  Main Thread    0    [INFO ]  [file]  /home/worker/webserver/test/1.0.0/test_log.cpp:28
  - 全部可格式化的数据
  - 无额外输入
- 第二次输出内容：
  - > %d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T%n
  - > 2025-02-04 20:12:07  1454321  Main Thread    0 
  - 部分可格式化的数据
  - 无额外输入
- 第三次输出内容：
  - > %T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
  - > 1454529  Main Thread    0    [INFO ]  [file]  /home/worker/webserver/test/1.0.0/test_log.cpp:28
  - 除时间外的全部可格式化的数据
  - 无额外输入

**测试结果**
_测试10次取平均_
- 第一次测试：
  - 输出总耗时：6315ms
  - 单条平均耗时：6.315us
  - 文件大小：114M
- 第二次测试：
  - 输出总耗时：4506ms
  - 单条平均耗时：4.506us
  - 文件大小：49M
- 第三次测试：
  - 输出总耗时：3558ms
  - 单条平均耗时：3.558us
  - 文件大小：96M


测试代码
```c++
void test_performance(){
    Timer timer;
    timer.start_count();
    for(int i = 0; i < 1e6; ++i){
        SERVER_LOG_INFO(file_logger);
    }
    timer.end_count();
    std::cout << std::to_string(timer.get_duration()) << std::endl;
}
```
测试输出(例)
> 6315ms

**性能测试结论**

时间格式化占用了近一半耗时，可以优化。其他部分符合预期。


## 异步日志

### 简介

> 异步日志使用线程池与时间轮定时器驱动，输出日志语句将日志推入任务队列，由任务线程进行输出
> 它由两个线程组成，一个负责输出日志，另一个负责驱动时间轮定时器。
> 引入时间轮定时器解决了时间格式化的性能问题，它定时格式化时间，这样输出时只需要直接输出格式化后的时间字符串即可。

### 性能测试

- 计时器：Timer(utils库中，chrono实现)
- 输出数量: 1e6条数据
- 测试10次取平均
- 第一次输出内容：
  - > %d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
  - > 2025-02-04 19:51:37  1453358  Main Thread    0    [INFO ]  [file]  /home/worker/webserver/test/1.0.0/test_log.cpp:28
  - 全部可格式化的数据
  - 无额外输入

### 测试结果
- 平均耗时：4613ms
- 单条输出耗时：4.613us
- 单条插入平均耗时：898ns
- 文件大小: 
  - 总大小：1.2g
  - 单次大小：114M

**测试代码**
```c++
int test_async_log_performance_() {
    auto as = server::log::AsyncLogPool::GetInstance();
    Timer timer;
    timer.start_count();
    for (int i = 0; i < 1e6; ++i) {
        SERVER_LOG_INFO(file_logger);
    }
    as->wait_empty();
    timer.end_count();
    std::cout << std::to_string(timer.get_duration()) << std::endl;
    return timer.get_duration().count();
}

void test_async_log_performance() {
    const int times = 10;
    int sum = 0;
    for (int i : range(times)) {
        sum += test_async_log_performance_();
    }
    std::cout << sum/times << std::endl;
}
```