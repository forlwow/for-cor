#ifndef ETHREAD_H
#define ETHREAD_H

#include <cstdint>
#include <string>
#include <thread>
#include <functional>
#include <memory>
#include <semaphore.h>
#include <pthread.h>
#include <mutex>
#include <atomic>

namespace server {

class EThread;

extern thread_local EThread* t_thread;
extern thread_local const char* t_thread_name;
extern thread_local int t_thread_id;


class Semaphore{
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();
    void wait();
    void notify();

private:
    Semaphore(const Semaphore&)=delete;
    Semaphore(const Semaphore&&)=delete;
    Semaphore& operator=(const Semaphore&)=delete;
    Semaphore& operator=(const Semaphore&&)=delete;
private:
    sem_t m_semaphore;
};

template<typename T>
struct LockGuard{
    LockGuard(T& mutex)
        :m_mutex(mutex)
    {
        m_mutex.lock();
        m_locked = true;
    }
    ~LockGuard(){
        unlock();
    }

    void lock(){
        if (!m_locked){
            m_mutex.lock();
            m_locked = true;
        }
    }
    void unlock(){
        if (m_locked){
            m_locked = false;
            m_mutex.unlock();
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};

template<typename T>
struct ReadLockGuard{
    ReadLockGuard(T& mutex)
        :m_mutex(mutex)
    {
        m_mutex.rdlock();
        m_locked = true;
    }
    ~ReadLockGuard(){
        if(m_locked)
            unlock();
    }

    void lock(){
        if (!m_locked){
            m_mutex.rdlock();
            m_locked = true;
        }
    }
    void unlock(){
        if (m_locked){
            m_locked = false;
            m_mutex.unlock();
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};
template<typename T>
struct WriteLockGuard{
    WriteLockGuard(T& mutex)
        :m_mutex(mutex)
    {
        m_mutex.wrlock();
        m_locked = true;
    }
    ~WriteLockGuard(){
        if(m_locked)
            unlock();
    }

    void lock(){
        if (!m_locked){
            m_mutex.wrlock();
            m_locked = true;
        }
    }
    void unlock(){
        if (m_locked){
            m_locked = false;
            m_mutex.unlock();
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};

class RWMutex_{
public:
    RWMutex_(){
        pthread_rwlock_init(&m_lock, nullptr);
    }
    ~RWMutex_(){
        pthread_rwlock_destroy(&m_lock);
    }
    void rdlock(){
        pthread_rwlock_rdlock(&m_lock);
    }
    void wrlock(){
        pthread_rwlock_wrlock(&m_lock);
    }
    void unlock(){
        pthread_rwlock_unlock(&m_lock);
    }

private:
    pthread_rwlock_t m_lock;
};

class SpinLock{
public:
    SpinLock(): m_mutex(ATOMIC_FLAG_INIT) {}
    ~SpinLock() {}
    void lock() {
        while(m_mutex.test_and_set(std::memory_order_acquire));
    }
    void unlock() {m_mutex.clear(std::memory_order_release);}
private:
    std::atomic_flag m_mutex;
};


// 空的锁 无效果 用于调试
struct NullMutex{
    NullMutex() {}
    ~NullMutex() {}
    void lock() {}
    void unlock() {}
};
struct NullRWMutex{
    NullRWMutex() {}
    ~NullRWMutex() {}
    void wrlock() {}
    void rdlock() {}
    void unlock() {}
};
// typedef NullMutex Mutex;
// typedef NullRWMutex RWMutex;

typedef RWMutex_ RWMutex;
typedef std::mutex Mutex;
// typedef SpinLock Mutex;

class EThread{
public:
    typedef std::shared_ptr<EThread> ptr;
    EThread(std::function<void()> cb, const std::string& name);
    ~EThread();
    
    int getId() const {return m_id;}
    const std::string& getName() const {return m_name;}
    std::atomic_bool& getStartFlag() {return m_start_flag;}

    void join();
    static EThread* GetThis();      // 获取当前处于的线程
    static const char* GetName();
    static void SetName(const std::string& name);

private:
    EThread(const EThread&)=delete;
    EThread(const EThread&&)=delete;
    EThread& operator=(const EThread&)=delete;
    EThread& operator=(const EThread&&)=delete;

    static void* run(void *arg);
private:
    int m_id;                       //
    std::thread m_thread;           //
    std::function<void()> m_cb;     //
    std::string m_name = "None";    // 线程名

    std::atomic_bool m_start_flag;          // 用于等待启动的信号量
};

}

#endif
