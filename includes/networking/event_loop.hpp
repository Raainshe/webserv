/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   event_loop.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/07/09 12:31:37 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef EVENT_LOOP_HPP
#define EVENT_LOOP_HPP

#include <vector>
#include <map>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream> // IWYU pragma: keep
#include <cstring>
#include "networking/socket_manager.hpp"
#include "networking/client_connection.hpp"

class EventLoop {
private:
    std::vector<struct pollfd> poll_fds;
    std::map<int, ClientConnection*> clients;
    SocketManager& socket_manager;
    bool running;
    time_t timeout_seconds;
    
    // Maximum buffer size for reading
    static const size_t MAX_BUFFER_SIZE = 8192;
    
    // Maximum number of clients
    static const int MAX_CLIENTS = 1000;

public:
    EventLoop(SocketManager& sm, time_t timeout = 60);
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
    void log_error(const std::string& message);
};

#endif // EVENT_LOOP_HPP 