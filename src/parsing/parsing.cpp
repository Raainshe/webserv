/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parsing.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 14:18:50 by rmakoni           #+#    #+#             */
/*   Updated: 2025/07/03 13:36:11 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"

int parse_config(std::string config_file)
{
    //check if the file exists
    std::ifstream file(config_file.c_str());
    if (!file.is_open())
    {
        std::cerr << "Error: Config file does not exist" << std::endl;
        return (1);
    }
    //read the file
    // std::string line;
    // TODO: Implement actual parsing logic
    return (0);
}