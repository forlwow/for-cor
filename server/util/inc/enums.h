#ifndef SERVER_ENUMS_H
#define SERVER_ENUMS_H

namespace server {
enum State{
    INIT,       // 初始化状态
    HOLD,       // 暂停挂起状态
    EXEC,       // 结束状态
    TERM,       // 结束状态
    READY,      // 可执行状态
    EXCEPT      // 出错状态
};

}


#endif // SERVER_ENUMS_H
