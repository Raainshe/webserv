#include "webserv.hpp" // IWYU pragma: keep

namespace {
    /**
     * @brief Expects a specific token type and throws an error if not found
     * 
     * This helper function validates that the next token in the token stream
     * matches the expected type. If the token type doesn't match, it throws
     * a runtime_error with a descriptive message.
     * 
     * @param ts Reference to the token stream to check
     * @param type The expected token type
     * @param msg A descriptive message to include in the error if validation fails
     * 
     * @throws std::runtime_error if the next token doesn't match the expected type
     * 
     * @note This function only checks the token type, it doesn't consume the token
     * @note The error message includes both the expected token and the actual token found
     */
    void expect(TokenStream& ts, TokenType type, const std::string& msg) {
        if (ts.peek().type != type)
            throw std::runtime_error("Parse error: expected " + msg + ", got '" + ts.peek().value + "'");
    }

    /**
     * @brief Checks if a string represents a true value
     * 
     * Validates whether the input string represents a boolean true value.
     * Accepts common representations: "on", "true", and "1".
     * 
     * @param val The string to check
     * @return bool True if the string represents a true value, false otherwise
     * 
     * @note Case-sensitive comparison
     * @note This function is used for parsing boolean configuration directives
     */
    bool isTrue(const std::string& val) {
        return val == "on" || val == "true" || val == "1";
    }

    /**
     * @brief Checks if a string represents a false value
     * 
     * Validates whether the input string represents a boolean false value.
     * Accepts common representations: "off", "false", and "0".
     * 
     * @param val The string to check
     * @return bool True if the string represents a false value, false otherwise
     * 
     * @note Case-sensitive comparison
     * @note This function is used for parsing boolean configuration directives
     */
    bool isFalse(const std::string& val) {
        return val == "off" || val == "false" || val == "0";
    }

    /**
     * @brief Parses a size string with optional suffix (K, M, G)
     * 
     * Converts a string representation of a size into a size_t value.
     * Supports optional suffixes for kilobytes (K/k), megabytes (M/m), and gigabytes (G/g).
     * 
     * @param str The size string to parse (e.g., "1024", "1K", "2M", "1G")
     * @return size_t The parsed size in bytes
     * 
     * @throws std::runtime_error if the string is not a valid number or has an invalid suffix
     * @throws std::runtime_error if the resulting value is negative
     * 
     * @note Suffixes are case-insensitive (K/k, M/m, G/g)
     * @note Multipliers: K=1024, M=1024², G=1024³
     * @note Used for parsing directives like client_max_body_size
     * 
     * @example
     * parseSizeWithSuffix("1024") returns 1024
     * parseSizeWithSuffix("1K") returns 1024
     * parseSizeWithSuffix("2M") returns 2097152
     */
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

    /**
     * @brief Validates if a port number is within the valid range
     * 
     * Checks whether the given port number is a valid TCP/UDP port.
     * Valid ports are in the range 1-65535.
     * 
     * @param port The port number to validate
     * @return bool True if the port is valid, false otherwise
     * 
     * @note Port 0 is reserved and not considered valid for server configuration
     * @note Used for validating listen directive values
     */
    bool isValidPort(int port) {
        return port >= 1 && port <= 65535;
    }

    /**
     * @brief Validates if a string represents a valid HTTP method
     * 
     * Checks whether the given string is one of the standard HTTP methods
     * supported by the web server.
     * 
     * @param method The HTTP method string to validate
     * @return bool True if the method is valid, false otherwise
     * 
     * @note Case-sensitive comparison
     * @note Supported methods: GET, POST, DELETE, PUT, HEAD, OPTIONS, TRACE, CONNECT
     * @note Used for validating allow_methods directive values
     */
    bool isValidHttpMethod(const std::string& method) {
        static const std::string valid_methods[] = {"GET", "POST", "DELETE", "PUT", "HEAD", "OPTIONS", "TRACE", "CONNECT"};
        for (size_t i = 0; i < sizeof(valid_methods)/sizeof(valid_methods[0]); ++i) {
            if (method == valid_methods[i]) return true;
        }
        return false;
    }

