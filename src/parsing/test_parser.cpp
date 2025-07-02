#include "webserv.hpp"
#include "tokenizer.hpp"
#include "parser.hpp"
#include <fstream>
#include <iostream>

int main() {
    std::ifstream file("configs/default.conf");
    if (!file) {
        std::cerr << "Failed to open config file." << std::endl;
        return 1;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::vector<Token> tokens = tokenize(content);
    try {
        MainConfig config = parseConfig(tokens);
        std::cout << "Parsed " << config.servers.size() << " server(s)." << std::endl;
        for (size_t i = 0; i < config.servers.size(); ++i) {
            const ServerConfig& srv = config.servers[i];
            std::cout << "  Server " << i+1 << ": " << srv.locations.size() << " location(s)." << std::endl;
            for (size_t j = 0; j < srv.locations.size(); ++j) {
                std::cout << "    Location " << j+1 << ": path = '" << srv.locations[j].path << "'" << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        return 2;
    }
    return 0;
} 