/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ksinn <ksinn@student.42heilbronn.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 14:38:19 by rmakoni           #+#    #+#             */
/*   Updated: 2025/08/04 18:09:43 by ksinn            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <algorithm> // IWYU pragma: keep
#include <algorithm>
#include <cstddef>   // IWYU pragma: keep
#include <cstdlib>   // IWYU pragma: keep
#include <cstring>   // for strerror and memset
#include <errno.h>   // IWYU pragma: keep
#include <fstream>   // IWYU pragma: keep
#include <iostream>  // IWYU pragma: keep
#include <map>       // IWYU pragma: keep
#include <set>       // IWYU pragma: keep
#include <sstream>   // IWYU pragma: keep
#include <stdexcept> // IWYU pragma: keep
#include <string>    // IWYU pragma: keep
#include <time.h>    // IWYU pragma: keep#include <fcntl.h>
#include <unistd.h>
#include <vector> // IWYU pragma: keep

// headers
#include "http/routing.hpp"                 // IWYU pragma: keep
#include "networking/client_connection.hpp" // IWYU pragma: keep
#include "networking/event_loop.hpp"        // IWYU pragma: keep
#include "networking/socket_manager.hpp"    // IWYU pragma: keep
#include "parser.hpp"                       // IWYU pragma: keep
#include "tokenizer.hpp"                    // IWYU pragma: keep

// structs
#include "structs/location_config.hpp" // IWYU pragma: keep
#include "structs/main_config.hpp"     // IWYU pragma: keep
#include "structs/server_config.hpp"   // IWYU pragma: keep

// parsing.cpp
int parse_config(std::string config_file, std::vector<ServerConfig> &servers);

#endif
