/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request_parser.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/07/10 13:29:33 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include <string>
#include "http/http_request.hpp"

class RequestParser {
private:
    static const size_t MAX_REQUEST_LINE_LENGTH = 8192;
    static const size_t MAX_HEADER_LENGTH = 8192;
    static const size_t MAX_HEADERS_COUNT = 100;
    
    // RFC 2616: Line endings are CRLF (\r\n)
    static const std::string CRLF;
    
    // Parsing state
    size_t current_pos;
    size_t headers_count;
    bool found_content_length;
    size_t expected_body_length;
    size_t body_bytes_read;

public:
    RequestParser();
    ~RequestParser();
    
    // Main parsing method
    bool parse_request(HttpRequest& request, const std::string& data);
    
    // Reset parser state
    void reset();

private:
    // Parsing methods for different parts of the request
    bool parse_request_line(HttpRequest& request, const std::string& data);
    bool parse_headers(HttpRequest& request, const std::string& data);
    bool parse_body(HttpRequest& request, const std::string& data);
    
    // Helper methods
    bool parse_method(HttpRequest& request, const std::string& line);
    bool parse_uri(HttpRequest& request, const std::string& line);
    bool parse_http_version(HttpRequest& request, const std::string& line);
    bool parse_header_line(HttpRequest& request, const std::string& line);
    
    // Utility methods
    std::string extract_line(const std::string& data, size_t& pos);
    bool is_valid_http_version(const std::string& version);
    bool is_valid_method(const std::string& method);
    bool is_valid_uri(const std::string& uri);
    void trim_whitespace(std::string& str);
    std::string to_lower(const std::string& str);
    
    // Error handling
    void set_parse_error(HttpRequest& request, int code, const std::string& message);
    
    // TODO: Chunked encoding parsing (for step 8 - CGI support)
    // bool parse_chunked_body(HttpRequest& request, const std::string& data);
    
    // TODO: Multipart parsing (for step 9 - file uploads)
    // bool parse_multipart_body(HttpRequest& request, const std::string& data);
};

#endif // REQUEST_PARSER_HPP 