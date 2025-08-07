/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parsing.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hpehliva <hpehliva@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 14:18:50 by rmakoni           #+#    #+#             */
/*   Updated: 2025/08/07 23:52:57 by hpehliva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/webserv.hpp"

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