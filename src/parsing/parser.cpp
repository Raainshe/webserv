#include "webserv.hpp" // IWYU pragma: keep

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

    size_t parseSizeWithSuffix(const std::string& str) {
        char* end;
        long long val = std::strtoll(str.c_str(), &end, 10);
        if (end == str.c_str())
            throw std::runtime_error("Parse error: invalid size value '" + str + "'");
        if (*end == 'K' || *end == 'k') val *= 1024LL;
        else if (*end == 'M' || *end == 'm') val *= 1024LL * 1024LL;
        else if (*end == 'G' || *end == 'g') val *= 1024LL * 1024LL * 1024LL;
        else if (*end != '\0')
            throw std::runtime_error("Parse error: invalid size suffix in '" + str + "'");
        if (val < 0) throw std::runtime_error("Parse error: negative size value");
        return static_cast<size_t>(val);
    }

    // Value validation helpers
    bool isValidPort(int port) {
        return port >= 1 && port <= 65535;
    }

    bool isValidHttpMethod(const std::string& method) {
        static const std::string valid_methods[] = {"GET", "POST", "DELETE", "PUT", "HEAD", "OPTIONS", "TRACE", "CONNECT"};
        for (size_t i = 0; i < sizeof(valid_methods)/sizeof(valid_methods[0]); ++i) {
            if (method == valid_methods[i]) return true;
        }
        return false;
    }

    bool isValidErrorCode(int code) {
        return code >= 400 && code <= 599;
    }

    void validateHttpMethods(const std::vector<std::string>& methods) {
        std::set<std::string> seen;
        for (size_t i = 0; i < methods.size(); ++i) {
            if (!isValidHttpMethod(methods[i])) {
                throw std::runtime_error("Parse error: invalid HTTP method '" + methods[i] + "'");
            }
            if (seen.count(methods[i])) {
                throw std::runtime_error("Parse error: duplicate HTTP method '" + methods[i] + "'");
            }
            seen.insert(methods[i]);
        }
    }

    LocationConfig parseLocation(TokenStream& ts) {
        LocationConfig loc;
        bool seen_root = false, seen_autoindex = false, seen_upload_store = false, seen_cgi_pass = false;
        std::set<std::string> seen_directives;
        expect(ts, TOKEN_WORD, "'location'");
        ts.next(); // 'location'
        expect(ts, TOKEN_WORD, "location path");
        loc.path = ts.next().value;
        expect(ts, TOKEN_LBRACE, "'{' after location path");
        ts.next(); // '{'
        while (ts.peek().type != TOKEN_RBRACE && !ts.eof()) {
            if (ts.peek().type == TOKEN_WORD) {
                std::string directive = ts.peek().value;
                ts.next();
                if (directive == "root") {
                    if (seen_root) throw std::runtime_error("Duplicate 'root' directive in location block");
                    seen_root = true;
                    expect(ts, TOKEN_WORD, "root value");
                    loc.root = ts.next().value;
                    expect(ts, TOKEN_SEMICOLON, "; after root");
                    ts.next();
                } else if (directive == "index") {
                    if (seen_directives.count("index")) throw std::runtime_error("Duplicate 'index' directive in location block");
                    seen_directives.insert("index");
                    while (ts.peek().type == TOKEN_WORD) {
                        loc.index.push_back(ts.next().value);
                    }
                    expect(ts, TOKEN_SEMICOLON, "; after index");
                    ts.next();
                } else if (directive == "autoindex") {
                    if (seen_autoindex) throw std::runtime_error("Duplicate 'autoindex' directive in location block");
                    seen_autoindex = true;
                    expect(ts, TOKEN_WORD, "autoindex value");
                    std::string val = ts.next().value;
                    if (isTrue(val)) loc.autoindex = true;
                    else if (isFalse(val)) loc.autoindex = false;
                    else throw std::runtime_error("Parse error: invalid value for autoindex: '" + val + "'");
                    expect(ts, TOKEN_SEMICOLON, "; after autoindex");
                    ts.next();
                } else if (directive == "allow_methods") {
                    if (seen_directives.count("allow_methods")) throw std::runtime_error("Duplicate 'allow_methods' directive in location block");
                    seen_directives.insert("allow_methods");
                    while (ts.peek().type == TOKEN_WORD) {
                        loc.allow_methods.push_back(ts.next().value);
                    }
                    validateHttpMethods(loc.allow_methods);
                    expect(ts, TOKEN_SEMICOLON, "; after allow_methods");
                    ts.next();
                } else if (directive == "upload_store") {
                    if (seen_upload_store) throw std::runtime_error("Duplicate 'upload_store' directive in location block");
                    seen_upload_store = true;
                    expect(ts, TOKEN_WORD, "upload_store value");
                    loc.upload_store = ts.next().value;
                    expect(ts, TOKEN_SEMICOLON, "; after upload_store");
                    ts.next();
                } else if (directive == "cgi_pass") {
                    if (seen_cgi_pass) throw std::runtime_error("Duplicate 'cgi_pass' directive in location block");
                    seen_cgi_pass = true;
                    expect(ts, TOKEN_WORD, "cgi_pass value");
                    loc.cgi_pass = ts.next().value;
                    expect(ts, TOKEN_SEMICOLON, "; after cgi_pass");
                    ts.next();
                } else {
                    while (ts.peek().type != TOKEN_SEMICOLON && ts.peek().type != TOKEN_RBRACE && !ts.eof())
                        ts.next();
                    if (ts.peek().type == TOKEN_SEMICOLON)
                        ts.next();
                }
            } else {
                ts.next();
            }
        }
        expect(ts, TOKEN_RBRACE, "'}' to close location block");
        ts.next(); // '}'
        return loc;
    }

    ServerConfig parseServer(TokenStream& ts) {
        ServerConfig srv;
        bool seen_listen = false, seen_server_name = false, seen_client_max_body_size = false;
        std::set<std::string> seen_directives;
        expect(ts, TOKEN_WORD, "'server'");
        ts.next(); // 'server'
        expect(ts, TOKEN_LBRACE, "'{' after server");
        ts.next(); // '{'
        while (ts.peek().type != TOKEN_RBRACE && !ts.eof()) {
            if (ts.peek().type == TOKEN_WORD && ts.peek().value == "location") {
                LocationConfig loc = parseLocation(ts);
                srv.locations.push_back(loc);
            } else if (ts.peek().type == TOKEN_WORD) {
                std::string directive = ts.peek().value;
                ts.next();
                if (directive == "listen") {
                    if (seen_listen) throw std::runtime_error("Duplicate 'listen' directive in server block");
                    seen_listen = true;
                    expect(ts, TOKEN_WORD, "listen value");
                    int port = std::atoi(ts.next().value.c_str());
                    if (!isValidPort(port)) {
                        throw std::runtime_error("Parse error: invalid port number '" + ts.peek().value + "' (must be 1-65535)");
                    }
                    srv.listen_port = port;
                    expect(ts, TOKEN_SEMICOLON, "; after listen");
                    ts.next();
                } else if (directive == "server_name") {
                    if (seen_server_name) throw std::runtime_error("Duplicate 'server_name' directive in server block");
                    seen_server_name = true;
                    expect(ts, TOKEN_WORD, "server_name value");
                    srv.server_name = ts.next().value;
                    expect(ts, TOKEN_SEMICOLON, "; after server_name");
                    ts.next();
                } else if (directive == "error_page") {
                    expect(ts, TOKEN_WORD, "error code");
                    int code = std::atoi(ts.next().value.c_str());
                    if (!isValidErrorCode(code)) {
                        throw std::runtime_error("Parse error: invalid error code '" + ts.peek().value + "' (must be 400-599)");
                    }
                    expect(ts, TOKEN_WORD, "error page path");
                    std::string path = ts.next().value;
                    srv.error_pages[code] = path;
                    expect(ts, TOKEN_SEMICOLON, "; after error_page");
                    ts.next();
                } else if (directive == "client_max_body_size") {
                    if (seen_client_max_body_size) throw std::runtime_error("Duplicate 'client_max_body_size' directive in server block");
                    seen_client_max_body_size = true;
                    expect(ts, TOKEN_WORD, "client_max_body_size value");
                    srv.client_max_body_size = parseSizeWithSuffix(ts.next().value);
                    expect(ts, TOKEN_SEMICOLON, "; after client_max_body_size");
                    ts.next();
                } else {
                    while (ts.peek().type != TOKEN_SEMICOLON && ts.peek().type != TOKEN_RBRACE && !ts.eof())
                        ts.next();
                    if (ts.peek().type == TOKEN_SEMICOLON)
                        ts.next();
                }
            } else {
                ts.next();
            }
        }
        expect(ts, TOKEN_RBRACE, "'}' to close server block");
        ts.next(); // '}'
        if (!seen_listen) throw std::runtime_error("Missing required 'listen' directive in server block");
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