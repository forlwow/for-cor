//
// Created by lwow on 2024/1/20.
//

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <exception>
#include <cassert>
#include <mutex>

#ifdef _WIN32
//#include <memoryapi.h>
#include <windows.h>
#else
#include <sys/mman.h>
#endif


static const size_t MAX_BYTES = 256 * 1024;     // 最大可申请的大小 256KB
static const size_t NFREELIST = 208;            // 链表最大总数208 线程桶和中心桶的大小
static const size_t NPAGES = 129;               // 页桶数量
static const size_t PAGE_SHIFT = 13;            // 1 << 13对应8KB，即一个页的大小

#ifdef _WIN64
typedef unsigned long long PAGE_ID;
#elif _WIN32
typedef size_t PAGE_ID;
#else
typedef size_t PAGE_ID;
#endif

//既然Pool中的内存没有经过任何封装，没有添加指针，那它是怎么将内存链起来的？
//内存本身天然就是数据的容器, 数据是写在内存上的，换句话说，只要我们能把下一段内存的地址写入当前内存中，就把他们链起来了
static void*& NextObj(void* obj){
    // 将对象内存强转成void**的类型，
    // 那么再对这个二级指针类型解引用就可以取出当前对象的前4个字节（32位系统）或8个字节（64位系统）。
    return *(void**)obj;
}

