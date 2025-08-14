/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ksinn <ksinn@student.42heilbronn.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 14:18:57 by rmakoni           #+#    #+#             */
/*   Updated: 2025/08/14 13:49:28 by ksinn            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/webserv.hpp"
#include "../includes/networking/event_loop.hpp"
#include <signal.h>
#include <unistd.h> // for getpid()

// Global pointer to event loop for signal handling
static EventLoop *g_event_loop = NULL;
static volatile sig_atomic_t g_shutdown_requested = 0;

void signal_handler(int sig) {
  g_shutdown_requested = 1;

  switch (sig) {
  case SIGINT:
    std::cout << "\nReceived SIGINT (Ctrl+C), shutting down..."
              << std::endl;
    if (g_event_loop) {
      g_event_loop->stop();
    }
    break;
  case SIGTERM:
    std::cout << "\nReceived SIGTERM, shutting down..." << std::endl;
    if (g_event_loop) {
      g_event_loop->stop();
    }
    break;
  case SIGUSR1:
    std::cout << "\nReceived SIGUSR1, performing shutdown with client "
                 "notification..."
              << std::endl;
    if (g_event_loop) {
      g_event_loop->shutdown_gracefully();
    }
    break;
  default:
    std::cout << "\nReceived signal " << sig << ", shutting down..."
              << std::endl;
    if (g_event_loop) {
      g_event_loop->stop();
    }
    break;
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
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
  std::cout << "Server is listening on "
            << socket_manager.get_server_sockets().size() << " socket(s)"
            << std::endl;

  // Create and run event loop
  EventLoop event_loop(socket_manager, 60); // 60 second timeout

  // Set global pointer for signal handling
  g_event_loop = &event_loop;

  // Set up signal handling for graceful shutdown
  signal(SIGINT, signal_handler);  // Ctrl+C
  signal(SIGTERM, signal_handler); // Termination request
  signal(SIGUSR1, signal_handler); // User-defined signal for graceful shutdown

  try {
    event_loop.run();
  } catch (const std::exception &e) {
    std::cerr << "Event loop error: " << e.what() << std::endl;
    g_event_loop = NULL;
    return 1;
  }

  // Clear global pointer
  g_event_loop = NULL;

  if (g_shutdown_requested) {
    std::cout << "Server shutdown completed." << std::endl;
  }

  return 0;
}
