#ifndef WEBSERVER_UTIL_H
#define WEBSERVER_UTIL_H

#include <vector>
#include <string>
#include <ostream>
#include <map>
#include <unordered_map>

namespace server{


void Backtrace(std::vector<std::string>& bt, int size, int skip);
std::string BacktraceToString(int size, int skip, const std::string& = "");

class NullStream : public std::ostream {
public:
    NullStream() : std::ostream(nullptr) {}
    template<typename T>
    NullStream& operator<<(const T&) { return *this; }
    NullStream& operator<<(const std::string_view&) { return *this; }
    NullStream& operator<<(const std::string&) {return *this; }
    NullStream& operator<<(const char*) { return *this; }
    NullStream& operator<<(int) { return *this; }
    NullStream& operator<<(char) { return *this; }
    NullStream& operator<<(std::map<std::string, std::string>) { return *this; }
    NullStream& operator<<(std::unordered_map<std::string, std::string>) { return *this; }
};

struct NullStruct{

};

/*
    合法路径检查
    基本路径：/user/profile
    带路径参数：/user/{id} （表示 id 是一个动态变量）
    带多个路径参数：/user/{id}/order/{order_id}
    根路径：/
*/
bool isVaildPath(std::string_view path);


static inline bool IsLeapYear(int year) {
    if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
        return true;
    return false;
}

// 替换dst字符串pos位置长度为len的子字符串为src从src_pos开始的长为len的字符串
static void replace(std::string& dst, std::string_view src, size_t pos, size_t len, size_t src_pos = 0) {
    while (len--) {
        dst[pos++] = src[src_pos++];
    }
}

}


#endif 
