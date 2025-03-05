# forweb
## 一个基于c++20协程的网络框架

+ sample
```cpp
server::CoRet test_sock(){
    auto sock = server::Socket::ptr(new server::Socket(AF_INET, SOCK_STREAM));
    auto address = server::IPv4Address::CreateAddress("192.168.2.18", 9999);
    SERVER_LOG_DEBUG(logger) << "start connect";
    int res = co_await server::connect(sock, address);
    SERVER_LOG_DEBUG(logger) << res;
    if(res)
        co_return server::TERM;
    char* buff = "write example";
    SERVER_LOG_DEBUG(logger) << "start write";
    int left = strlen(buff);
    auto writer = server::send(sock, buff, 11);
    while(1){
        int res = co_await writer;
        if(res == server::SOCK_SUCCESS){
            SERVER_LOG_DEBUG(logger) << "write success";
            break;
        }
        else if(res == server::SOCK_REMAIN_DATA || res == server::SOCK_EAGAIN){
            continue;
        }
        else {
            SERVER_LOG_DEBUG(logger) << "write failed";
            break;
        }
    }
    char buf[10];
    std::string recvData;
    auto recver = server::recv(sock, buf, 10);
    while (1) {
        int res = co_await recver;
        if(res > 0){
            recvData.append(buf, res);
            SERVER_LOG_DEBUG(logger) << "recved:" << std::string(buf, res);
            if (recvData.back() == '\0') {
                SERVER_LOG_DEBUG(logger) << "recv success";
                break;
            }
        }
        else {
            if(res == server::SOCK_EAGAIN){
                continue;
            }
            else{
                SERVER_LOG_DEBUG(logger) << "recv fail";
                break;
            }
        }

    }
    co_return server::TERM;
}

```