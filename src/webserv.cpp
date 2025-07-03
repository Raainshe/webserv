/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 14:18:57 by rmakoni           #+#    #+#             */
/*   Updated: 2025/07/03 13:34:21 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./webserv <config_file>" << std::endl;
        return 1;
    }
    
    std::string config_file = argv[1];
    
    // Parse configuration file
    int parse_result = parse_config(config_file);
    if (parse_result != 0) {
        std::cerr << "Failed to parse configuration file" << std::endl;
        return 1;
    }
    
    // TODO: Get parsed servers from parser
    // For now, create a test server configuration
    std::vector<ServerConfig> servers;
    ServerConfig test_server;
    test_server.listen_port = 8080;
    test_server.server_name = "localhost";
    test_server.client_max_body_size = 1024 * 1024; // 1MB
    servers.push_back(test_server);
    
    // Initialize socket manager
    SocketManager socket_manager;
    if (!socket_manager.initialize_sockets(servers)) {
        std::cerr << "Failed to initialize server sockets" << std::endl;
        return 1;
    }
    
    std::cout << "Webserv started successfully!" << std::endl;
    std::cout << "Server is listening on " << socket_manager.get_server_sockets().size() 
              << " socket(s)" << std::endl;
    
    // TODO: Implement event loop (Step 4)
    // For now, just keep the program running
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}