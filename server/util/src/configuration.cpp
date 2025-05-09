//
// Created by worker on 25-4-4.
//
#include "configuration.h"
#include "log.h"

namespace server::util
{

static Logger::ptr logger = SERVER_LOGGER_SYSTEM;

Config::Config() {
    config = YAML::LoadFile("config.yml");
}

Config::~Config() {
}

std::string Config::ReadString(std::string_view s) {
    if (config[s]) return config[s].as<std::string>();
    SERVER_LOG_ERROR(logger) << "Configure::Read: Invalid config value: '" << s << "'";
    return {};
}

int Config::ReadInt(std::string_view s) {
    if (config[s]) return config[s].as<int>();
    SERVER_LOG_ERROR(logger) << "Configure::Read: Invalid config value: '" << s << "'";
    return {};
}

}

