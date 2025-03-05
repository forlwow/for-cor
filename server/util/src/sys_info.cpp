//
// Created by worker on 25-2-23.
//
#include "sys_info.h"

#include <chrono>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include <sys/statfs.h>

#include "log.h"

namespace server::util
{
    static auto logger = SERVER_LOGGER_SYSTEM;

    // 输入两个CPUINFO
    double ComCpuUse_(const CPUInfo &fir, const CPUInfo &sec) {
        unsigned long f, s;
        f = fir.user + fir.nice + fir.system + fir.idle + fir.lowait + fir.irq + fir.softirq;
        s = sec.user + sec.nice + sec.system + sec.idle + sec.lowait + sec.irq + sec.softirq;
        double sum = s - f;
        double idle = sec.idle - fir.idle;
        return (sum - idle) / sum;
    }

    bool getCpuUse_(CPUInfo &cinfo) {
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
        CPUInfo f, s;
        if (!getCpuUse_(f))
            return -1;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (!getCpuUse_(s))
            return -1;
        return ComCpuUse_(f, s);
    }

    bool getMemInfo_(MemInfo& mem){
        std::ifstream file("/proc/meminfo");
        if (!file.is_open()){
            SERVER_LOG_ERROR(logger) << "System Info: failed to open /proc/meminfo";
            return false;
        }
        std::string line;
        while (getline(file, line)) {
            std::istringstream ss(line);
            std::string key;
            unsigned long value;
            std::string unit;
            // 循环条件
            uint8_t mask = (1 << 4) - 1;
            uint8_t flag = 0;
            ss >> key >> value >> unit;  // 解析 "MemTotal: 16371556 kB" 这种格式
            if (key == "MemTotal:") {
                mem.total = value;  // 以 KB 为单位
                flag = flag | (1 << 0);
            } else if (key == "MemFree:") {
                mem.free = value;
                flag = flag | (1 << 1);
            } else if (key == "MemAvailable:") {
                mem.available = value;
                flag = flag | (1 << 2);
            } else if (key == "Cached:") {
                mem.cached = value;
                flag = flag | (1 << 3);
            }
            // 全部获取完
            if ((flag & mask) == mask) {
                break;
            }
        }
        file.close();
        return true;
    }

    double getMemUsage() {
        MemInfo m = {};
        if (!getMemInfo_(m)) {
            SERVER_LOG_ERROR(logger) << "System Info: failed to get memory usage";
            return -1;
        }
        if (m.total == 0) {
            SERVER_LOG_ERROR(logger) << "System Info: failed to get memory usage";
            return -1;
        }
        return static_cast<double>(m.total-m.free) / static_cast<double>(m.total);
    }

    bool getDiskInfo_(DiskInfo& d, std::string_view path) {
        struct statfs diskInfo;
        if (statfs(path.data(), &diskInfo) == -1) {
            SERVER_LOG_ERROR(logger) << "System Info: failed to get disk info";
            return false;
        }
        d.total = static_cast<uint64_t>(diskInfo.f_blocks * diskInfo.f_bsize);
        d.free = static_cast<uint64_t>(diskInfo.f_bfree * diskInfo.f_bsize);
        d.available = static_cast<uint64_t>(diskInfo.f_bavail * diskInfo.f_bsize);
        d.used = d.total - d.free;
        return true;
    }

    bool getNetInfo_(NetInfo &net, std::string_view iface) {
        using std::ws;

        std::ifstream file("/proc/net/dev");
        if (!file.is_open()) {
            SERVER_LOG_ERROR(logger) << "System Info: failed to open /proc/net/dev";
            return false;
        }
        std::string line;
        while (getline(file, line)) {
            if (line.find(iface) ==  std::string::npos) {continue;}
            std::istringstream ss(line);
            std::string name;
            ss >> name >> net.rbytes >> ws >> ws >> ws >> ws >> ws >> ws >> ws >> ws >> net.wbytes;
            file.close();
            return true;
        }
        file.close();
        SERVER_LOG_ERROR(logger) << "System Info: failed to get net";
        return false;
    }

    bool getNetRate(NetRateInfo& info, uint32_t step, std::string_view name) {
        NetInfo n1, n2;
        if (!getNetInfo_(n1, name)) {
            SERVER_LOG_ERROR(logger) <<  "System Info: failed to get net1";
            return false;
        }
        std::this_thread::sleep_for(std::chrono::seconds(step));
        if (!getNetInfo_(n2, name)) {
            SERVER_LOG_ERROR(logger) << "System Info: failed to get net2";
            return false;
        }
        info.step = step;
        info.rrate = getNetRxRate(n1, n2, step);
        info.wrate = getNetTxRate(n1, n2, step);
        return true;
    }
}

