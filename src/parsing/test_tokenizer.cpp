#include "webserv.hpp" // IWYU pragma: keep.


std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TOKEN_WORD: return "WORD";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_COMMENT: return "COMMENT";
        case TOKEN_EOF: return "EOF";
        default: return "UNKNOWN";
    }
}

int main() {
    std::ifstream file("configs/default.conf");
    if (!file) {
        std::cerr << "Failed to open config file." << std::endl;
        return 1;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::vector<Token> tokens = tokenize(content);
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << tokenTypeToString(tokens[i].type) << ": '" << tokens[i].value << "'" << std::endl;
    }
    return 0;
} 