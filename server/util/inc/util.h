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
    NullStream& operator<<(const char*) { return *this; }
    NullStream& operator<<(int) { return *this; }
    NullStream& operator<<(std::map<std::string, std::string>) { return *this; }
    NullStream& operator<<(std::unordered_map<std::string, std::string>) { return *this; }
};

}


#endif 
