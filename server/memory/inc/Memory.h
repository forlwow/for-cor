//
// Created by lwow on 2024/1/19.
// https://zhuanlan.zhihu.com/p/649965031
#ifndef TEST_MEMORY_H
#define TEST_MEMORY_H

#include "threadCache.h"
#include "pageCache.h"
#include "common.h"

static ObjectPool<ThreadCache> tcPool;


// 在多线程环境下申请内存
static void* ConcurrentAlloc(size_t size)
{
    if (size > MAX_BYTES) // 大于256kb的超大内存
    {
        size_t alignSize = SizeClass::RoundUp(size);// size对齐
        size_t kPage = alignSize >> PAGE_SHIFT;// 获取页数

        PageCache::GetInstance()->_pageMtx.lock();
        Span* span = PageCache::GetInstance()->NewSpan(kPage);// 找页缓存要kpage页span
        span->_objSize = size;// 会有一点内碎片，标记成alignSize也行
        PageCache::GetInstance()->_pageMtx.unlock();

        void* ptr = (void*)(span->_pageId << PAGE_SHIFT);// 获取对应地址
        return ptr;
    }
    else
    {
        // 检查当前线程是否有对应的ThreadCache对象，如果没有，就通过TLS 每个线程无锁的获取自己的专属的ThreadCache对象
        if (t_threadCache == nullptr)
        {
            //t_threadCache = new ThreadCache;
            t_threadCache = tcPool.New();
        }

        // cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;

        // 调用该线程的ThreadCache对象的Allocate函数申请内存
        return t_threadCache->Allocate(size);
    }
}

// 在多线程环境下释放内存
static void ConcurrentFree(void* ptr)
{
    Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
    size_t size = span->_objSize;
    if (size > MAX_BYTES)
    {
        PageCache::GetInstance()->_pageMtx.lock();
        PageCache::GetInstance()->ReleaseSpanToPageCache(span);
        PageCache::GetInstance()->_pageMtx.unlock();
    }
    else
    {
        assert(t_threadCache);
        // 调用当前线程的ThreadCache对象的Deallocate函数释放内存
        t_threadCache->Deallocate(ptr, size);
    }
}

#endif //TEST_MEMORY_H
