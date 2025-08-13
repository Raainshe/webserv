/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   event_loop.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ksinn <ksinn@student.42heilbronn.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/08/13 17:45:10 by ksinn            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/networking/event_loop.hpp"
#include "../../includes/http/http_response_handling.hpp"
#include "../includes/http/http_cgi_handler.hpp"
#include "webserv.hpp" // IWYU pragma: keep
#include <algorithm>

EventLoop::EventLoop(SocketManager &sm, time_t timeout)
    : socket_manager(sm), running(false), timeout_seconds(timeout) {}

EventLoop::~EventLoop() {
  stop();
  // Clean up all client connections
  for (std::map<int, ClientConnection *>::iterator it = clients.begin();
       it != clients.end(); ++it) {
    delete it->second;
  }
  clients.clear();
}

void EventLoop::run() {
  if (running) {
    std::cerr << "Event loop is already running" << std::endl;
    return;
  }

  if (!socket_manager.is_initialized()) {
    std::cerr << "Socket manager not initialized" << std::endl;
    return;
  }

  running = true;
  setup_poll_fds();

  std::cout << "Event loop started. Listening for connections..." << std::endl;

  while (running) {
    // Clean up timed out clients
    cleanup_timed_out_clients();

    // Wait for events with a timeout
    int poll_result =
        poll(&poll_fds[0], poll_fds.size(), 1000); // 1 second timeout

    if (poll_result < 0) {
      if (errno == EINTR) {
        // Interrupted by signal, continue
        continue;
      }
      log_error("poll() failed");
      break;
    }

    if (poll_result > 0) {
      handle_events(poll_result);
    }
  }

  std::cout << "Event loop stopped" << std::endl;
}

void EventLoop::stop() { running = false; }

bool EventLoop::is_running() const { return running; }

void EventLoop::handle_events(int poll_result) {
  for (size_t i = 0; i < poll_fds.size() && poll_result > 0; ++i) {
    if (poll_fds[i].revents == 0) {
      continue;
    }

    poll_result--;

    if (poll_fds[i].revents & POLLERR) {
      handle_client_error(poll_fds[i].fd);
    } else if (poll_fds[i].revents & POLLHUP) {
      remove_client(poll_fds[i].fd);
    } else if (poll_fds[i].revents & POLLIN) {
      if (is_server_socket(poll_fds[i].fd)) {
        handle_new_connection(poll_fds[i].fd);
      } else {
        handle_client_read(poll_fds[i].fd);
      }
    } else if (poll_fds[i].revents & POLLOUT) {
      handle_client_write(poll_fds[i].fd);
    }
  }
}

void EventLoop::handle_new_connection(int server_fd) {
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  int client_fd =
      accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
  if (client_fd < 0) {
    log_error("accept() failed");
    return;
  }

  // Set client socket to non-blocking
  int flags = fcntl(client_fd, F_GETFL, 0);
  if (flags < 0 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    log_error("Failed to set client socket non-blocking");
    close(client_fd);
    return;
  }

  // Check if we've reached the maximum number of clients
  if (clients.size() >= MAX_CLIENTS) {
    std::cout << "Maximum number of clients reached, rejecting connection"
              << std::endl;
    close(client_fd);
    return;
  }

  add_client(client_fd, server_fd);
  std::cout << "New client connected (fd: " << client_fd << ")" << std::endl;
}

