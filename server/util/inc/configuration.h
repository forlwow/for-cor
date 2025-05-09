//
// Created by worker on 25-4-4.
//

#ifndef SERVER_CONFIGURE_H
#define SERVER_CONFIGURE_H

#include "singleton.h"
#include "yaml-cpp/yaml.h"
#include "log.h"

namespace server::util
{

class Config: public Singleton<Config>{
public:
    Config();
    ~Config();

    std::string ReadString(std::string_view);
    int ReadInt(std::string_view);
    template<typename T>
    T Read(std::string_view s) {
        if (config[s]) return config[s].as<T>();
        SERVER_LOG_ERROR(SERVER_LOGGER_SYSTEM) << "Configure::Read: Invalid config value: '" << s << "'";
        return {};
    }

private:
    YAML::Node config;
};

}
#endif //SERVER_CONFIGURE_H
