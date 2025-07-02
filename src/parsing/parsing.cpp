/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parsing.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 14:18:50 by rmakoni           #+#    #+#             */
/*   Updated: 2025/07/02 14:37:06 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"

int parse_config(std::string config_file)
{
    //check if the file exists
    if (!std::ifstream(config_file))
    {
        std::cerr << "Error: Config file does not exist" << std::endl;
        return (1);
    }
    //read the file
    // std::ifstream file(config_file);
    // std::string line;
}