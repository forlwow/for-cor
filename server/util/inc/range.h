#ifndef RANGE_H
#define RANGE_H

#include <coroutine>

template<typename T>
struct CoRet{
    struct promise_type{
        T n;
        std::suspend_never initial_suspend() const noexcept {return {};}
        std::suspend_always final_suspend() const noexcept {return {};}
        void unhandled_exception() {}
        CoRet get_return_object(){
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always yield_value(T r){
            n = r;
            return {};
        }
        void return_void(){}
    };

    struct iterator{
        std::coroutine_handle<promise_type>& h;
        T& operator*() {
            return h.promise().n;
        }
        iterator operator++(){
            if (!h.done())
                h.resume();
            return *this;
        }
        bool operator!=(const iterator&) const{
            return !h.done();
        }
    };
    
    iterator begin() {return {_h};}
    iterator end() {return {_h};}
    std::coroutine_handle<promise_type> _h;
};

template<typename T>
CoRet<T> _range(T begin, T end, T step){
   while (begin < end){
        co_yield begin;
        begin += step;
   }
   co_return;
}

template<typename T = int>
CoRet<T> range(T end){return _range(0, end, 1);}
template<typename T = int>
CoRet<T> range(T begin, T end){return _range(begin, end, 1);}
template<typename T = int>
CoRet<T> range(T begin, T end, T step){return _range(begin, end, step);}


#endif
