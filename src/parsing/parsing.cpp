/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parsing.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 14:18:50 by rmakoni           #+#    #+#             */
/*   Updated: 2025/07/09 11:50:26 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"

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