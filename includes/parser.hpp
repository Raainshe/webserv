/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 15:14:44 by rmakoni           #+#    #+#             */
/*   Updated: 2025/07/02 16:02:28 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
#define PARSER_HPP

#include "structs/main_config.hpp"
#include "webserv.hpp"
#include "tokenizer.hpp"

// Parses a vector of tokens into a MainConfig object.
// Throws std::runtime_error on syntax or semantic errors.
MainConfig parseConfig(const std::vector<Token>& tokens);

#endif // PARSER_HPP 