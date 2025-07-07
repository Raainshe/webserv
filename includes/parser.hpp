/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 14:38:19 by rmakoni           #+#    #+#             */
/*   Updated: 2025/07/07 12:48:04 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>
#include "tokenizer.hpp"
#include "structs/main_config.hpp"

// Main parsing function that takes tokens and returns parsed configuration
MainConfig parseConfig(const std::vector<Token>& tokens);

 // Helper to advance and check tokens
 struct TokenStream {
    const std::vector<Token>& tokens;
    size_t pos;
    TokenStream(const std::vector<Token>& t) : tokens(t), pos(0) {}
    const Token& peek() const { return tokens[pos]; }
    const Token& next() { return tokens[pos++]; }
    bool eof() const { return tokens[pos].type == TOKEN_EOF; }
};

#endif // PARSER_HPP 