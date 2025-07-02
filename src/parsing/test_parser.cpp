#include "webserv.hpp"
#include "tokenizer.hpp"
#include "parser.hpp"
#include <fstream>
#include <iostream>

void printLocation(const LocationConfig& loc) {
    std::cout << "      path: '" << loc.path << "'\n";
    std::cout << "      root: '" << loc.root << "'\n";
    std::cout << "      index:";
    for (size_t k = 0; k < loc.index.size(); ++k)
        std::cout << (k == 0 ? " " : ", ") << loc.index[k];
    std::cout << "\n";
    std::cout << "      autoindex: " << (loc.autoindex ? "on" : "off") << "\n";
    std::cout << "      allow_methods:";
    for (size_t k = 0; k < loc.allow_methods.size(); ++k)
        std::cout << (k == 0 ? " " : ", ") << loc.allow_methods[k];
    std::cout << "\n";
    std::cout << "      upload_store: '" << loc.upload_store << "'\n";
    std::cout << "      cgi_pass: '" << loc.cgi_pass << "'\n";
}

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
                std::cout << "    Location " << j+1 << ":\n";
                printLocation(srv.locations[j]);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        return 2;
    }
    return 0;
} 