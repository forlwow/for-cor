#ifndef SERVER_HOOK_H
#define SERVER_HOOK_H

#include <sys/types.h>
#include <unistd.h>

namespace server{
    bool is_hook_enable();
    void set_hook_enable(bool flag);

}// namespace server
 
extern "C"{
typedef unsigned int (*sleep_fun)(unsigned int seconds);
extern sleep_fun sleep_f;

typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;

 }


#endif // SERVER_HOOK_H
