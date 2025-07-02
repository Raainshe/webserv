/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 14:38:19 by rmakoni           #+#    #+#             */
/*   Updated: 2025/07/02 16:00:26 by rmakoni          ###   ########.fr       */
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
#include <stdexcept>
#include <algorithm>
#include <cstdlib>

//headers
#include "tokenizer.hpp"
#include "parser.hpp"
#include "conf.hpp"

//structs
#include "structs/location_config.hpp"
#include "structs/server_config.hpp"
#include "structs/main_config.hpp"

//parsing.cpp
int parse_config(std::string config_file);


#endif