    /**
     * @brief Validates if a number represents a valid HTTP error code
     * 
     * Checks whether the given number is a valid HTTP error status code.
     * Valid error codes are in the range 400-599 (client and server errors).
     * 
     * @param code The error code to validate
     * @return bool True if the error code is valid, false otherwise
     * 
     * @note Covers both 4xx (client errors) and 5xx (server errors)
     * @note Used for validating error_page directive codes
     */
    bool isValidErrorCode(int code) {
        return code >= 400 && code <= 599;
    }

    /**
     * @brief Validates a list of HTTP methods and checks for duplicates
     * 
     * Performs comprehensive validation of HTTP method lists:
     * - Ensures each method is a valid HTTP method
     * - Checks for duplicate methods in the list
     * 
     * @param methods Vector of HTTP method strings to validate
     * 
     * @throws std::runtime_error if any method is invalid
     * @throws std::runtime_error if any method appears more than once
     * 
     * @note Used for validating allow_methods directive values
     * @note Case-sensitive comparison for both validity and duplicates
     */
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

    /**
     * @brief Parses a location block from the token stream
     * 
     * This function parses a complete location block from the configuration file.
     * A location block defines how the server should handle requests for specific URL paths.
     * 
     * Expected syntax:
     * location <path> {
     *     root <directory>;
     *     index <file1> <file2> ...;
     *     autoindex <on|off>;
     *     allow_methods <method1> <method2> ...;
     *     upload_store <directory>;
     *     cgi_pass <executable>;
     * }
     * 
     * @param ts Reference to the token stream to parse from
     * @return LocationConfig A fully populated location configuration object
     * 
     * @throws std::runtime_error if the location block syntax is invalid
     * @throws std::runtime_error if duplicate directives are found
     * @throws std::runtime_error if directive values are invalid
     * 
     * @note The function enforces that certain directives can only appear once per location
     * @note Unknown directives are silently ignored
     * @note The function consumes tokens until it finds the closing brace
     * 
     * @example
     * Input tokens: [TOKEN_WORD("location"), TOKEN_WORD("/"), TOKEN_LBRACE, ...]
     * Output: LocationConfig with path="/", root="/var/www", etc.
     */
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

    /**
     * @brief Parses a server block from the token stream
     * 
     * This function parses a complete server block from the configuration file.
     * A server block defines a virtual server that listens on a specific port
     * and handles requests according to its configuration.
     * 
     * Expected syntax:
     * server {
     *     listen <port>;
     *     server_name <name>;
     *     error_page <code> <path>;
     *     client_max_body_size <size>;
     *     location <path> { ... }
     * }
     * 
     * @param ts Reference to the token stream to parse from
     * @return ServerConfig A fully populated server configuration object
     * 
     * @throws std::runtime_error if the server block syntax is invalid
     * @throws std::runtime_error if required directives are missing (e.g., listen)
     * @throws std::runtime_error if duplicate directives are found
     * @throws std::runtime_error if directive values are invalid
     * 
     * @note The 'listen' directive is required and must appear exactly once
     * @note Multiple location blocks can be defined within a server block
     * @note Unknown directives are silently ignored
     * @note The function consumes tokens until it finds the closing brace
     * 
     * @example
     * Input tokens: [TOKEN_WORD("server"), TOKEN_LBRACE, TOKEN_WORD("listen"), ...]
     * Output: ServerConfig with listen_port=8080, server_name="example.com", etc.
     */
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

/**
 * @brief Parses a complete configuration from a token vector
 * 
 * This is the main parsing function that processes a complete configuration file.
 * It expects the configuration to consist of one or more server blocks at the top level.
 * 
 * Expected syntax:
 * server { ... }
 * server { ... }
 * # Comments are allowed
 * 
 * @param tokens Vector of tokens representing the entire configuration file
 * @return MainConfig A configuration object containing all parsed server configurations
 * 
 * @throws std::runtime_error if the configuration syntax is invalid
 * @throws std::runtime_error if no server blocks are found
 * @throws std::runtime_error if unexpected tokens are found at the top level
 * 
 * @note Only server blocks are allowed at the top level of the configuration
 * @note Comments and whitespace tokens are automatically handled by the tokenizer
 * @note The function processes all server blocks and returns them in a MainConfig object
 * 
 * @example
 * Input: [TOKEN_WORD("server"), TOKEN_LBRACE, ..., TOKEN_WORD("server"), TOKEN_LBRACE, ...]
 * Output: MainConfig with vector of ServerConfig objects
 */
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