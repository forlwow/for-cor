//
// Created by lwow on 2024/1/20.
//

#include "threadCache.h"
#include "centralCache.h"
#include <algorithm>

#define min(a, b) (a < b ? a : b)

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size) {
    // 慢开始反馈调节算法
    // 1、最开始不会一次向central cache一次批量要太多，因为要太多了可能用不完
    // 2、如果你不要这个size大小内存需求，那么batchNum就会不断增长，直到上限
    // 3、size越大，一次向central cache要的batchNum就越小
    // 4、size越小，一次向central cache要的batchNum就越大
    size_t batchNum = min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));
    if(_freeLists[index].MaxSize() == batchNum){
        _freeLists[index].MaxSize() += 1;
    }
    void *start = nullptr;
    void *end = nullptr;
    size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
    // 至少获得一块
    assert(actualNum > 0);
    if(actualNum == 1){
        assert(start == end);
        return start;
    }
    else{
        // 除了起始地址外的其他内存块插入当前线程的缓存的自由链表中
        _freeLists[index].PushRange(NextObj(start), end, actualNum - 1);
        return start;
    }
}

void* ThreadCache::Allocate(size_t size) {
    assert(size <= MAX_BYTES);
    // 计算出内存块的对齐大小 alignSize 和内存块所在的自由链表的下标 index
    size_t alignSize = SizeClass::RoundUp(size);
    size_t index = SizeClass::Index(size);

    // _freeLists[index] 如果不为空，就从 _freeLists[index] 中弹出一个内存块并返回。
    if (!_freeLists[index].Empty()){
        return _freeLists[index].Pop();
    }
    else{ // 如果为空，就调用 FetchFromCentralCache 函数从中央缓存获取内存块；
        return FetchFromCentralCache(index, alignSize);
    }
}

void ThreadCache::Deallocate(void *ptr, size_t size) {
    assert(ptr);
    assert(size <= MAX_BYTES);

    size_t index = SizeClass::Index(size);
    _freeLists[index].Push(ptr);
    // 当链表长度大于一次批量申请的内存时，就开始还一段list给CentralCache
    if (_freeLists[index].Size() >= _freeLists[index].MaxSize()){
        ListTooLong(_freeLists[index], size);
    }
}

void ThreadCache::ListTooLong(FreeList &list, size_t size) {
    void* start = nullptr;
    void* end = nullptr;
    list.PopRange(start, end, list.MaxSize());
    CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}
