/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request_parser.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/07/10 13:30:11 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "http/request_parser.hpp"
#include <cctype>
#include <sstream>

// RFC 2616: Line endings are CRLF (\r\n)
const std::string RequestParser::CRLF = "\r\n";

RequestParser::RequestParser() 
    : current_pos(0), headers_count(0), found_content_length(false), 
      expected_body_length(0), body_bytes_read(0) {
}

RequestParser::~RequestParser() {
}

bool RequestParser::parse_request(HttpRequest& request, const std::string& data) {
    // Reset request if starting fresh
    if (current_pos == 0) {
        request.clear();
    }
    
    // Parse request line
    if (request.get_state() == PARSING_REQUEST_LINE) {
        if (!parse_request_line(request, data)) {
            return false;
        }
    }
    
    // Parse headers
    if (request.get_state() == PARSING_HEADERS) {
        if (!parse_headers(request, data)) {
            return false;
        }
    }
    
    // Parse body if needed
    if (request.get_state() == PARSING_BODY) {
        if (!parse_body(request, data)) {
            return false;
        }
    }
    
    return true;
}

void RequestParser::reset() {
    current_pos = 0;
    headers_count = 0;
    found_content_length = false;
    expected_body_length = 0;
    body_bytes_read = 0;
}

bool RequestParser::parse_request_line(HttpRequest& request, const std::string& data) {
    std::string line = extract_line(data, current_pos);
    
    if (line.empty()) {
        // No complete line yet, wait for more data
        return true;
    }
    
    // RFC 2616: Request-Line = Method SP Request-URI SP HTTP-Version CRLF
    if (line.length() > MAX_REQUEST_LINE_LENGTH) {
        set_parse_error(request, 414, "Request-URI Too Long");
        return false;
    }
    
    // Split the line into method, URI, and version
    std::istringstream iss(line);
    std::string method_str, uri_str, version_str;
    
    if (!(iss >> method_str >> uri_str >> version_str)) {
        set_parse_error(request, 400, "Bad Request - Invalid request line");
        return false;
    }
    
    // Parse each component
    if (!parse_method(request, method_str)) {
        return false;
    }
    
    if (!parse_uri(request, uri_str)) {
        return false;
    }
    
    if (!parse_http_version(request, version_str)) {
        return false;
    }
    
    // Move to headers parsing
    request.set_state(PARSING_HEADERS);
    return true;
}

bool RequestParser::parse_headers(HttpRequest& request, const std::string& data) {
    while (current_pos < data.length()) {
        std::string line = extract_line(data, current_pos);
        
        if (line.empty()) {
            // No complete line yet, wait for more data
            return true;
        }
        
        // Empty line marks end of headers (RFC 2616)
        if (line == "\r\n" || line == "\n") {
            // Check if we need to parse body
            if (request.get_method() == POST) {
                if (request.has_header("Content-Length") || request.is_chunked()) {
                    request.set_state(PARSING_BODY);
                    expected_body_length = request.get_content_length();
                    return true;
                }
            }
            
            // No body needed, request is complete
            request.set_state(COMPLETE);
            return true;
        }
        
        // Parse header line
        if (!parse_header_line(request, line)) {
            return false;
        }
        
        headers_count++;
        
        // RFC 2616: Limit number of headers to prevent DoS
        if (headers_count > MAX_HEADERS_COUNT) {
            set_parse_error(request, 431, "Request Header Fields Too Large");
            return false;
        }
    }
    
    return true;
}

bool RequestParser::parse_body(HttpRequest& request, const std::string& data) {
    // TODO: Implement chunked encoding parsing for step 8 (CGI support)
    if (request.is_chunked()) {
        set_parse_error(request, 501, "Chunked encoding not implemented yet");
        return false;
    }
    
    // TODO: Implement multipart parsing for step 9 (file uploads)
    if (request.is_multipart()) {
        set_parse_error(request, 501, "Multipart parsing not implemented yet");
        return false;
    }
    
    // Content-Length based body parsing
    if (found_content_length) {
        size_t remaining_data = data.length() - current_pos;
        size_t needed_bytes = expected_body_length - body_bytes_read;
        size_t bytes_to_read = (remaining_data < needed_bytes) ? remaining_data : needed_bytes;
        
        if (bytes_to_read > 0) {
            request.set_body(request.get_body() + data.substr(current_pos, bytes_to_read));
            body_bytes_read += bytes_to_read;
            current_pos += bytes_to_read;
        }
        
        if (body_bytes_read >= expected_body_length) {
            request.set_state(COMPLETE);
            return true;
        }
    }
    
    return true;
}

