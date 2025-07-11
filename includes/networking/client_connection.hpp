/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client_connection.hpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/07/10 13:30:35 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef CLIENT_CONNECTION_HPP
#define CLIENT_CONNECTION_HPP

#include <sys/time.h>
#include <string>
#include "http/http_request.hpp"

enum ConnectionState {
    READING,
    WRITING,
    CLOSING
};

class ClientConnection {
private:
    int socket_fd;
    ConnectionState state;
    time_t last_activity;
    std::string buffer;
    int server_socket_fd; // Which server this client belongs to
    HttpRequest http_request; // HTTP request being parsed

public:
    ClientConnection(int fd, int server_fd);
    ~ClientConnection();
    
    // Getters
    int get_socket_fd() const;
    ConnectionState get_state() const;
    time_t get_last_activity() const;
    const std::string& get_buffer() const;
    int get_server_socket_fd() const;
    
    // Setters
    void set_state(ConnectionState new_state);
    void update_activity();
    void append_to_buffer(const std::string& data);
    void clear_buffer();
    
    // Utility
    bool is_timed_out(time_t timeout_seconds) const;
    void close_connection();
    
    // HTTP request access
    HttpRequest& get_http_request();
    const HttpRequest& get_http_request() const;
};

#endif // CLIENT_CONNECTION_HPP 