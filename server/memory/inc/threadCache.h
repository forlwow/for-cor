//
// Created by lwow on 2024/1/20.
//

#ifndef TEST_THREADCACHE_H
#define TEST_THREADCACHE_H

#include "common.h"

class ThreadCache{
public:
    // 申请内存对象
    void* Allocate(size_t size);
    // 释放内存对象
    void Deallocate(void* ptr, size_t size);

    // 从中心缓存获取对象
    void* FetchFromCentralCache(size_t index, size_t size);

    // 释放内存时，如果自由链表过长，回收内存到CentralCache中心缓存
    void ListTooLong(FreeList& list, size_t size);
private:
    // 哈希桶，每个桶中是自由链表对象
    FreeList _freeLists[NFREELIST];
};

static thread_local ThreadCache* t_threadCache = nullptr;

#endif //TEST_THREADCACHE_H
