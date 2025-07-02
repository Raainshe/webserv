#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <string>
#include <vector>

struct LocationConfig {
    std::string path;
    std::string root;
    std::vector<std::string> index;
    bool autoindex;
    std::vector<std::string> allow_methods;
    std::string upload_store;
    std::string cgi_pass;
};

#endif // LOCATION_CONFIG_HPP 