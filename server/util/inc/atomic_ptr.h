#ifndef SERVER_ATOMIC_PTR_H
#define SERVER_ATOMIC_PTR_H

#include <atomic>
#include <cstddef>
namespace server{


template<typename T>
class atomic_ptr_{
public:
    atomic_ptr_() noexcept {m_ptr = nullptr;}
    // 非线程安全设置指针
    void set(T *ptr) noexcept {m_ptr = ptr;}
    // 线程安全 交换指针 返回旧指针
    T* xchg(T *ptr) noexcept {
        return m_ptr.exchange(ptr, std::memory_order_acq_rel);
    }

    // 线程安全 比较与交换
    // 旧指针与cmp比较，相等则将指针换为val
    T* cas(T *cmp, T *val) noexcept {
        m_ptr.compare_exchange_strong(cmp, val, std::memory_order_acq_rel);
        return cmp;
    }

private:
    std::atomic<T *> m_ptr;
};


};

#endif 