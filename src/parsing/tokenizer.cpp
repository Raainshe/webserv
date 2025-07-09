#include "webserv.hpp" // IWYU pragma: keep.

/**
 * @brief Tokenizes a configuration file string into a sequence of tokens
 * 
 * This function parses the input string character by character and converts it into
 * a vector of Token objects. It handles various token types including:
 * - Whitespace (skipped)
 * - Comments (lines starting with #)
 * - Braces ({ and })
 * - Semicolons (;)
 * - Words (alphanumeric characters, slashes, dots, underscores, hyphens)
 * - Quoted strings (double quotes)
 * - Unknown characters (skipped)
 * 
 * The function implements a simple lexical analyzer that recognizes the basic
 * syntax elements needed for parsing nginx-style configuration files.
 * 
 * @param input The configuration file content as a string
 * @return std::vector<Token> A vector of tokens representing the parsed input
 * 
 * @note The function automatically adds a TOKEN_EOF token at the end of the token stream
 * @note Comments are preserved as tokens but can be ignored during parsing
 * @note Quoted strings are treated as TOKEN_WORD tokens with the quotes removed
 * @note Unknown characters are silently skipped to provide robustness
 * 
 * @example
 * Input: "server { listen 8080; }"
 * Output: [TOKEN_WORD("server"), TOKEN_LBRACE, TOKEN_WORD("listen"), 
 *          TOKEN_WORD("8080"), TOKEN_SEMICOLON, TOKEN_RBRACE, TOKEN_EOF]
 */
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