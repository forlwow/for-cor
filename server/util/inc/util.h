#ifndef WEBSERVER_UTIL_H
#define WEBSERVER_UTIL_H

#include <vector>
#include <string>

namespace server{


void Backtrace(std::vector<std::string>& bt, int size, int skip);
std::string BacktraceToString(int size, int skip, const std::string& = "");



}


#endif 
