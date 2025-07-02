#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <string>
#include <vector>

enum TokenType {
    TOKEN_WORD,
    TOKEN_LBRACE,   // {
    TOKEN_RBRACE,   // }
    TOKEN_SEMICOLON,// ;
    TOKEN_COMMENT,
    TOKEN_EOF
};

struct Token {
    TokenType type;
    std::string value;
};

std::vector<Token> tokenize(const std::string& input);

#endif // TOKENIZER_HPP 