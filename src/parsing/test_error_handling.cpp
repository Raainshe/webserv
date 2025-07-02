#include "webserv.hpp"
#include "tokenizer.hpp"
#include "parser.hpp"
#include <fstream>
#include <iostream>
#include <string>

void testConfig(const std::string& config, const std::string& testName) {
    std::cout << "\n=== Testing: " << testName << " ===" << std::endl;
    std::vector<Token> tokens = tokenize(config);
    try {
        MainConfig result = parseConfig(tokens);
        std::cout << "✓ PASSED: Config parsed successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✗ FAILED: " << e.what() << std::endl;
    }
}

int main() {
    // Test 1: Valid config (should pass)
    testConfig(
        "server {\n"
        "    listen 8080;\n"
        "    location / {\n"
        "        root /var/www;\n"
        "    }\n"
        "}\n",
        "Valid config"
    );

    // Test 2: Missing required 'listen' directive
    testConfig(
        "server {\n"
        "    server_name localhost;\n"
        "    location / {\n"
        "        root /var/www;\n"
        "    }\n"
        "}\n",
        "Missing required 'listen' directive"
    );

    // Test 3: Duplicate 'listen' directive
    testConfig(
        "server {\n"
        "    listen 8080;\n"
        "    listen 9090;\n"
        "    location / {\n"
        "        root /var/www;\n"
        "    }\n"
        "}\n",
        "Duplicate 'listen' directive"
    );

    // Test 4: Duplicate 'root' directive in location
    testConfig(
        "server {\n"
        "    listen 8080;\n"
        "    location / {\n"
        "        root /var/www;\n"
        "        root /var/www2;\n"
        "    }\n"
        "}\n",
        "Duplicate 'root' directive in location"
    );

    // Test 5: Duplicate 'autoindex' directive
    testConfig(
        "server {\n"
        "    listen 8080;\n"
        "    location / {\n"
        "        root /var/www;\n"
        "        autoindex on;\n"
        "        autoindex off;\n"
        "    }\n"
        "}\n",
        "Duplicate 'autoindex' directive"
    );

    // Test 6: Invalid autoindex value
    testConfig(
        "server {\n"
        "    listen 8080;\n"
        "    location / {\n"
        "        root /var/www;\n"
        "        autoindex maybe;\n"
        "    }\n"
        "}\n",
        "Invalid autoindex value"
    );

    // Test 7: Missing semicolon
    testConfig(
        "server {\n"
        "    listen 8080\n"
        "    location / {\n"
        "        root /var/www;\n"
        "    }\n"
        "}\n",
        "Missing semicolon after listen"
    );

    // Test 8: Multiple servers (should pass)
    testConfig(
        "server {\n"
        "    listen 8080;\n"
        "    location / {\n"
        "        root /var/www;\n"
        "    }\n"
        "}\n"
        "server {\n"
        "    listen 9090;\n"
        "    location / {\n"
        "        root /var/www2;\n"
        "    }\n"
        "}\n",
        "Multiple servers"
    );

    return 0;
} 