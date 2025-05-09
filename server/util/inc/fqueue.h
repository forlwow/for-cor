#ifndef SERVER_FQUEUE_H
#define SERVER_FQUEUE_H

#include "atomic_ptr.h"
#include "ethread.h"
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <vector>


namespace server{
namespace util{

template<typename T, int N>
class fqueue{
public:
    inline fqueue(){
        begin_chunk = allocate_chunk();
        begin_pos = 0;
        back_chunk = nullptr;
        back_pos = 0;
        end_chunk = begin_chunk;
        end_pos = 0;
    }

    inline ~fqueue(){
        while (true){
            if (begin_chunk == end_chunk){
                delete begin_chunk;
                break;
            }
            chunk *tmp = begin_chunk;
            begin_chunk = begin_chunk->next;
            delete tmp;
        }

        chunk *sc = m_space_chunk.xchg(nullptr);
        delete sc;
    }

    inline T &front() {return begin_chunk->values[begin_pos];}
    inline T &back() {return back_chunk->values[back_pos];}
    inline void push(){
        back_chunk = end_chunk;
        back_pos = end_pos;
        if (++end_pos != N) return; // 如果=N则表示满了
        chunk *tmp = m_space_chunk.xchg(nullptr);
        if (tmp){
            end_chunk->next = tmp;
            tmp->prev = end_chunk;
        }
        else{
            end_chunk->next = allocate_chunk();
            end_chunk->next->prev = end_chunk;
        }
        end_chunk = end_chunk->next;
        end_pos = 0;
    }

    // pop不会销毁元素
    inline void pop(){
        if (++begin_pos == N){
            chunk *tmp = begin_chunk;
            begin_chunk = begin_chunk->next;
            begin_chunk->prev = nullptr;
            begin_pos = 0;

            // 由于tmp更加新，因此为了缓存机制，丢弃老的，保存新的
            chunk *old = m_space_chunk.xchg(tmp);
            delete old;
        }
    }

    // 回滚
    inline void unpush(){
        if (back_pos) --back_pos;
        else {
            back_pos = N-1;
            back_chunk = back_chunk->prev;
        }
        if (end_pos) --end_pos;
        else{
            end_pos = N-1;
            end_chunk = end_chunk->prev;
            delete end_chunk->next;
            end_chunk->next = nullptr;
        }
    }

private:
    struct chunk{
        T values[N];
        chunk *prev = nullptr;
        chunk *next = nullptr;
    };
    static inline chunk *allocate_chunk (){
        chunk* res = new chunk;
        return res;
    }

    chunk *begin_chunk;
    int begin_pos;
    chunk *back_chunk;
    int back_pos;
    chunk *end_chunk;
    int end_pos;

    atomic_ptr_<chunk> m_space_chunk;
};

template<typename T, int N> 
class fpipe_base_{
public:
    fpipe_base_()=default;
    virtual ~fpipe_base_() = default;
    virtual void write(const T&value, bool incomplete) = 0;
    virtual bool unwrite(T *value) = 0;
    virtual bool flush() = 0;
    virtual bool check_read() = 0;
    virtual bool read(T *valute) = 0;
    virtual bool probe(bool(*fn)(const T&)) = 0;

};

// 无锁队列
// 只适用于单生产者-单消费者模式
template<typename T, int N>
class fpipe_: public fpipe_base_<T, N>{
public:
    inline fpipe_(){
        m_queue.push();
        r = w = f = &m_queue.back();
        c.set(&m_queue.back());
    }

    // 向管道写入数据，并且不刷新
    // incomplete为true，则表示还有其他数据要写入
    inline void write(const T &value_, bool incomplete_) override {
        m_queue.back() = value_;
        m_queue.push();
        if (!incomplete_)
            f = &m_queue.back();
    }

    // 刷新已经准备好的元素到管道内
    // 返回false表示读进程正在睡眠，应该去唤醒
    inline bool flush() override {
        // 没有需要刷新的东西
        if (w == f) return true;
        // 尝试将c设置为f
        if (c.cas(w, f) != w){
            // 若c原来为空
            // 表示读进程正在睡眠,这样就不用去关心线程安全问题
            c.set(f);
            w = f;
            return false;
        }
        // 读进程醒着
        w = f;
        return true;
    }

    // 读数据，没有返回false
    inline bool read(T *value) override {
        if(!check_read()){
            return false;
        }
        // *value = std::move(m_queue.front());     // 使用move解决内存销毁问题
        *value = m_queue.front();
        m_queue.pop();
        return true;
    }

    // 检查是否有数据可读
    inline bool check_read() override {
        // 判断是否已经预取过
        if (&m_queue.front() != r && r){
            return true;
        }
        r = c.cas(&m_queue.front(), nullptr);
        if (&m_queue.front() == r || !r){
            return false;
        }
        return true;
    }
    inline bool unwrite(T *value) override {
        if (f == &m_queue.back()) return false;
        m_queue.unpush();
        *value = m_queue.back();
        return true;
    }

    inline bool probe(bool(*fn)(const T&)) override {
        const bool rc = check_read();
        assert(rc);
        return (*fn)(m_queue.front());
    }

protected:
    fqueue<T, N> m_queue;

    T *w;   // 指向第一个未被刷新的元素，只被写进程使用
    T *r;   // 指向第一个还没有被读的元素，只被读进程使用
    T *f;   // 指向下一轮要被刷新的一批元素的第一个
    atomic_ptr_<T> c;   // 读写指针共享的指针，指向每一轮刷新的起点， c为空时表示读进程睡眠

};

template<typename T, int N=8>
class fpipe{
public:
    fpipe()=default;
    virtual ~fpipe()=default;

    virtual void read_block(T &value_){
        T tmp;
        while (!m_pipe.read(&tmp)){
            std::unique_lock<std::mutex> lock(m_lock);
            m_condition_var.wait(lock);
            assert(1);
        }
        value_ = std::move(tmp);
        // value_ = tmp;
    }

    virtual bool read_noblock(T &value_){
        T tmp;
        if (m_pipe.read(&tmp)){
            value_ = std::move(tmp);
            return true;
        }
        return false;
    }

    virtual bool check_read() {
        return m_pipe.check_read();
    }

    virtual void write(const T& value_, bool incomplete = false){
        m_pipe.write(value_, incomplete);
        if (incomplete) return ;
        if (!m_pipe.flush()){
            m_condition_var.notify_all();
        }
    }

    virtual void write(const std::vector<T> &values_){
        int n = values_.size();
        for (int i = 0; i < n-1; ++i){
            m_pipe.write(values_[i], true);
        }
        m_pipe.write(values_[n], false);
        if (!m_pipe.flush()){
            m_condition_var.notify_all();
        }
    }

    virtual void notify() {
        m_pipe.flush();
        m_condition_var.notify_all();
    }

private:
    fpipe_<T, N> m_pipe;
    std::condition_variable m_condition_var;
    std::mutex m_lock;
};

template<typename T, int N = 8>
class fpipe_muti_write: public fpipe<T, N> {
public:
    void write(const T &value_, bool incomplete = false) override {
        LockGuard lock(m_wlock);
        fpipe<T, N>::write(value_, incomplete);
    }

    void notify() override {
        LockGuard lock(m_wlock);
        fpipe<T, N>::notify();
    }
    bool read_noblock(T &value) override {
        LockGuard lock(m_rlock);
        return fpipe<T, N>::read_noblock(value);
    }
    void read_block(T &value) override {
        LockGuard lock(m_rlock);
        fpipe<T, N>::read_block(value);
    }

private:
    SpinLock m_wlock;
    SpinLock m_rlock;
};


}

}

#endif