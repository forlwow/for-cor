//
// Created by worker on 25-5-9.
//

#ifndef FILE_READER_H
#define FILE_READER_H
#include <string>
#include <fstream>
#include <sstream>


namespace server::util {
class FileReader {
public:
    static std::string readFile(std::string_view path){
        std::fstream file;
        file.open(path.data(), std::ios::in);
        if (!file.is_open()){return {};}
        std::string ss((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return ss;
    }
};

};

#endif //FILE_READER_H
