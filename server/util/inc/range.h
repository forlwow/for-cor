#ifndef RANGE_H
#define RANGE_H

#if __cplusplus >= 202002L

#include <coroutine>
#include <memory>

template<typename T>
struct Generater{
    struct promise_type{
        T n;
        std::suspend_never initial_suspend() const noexcept {return {};}
        std::suspend_always final_suspend() const noexcept {return {};}
        void unhandled_exception() {}
        Generater get_return_object(){
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always yield_value(T r){
            n = r;
            return {};
        }
        void return_void(){}
    };
    struct iterator{
        std::coroutine_handle<promise_type> *h;
        T& operator*() {
            return h->promise().n;
        }
        iterator operator++(){
            if (!h->done())
                h->resume();
            return *this;
        }
        bool operator!=(const iterator&) const{
            if(h->done()){
                return false;
            }
            else {
                return true;
            }
        }
    };
    
    iterator begin() {return {&_h};}
    iterator end() {return {&_h};}
    std::coroutine_handle<promise_type> _h;
    std::shared_ptr<bool> destroyed = std::make_shared<bool>(false);
    ~Generater(){
        if(_h.done() && !*(destroyed.get())){
            _h.destroy();
            *destroyed.get() = true;
        }
    }
};

template<typename T>
Generater<T> _range(T begin, T end, T step){
    while (begin < end){
        co_yield begin;
        begin += step;
   }
   co_return;
}

template<typename T = int>
Generater<T> range(T end){return _range(0, end, 1);}
template<typename T = int>
Generater<T> range(T begin, T end){return _range(begin, end, 0);}
template<typename T = int>
Generater<T> range(T begin, T end, T step){return _range(begin, end, step);}



#else
    #define range(n) std::vector<int>(n)
    

#endif

#endif // RANGE_H
