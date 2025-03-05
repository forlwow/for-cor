//
// Created by lwow on 2024/1/20.
//

#ifndef TEST_CENTRALCACHE_H
#define TEST_CENTRALCACHE_H

#include "common.h"

// CentralCache框架

class CentralCache{
public:
    static CentralCache* GetInstance(){
        return &_sInst;
    }
    // 获取一个非空Span
    Span* GetOneSpan(SpanList& list, size_t byte_size);
    // 从中心缓存获取一定数量的对象给ThreadCache缓存
    size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);
    // 将一定数量的对象释放给中心缓存的span
    void ReleaseListToSpans(void* start, size_t byte_size);
    CentralCache(const CentralCache&)=delete;
private:
    CentralCache()= default;
private:
    // 一个静态的变量 _sInst，该变量保存着 CentralCache 类的唯一实例
    static CentralCache _sInst;
    SpanList _spanLists[NFREELIST];
};

#endif //TEST_CENTRALCACHE_H
