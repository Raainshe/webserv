# webserv

A high-performance HTTP/1.1 web server implementation in C++17, built as part of the 42 School curriculum. This project implements a non-blocking, event-driven web server capable of serving static content, handling CGI scripts, and managing file uploads.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Requirements](#requirements)
- [Building](#building)
- [Usage](#usage)
- [Configuration](#configuration)
- [Testing](#testing)
- [Project Structure](#project-structure)
- [Authors](#authors)

## Overview

webserv is a fully-featured HTTP server written in C++17 that demonstrates low-level network programming concepts and HTTP protocol implementation. The server is designed to be:

- **Non-blocking**: Uses poll/select/kqueue/epoll for efficient I/O operations
- **Configurable**: NGINX-inspired configuration file system
- **Standards-compliant**: HTTP/1.1 compatible with accurate status codes
- **Resilient**: Stress-tested to remain available under heavy load

## Features

### Core HTTP Features
- ✅ **HTTP Methods**: GET, POST, DELETE support
- ✅ **Static File Serving**: Efficiently serves static websites
- ✅ **File Uploads**: Client file upload capabilities
- ✅ **Directory Listing**: Optional directory browsing
- ✅ **Error Pages**: Default and custom error page support
- ✅ **HTTP Redirections**: Configurable URL redirections

### Advanced Features
- ✅ **CGI Support**: Execute CGI scripts (PHP, Python, etc.)
- ✅ **Multiple Servers**: Multi-port and virtual host support
- ✅ **Non-blocking I/O**: Single-threaded event-driven architecture
- ✅ **Request Body Limiting**: Configurable client body size limits
- ✅ **Chunked Transfer Encoding**: Handle chunked requests properly

### Configuration
- ✅ **NGINX-style Config**: Familiar configuration syntax
- ✅ **Virtual Hosts**: Multiple server blocks with different host:port combinations
- ✅ **Route Configuration**: Flexible URL routing with location blocks
- ✅ **Default Server**: First server block acts as default for host:port

## Requirements

### System Requirements
- Unix-like operating system (Linux, macOS)
- C++17 compatible compiler (g++, clang++)
- Make utility

### External Functions Used
The server is built using only these allowed system calls:
- **Network**: `socket`, `accept`, `listen`, `send`, `recv`, `bind`, `connect`
- **I/O Multiplexing**: `select`, `poll`, `epoll_*`, `kqueue`, `kevent`
- **Process Management**: `fork`, `execve`, `waitpid`, `kill`, `signal`
- **File Operations**: `open`, `close`, `read`, `write`, `access`, `stat`
- **Directory Operations**: `opendir`, `readdir`, `closedir`, `chdir`
- **Utilities**: `dup`, `dup2`, `pipe`, `fcntl`, `strerror`, `errno`

## Building

### Compilation

```bash
# Clone the repository
git clone [repository-url]
cd webserv

# Build the project
make

# Clean object files
make clean

# Full clean (removes executable)
make fclean

# Rebuild everything
make re
```

### Makefile Targets
- `make` or `make all`: Build the webserv executable
- `make clean`: Remove object files
- `make fclean`: Remove object files and executable
- `make re`: Full rebuild

## Usage

### Basic Usage

```bash

# Run with custom configuration file
./webserv configs/custom.conf
```

### Example Configurations

The project includes several example configuration files in the `configs/` directory:

- **`default.conf`**: Basic single-server configuration
- **`multi_server_test.conf`**: Multiple virtual hosts example
- **`cgi_test.conf`**: CGI execution configuration
- **`upload_delete.conf`**: File upload and deletion setup

### Testing the Server

```bash
# Start the server
./webserv configs/default.conf

# Test with curl
curl http://localhost:8080
curl -X POST -d "data" http://localhost:8080/upload
curl -X DELETE http://localhost:8080/file.txt

# Test with a web browser
open http://localhost:8080
```

## Configuration

The configuration file uses NGINX-inspired syntax:

```nginx
server {
    listen 8080;
    server_name localhost;
    root ./test_files/www;
    index index.html;
    client_max_body_size 1M;

    error_page 404 /404.html;
    error_page 500 502 503 504 /50x.html;

    location / {
        allow_methods GET POST;
        autoindex on;
    }

    location /cgi-bin/ {
        allow_methods GET POST;
        cgi_extension .py;
        cgi_path /usr/bin/python3;
        root ./test_files;
    }

    location /upload {
        allow_methods POST DELETE;
        upload_path ./test_files/uploads;
    }
}
```

### Configuration Directives

#### Server Block
- `listen`: Port number to listen on
- `server_name`: Server name (virtual host)
- `root`: Document root directory
- `index`: Default index file
- `client_max_body_size`: Maximum request body size
- `error_page`: Custom error pages

#### Location Block
- `allow_methods`: Allowed HTTP methods
- `return`: HTTP redirection
- `root`: Override document root for this location
- `autoindex`: Enable/disable directory listing
- `index`: Default file for directory requests
- `cgi_extension`: File extension for CGI execution
- `cgi_path`: Path to CGI interpreter
- `upload_path`: Directory for uploaded files

## Testing

### Manual Testing

```bash
# Test static file serving
curl http://localhost:8080/index.html

# Test directory listing
curl http://localhost:8080/images/

# Test CGI execution
curl http://localhost:8080/cgi-bin/hello.py

# Test file upload
curl -X POST -F "file=@test.txt" http://localhost:8080/upload

# Test file deletion
curl -X DELETE http://localhost:8080/uploads/test.txt
```

### Stress Testing

The server is designed to handle high load. Test with tools like:
- Apache Bench (ab)
- curl with multiple concurrent connections
- Browser with multiple tabs

## Project Structure

```
webserv/
├── Makefile                 # Build configuration
├── README.md               # This file
├── subject.txt             # Project requirements
├── configs/                # Configuration files
│   ├── default.conf
│   ├── cgi_test.conf
│   ├── multi_server_test.conf
│   └── upload_delete.conf
├── includes/               # Header files
│   ├── webserv.hpp
│   ├── parser.hpp
│   ├── tokenizer.hpp
│   ├── structs/
│   ├── http/
│   └── networking/
├── src/                    # Source files
│   ├── webserv.cpp
│   ├── parsing/
│   ├── http/
│   └── networking/
└── test_files/             # Test content
    ├── www/                # Static files
    ├── cgi-bin/           # CGI scripts
    ├── uploads/           # Upload directory
    └── api/               # API test files
```

## Authors

This project was developed by:

- **Ryan Makoni** ([@Raainshe](https://github.com/Raainshe/))
- **Halime Pehlivan** ([@phlvnhalime](https://github.com/phlvnhalime))
- **Kevin Sinn** ([@Fearcon14](https://github.com/Fearcon14))

---

**Note**: This project is part of the 42 School curriculum and implements a subset of HTTP/1.1 functionality. While feature-complete for educational purposes, it's not intended for production use.

For questions about implementation details or configuration, please refer to the NGINX documentation as our configuration syntax is largely compatible.
