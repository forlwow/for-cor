#ifndef WEBSERVER_SINGLETON_H
#define WEBSERVER_SINGLETON_H


#include <memory>
#include <mutex>
template<typename T>
class Singleton{
public:
    static T* GetInstance(){
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]{
            m_pSington.reset(new T);
            });
        return m_pSington.get();
    }

protected:
    Singleton() {}
    ~Singleton() {}

    static std::shared_ptr<T> m_pSington;
private:
    Singleton(const Singleton&)=delete;
    Singleton& operator=(const Singleton&)=delete;
};

template<typename T>
std::shared_ptr<T> Singleton<T>::m_pSington;

#endif 
