//
// Created by lwow on 25-5-14.
//
#include "log.h"
#include "file_reader.h"

static auto logger = SERVER_LOGGER_SYSTEM;

namespace server::util
{
std::vector<std::string> FileReader::readLines(const std::string& filename, int startLine, int count) {
    std::vector<std::string> lines;
    std::ifstream file(filename);

    if (!file.is_open()) {
        SERVER_LOG_ERROR(logger) << "Failed to open file: " << filename;
        return lines;
    }

    std::string line;
    int currentLine = 0;

    // 跳过前面不需要的行
    while (currentLine < startLine && std::getline(file, line)) {
        currentLine++;
    }

    // 读取指定的行数
    while (currentLine < startLine + count && std::getline(file, line)) {
        lines.push_back(line);
        currentLine++;
    }

    return lines;
}

std::vector<std::string> FileReader::readLastLines(const std::string& filename, int n) {
    std::vector<std::string> lines;
    std::ifstream file(filename, std::ios::ate);  // 从文件末尾开始读取

    if (!file.is_open()) {
        SERVER_LOG_ERROR(logger) << "Failed to open file: " << filename;
        return lines;
    }

    std::streampos file_size = file.tellg();
    file.seekg(file_size - 1);  // 从文件末尾开始读取

    std::string line;
    while (file_size > 0 && n > 0) {
        char ch;
        file.get(ch);

        if (ch == '\n') {
            lines.push_back(line);
            line.clear();
            n--;
        } else {
            line = ch + line;
        }

        file_size = file.tellg();
        file.seekg(file_size - 2, std::ios::beg);
    }

    if (!line.empty()) {
        lines.push_back(line);
    }

    std::reverse(lines.begin(), lines.end());

    return lines;
}

};
