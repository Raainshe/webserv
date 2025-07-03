/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket_manager.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/03 13:28:00 by rmakoni           #+#    #+#             */
/*   Updated: 2025/07/03 13:36:11 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SOCKET_MANAGER_HPP
#define SOCKET_MANAGER_HPP

#include <vector>
#include <map>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include "structs/server_config.hpp"

class SocketManager {
private:
    std::vector<int> server_sockets;
    std::map<int, ServerConfig> socket_to_config;
    bool initialized;

public:
    SocketManager();
    ~SocketManager();
    
    // Initialize sockets for all servers in the configuration
    bool initialize_sockets(const std::vector<ServerConfig>& servers);
    
    // Get all server socket file descriptors
    const std::vector<int>& get_server_sockets() const;
    
    // Get configuration for a specific socket
    const ServerConfig* get_config_for_socket(int socket_fd) const;
    
    // Close all sockets
    void close_all_sockets();
    
    // Check if initialization was successful
    bool is_initialized() const;

private:
    // Create and setup a single server socket
    bool setup_server_socket(const ServerConfig& server);
    
    // Set socket to non-blocking mode
    bool set_non_blocking(int socket_fd);
    
    // Bind socket to address and port
    bool bind_socket(int socket_fd, int port);
    
    // Start listening on socket
    bool listen_socket(int socket_fd);
};

#endif // SOCKET_MANAGER_HPP 