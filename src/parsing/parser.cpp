#include "webserv.hpp"
#include "tokenizer.hpp"
#include "parser.hpp"
#include <stdexcept>

namespace {
    // Helper to advance and check tokens
    struct TokenStream {
        const std::vector<Token>& tokens;
        size_t pos;
        TokenStream(const std::vector<Token>& t) : tokens(t), pos(0) {}
        const Token& peek() const { return tokens[pos]; }
        const Token& next() { return tokens[pos++]; }
        bool eof() const { return tokens[pos].type == TOKEN_EOF; }
    };

    void expect(TokenStream& ts, TokenType type, const std::string& msg) {
        if (ts.peek().type != type)
            throw std::runtime_error("Parse error: expected " + msg + ", got '" + ts.peek().value + "'");
    }

    LocationConfig parseLocation(TokenStream& ts) {
        LocationConfig loc;
        // Expect: location <path> { ... }
        expect(ts, TOKEN_WORD, "'location'");
        ts.next(); // 'location'
        expect(ts, TOKEN_WORD, "location path");
        loc.path = ts.next().value;
        expect(ts, TOKEN_LBRACE, "'{' after location path");
        ts.next(); // '{'
        // Parse directives inside location block
        while (ts.peek().type != TOKEN_RBRACE && !ts.eof()) {
            // TODO: parse location directives
            // For now, skip to next semicolon or brace
            while (ts.peek().type != TOKEN_SEMICOLON && ts.peek().type != TOKEN_RBRACE && !ts.eof())
                ts.next();
            if (ts.peek().type == TOKEN_SEMICOLON)
                ts.next();
        }
        expect(ts, TOKEN_RBRACE, "'}' to close location block");
        ts.next(); // '}'
        return loc;
    }

    ServerConfig parseServer(TokenStream& ts) {
        ServerConfig srv;
        // Expect: server { ... }
        expect(ts, TOKEN_WORD, "'server'");
        ts.next(); // 'server'
        expect(ts, TOKEN_LBRACE, "'{' after server");
        ts.next(); // '{'
        // Parse directives inside server block
        while (ts.peek().type != TOKEN_RBRACE && !ts.eof()) {
            if (ts.peek().type == TOKEN_WORD && ts.peek().value == "location") {
                LocationConfig loc = parseLocation(ts);
                srv.locations.push_back(loc);
            } else {
                // TODO: parse server directives
                // For now, skip to next semicolon or brace
                while (ts.peek().type != TOKEN_SEMICOLON && ts.peek().type != TOKEN_RBRACE && !ts.eof())
                    ts.next();
                if (ts.peek().type == TOKEN_SEMICOLON)
                    ts.next();
            }
        }
        expect(ts, TOKEN_RBRACE, "'}' to close server block");
        ts.next(); // '}'
        return srv;
    }
}

MainConfig parseConfig(const std::vector<Token>& tokens) {
    TokenStream ts(tokens);
    MainConfig config;
    while (!ts.eof()) {
        if (ts.peek().type == TOKEN_WORD && ts.peek().value == "server") {
            ServerConfig srv = parseServer(ts);
            config.servers.push_back(srv);
        } else if (ts.peek().type == TOKEN_EOF) {
            break;
        } else {
            throw std::runtime_error("Parse error: expected 'server' block at top level, got '" + ts.peek().value + "'");
        }
    }
    return config;
} 