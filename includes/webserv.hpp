/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 14:38:19 by rmakoni           #+#    #+#             */
/*   Updated: 2025/07/02 14:51:26 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cstddef>

struct LocationConfig {
    std::string path;
    std::string root;
    std::vector<std::string> index;
    bool autoindex;
    std::vector<std::string> allow_methods;
    std::string upload_store;
    std::string cgi_pass;
};

struct ServerConfig {
    int listen_port;
    std::string server_name;
    std::map<int, std::string> error_pages;
    size_t client_max_body_size;
    std::vector<LocationConfig> locations;
};

struct MainConfig {
    std::vector<ServerConfig> servers;
};

//parsing.cpp
int parse_config(std::string config_file);


#endif