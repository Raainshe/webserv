/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket_manager.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/03 13:28:00 by rmakoni           #+#    #+#             */
/*   Updated: 2025/07/09 11:48:14 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp" // IWYU pragma: keep

/**
 * @brief Default constructor for SocketManager
 * 
 * Initializes a new SocketManager instance with the initialized flag set to false.
 * No sockets are created or configured at this point.
 * 
 * @note The manager must be explicitly initialized with initialize_sockets() before use
 */
SocketManager::SocketManager() : initialized(false) {
}

/**
 * @brief Destructor for SocketManager
 * 
 * Automatically closes all managed server sockets and cleans up resources
 * when the SocketManager object is destroyed.
 * 
 * @note This ensures proper cleanup even if the object goes out of scope unexpectedly
 */
SocketManager::~SocketManager() {
    close_all_sockets();
}

/**
 * @brief Initializes server sockets for all configured servers
 * 
 * This function creates and configures listening sockets for each server
 * in the provided configuration. It performs the following operations:
 * - Creates TCP sockets for each server
 * - Sets socket options (address reuse, non-blocking mode)
 * - Binds sockets to their respective ports
 * - Starts listening for incoming connections
 * - Stores socket file descriptors and configurations
 * 
 * @param servers Vector of ServerConfig objects containing server configurations
 * @return bool True if all sockets were initialized successfully, false otherwise
 * 
 * @note If any socket fails to initialize, the function returns false
 * @note The function will not reinitialize if already initialized
 * @note All sockets are set to non-blocking mode for event-driven operation
 * @note Socket file descriptors are stored for later use by the event loop
 * 
 * @example
 * std::vector<ServerConfig> servers = load_config();
 * SocketManager manager;
 * if (manager.initialize_sockets(servers)) {
 *     // Sockets ready for accepting connections
 * }
 */
bool SocketManager::initialize_sockets(const std::vector<ServerConfig>& servers) {
    if (initialized) {
        std::cerr << "Sockets already initialized" << std::endl;
        return false;
    }
    
    bool all_success = true;
    
    for (std::vector<ServerConfig>::const_iterator it = servers.begin(); 
         it != servers.end(); ++it) {
        if (!setup_server_socket(*it)) {
            std::cerr << "Failed to setup socket for server on port " 
                      << it->listen_port << std::endl;
            all_success = false;
        }
    }
    
    if (all_success && !server_sockets.empty()) {
        initialized = true;
        std::cout << "Successfully initialized " << server_sockets.size() 
                  << " server socket(s)" << std::endl;
    }
    
    return all_success;
}

/**
 * @brief Returns a reference to the vector of server socket file descriptors
 * 
 * Provides access to all managed server socket file descriptors for use
 * in event loops or other socket operations.
 * 
 * @return const std::vector<int>& Reference to the vector of socket file descriptors
 * 
 * @note The returned vector contains only valid socket file descriptors
 * @note This is typically used with select(), poll(), or epoll() for event monitoring
 * @note The vector is empty if no sockets have been initialized
 * 
 * @example
 * const std::vector<int>& sockets = manager.get_server_sockets();
 * for (size_t i = 0; i < sockets.size(); ++i) {
 *     // Monitor socket[i] for events
 * }
 */
const std::vector<int>& SocketManager::get_server_sockets() const {
    return server_sockets;
}

/**
 * @brief Retrieves the server configuration associated with a specific socket
 * 
 * Looks up the ServerConfig object that corresponds to the given socket
 * file descriptor. This is useful for determining which server configuration
 * should be used when handling a connection on a specific socket.
 * 
 * @param socket_fd The socket file descriptor to look up
 * @return const ServerConfig* Pointer to the associated server configuration, or NULL if not found
 * 
 * @note Returns NULL if the socket file descriptor is not managed by this SocketManager
 * @note The returned pointer is valid as long as the SocketManager object exists
 * @note This is typically used when accepting connections to determine server settings
 * 
 * @example
 * int client_socket = accept(server_socket, ...);
 * const ServerConfig* config = manager.get_config_for_socket(server_socket);
 * if (config) {
 *     // Use config->server_name, config->locations, etc.
 * }
 */
const ServerConfig* SocketManager::get_config_for_socket(int socket_fd) const {
    std::map<int, ServerConfig>::const_iterator it = socket_to_config.find(socket_fd);
    if (it != socket_to_config.end()) {
        return &(it->second);
    }
    return NULL;
}

/**
 * @brief Closes all managed server sockets and resets the manager state
 * 
 * This function performs cleanup by:
 * - Closing all server socket file descriptors
 * - Clearing the internal socket and configuration mappings
 * - Resetting the initialized flag to false
 * 
 * This allows the SocketManager to be reused for a new set of servers
 * or ensures proper cleanup when shutting down.
 * 
 * @note This function is automatically called by the destructor
 * @note After calling this function, the manager must be reinitialized before use
 * @note All socket file descriptors are properly closed to prevent resource leaks
 * 
 * @example
 * manager.close_all_sockets();
 * // Manager is now in uninitialized state
 * // Can be reinitialized with new server configurations
 */
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

