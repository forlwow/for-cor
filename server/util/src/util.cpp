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

    for(auto i: range<int>(skip, s)){
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


}
