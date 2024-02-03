#include "hook.h"
#include "fiber.h"
#include "iomanager.h"
#include <dlfcn.h>
#include <sys/types.h>

namespace server{
static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) XX(sleep) XX(usleep)

void hook_init(){
    static bool is_inited = false;
    if(is_inited){return ;}
    #define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX);
    #undef XX
}

// 初始化器，用于在main函数执行前初始化Hook
struct _HookIniter{
    _HookIniter(){hook_init();}
};

static _HookIniter s_hook_initer;

bool is_hook_enable(){
    return t_hook_enable;
}

void set_hook_enable(bool flag){
    t_hook_enable = flag;
}

} // namespace server


extern "C"{
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds){
    if(!server::t_hook_enable){
        return sleep_f(seconds);
    }
    return 0;
}

int usleep(useconds_t usec){

}

}
