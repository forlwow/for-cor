//
// Created by lwow on 2024/1/20.
//

#ifndef TEST_OBJECTPOOL_H
#define TEST_OBJECTPOOL_H

#include "common.h"
#include <set>
#include <queue>

const static size_t setZero = ~0 - 1;
const static size_t newSize = 128 << PAGE_SHIFT;

// 定长内存池
template<typename T>
class ObjectPool{
public:
    ObjectPool(size_t init_mem = 8 << PAGE_SHIFT)
    {
        _memory = reinterpret_cast<char*>(SystemAllocBytes(init_mem));
        _remainBytes = init_mem;
    }

    T* New(){
        T* obj;
        if(_freeList){
            void* next = NextObj(_freeList);
            obj = (T*)_freeList;
            _freeList = next;
        }
        else{
            // 剩余内存_remainBytes不够一个对象大小时，重新开一块大空间
            if(_remainBytes < sizeof(T)){
                _remainBytes = newSize;      // 128KB
                // 分配8KB
                _memory = reinterpret_cast<char*>(SystemAllocBytes(_remainBytes));
            }
            obj = reinterpret_cast<T*>(_memory);
            // 保证一次分配的空间够存放下当前平台的指针
            // 大块内存块往后走，前面objSize大小的内存该分配出去了
            _memory += objSize;
            _remainBytes -= objSize;
        }
        // 定位new显式调用T类型构造函数:在内存地址obj处创建一个新的T类型的对象，并调用该对象的构造函数。
        // 与普通的new运算符不同的是，它不会使用动态内存分配器来分配内存，而是使用指定的内存地址
        new(obj)T;
        return obj;
    }

    //将obj这块内存链接到_freeList中
    void Delete(T* obj){
        //显式调用obj对象的析构函数,清理空间
        obj->~T();
        NextObj(obj) = _freeList;
        _freeList = obj;
    }
private:
    size_t _growFactor = 1;                // 增长因子 每次获取它个数块内存
    size_t _remainBytes = 0;            // 大块内存在切分过程中剩余字节数
    size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
    char* _memory = nullptr;            // 指向大内存的指针

    void* _freeList = nullptr;          // 自由链表的头指针，用于保存当前有哪些对象可以被重复利用。
};


template<typename T>
class ObjectPool_{
    typedef struct memory{
        memory()=default;
        explicit memory(size_t objNum): _freeList(){
            size_t tsize = sizeof(T);
            void* ptr = SystemAllocBytes(objNum * tsize);
            char* start = reinterpret_cast<char*>(ptr) + tsize;
            void* cur = ptr;
            size_t tmp = objNum;
            while(--tmp){
                NextObj(cur) = start;
                cur = start;
                start += tsize;
            }
            NextObj(cur) = nullptr;

            _freeList.PushRange(ptr, cur, objNum);
        }
        memory& operator=(const memory&)=default;
        memory& operator=(memory&&)=default;

        FreeList _freeList;
    };
    struct Compare{
        bool operator()(const memory *l, const memory* r) const{
            return l->_freeList.Size() > r->_freeList.Size();
        }
    };
public:
    void* New(){
        memory* tmem;
        if(m_availabe.empty()) {
            m_growFact = (m_growFact << 1) ? m_growFact << 1 : m_growFact;
            tmem = new memory(m_growFact);
        }
        else {
            tmem = *m_availabe.begin();
            m_availabe.erase(m_availabe.begin());
        }
        void *ret = tmem->_freeList.Pop();
        if(tmem->_freeList.Empty())
            m_empty.insert(tmem);
        else
            m_availabe.insert(tmem);
        new(ret)T;
        return ret;
    }

    void Delete(T* ptr){
        memory* tmem;
        if(!m_availabe.empty()){
            tmem = *m_availabe.begin();
            m_availabe.erase(tmem);
        }
        else{
            tmem = *m_empty.begin();
            m_empty.erase(tmem);
        }
        (*tmem)->Push(ptr);
        m_availabe.insert(tmem);
    }

private:
    size_t m_growFact = 1;

    std::set<memory*, Compare> m_empty;
    std::set<memory*, Compare> m_availabe;
};


#endif //TEST_OBJECTPOOL_H
