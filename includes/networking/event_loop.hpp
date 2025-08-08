/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   event_loop.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hpehliva <hpehliva@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/08/08 01:38:36 by hpehliva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EVENT_LOOP_HPP
#define EVENT_LOOP_HPP

#include "http/request_parser.hpp"
#include "http/routing.hpp"
#include "networking/client_connection.hpp"
#include "networking/socket_manager.hpp"
#include <cstring>
#include <iostream> // IWYU pragma: keep
#include <map>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <iostream> // IWYU pragma: keep
#include <cstring>
#include "socket_manager.hpp"
#include "client_connection.hpp"
#include "../http/request_parser.hpp"
#include "../http/http_request.hpp"
#include "../http/http_response_handling.hpp"

class EventLoop {
private:
  std::vector<struct pollfd> poll_fds;
  std::map<int, ClientConnection *> clients;
  SocketManager &socket_manager;
  bool running;
  time_t timeout_seconds;
  RequestParser request_parser;
  Router router;

  // Maximum buffer size for reading
  static const size_t MAX_BUFFER_SIZE = 8192;

  // Maximum number of clients
  static const int MAX_CLIENTS = 1000;

public:
  EventLoop(SocketManager &sm, time_t timeout = 60);
  ~EventLoop();

  // Main event loop
  void run();
  void stop();

  // Check if the event loop is running
  bool is_running() const;

private:
  // Event handling methods
  void handle_events(int poll_result);
  void handle_new_connection(int server_fd);
  void handle_client_read(int client_fd);
  void handle_client_write(int client_fd);
  void handle_client_error(int client_fd);

  // Client management
  void add_client(int client_fd, int server_fd);
  void remove_client(int client_fd);
  void cleanup_timed_out_clients();

  // Poll management
  void setup_poll_fds();
  void add_to_poll(int fd, short events);
  void remove_from_poll(int fd);
  void update_poll_events(int fd, short events);

  // Utility methods
  bool is_server_socket(int fd) const;
  void log_error(const std::string &message);

  // Server selection for multiple servers/ports (Step 10)
  const ServerConfig *select_server_config(ClientConnection *client,
                                           const HttpRequest &request);
};

#endif // EVENT_LOOP_HPP
