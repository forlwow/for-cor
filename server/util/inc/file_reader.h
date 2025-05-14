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

    static std::vector<std::string> readLines(const std::string &filename, int startLine, int count);
    static std::vector<std::string> readLastLines(const std::string &filename, int n);
    static uint64_t getFileLineCount(const std::string &filename){
        std::ifstream file(filename);
        return std::count(std::istreambuf_iterator<char>(file),
                          std::istreambuf_iterator<char>(), '\n');
    }
    static uint64_t getFileSize(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);  // 打开并跳到末尾
        if (!file) return -1;
        return file.tellg();  // 返回当前位置，即文件大小
    }
};

};

#endif //FILE_READER_H
