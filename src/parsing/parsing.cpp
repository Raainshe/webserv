/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parsing.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 14:18:50 by rmakoni           #+#    #+#             */
/*   Updated: 2025/07/09 11:47:10 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"

/**
 * @brief Parses a configuration file and populates a vector of server configurations
 * 
 * This function serves as the main entry point for configuration parsing. It reads
 * the specified configuration file, tokenizes its content, parses the tokens into
 * server configurations, and stores the results in the provided vector.
 * 
 * The function performs the following steps:
 * 1. Opens and reads the configuration file
 * 2. Tokenizes the file content using the tokenize() function
 * 3. Parses the tokens into a MainConfig object using parseConfig()
 * 4. Extracts the server configurations and stores them in the output vector
 * 
 * @param config_file Path to the configuration file to parse
 * @param servers Reference to a vector that will be populated with ServerConfig objects
 * @return int 0 on success, 1 on failure
 * 
 * @note The function prints success/failure messages to std::cout and std::cerr
 * @note On success, the servers vector will contain all parsed server configurations
 * @note On failure, the servers vector remains unchanged
 * @note The function handles file I/O errors and parsing errors gracefully
 * 
 * @example
 * std::vector<ServerConfig> servers;
 * int result = parse_config("nginx.conf", servers);
 * if (result == 0) {
 *     // Configuration parsed successfully
 *     std::cout << "Found " << servers.size() << " servers" << std::endl;
 * }
 */
int parse_config(std::string config_file, std::vector<ServerConfig>& servers)
{
    //check if the file exists
    std::ifstream file(config_file.c_str());
    if (!file.is_open())
    {
        std::cerr << "Error: Config file does not exist" << std::endl;
        return (1);
    }
    
    // Read the entire file content
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    try {
        // Tokenize the content
        std::vector<Token> tokens = tokenize(content);
        
        // Parse the tokens into configuration
        MainConfig config = parseConfig(tokens);
        
        // Extract servers from the parsed configuration
        servers = config.servers;
        
        std::cout << "Successfully parsed " << servers.size() << " server(s)" << std::endl;
        return (0);
    }
    catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        return (1);
    }
}