// 以页(2^13)为单位分配内存
//这里我们为了避免使用malloc及free函数接口去向堆申请和释放内存，因此使用系统调用接口直接向堆申请和释放内存。
//这里的系统调用接口在window下为VirtualAlloc与VirtualFree系统调用接口；在Linux系统下为mmap与munmap，brk与sbrk两对系统调用接口。
inline static void* SystemAlloc(size_t kPage){
#ifdef _WIN32
   void* ptr = VirtualAlloc(nullptr, kPage << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    void* ptr = mmap(0, kPage << PAGE_SHIFT, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    if (ptr == nullptr)
        throw std::bad_alloc();
    return ptr;
}

inline static void SystemDealloc(void* ptr){
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    munmap(ptr, 0);
#endif
}

// 以字节为单位分配内存
inline static void* SystemAllocBytes(size_t size){
#ifdef _WIN32
    void* ptr = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    void* ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    if (ptr == nullptr)
        throw std::bad_alloc();
    return ptr;
}

inline static void SystemDeallocBytes(void* ptr){
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    munmap(ptr, 0);
#endif
}

// 整体控制在最多10%左右的内碎片浪费
// [1,128]					8byte对齐	    freelist[0,16)
// [128+1,1024]				16byte对齐	    freelist[16,72)
// [1024+1,8*1024]			128byte对齐	    freelist[72,128)
// [8*1024+1,64*1024]		1024byte对齐     freelist[128,184)
// [64*1024+1,256*1024]		8*1024byte对齐   freelist[184,208)
class SizeClass{
public:
    // 使用位运算将 size 对齐到最接近它的大于等于它的 alignNum 的倍数
    // 比如size = 11对齐到16
    static inline size_t _RoundUp(size_t bytes, size_t alignNum)
    {
        return ((bytes + alignNum - 1) & ~(alignNum - 1));
    }

    static inline size_t RoundUp(size_t size)
    {
        if (size <= 128)
        {
            return _RoundUp(size, 8);
        }
        else if (size <= 1024)
        {
            return _RoundUp(size, 16);
        }
        else if (size <= 8 * 1024)
        {
            return _RoundUp(size, 128);
        }
        else if (size <= 64 * 1024)
        {
            return _RoundUp(size, 1024);
        }
        else if (size <= 256 * 1024)
        {
            return _RoundUp(size, 8 * 1024);
        }
        else
        {
            return _RoundUp(size, 1 << PAGE_SHIFT);
        }
    }

    // 将 size 映射到桶链的下标：
    // 这个函数的作用和 _RoundUp 函数类似，但是它返回的是下标而不是对齐后的值。
    // 比如size = 11映射下标到(2 - 1 = 1)
    static inline size_t _Index(size_t bytes, size_t align_shift)
    {
        return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
    }
    // 计算映射的哪一个自由链表桶
    static inline size_t Index(size_t bytes)
    {
        assert(bytes <= MAX_BYTES);
        // 每个区间有多少个链
        //static int const group_array[4] = { 16, 56, 56, 56 };// 打表
        const int g0 = 16, g1 = 56, g2 = 56, g3 = 56;
        if (bytes <= 128)
        {
            return _Index(bytes, 3);
        }
        else if (bytes <= 1024)
        {
            return _Index(bytes - 128, 4) + /*group_array[0]*/g0;
        }
        else if (bytes <= 8 * 1024)
        {
            return _Index(bytes - 1024, 7) + g1 + g0;
        }
        else if (bytes <= 64 * 1024)
        {
            return _Index(bytes - 8 * 1024, 10) + g2 + g1 + g0;
        }
        else if (bytes <= 256 * 1024)
        {
            return _Index(bytes - 64 * 1024, 13) + g3 + g2 + g1 + g0;
        }
        else
        {
            assert(false);
        }
        return -1;
    }

    // 计算ThreadCache一次从中心缓存CentralCache获取多少个小对象，总的大小就是MAX_BYTES = 256KB
    static size_t NumMoveSize(size_t size){
        assert(size > 0);
        // [2, 512]，一次批量移动多少个对象的(慢启动)上限值
        // 小对象一次批量上限高
        // 大对象一次批量上限低
        int num = MAX_BYTES / size;
        if (num < 2)
            num = 2;
        if (num > 512)
            num = 512;
        return num;
    }

    // 计算中心缓存CentralCache一次向PageCache获取多少页
    static size_t NumMovePage(size_t size)
    {
        // 计算一次从中心缓存获取的对象个数num
        size_t num = NumMoveSize(size);
        // 单个对象大小与对象个数相乘,获得一次需要向PageCache申请的内存大小
        size_t npage = num * size;

        npage >>= PAGE_SHIFT;
        if (npage == 0)
        {
            npage = 1;
        }
        return npage;
    }
};

// 一个自由链表 管理相同大小的一组内存
class FreeList{
public:
    // 将归还的内存块对象头插进自由链表
    void Push(void *obj){
        assert(obj);
        NextObj(obj) = _freeList;
        _freeList = obj;
        ++_size;
    }

    void PushRange(void *start, void *end, size_t size){
        NextObj(end) = _freeList;
        _freeList = start;
        _size += size;
    }

    void* Pop(){
        assert(_freeList);
        void* obj = _freeList;
        _freeList = NextObj(obj);
        --_size;
        return obj;
    }

    void PopRange(void*& start, void*& end, size_t n){
        assert(n >= _size);
        start = _freeList;
        end = start;
        for (size_t i = 0; i < n - 1; ++i) {
            end = NextObj(end);
        }
        _freeList = NextObj(end);
        _size -= n;
        NextObj(end) = nullptr;
    }

    bool Empty() const{
        return _freeList == nullptr;
    }

    size_t& MaxSize(){
        return _maxSize;
    }

    const size_t MaxSize() const {
        return _maxSize;
    }

    size_t& Size(){
        return _size;
    }
    const size_t Size() const {
        return _size;
    }

private:
    void* _freeList = nullptr;  // 头指针
    size_t _maxSize = 1;        // 慢增长用于保住申请批次的下限
    size_t _size = 0;           // 计算链表长度
};

// 管理多个大块内存的跨度结构
struct Span{


    PAGE_ID _pageId = 0;  // 大块内存起始页的页号
    size_t _n = 0;        // 页的数量

    Span* _next = nullptr;	// 指向下一个内存块的指针
    Span* _prev = nullptr;  // 指向上一个内存块的指针

    size_t _objSize = 0; // 切好的小对象大小
    size_t _useCount = 0; // 已分配给ThreadCache的小块内存的数量
    void* _freeList = nullptr;  // 已分配给ThreadCache的小块内存的自由链表

    bool _isUse = false; // 标记当前span内存跨度是否在被使用
};

// 带头的双向循环链表
class SpanList{
public:
    SpanList(){
        _head = new Span;
        _head->_next = _head;
        _head->_prev = _head;
    }
    Span* Begin(){
        return _head->_next;
    }
    Span* End(){
        return _head;
    }
    bool Empty(){
        return _head->_next == _head;
    }

    void Insert(Span* pos, Span* newSpan){
        assert(pos);
        assert(newSpan);
        Span* prev = pos->_prev;
        prev->_next = newSpan;
        newSpan->_next = pos;
        newSpan->_prev = prev;
        pos->_prev = newSpan;
    }
    // 只是从链表中删除，不会释放内存
    void Erase(Span *pos){
        assert(pos);
        assert(pos != _head);
        Span* prev = pos->_prev;
        Span* next = pos->_next;

        prev->_next = next;
        next->_prev = prev;
    }

    void PushFront(Span* span){
        Insert(Begin(), span);
    }
    Span* PopFront(){
        Span* front = _head->_next;
        Erase(front);
        return front;
    }

private:
    Span* _head;
public:
    std::mutex _mtx;        // 桶锁
};

#endif //TEST_COMMON_H
