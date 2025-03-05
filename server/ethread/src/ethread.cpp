#include "ethread.h"
#include "log.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <stdexcept>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>


namespace server {

static Logger::ptr g_logger = SERVER_LOGGER_SYSTEM;

thread_local EThread* t_thread = nullptr;
thread_local const char* t_thread_name = "Main Thread";
thread_local int t_thread_id = syscall(SYS_gettid);

Semaphore::Semaphore(uint32_t count){
    if (sem_init(&m_semaphore, 0, count)){
        throw std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore(){
    sem_destroy(&m_semaphore);
}

void Semaphore::wait(){
    if(sem_wait(&m_semaphore)){
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify(){
    if(sem_post(&m_semaphore)){
        throw std::logic_error("sem_post error");
    }
}


EThread* EThread::GetThis(){
    return t_thread;
}

const char* EThread::GetName(){
    return t_thread_name;
}

void EThread::SetName(const std::string &name){
    if (t_thread){
        t_thread->m_name = name;
    }
    t_thread_name = name.data();
}

EThread::EThread(std::function<void()> cb, const std::string& name)
    :m_cb(cb), m_name(name), m_start_flag(false)
{
    if (name.empty()){
        m_name = "UNKNOW";
    }
    m_thread = std::thread(&EThread::run, this);
    while (!m_start_flag);     // 保证在线程构造完毕开始运行后才完成构造
    if(!m_thread.joinable()){
        SERVER_LOG_ERROR(g_logger) << "create thread error";
        throw std::logic_error("thread create error");
    }

    SERVER_LOG_INFO(g_logger) << "create ethread:" << m_name;
}

EThread::~EThread(){
    if(m_thread.joinable())
        m_thread.join();
    SERVER_LOG_INFO(g_logger) << m_name << " delete";
}

void* EThread::run(void *arg){
    EThread* thread = (EThread*)arg;

    t_thread = thread;
    t_thread_name = thread->m_name.data();
    t_thread_id = syscall(SYS_gettid);
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    thread->m_id = t_thread_id;

    std::function<void()> cb;
    cb.swap(thread->m_cb);

    thread->m_start_flag.store(true, std::memory_order_acq_rel);       // 初始化完成 主线程返回
    cb();                               // 执行任务
    SERVER_LOG_INFO(g_logger) << t_thread_name << " finish thread";
    return 0;
}

void EThread::join(){
    if(m_thread.joinable())
        m_thread.join();
}

}
