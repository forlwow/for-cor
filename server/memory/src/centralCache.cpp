//
// Created by lwow on 2024/1/20.
//

#include "centralCache.h"
#include "pageCache.h"

#include <atomic>

std::atomic<int> at{0};

CentralCache CentralCache::_sInst;

Span* CentralCache::GetOneSpan(SpanList &list, size_t size) {
    // 查看当前的spanlist中是否有还有未分配对象的span
    Span* it = list.Begin();
    while (it != list.End()) {
        if (it->_freeList != nullptr) {
            return it;
        } else {
            it = it->_next;
        }
    }
    ++at;
    list._mtx.unlock();
    PageCache::GetInstance()->_pageMtx.lock();
    Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
    span->_isUse = true;
    span->_objSize = size;
    PageCache::GetInstance()->_pageMtx.unlock();
    // 对获取span进行切分，不需要加锁，因为这时候这个span是当前进程单例创建的，其他线程访问不到这个span
    char* start = reinterpret_cast<char*>(span->_pageId << PAGE_SHIFT);
    size_t bytes = span->_n << PAGE_SHIFT;
    char* end = start + bytes;

    // 把大块内存切成自由链表链接起来
    span->_freeList = start;
    start += size;
    void *tail = span->_freeList;
    while (start < end){
        NextObj(tail) = start;
        tail = NextObj(tail);
        start += size;
    }
    NextObj(tail) = nullptr;

    // 切好span以后，需要把span挂到中心缓存对应的哈希桶里面去的时候，再加锁
    list._mtx.lock();
    list.PushFront(span);

    return span;
}

//从中心缓存获取一定数量的对象给thread cache
//值得注意void *& start 和 void *& end 都是传址的形式传入的参数,也就是所谓的输入输出型参数
//void *& start：输出参数，返回获取到的内存块的起始地址。
//void *& end：输出参数，返回获取到的内存块的结束地址。
//size_t batchNum：输入参数，指定从中心缓存获取的内存块的数量。
//size_t size：输入参数，指定要获取的内存块的大小
size_t CentralCache::FetchRangeObj(void *&start, void *&end, size_t batchNum, size_t size) {
    // 中央缓存CentralCache哈希桶的映射规则和线程缓存ThreadCache哈希桶映射规则一样
    size_t index = SizeClass::Index(size);
    _spanLists[index]._mtx.lock();

    Span* span = GetOneSpan(_spanLists[index] ,size);
    assert(span);                   // 检查span是否为空
    assert(span->_freeList);        // 检查span的自由链表是否为空

    // 从span中获取batchNum个对象
    // 如果不够batchNum个，有多少拿多少
    start = span->_freeList;
    end = start;
    size_t i = 0, actualNum = 1;
    while(i < batchNum - 1 && NextObj(end) != nullptr){
        end = NextObj(end);
        ++i;
        ++actualNum;
    }
    span->_freeList = NextObj(end);         // 更新span的freeList头
    NextObj(end) = nullptr;                 // 置空
    span->_useCount += actualNum;

    _spanLists[index]._mtx.unlock();
    return actualNum;
}

// 将一段线程缓存的自由链表还给中心缓存的span。
void CentralCache::ReleaseListToSpans(void *start, size_t size) {
    size_t index = SizeClass::Index(size);
    _spanLists[index]._mtx.lock();
    while(start){
        void* next = NextObj(start);

        Span* span = PageCache::GetInstance()->MapObjectToSpan(start);
        NextObj(start) = span->_freeList;
        span->_freeList = start;
        --span->_useCount;

        if(span->_useCount == 0){
            _spanLists[index].Erase(span);
            span->_freeList = nullptr;
            span->_next = nullptr;
            span->_prev = nullptr;

            // 释放span给page cache时，span已经从_spanLists[index]删除了，不需要再加桶锁了
            // 这时把桶锁解掉，使用page cache的锁就可以了,方便其他线程申请/释放内存
            _spanLists[index]._mtx.unlock();
            PageCache::GetInstance()->_pageMtx.lock();
            PageCache::GetInstance()->ReleaseSpanToPageCache(span);
            PageCache::GetInstance()->_pageMtx.unlock();

            _spanLists[index]._mtx.lock();
        }
        start = next;
    }
    _spanLists[index]._mtx.unlock();
}
