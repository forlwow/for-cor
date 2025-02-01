#include "util.h"
#include "log.h"
#include "range.h"

#include <cstddef>
#include <cstdlib>
#include <execinfo.h>

namespace server{

void Backtrace(std::vector<std::string> &bt, int size, int skip){
    void** array = (void**)malloc(sizeof(void*) * size);
    size_t s = ::backtrace(array, size);

    char** strings = backtrace_symbols(array, s);
    if (strings == NULL){

    }

    for(auto i = skip; i < s; ++i){
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);

}

std::string BacktraceToString(int size, int skip, const std::string& prefix){
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::string ss;
    for(auto &i: bt){
        ss.append(i);
        ss.append(prefix);
    }

    return ss;
}

bool isVaildPath(std::string_view path) {
    if (path.empty() || path[0] != '/') return false;   // 不能为空，必须以 / 开头
    if (path.size() == 1) return true;
    if (path[path.size() - 1] == '/') return false;     // 不能以 / 结尾

    size_t len = path.size();
    bool lastSlash = false;
    bool inParam = false;
    for (size_t i = 0; i < len; i++) {
        char c = path[i];
        if (c == '/') {
            if (lastSlash) return false;    // 不能连续//
            lastSlash = true;
            continue;
        }
        lastSlash = false;
        if (c == '{') {
            if (inParam) return false;  // 不能嵌套{}
            inParam = true;
            continue;
        }
        if (c == '}') {
            if (!inParam) return false; // 没有{就出现}
            inParam = false;
            continue;
        }
        if (inParam) {
            if (!std::isalnum(c) && c != '_') return false; // 参数只能是字母、数字、_
        }
        else {
            if (!std::isalnum(c) && c != '_' && c != '-') return false; // 普通路径字符
        }
    }
    return !inParam;
}
}
