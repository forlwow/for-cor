//
// Created by lwow on 2024/1/20.
//

#include "pageCache.h"


PageCache PageCache::_sInst;

// 首先会检查第k个桶里面是否有span，如果有就直接返回；
// 如果没有，则检查后面的桶里面是否有更大的span，
// 如果有就可以将它进行切分，切出一个k页的span返回，
// 剩下的页数的span放到对应的桶里；
// 如果后面的桶里也没有span，就去系统堆申请一个大小为128页的span，
// 并把它放到对应的桶里。然后再递归调用自己，直到获取到一个k页的span为止。
Span* PageCache::NewSpan(size_t k) {
    assert(k > 0);
    // 大于128 page的直接向堆申请，这里的128页相当于128*8*1024 = 1M
    if(k > NPAGES - 1){
        void* ptr = SystemAlloc(k);
        Span* span = _spanPool.New();
        // 页号：地址右移PAGE_SHIFT获得
        span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
        span->_n = k;

        _idSpanMap.set(span->_pageId, span);

        return span;
    }
    // 先检查第k个桶里面有没有span
    if(!_spanLists[k].Empty()){
        Span* kspan = _spanLists[k].PopFront();
        // 建立id和span的映射，方便central cache回收小块内存时，查找对应的span
        for(PAGE_ID i = 0; i < kspan->_n; ++i){
            _idSpanMap.set(kspan->_pageId + i, kspan);
        }
        return kspan;
    }
    // 检查一下后面的桶里面有没有span，如果有可以把他它进行切分
    for(size_t i = k + 1; i < NPAGES; ++i){
        if(!_spanLists[i].Empty()){
            Span* nSpan = _spanLists[i].PopFront();
            Span* kSpan = _spanPool.New();
            // 在nSpan的头部切一个k页下来
            // k页span返回
            // nSpan再挂到对应映射的位置
            kSpan->_pageId = nSpan->_pageId;
            kSpan->_n = k;

            nSpan->_pageId += k;
            nSpan->_n -= k;

//            // 被切分掉的另一块放入对应哈希桶
//            _spanLists[nSpan->_n].PushFront(nSpan);
//
//            _idSpanMap.set(nSpan->_pageId, nSpan);
//            _idSpanMap.set(nSpan->_pageId + nSpan->_n - 1, nSpan);
            // 由于程序需要k大小内存 以后可能还会需要相同大小
            // 因此将余下的内存直接切成
            // 大小为k页的span共(n % k)-1个和(n - (n % k)-1)k的1个span
            int times = (nSpan->_n) % k - 1;
            for(int i = 0; i < times; ++i){
                Span* tmpSpan = _spanPool.New();
                tmpSpan->_pageId = nSpan->_pageId;
                tmpSpan->_n = k;

                nSpan->_pageId -= k;
                nSpan->_pageId += k;
                _idSpanMap.set(tmpSpan->_pageId, tmpSpan);
                _idSpanMap.set(tmpSpan->_pageId + k - 1, tmpSpan);
            }
            _idSpanMap.set(nSpan->_pageId, nSpan);
            _idSpanMap.set(nSpan->_pageId + nSpan->_n - 1, nSpan);

            // 建立id和span的映射，方便central cache回收小块内存时，查找对应的span
            for(PAGE_ID i = 0; i < kSpan->_n; ++i){
                _idSpanMap.set(kSpan->_pageId + i, kSpan);
            }
            return kSpan;
        }
    }
    // 走到这个位置就说明后面没有大页的span了
    // 这时就去找堆要一个128页的span
//    Span* bigSpan = new Span;
    Span* bigSpan = _spanPool.New();
    void* ptr = SystemAlloc(NPAGES - 1);
    // 通过将 ptr 地址强制转换为 PAGE_ID 类型，再使用二进制位运算符 >> 将指针的地址右移 PAGE_SHIFT 位
    // 最终得到的结果就是这个指针所在的“页的编号”
    bigSpan->_pageId = reinterpret_cast<PAGE_ID>(ptr) >> PAGE_SHIFT;
    assert(bigSpan->_pageId);
    bigSpan->_n = NPAGES - 1;

    _spanLists[bigSpan->_n].PushFront(bigSpan);
    return NewSpan(k);
}

// 获取从对象到Span的映射
Span* PageCache::MapObjectToSpan(void* obj){
    PAGE_ID id = (PAGE_ID)obj >> PAGE_SHIFT;
    auto ret = (Span*)_idSpanMap.get(id);
    assert(ret != nullptr);
    return ret;
}

// 释放空间span货到PageCache，合并相邻span
void PageCache::ReleaseSpanToPageCache(Span* span){
    // 大于128 page的直接还给堆，这里的128页相当于128*8*1024 = 1M
    if(span->_n > NPAGES - 1){
        void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
        SystemDealloc(ptr);
        _spanPool.Delete(span);
        return ;
    }
    // 向前合并
    while (1){
        PAGE_ID prevId = span->_pageId - 1;

        auto it = reinterpret_cast<Span*>(_idSpanMap.get(prevId));
        if(it == nullptr)
            break;
        Span* prevSpan = it;
        // 前面相邻页的span在使用，不合并了
        if(prevSpan->_isUse)
            break;
        // 合并出超过128页的span没办法管理，不合并了
        if(prevSpan->_n + span->_n > NPAGES - 1)
            break;
        span->_pageId = prevSpan->_pageId;
        span->_n += prevSpan->_n;
        _spanLists[prevSpan->_n].Erase(prevSpan);
        _spanPool.Delete(prevSpan);
    }
    // 向后合并
    while(1){
        PAGE_ID nextId = span->_pageId + span->_n;
        auto it = (Span*)_idSpanMap.get(nextId);
        if(it == nullptr)
            break;

        Span* nextSpan = it;
        if(nextSpan->_isUse)
            break;
        if(nextSpan->_n + span->_n > NPAGES - 1)
            break;
        span->_n += nextSpan->_n;

        _spanLists[nextSpan->_n].Erase(nextSpan);
        _spanPool.Delete(nextSpan);
    }
    span->_isUse = false;
    // 将合并完的span挂到页缓存的对应的哈希桶里面。
    _spanLists[span->_n].PushFront(span);

    _idSpanMap.set(span->_pageId, span);
    _idSpanMap.set(span->_pageId + span->_n - 1, span);
}
