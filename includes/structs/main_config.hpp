#ifndef MAIN_CONFIG_HPP
#define MAIN_CONFIG_HPP

#include <vector>
#include "structs/server_config.hpp"

struct MainConfig {
    std::vector<ServerConfig> servers;
};

#endif // MAIN_CONFIG_HPP 