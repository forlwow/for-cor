//
// Created by lwow on 2024/1/20.
//

#ifndef TEST_PAGECACHE_H
#define TEST_PAGECACHE_H

#include "common.h"
#include "objectPool.h"
#include "unordered_map"
#include "pageTree.h"

// PageCache
class PageCache{
public:
    static PageCache* GetInstance(){
        return &_sInst;
    }

    // 获取从对象到Span的映射
    Span* MapObjectToSpan(void* obj);

    // 释放空间span货到PageCache，合并相邻span
    void ReleaseSpanToPageCache(Span* span);

    // 获取一个k页的span
    Span* NewSpan(size_t k);

    std::mutex _pageMtx;
private:
    SpanList _spanLists[NPAGES];            // PageCache自己的双链表哈希桶，映射方式是按照页数直接映射
    ObjectPool<Span> _spanPool;

//    std::unordered_map<PAGE_ID, Span*> _idSpanMap;
//    TCMalloc_PageMap3<64 - PAGE_SHIFT> _idSpanMap{[](size_t t)->void*{ return malloc(t);}};
    TCMalloc_PageMap3<64 - PAGE_SHIFT> _idSpanMap{[](size_t size){return malloc(size);}};

    PageCache(){};
    PageCache(const PageCache&)=delete;
    static PageCache _sInst;
};


#endif //TEST_PAGECACHE_H