/**
 * @brief Checks if the SocketManager has been initialized with server sockets
 * 
 * Returns the current initialization state of the SocketManager.
 * This can be used to determine if the manager is ready to handle
 * socket operations or if it needs to be initialized first.
 * 
 * @return bool True if the manager has been initialized, false otherwise
 * 
 * @note This is useful for checking the manager state before performing operations
 * @note The flag is set to true after successful initialization and false after cleanup
 * 
 * @example
 * if (!manager.is_initialized()) {
 *     // Initialize the manager first
 *     manager.initialize_sockets(servers);
 * }
 */
bool SocketManager::is_initialized() const {
    return initialized;
}

/**
 * @brief Sets up a single server socket for the given server configuration
 * 
 * This function creates and configures a listening socket for a specific server.
 * It performs all the necessary steps to make the socket ready for accepting
 * incoming connections:
 * - Creates a TCP socket
 * - Sets socket options (address reuse, non-blocking mode)
 * - Binds the socket to the server's port
 * - Starts listening for connections
 * - Stores the socket and configuration mapping
 * 
 * @param server The ServerConfig object containing the server's configuration
 * @return bool True if the socket was set up successfully, false otherwise
 * 
 * @throws std::runtime_error (via error logging) if socket operations fail
 * 
 * @note The socket is bound to INADDR_ANY (all interfaces)
 * @note The socket is set to non-blocking mode for event-driven operation
 * @note The socket is configured to reuse addresses to avoid "Address already in use" errors
 * @note The socket file descriptor and configuration are stored for later use
 * 
 * @example
 * ServerConfig server;
 * server.listen_port = 8080;
 * if (manager.setup_server_socket(server)) {
 *     // Socket is ready to accept connections on port 8080
 * }
 */
bool SocketManager::setup_server_socket(const ServerConfig& server) {
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
    
    // Store socket and configuration
    server_sockets.push_back(socket_fd);
    socket_to_config[socket_fd] = server;
    
    std::cout << "Server socket created and listening on port " 
              << server.listen_port << " (fd: " << socket_fd << ")" << std::endl;
    
    return true;
}

/**
 * @brief Sets a socket to non-blocking mode
 * 
 * Configures a socket file descriptor to operate in non-blocking mode.
 * This is essential for event-driven I/O operations where the application
 * should not block waiting for socket operations to complete.
 * 
 * @param socket_fd The socket file descriptor to configure
 * @return bool True if the operation was successful, false otherwise
 * 
 * @throws std::runtime_error (via error logging) if fcntl operations fail
 * 
 * @note Non-blocking mode allows select(), poll(), or epoll() to work properly
 * @note In non-blocking mode, socket operations return immediately if they would block
 * @note This function modifies the socket's file descriptor flags using fcntl()
 * 
 * @example
 * int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
 * if (manager.set_non_blocking(socket_fd)) {
 *     // Socket is now in non-blocking mode
 * }
 */
bool SocketManager::set_non_blocking(int socket_fd) {
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags < 0) {
        std::cerr << "Failed to get socket flags: " << strerror(errno) << std::endl;
        return false;
    }
    
    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "Failed to set non-blocking mode: " << strerror(errno) << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief Binds a socket to a specific port on all interfaces
 * 
 * Associates a socket file descriptor with a specific port number
 * on all available network interfaces (INADDR_ANY). This makes the
 * socket ready to accept incoming connections on the specified port.
 * 
 * @param socket_fd The socket file descriptor to bind
 * @param port The port number to bind the socket to (host byte order)
 * @return bool True if the binding was successful, false otherwise
 * 
 * @throws std::runtime_error (via error logging) if bind() operation fails
 * 
 * @note The socket must be created before calling this function
 * @note The port is converted to network byte order internally
 * @note Binding to INADDR_ANY means the socket will accept connections on all interfaces
 * @note This function is typically called after creating a socket and before listening
 * 
 * @example
 * int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
 * if (manager.bind_socket(socket_fd, 8080)) {
 *     // Socket is now bound to port 8080
 * }
 */
bool SocketManager::bind_socket(int socket_fd, int port) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket to port " << port 
                  << ": " << strerror(errno) << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief Starts listening for incoming connections on a socket
 * 
 * Configures a bound socket to accept incoming connections by placing it
 * in a listening state. This is the final step in setting up a server socket
 * before it can accept client connections.
 * 
 * @param socket_fd The socket file descriptor to start listening on
 * @return bool True if the socket started listening successfully, false otherwise
 * 
 * @throws std::runtime_error (via error logging) if listen() operation fails
 * 
 * @note The socket must be bound to a port before calling this function
 * @note Uses SOMAXCONN as the backlog size (maximum pending connections)
 * @note After this call, the socket is ready to accept() incoming connections
 * @note This is typically the last step in server socket setup
 * 
 * @example
 * int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
 * manager.bind_socket(socket_fd, 8080);
 * if (manager.listen_socket(socket_fd)) {
 *     // Socket is now listening for connections on port 8080
 *     int client_fd = accept(socket_fd, ...);
 * }
 */
bool SocketManager::listen_socket(int socket_fd) {
    if (listen(socket_fd, SOMAXCONN) < 0) {
        std::cerr << "Failed to listen on socket: " << strerror(errno) << std::endl;
        return false;
    }
    
    return true;
} 