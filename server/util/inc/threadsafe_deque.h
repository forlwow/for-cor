#ifndef THREADSAFE_DEQUE_H
#define THREADSAFE_DEQUE_H

#include <atomic>
#include <memory>
#include <mutex>
#include <iostream>


namespace server{
/*
 * 线程安全的双端队列
 * 在内存不足的情况下有几率会损坏
 */
template<typename T>
class threadsafe_deque{
private:

    typedef std::unique_lock<std::mutex> unique_lock_t;
    typedef std::lock_guard<std::mutex> lock_guard_t;
    typedef std::allocator<T> generator_type;
    typedef std::allocator_traits<generator_type> alloc_type;

    std::atomic<int> max_size; // 分配数组的大小
    T *true_head;   // 分配数组的头部指针
    T *true_tail;   // 分配数组的尾部指针
    generator_type generator; // 分配器

    T *head;
    T *tail;
    mutable std::mutex head_mutex;
    mutable std::mutex tail_mutex;

    void resize_with_out_lock() noexcept {
        // 分配新内存
        auto old_true_head = true_head;
        auto new_size = max_size * 2;
        auto _new_true_head = generator.allocate(new_size);
        // 转移数据
        if(head < tail) {
            std::uninitialized_move_n(true_head, max_size.load(), _new_true_head);
            head = _new_true_head + (head - old_true_head);
            tail = _new_true_head + (tail - old_true_head);
            true_head = _new_true_head;
            true_tail = _new_true_head + (new_size - 1);
        }
        else{
            std::uninitialized_move_n(true_head, tail - true_head, _new_true_head);
            std::uninitialized_move_n(head, true_tail - head + 1, _new_true_head + new_size - (true_tail - head) - 1);
            head = _new_true_head + new_size - (true_tail - head) - 1;
            tail = _new_true_head + (tail - true_head);
            true_head = _new_true_head;
            true_tail = _new_true_head + (new_size - 1);
        }
        // 销毁原数据
        generator.deallocate(old_true_head, max_size);
        // 更新数据
        max_size.store(new_size);
    }

    inline bool empty_without_lock() const{
        return head == tail;
    }

public:
    threadsafe_deque(): max_size(8), head_mutex(), tail_mutex(){
        tail = head = true_head = alloc_type ::allocate(generator, max_size.load());
        true_tail = true_head + (max_size - 1);
    }

    ~threadsafe_deque() noexcept {
        alloc_type ::deallocate(generator, true_head, max_size.load());
    }

    void resize(){
        unique_lock_t lk_head(head_mutex, std::defer_lock);
        unique_lock_t lk_tail(tail_mutex, std::defer_lock);
        std::lock(lk_head, lk_tail);
        resize_with_out_lock();
    }

    bool empty() const {
        unique_lock_t lk_head(head_mutex, std::defer_lock);
        unique_lock_t lk_tail(tail_mutex, std::defer_lock);
        std::lock(lk_head, lk_tail);
        return empty_without_lock();
    }

    void push_front(T data){
        lock_guard_t lk_head(head_mutex);
        {
            lock_guard_t lk_tail(tail_mutex);
            if (head - 1 == tail || head - 1 + max_size == tail) {
                resize_with_out_lock();
            }
            --head; // TODO: 可能导致少一个空间 建议放在resize后
        }
        if (head < true_head)
            head += max_size;
        alloc_type::construct(generator, head, std::move(data));
    }

    bool pop_front(T &data){
        lock_guard_t lk_head(head_mutex);
        auto old_head = head;
        {
            lock_guard_t lk_tail(tail_mutex);
            if (empty_without_lock()) {
                return false;
            }
            head++;
            if (head > true_tail)
                head -= max_size;
        }
        data = std::move(*old_head);
        alloc_type::destroy(generator, old_head);
        return true;
    }

    void push_back(T data){
        unique_lock_t lk_head(head_mutex);
        lock_guard_t lk_tail(tail_mutex);
        if (tail + 1 == head || tail + 1 - max_size == head)
            resize_with_out_lock();
        ++tail;
        lk_head.unlock();

        alloc_type::construct(generator, tail-1,  std::move(data));
        if (tail > true_tail){
            tail -= max_size;
        }
    }

    bool pop_back(T &data){
        unique_lock_t lk_head(head_mutex);
        lock_guard_t lk_tail(tail_mutex);
        if(empty_without_lock())
            return false;
        auto new_tail = tail - 1;
        if (new_tail < true_head)
            new_tail += max_size;
        tail = new_tail;
        lk_head.unlock();
        data = std::move(*new_tail);
        return true;
    }

    int size() const __attribute__((noinline)) {
        std::scoped_lock<std::mutex, std::mutex> lk(head_mutex, tail_mutex);
        if (head > tail){
            return true_tail - head + 1 + tail - true_head;
        }
        else
            return tail - head;
    } 

};

}
#endif
