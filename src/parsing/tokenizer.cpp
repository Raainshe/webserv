#include "webserv.hpp" // IWYU pragma: keep.

std::vector<Token> tokenize(const std::string& input) {
    std::vector<Token> tokens;
    size_t i = 0;
    while (i < input.size()) {
        // Skip whitespace
        if (isspace(input[i])) {
            ++i;
            continue;
        }
        // Comments (start with #, go to end of line)
        if (input[i] == '#') {
            size_t start = i;
            while (i < input.size() && input[i] != '\n') ++i;
            Token t; t.type = TOKEN_COMMENT; t.value = input.substr(start, i - start); tokens.push_back(t);
            continue;
        }
        // Braces
        if (input[i] == '{') {
            Token t; t.type = TOKEN_LBRACE; t.value = "{"; tokens.push_back(t);
            ++i;
            continue;
        }
        if (input[i] == '}') {
            Token t; t.type = TOKEN_RBRACE; t.value = "}"; tokens.push_back(t);
            ++i;
            continue;
        }
        // Semicolon
        if (input[i] == ';') {
            Token t; t.type = TOKEN_SEMICOLON; t.value = ";"; tokens.push_back(t);
            ++i;
            continue;
        }
        // Words (directive names, values, etc.)
        if (isalnum(input[i]) || input[i] == '/' || input[i] == '.' || input[i] == '_' || input[i] == '-') {
            size_t start = i;
            while (i < input.size() && (isalnum(input[i]) || input[i] == '/' || input[i] == '.' || input[i] == '_' || input[i] == '-')) ++i;
            Token t; t.type = TOKEN_WORD; t.value = input.substr(start, i - start); tokens.push_back(t);
            continue;
        }
        // Quoted strings (optional, for future extension)
        if (input[i] == '"') {
            size_t start = ++i;
            while (i < input.size() && input[i] != '"') ++i;
            Token t; t.type = TOKEN_WORD; t.value = input.substr(start, i - start); tokens.push_back(t);
            if (i < input.size()) ++i; // skip closing quote
            continue;
        }
        // Unknown character, skip
        ++i;
    }
    Token t; t.type = TOKEN_EOF; t.value = ""; tokens.push_back(t);
    return tokens;
} 