/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket_manager.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/03 13:28:00 by rmakoni           #+#    #+#             */
/*   Updated: 2025/08/14 09:54:52 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SOCKET_MANAGER_HPP
#define SOCKET_MANAGER_HPP

#include <vector>
#include <map>
#include <string> // IWYU pragma: keep.
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>// IWYU pragma: keep.
#include <iostream>// IWYU pragma: keep.
#include "structs/server_config.hpp"

class SocketManager {
private:
    std::vector<int> server_sockets;
    std::map<int, ServerConfig> socket_to_config;
    std::map<int, std::vector<ServerConfig>> socket_to_server_list;
    std::map<int, int> port_to_socket_fd; // listen_port -> socket fd
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
    
    // Get servers for a specific socket
    const std::vector<ServerConfig> *get_servers_for_socket(int socket_fd) const;
    
    // Get socket fd for a specific port
    int get_socket_fd_for_port(int port) const;

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