void EventLoop::handle_client_read(int client_fd) {
  std::map<int, ClientConnection *>::iterator it = clients.find(client_fd);
  if (it == clients.end()) {
    return;
  }
  ClientConnection *client = it->second;
  char buffer[MAX_BUFFER_SIZE];

  ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

  if (bytes_read <= 0) {
    // Connection closed or error
    remove_client(client_fd);
    return;
  }

  // Null-terminate the buffer
  buffer[bytes_read] = '\0';

  // Append to client's buffer
  client->append_to_buffer(std::string(buffer, bytes_read));

  // Parse HTTP request
  HttpRequest &request = client->get_http_request();
  if (!request_parser.parse_request(request, client->get_buffer())) {
    // Parsing error occurred
    if (request.has_error()) {
      std::cout << "HTTP parsing error: " << request.get_error_code() << " - "
                << request.get_error_message() << std::endl;
    }
    // Reset shared parser state to avoid poisoning subsequent requests
    request_parser.reset();
    request.clear();
    remove_client(client_fd);
    return;
  }

  // Enforce client_max_body_size early (when headers parsed and entering body)
  if (request.get_state() == PARSING_BODY) {
    const ServerConfig *server_config_for_limit =
        select_server_config(client, request);
    if (server_config_for_limit) {
      size_t limit = server_config_for_limit->client_max_body_size;
      size_t content_length = request.get_content_length();
      if (content_length == 0) {
        std::string cl_hdr = request.get_header("content-length");
        if (!cl_hdr.empty()) {
          content_length =
              static_cast<size_t>(std::strtoul(cl_hdr.c_str(), NULL, 10));
        }
      }
      if (content_length > 0 && limit > 0 && content_length > limit) {
        HttpResponseHandling responder(server_config_for_limit);
        std::string resp =
            responder.build_error_response(413, "Payload Too Large");
        client->clear_buffer();
        client->append_to_buffer(resp);
        client->set_state(WRITING);
        update_poll_events(client_fd, POLLOUT);
        request_parser.reset();
        request.clear();
        return;
      }
      // Safety: if body grows beyond limit (for future non-length cases)
      if (limit > 0 && request.get_body().size() > limit) {
        HttpResponseHandling responder(server_config_for_limit);
        std::string resp =
            responder.build_error_response(413, "Payload Too Large");
        client->clear_buffer();
        client->append_to_buffer(resp);
        client->set_state(WRITING);
        update_poll_events(client_fd, POLLOUT);
        request_parser.reset();
        request.clear();
        return;
      }
    }
  }

  // Check if request is complete
  if (request.is_complete()) {
    std::cout << "HTTP request completed: " << request.get_method() << " "
              << request.get_uri() << std::endl;
    // Step 10: Multiple Servers/Ports - Select correct server config
    const ServerConfig *server_config = select_server_config(client, request);
    if (!server_config) {
      // Server selection failed - send 500 error
      std::string error_response = "HTTP/1.1 500 Internal Server Error\r\n";
      error_response += "Content-Type: text/plain\r\n";
      error_response += "Content-Length: 21\r\n";
      error_response += "\r\n";
      error_response += "Server config error!";

      client->clear_buffer();
      client->append_to_buffer(error_response);
      client->set_state(WRITING);
      update_poll_events(client_fd, POLLOUT);

      request_parser.reset();
      request.clear();
      return;
    }

    std::cout << "Selected server: " << server_config->server_name << " (port "
              << server_config->listen_port << ")" << std::endl;

    // Enforce client_max_body_size on completed requests as well
    size_t limit = server_config->client_max_body_size;
    size_t content_length = request.get_content_length();
    if (content_length == 0) {
      std::string cl_hdr = request.get_header("content-length");
      if (!cl_hdr.empty()) {
        content_length =
            static_cast<size_t>(std::strtoul(cl_hdr.c_str(), NULL, 10));
      }
    }
    if ((limit > 0 && content_length > limit) ||
        (limit > 0 && request.get_body().size() > limit)) {
      HttpResponseHandling responder(server_config);
      std::string resp =
          responder.build_error_response(413, "Payload Too Large");

      client->clear_buffer();
      client->append_to_buffer(resp);
      client->set_state(WRITING);
      update_poll_events(client_fd, POLLOUT);

      request_parser.reset();
      request.clear();
      return;
    }

    // Step 7: Routing and Methods - Route the request
    RouteResult route_result = router.route_request(*server_config, request);

    std::string response;
    HttpResponseHandling responder(server_config);
    if (route_result.status == ROUTE_OK) {
      if (route_result.is_cgi_request) {
        std::cout << "Processing CGI request for URI: "
                  << route_result.file_path << std::endl;
        CgiHandler cgi_handler;
        response = cgi_handler.execute_cgi(request, *route_result.location,
                                           route_result.file_path);
      } else {
        response = responder.handle_request(request, route_result);
      }
    } else {
      int code = route_result.http_status_code;
      std::string message = route_result.error_message.empty()
                                ? "Error"
                                : route_result.error_message;
      response = responder.build_error_response(code, message);
    }

    client->clear_buffer();
    client->append_to_buffer(response);
    client->set_state(WRITING);
    update_poll_events(client_fd, POLLOUT);
    // Reset parser for next request
    request_parser.reset();
    request.clear();
  }
  std::cout << "Read " << bytes_read << " bytes from client " << client_fd
            << std::endl;
}

void EventLoop::handle_client_write(int client_fd) {
  std::map<int, ClientConnection *>::iterator it = clients.find(client_fd);
  if (it == clients.end()) {
    return;
  }

  ClientConnection *client = it->second;
  const std::string &data = client->get_buffer();

  if (data.empty()) {
    // No data to write, switch back to reading
    client->set_state(READING);
    update_poll_events(client_fd, POLLIN);
    return;
  }

  ssize_t bytes_sent = send(client_fd, data.c_str(), data.length(), 0);

  if (bytes_sent < 0) {
    // Error occurred
    remove_client(client_fd);
    return;
  }

  if (bytes_sent == static_cast<ssize_t>(data.length())) {
    // All data sent
    client->clear_buffer();
    client->set_state(READING);
    update_poll_events(client_fd, POLLIN);
  } else {
    // Partial send, keep the remaining data
    client->append_to_buffer(data.substr(bytes_sent));
  }

  std::cout << "Sent " << bytes_sent << " bytes to client " << client_fd
            << std::endl;
}

