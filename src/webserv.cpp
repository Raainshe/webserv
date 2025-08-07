/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hpehliva <hpehliva@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 14:18:57 by rmakoni           #+#    #+#             */
/*   Updated: 2025/08/07 23:54:25 by hpehliva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/webserv.hpp"
#include "../includes/networking/event_loop.hpp"
#include <signal.h>

void signal_handler(int sig) {
    (void)sig;
    std::cout << "\nReceived SIGINT, shutting down..." << std::endl;
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./webserv <config_file>" << std::endl;
        return 1;
    }
    
    std::string config_file = argv[1];
    
    // Parse configuration file
    std::vector<ServerConfig> servers;
    int parse_result = parse_config(config_file, servers);
    if (parse_result != 0) {
        std::cerr << "Failed to parse configuration file" << std::endl;
        return 1;
    }
    
    if (servers.empty()) {
        std::cerr << "No servers found in configuration file" << std::endl;
        return 1;
    }
    
    // Initialize socket manager
    SocketManager socket_manager;
    if (!socket_manager.initialize_sockets(servers)) {
        std::cerr << "Failed to initialize server sockets" << std::endl;
        return 1;
    }
    
    std::cout << "Webserv started successfully!" << std::endl;
    std::cout << "Server is listening on " << socket_manager.get_server_sockets().size() 
              << " socket(s)" << std::endl;
    
    // Create and run event loop
    EventLoop event_loop(socket_manager, 60); // 60 second timeout
    
    // Set up signal handling for graceful shutdown
    signal(SIGINT, signal_handler);
    
    try {
        event_loop.run();
    } catch (const std::exception& e) {
        std::cerr << "Event loop error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}