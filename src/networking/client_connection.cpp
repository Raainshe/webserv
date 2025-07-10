/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client_connection.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/07/09 12:31:28 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "webserv.hpp" // IWYU pragma: keep

ClientConnection::ClientConnection(int fd, int server_fd) 
    : socket_fd(fd), state(READING), last_activity(time(NULL)), 
      server_socket_fd(server_fd) {
}

ClientConnection::~ClientConnection() {
    close_connection();
}

int ClientConnection::get_socket_fd() const {
    return socket_fd;
}

ConnectionState ClientConnection::get_state() const {
    return state;
}

time_t ClientConnection::get_last_activity() const {
    return last_activity;
}

const std::string& ClientConnection::get_buffer() const {
    return buffer;
}

int ClientConnection::get_server_socket_fd() const {
    return server_socket_fd;
}

void ClientConnection::set_state(ConnectionState new_state) {
    state = new_state;
    update_activity();
}

void ClientConnection::update_activity() {
    last_activity = time(NULL);
}

void ClientConnection::append_to_buffer(const std::string& data) {
    buffer += data;
    update_activity();
}

void ClientConnection::clear_buffer() {
    buffer.clear();
}

bool ClientConnection::is_timed_out(time_t timeout_seconds) const {
    return (time(NULL) - last_activity) > timeout_seconds;
}

void ClientConnection::close_connection() {
    if (socket_fd >= 0) {
        close(socket_fd);
        socket_fd = -1;
    }
} 