bool RequestParser::parse_method(HttpRequest& request, const std::string& method_str) {
    if (!is_valid_method(method_str)) {
        set_parse_error(request, 400, "Bad Request - Invalid HTTP method");
        return false;
    }
    
    if (method_str == "GET") {
        request.set_method(GET);
    } else if (method_str == "POST") {
        request.set_method(POST);
    } else if (method_str == "DELETE") {
        request.set_method(DELETE);
    } else {
        request.set_method(UNKNOWN);
    }
    
    return true;
}

bool RequestParser::parse_uri(HttpRequest& request, const std::string& uri_str) {
    if (!is_valid_uri(uri_str)) {
        set_parse_error(request, 400, "Bad Request - Invalid URI");
        return false;
    }
    
    request.set_uri(uri_str);
    return true;
}

bool RequestParser::parse_http_version(HttpRequest& request, const std::string& version_str) {
    if (!is_valid_http_version(version_str)) {
        set_parse_error(request, 400, "Bad Request - Invalid HTTP version");
        return false;
    }
    
    request.set_http_version(version_str);
    return true;
}

bool RequestParser::parse_header_line(HttpRequest& request, const std::string& line) {
    // RFC 2616: message-header = field-name ":" [ field-value ]
    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos) {
        set_parse_error(request, 400, "Bad Request - Invalid header format");
        return false;
    }
    
    std::string name = line.substr(0, colon_pos);
    std::string value = line.substr(colon_pos + 1);
    
    // Trim whitespace
    trim_whitespace(name);
    trim_whitespace(value);
    
    // RFC 2616: Header names are case-insensitive
    name = to_lower(name);
    
    // Validate header name (RFC 2616: field-name = token)
    for (size_t i = 0; i < name.length(); ++i) {
        if (!isprint(name[i]) || name[i] == ' ' || name[i] == ':') {
            set_parse_error(request, 400, "Bad Request - Invalid header name");
            return false;
        }
    }
    
    // Check for Content-Length header
    if (name == "content-length") {
        found_content_length = true;
        // Validate Content-Length value
        for (size_t i = 0; i < value.length(); ++i) {
            if (!isdigit(value[i])) {
                set_parse_error(request, 400, "Bad Request - Invalid Content-Length");
                return false;
            }
        }
    }
    
    request.set_header(name, value);
    return true;
}

std::string RequestParser::extract_line(const std::string& data, size_t& pos) {
    if (pos >= data.length()) {
        return "";
    }
    
    size_t crlf_pos = data.find(CRLF, pos);
    if (crlf_pos == std::string::npos) {
        // No complete line yet
        return "";
    }
    
    std::string line = data.substr(pos, crlf_pos - pos + CRLF.length());
    pos = crlf_pos + CRLF.length();
    return line;
}

bool RequestParser::is_valid_http_version(const std::string& version) {
    // RFC 2616: HTTP-Version = "HTTP" "/" 1*DIGIT "." 1*DIGIT
    if (version.substr(0, 5) != "HTTP/") {
        return false;
    }
    
    std::string version_num = version.substr(5);
    size_t dot_pos = version_num.find('.');
    if (dot_pos == std::string::npos) {
        return false;
    }
    
    std::string major = version_num.substr(0, dot_pos);
    std::string minor = version_num.substr(dot_pos + 1);
    
    // Check that both parts are digits
    for (size_t i = 0; i < major.length(); ++i) {
        if (!isdigit(major[i])) {
            return false;
        }
    }
    for (size_t i = 0; i < minor.length(); ++i) {
        if (!isdigit(minor[i])) {
            return false;
        }
    }
    
    return true;
}

bool RequestParser::is_valid_method(const std::string& method) {
    // RFC 2616: Method = "OPTIONS" | "GET" | "HEAD" | "POST" | "PUT" | "DELETE" | "TRACE"
    // We only implement GET, POST, DELETE as per subject requirements
    return (method == "GET" || method == "POST" || method == "DELETE");
}

bool RequestParser::is_valid_uri(const std::string& uri) {
    // Basic URI validation - must not be empty and should start with /
    if (uri.empty()) {
        return false;
    }
    
    // Absolute URI (http://...) or relative URI (starting with /)
    if (uri[0] != '/' && uri.substr(0, 7) != "http://") {
        return false;
    }
    
    return true;
}

void RequestParser::trim_whitespace(std::string& str) {
    // Remove leading whitespace
    while (!str.empty() && isspace(str[0])) {
        str.erase(0, 1);
    }
    
    // Remove trailing whitespace
    while (!str.empty() && isspace(str[str.length() - 1])) {
        str.erase(str.length() - 1, 1);
    }
}

std::string RequestParser::to_lower(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.length(); ++i) {
        result[i] = tolower(result[i]);
    }
    return result;
}

void RequestParser::set_parse_error(HttpRequest& request, int code, const std::string& message) {
    request.set_error(code, message);
} 