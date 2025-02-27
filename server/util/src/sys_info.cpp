//
// Created by worker on 25-2-23.
//
#include "sys_info.h"

#include <chrono>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <thread>

namespace server::util
{

// 输入两个CPUINFO
double ComCpuUse_(const CPU_INFO &fir, const CPU_INFO &sec) {
    unsigned long f, s;
    f = fir.user + fir.nice + fir.system + fir.idle + fir.lowait + fir.irq + fir.softirq;
    s = sec.user + sec.nice + sec.system + sec.idle + sec.lowait + sec.irq + sec.softirq;
    double sum = s - f;
    double idle = sec.idle - fir.idle;
    return (sum - idle) / sum;
}

bool getCpuUse_(CPU_INFO &cinfo) {
    char buf[256];
    FILE *fd = fopen("/proc/stat", "r");
    if (fd == nullptr) {
        return false;
    }
    fgets(buf, sizeof(buf), fd);
    if (strstr(buf, "cpu") != nullptr) {
        sscanf(buf, "%s %u %u %u %u %u %u %u",
            &cinfo.name, &cinfo.user, &cinfo.nice, &cinfo.system,
            &cinfo.idle, &cinfo.lowait, &cinfo.irq, &cinfo.softirq
            );
        fclose(fd);
        return true;
    }
    fclose(fd);
    return false;
}

double getCpuUse() {
    CPU_INFO f, s;
    if (!getCpuUse_(f))
        return -1;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (!getCpuUse_(s))
        return -1;
    return ComCpuUse_(f, s);
}


}
