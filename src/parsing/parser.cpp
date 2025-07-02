#include "webserv.hpp"
#include "tokenizer.hpp"
#include "parser.hpp"
#include <stdexcept>
#include <algorithm>

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

    bool isTrue(const std::string& val) {
        return val == "on" || val == "true" || val == "1";
    }

    bool isFalse(const std::string& val) {
        return val == "off" || val == "false" || val == "0";
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
            if (ts.peek().type == TOKEN_WORD) {
                std::string directive = ts.peek().value;
                ts.next();
                if (directive == "root") {
                    expect(ts, TOKEN_WORD, "root value");
                    loc.root = ts.next().value;
                    expect(ts, TOKEN_SEMICOLON, "; after root");
                    ts.next();
                } else if (directive == "index") {
                    // index can have multiple values
                    while (ts.peek().type == TOKEN_WORD) {
                        loc.index.push_back(ts.next().value);
                    }
                    expect(ts, TOKEN_SEMICOLON, "; after index");
                    ts.next();
                } else if (directive == "autoindex") {
                    expect(ts, TOKEN_WORD, "autoindex value");
                    std::string val = ts.next().value;
                    if (isTrue(val)) loc.autoindex = true;
                    else if (isFalse(val)) loc.autoindex = false;
                    else throw std::runtime_error("Parse error: invalid value for autoindex: '" + val + "'");
                    expect(ts, TOKEN_SEMICOLON, "; after autoindex");
                    ts.next();
                } else if (directive == "allow_methods") {
                    // allow_methods can have multiple values
                    while (ts.peek().type == TOKEN_WORD) {
                        loc.allow_methods.push_back(ts.next().value);
                    }
                    expect(ts, TOKEN_SEMICOLON, "; after allow_methods");
                    ts.next();
                } else if (directive == "upload_store") {
                    expect(ts, TOKEN_WORD, "upload_store value");
                    loc.upload_store = ts.next().value;
                    expect(ts, TOKEN_SEMICOLON, "; after upload_store");
                    ts.next();
                } else if (directive == "cgi_pass") {
                    expect(ts, TOKEN_WORD, "cgi_pass value");
                    loc.cgi_pass = ts.next().value;
                    expect(ts, TOKEN_SEMICOLON, "; after cgi_pass");
                    ts.next();
                } else {
                    // Unknown directive, skip to next semicolon
                    while (ts.peek().type != TOKEN_SEMICOLON && ts.peek().type != TOKEN_RBRACE && !ts.eof())
                        ts.next();
                    if (ts.peek().type == TOKEN_SEMICOLON)
                        ts.next();
                }
            } else {
                // Skip non-word tokens (shouldn't happen in valid config)
                ts.next();
            }
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