//
// Created by worker on 25-2-23.
//

#ifndef SERVER_SYS_INFO_H
#define SERVER_SYS_INFO_H
#include <cstdint>
#include <string_view>

namespace server::util {

/*
    user:用户态的CPU时间
    nice：低优先级程序所占用的用户态的cpu时间
    system：处于核心态的运行时间
    idle：CPU空闲的时间（不包含IO等待）
    iowait：等待IO响应的时间
    irq：处理硬件中断的时间
    softirq：处理软中断的时间
    steal:其他系统所花的时间
    guest：运行时间为客户操作系统下的虚拟CPU控制
    guest_nice：低优先级程序所占用的用户态的cpu时间
    注意：以上所说的时间是指从开机后到现在的总时间（即从CPU加电到当前的累计值）。
*/
struct CPUInfo{
    char name[8];
    unsigned int user;
    unsigned int nice;
    unsigned int system;
    unsigned int idle;
    unsigned int lowait;
    unsigned int irq;
    unsigned int softirq;
};

// 单位KB
struct MemInfo {
    uint32_t total;
    uint32_t free;
    uint32_t available;
    uint32_t cached;
};

// 单位B
struct DiskInfo {
    uint64_t total;
    uint64_t free;
    uint64_t available;
    uint64_t used;
};

struct NetInfo {
    uint64_t rbytes;    // 接收字节数
    uint64_t wbytes;    // 发送字节数
};

struct NetRateInfo {
    uint32_t step;      // 采样间隔 单位s
    uint64_t rrate;     // 接收速率 B/s
    uint64_t wrate;
};

// 获取CPU利用率
double getCpuUse();


bool getMemInfo_(MemInfo&);
double getMemUsage();

bool getDiskInfo_(DiskInfo&, std::string_view);

bool getNetInfo_(NetInfo&, std::string_view);
// 计算发送速率 单位B step为采样间隔 单位s
// fir应早于sec
inline double getNetRxRate(const NetInfo& fir, const NetInfo& sec, uint32_t step) {
    return (sec.rbytes - fir.rbytes) / step;
}
// 计算接收速率
inline double getNetTxRate(const NetInfo& fir, const NetInfo& sec, uint32_t step) {
    return (sec.wbytes - fir.wbytes) / step;
}

bool getNetRate(NetRateInfo&, uint32_t step = 1, std::string_view name = "eth0");

}
#endif