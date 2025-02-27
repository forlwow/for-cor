//
// Created by worker on 25-2-23.
//

#ifndef SERVER_SYS_INFO_H
#define SERVER_SYS_INFO_H

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
struct CPU_INFO{
    char name[8];       //定义一个char类型的数组名name有20个元素
    unsigned int user;
    unsigned int nice;
    unsigned int system;
    unsigned int idle;
    unsigned int lowait;
    unsigned int irq;
    unsigned int softirq;
};

// 获取CPU利用率
double getCpuUse();

}
#endif