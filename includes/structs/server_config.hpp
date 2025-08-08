#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include "location_config.hpp"

struct ServerConfig {
    int listen_port;
    std::string server_name;
    std::map<int, std::string> error_pages;
    size_t client_max_body_size;
    std::vector<LocationConfig> locations;
};

#endif // SERVER_CONFIG_HPP 