void EventLoop::handle_client_error(int client_fd) {
  std::cout << "Error on client socket " << client_fd << std::endl;
  remove_client(client_fd);
}

void EventLoop::add_client(int client_fd, int server_fd) {
  ClientConnection *client = new ClientConnection(client_fd, server_fd);
  clients[client_fd] = client;
  add_to_poll(client_fd, POLLIN);
}

void EventLoop::remove_client(int client_fd) {
  std::map<int, ClientConnection *>::iterator it = clients.find(client_fd);
  if (it != clients.end()) {
    delete it->second;
    clients.erase(it);
    remove_from_poll(client_fd);
    std::cout << "Client " << client_fd << " disconnected" << std::endl;
  }
}

void EventLoop::cleanup_timed_out_clients() {
  std::vector<int> to_remove;

  for (std::map<int, ClientConnection *>::iterator it = clients.begin();
       it != clients.end(); ++it) {
    if (it->second->is_timed_out(timeout_seconds)) {
      to_remove.push_back(it->first);
    }
  }

  for (std::vector<int>::iterator it = to_remove.begin(); it != to_remove.end();
       ++it) {
    std::cout << "Client " << *it << " timed out" << std::endl;
    remove_client(*it);
  }
}

void EventLoop::setup_poll_fds() {
  poll_fds.clear();

  // Add all server sockets to poll
  const std::vector<int> &server_sockets = socket_manager.get_server_sockets();
  for (std::vector<int>::const_iterator it = server_sockets.begin();
       it != server_sockets.end(); ++it) {
    add_to_poll(*it, POLLIN);
  }
}

void EventLoop::add_to_poll(int fd, short events) {
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = events;
  pfd.revents = 0;
  poll_fds.push_back(pfd);
}

void EventLoop::remove_from_poll(int fd) {
  for (std::vector<struct pollfd>::iterator it = poll_fds.begin();
       it != poll_fds.end(); ++it) {
    if (it->fd == fd) {
      poll_fds.erase(it);
      break;
    }
  }
}

void EventLoop::update_poll_events(int fd, short events) {
  for (std::vector<struct pollfd>::iterator it = poll_fds.begin();
       it != poll_fds.end(); ++it) {
    if (it->fd == fd) {
      it->events = events;
      break;
    }
  }
}

bool EventLoop::is_server_socket(int fd) const {
  const std::vector<int> &server_sockets = socket_manager.get_server_sockets();
  return std::find(server_sockets.begin(), server_sockets.end(), fd) !=
         server_sockets.end();
}

void EventLoop::log_error(const std::string &message) {
  std::cerr << "EventLoop Error: " << message << std::endl;
}

const ServerConfig *
EventLoop::select_server_config(ClientConnection *client,
                                const HttpRequest &request) {
  // Get the server socket that this client connected to
  int server_socket_fd = client->get_server_socket_fd();

  // Get the base server config for this socket
  const ServerConfig *base_config =
      socket_manager.get_config_for_socket(server_socket_fd);
  if (!base_config) {
    log_error("No server config found for socket FD: " +
              std::to_string(server_socket_fd));
    return NULL;
  }

  // Extract Host header from request
  std::string host_header = request.get_header("host");
  if (host_header.empty()) {
    // No Host header - use the default server for this socket
    std::cout << "No Host header found, using default server: "
              << base_config->server_name << std::endl;
    return base_config;
  }

  // Extract hostname from Host header (remove port if present)
  // Host header format: "hostname" or "hostname:port"
  std::string hostname = host_header;
  size_t colon_pos = hostname.find(':');
  if (colon_pos != std::string::npos) {
    hostname = hostname.substr(0, colon_pos);
  }

  // Search all servers bound to this socket for a matching server_name
  const std::vector<ServerConfig> *servers_on_socket =
      socket_manager.get_servers_for_socket(server_socket_fd);
  if (servers_on_socket) {
    for (std::vector<ServerConfig>::const_iterator it =
             servers_on_socket->begin();
         it != servers_on_socket->end(); ++it) {
      if (it->server_name == hostname) {
        std::cout << "Host header '" << hostname << "' matches server_name '"
                  << it->server_name << "'" << std::endl;
        return &(*it);
      }
    }
  }
  // No match found - use default server (first server for this port)
  std::cout << "Host header '" << hostname
            << "' did not match any server on this port, using default: "
            << base_config->server_name << std::endl;
  return base_config;
}
