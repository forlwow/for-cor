#include "address.h"
#include "log.h"
#include <sys/socket.h>
#include <vector>

server::Logger::ptr g_logger = SERVER_LOGGER_SYSTEM;

void test(){
    std::vector<server::Address::ptr> v;
    int res = server::Address::host2Address(v, "www.baidu.com", "https", AF_INET6, SOCK_STREAM);
    if(!res)
        return;
    for(auto &i: v){
        SERVER_LOG_INFO(g_logger) << i->toString();
    }
}

int main(){
    test();
}


