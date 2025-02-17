#include <cstdlib>

namespace server{

enum INIT_ERROR{
INIT_OK = 0,
// LogManager初始化错误
LOGMANAGER_INIT_ERR_EMPTY_LOG_NAME = 1,
LOGMANAGER_INIT_ERR_EMPTY_ITEM,
LOGMANAGER_INIT_ERR_EMPTY_FILENAME,
LOGMANAGER_INIT_ERR_MODE_NAME_ERR,
LOGMANAGER_INIT_ERR_LEVEL_NAME_ERR,
LOGMANAGER_INIT_ERR_YAML_ERR,
// LogFormatter初始化错误
LOGFORMATTER_INIT_ERR,
// LogAppender
LOGAPPENDER_FILE_INIT_ERR,
// AsyncLogPool
ASYNCLOGPOOL_INIT_ERR,
ASYNCLOGPOOL_TIMERFD_INIT_ERR,
};

inline int init_error(int err = -1){
    exit(err);
    return err;
}

inline int init_error(INIT_ERROR err){
    return init_error((int)err);
}


}