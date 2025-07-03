/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 14:38:19 by rmakoni           #+#    #+#             */
/*   Updated: 2025/07/03 13:36:11 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <iostream> // IWYU pragma: keep
#include <fstream> // IWYU pragma: keep
#include <sstream> // IWYU pragma: keep
#include <string> // IWYU pragma: keep
#include <vector> // IWYU pragma: keep
#include <map> // IWYU pragma: keep
#include <cstddef> // IWYU pragma: keep
#include <stdexcept> // IWYU pragma: keep
#include <algorithm> // IWYU pragma: keep
#include <cstdlib> // IWYU pragma: keep
#include <set> // IWYU pragma: keep


//headers
#include "tokenizer.hpp" // IWYU pragma: keep
#include "parser.hpp" // IWYU pragma: keep
#include "networking/socket_manager.hpp" // IWYU pragma: keep

//structs
#include "structs/location_config.hpp" // IWYU pragma: keep
#include "structs/server_config.hpp" // IWYU pragma: keep
#include "structs/main_config.hpp" // IWYU pragma: keep

//parsing.cpp
int parse_config(std::string config_file);


#endif