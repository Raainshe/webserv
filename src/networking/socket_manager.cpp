/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket_manager.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/03 13:28:00 by rmakoni           #+#    #+#             */
/*   Updated: 2025/08/14 09:54:52 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/networking/socket_manager.hpp"
#include <cerrno>
#include <cstring>
#include <set>

SocketManager::SocketManager() : initialized(false) {}

SocketManager::~SocketManager() { close_all_sockets(); }

bool SocketManager::initialize_sockets(
    const std::vector<ServerConfig> &servers) {
  if (initialized) {
    std::cerr << "Sockets already initialized" << std::endl;
    return false;
  }

  bool all_success = true;

  std::set<int> opened_ports;
  for (std::vector<ServerConfig>::const_iterator it = servers.begin();
       it != servers.end(); ++it) {
    int port = it->listen_port;
    if (opened_ports.count(port) == 0) {
      if (!setup_server_socket(*it)) {
        std::cerr << "Failed to setup socket for server on port " << port
                  << std::endl;
        all_success = false;
        continue;
      }
      opened_ports.insert(port);
      int fd = server_sockets.back();
      port_to_socket_fd[port] = fd;
    }
    // Find socket fd for this port and append server config to its list
    int fd_for_port = port_to_socket_fd[port];
    socket_to_server_list[fd_for_port].push_back(*it);
    // Also set base config for fd if not set
    if (socket_to_config.find(fd_for_port) == socket_to_config.end()) {
      socket_to_config[fd_for_port] = *it;
    }
  }

  if (all_success && !server_sockets.empty()) {
    initialized = true;
    std::cout << "Successfully initialized " << server_sockets.size()
              << " server socket(s)" << std::endl;
  }

  return all_success;
}

const std::vector<int> &SocketManager::get_server_sockets() const {
  return server_sockets;
}

const ServerConfig *SocketManager::get_config_for_socket(int socket_fd) const {
  std::map<int, ServerConfig>::const_iterator it =
      socket_to_config.find(socket_fd);
  if (it != socket_to_config.end()) {
    return &(it->second);
  }
  return NULL;
}

const std::vector<ServerConfig> *
SocketManager::get_servers_for_socket(int socket_fd) const {
  std::map<int, std::vector<ServerConfig>>::const_iterator it =
      socket_to_server_list.find(socket_fd);
  if (it != socket_to_server_list.end()) {
    return &(it->second);
  }
  return NULL;
}

int SocketManager::get_socket_fd_for_port(int port) const {
  std::map<int, int>::const_iterator it = port_to_socket_fd.find(port);
  if (it != port_to_socket_fd.end())
    return it->second;
  return -1;
}

void SocketManager::close_all_sockets() {
  for (std::vector<int>::iterator it = server_sockets.begin();
       it != server_sockets.end(); ++it) {
    if (*it >= 0) {
      close(*it);
    }
  }
  server_sockets.clear();
  socket_to_config.clear();
  initialized = false;
}

bool SocketManager::is_initialized() const { return initialized; }

bool SocketManager::setup_server_socket(const ServerConfig &server) {
  // Create socket
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
    return false;
  }

  // Set socket options to reuse address
  int opt = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    std::cerr << "Failed to set SO_REUSEADDR: " << strerror(errno) << std::endl;
    close(socket_fd);
    return false;
  }

  // Set non-blocking mode
  if (!set_non_blocking(socket_fd)) {
    close(socket_fd);
    return false;
  }

  // Bind socket
  if (!bind_socket(socket_fd, server.listen_port)) {
    close(socket_fd);
    return false;
  }

  // Start listening
  if (!listen_socket(socket_fd)) {
    close(socket_fd);
    return false;
  }

  // Store socket
  server_sockets.push_back(socket_fd);

  std::cout << "Server socket created and listening on port "
            << server.listen_port << " (fd: " << socket_fd << ")" << std::endl;

  return true;
}

bool SocketManager::set_non_blocking(int socket_fd) {
  int flags = fcntl(socket_fd, F_GETFL, 0);
  if (flags < 0) {
    std::cerr << "Failed to get socket flags: " << strerror(errno) << std::endl;
    return false;
  }

  if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    std::cerr << "Failed to set non-blocking mode: " << strerror(errno)
              << std::endl;
    return false;
  }

  return true;
}

bool SocketManager::bind_socket(int socket_fd, int port) {
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    std::cerr << "Failed to bind socket to port " << port << ": "
              << strerror(errno) << std::endl;
    return false;
  }

  return true;
}

bool SocketManager::listen_socket(int socket_fd) {
  if (listen(socket_fd, SOMAXCONN) < 0) {
    std::cerr << "Failed to listen on socket: " << strerror(errno) << std::endl;
    return false;
  }

  return true;
}
