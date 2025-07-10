/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_request.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmakoni <rmakoni@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/07/10 13:29:11 by rmakoni          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "http/http_request.hpp"
#include <cstdlib> // for atoi
#include <cctype>  // for isdigit, tolower

HttpRequest::HttpRequest() 
    : method(UNKNOWN), state(PARSING_REQUEST_LINE), port(80), error_code(0) {
}

HttpRequest::~HttpRequest() {
}

HttpMethod HttpRequest::get_method() const {
    return method;
}

const std::string& HttpRequest::get_uri() const {
    return uri;
}

const std::string& HttpRequest::get_http_version() const {
    return http_version;
}

const std::map<std::string, std::string>& HttpRequest::get_headers() const {
    return headers;
}

const std::string& HttpRequest::get_body() const {
    return body;
}

RequestState HttpRequest::get_state() const {
    return state;
}

const std::string& HttpRequest::get_host() const {
    return host;
}

int HttpRequest::get_port() const {
    return port;
}

const std::string& HttpRequest::get_query_string() const {
    return query_string;
}

const std::string& HttpRequest::get_path() const {
    return path;
}

int HttpRequest::get_error_code() const {
    return error_code;
}

const std::string& HttpRequest::get_error_message() const {
    return error_message;
}

std::string HttpRequest::get_header(const std::string& name) const {
    std::map<std::string, std::string>::const_iterator it = headers.find(name);
    if (it != headers.end()) {
        return it->second;
    }
    return "";
}

bool HttpRequest::has_header(const std::string& name) const {
    return headers.find(name) != headers.end();
}

size_t HttpRequest::get_content_length() const {
    std::string content_length = get_header("Content-Length");
    if (content_length.empty()) {
        return 0;
    }
    
    // RFC 2616: Content-Length must be a positive integer
    for (size_t i = 0; i < content_length.length(); ++i) {
        if (!isdigit(content_length[i])) {
            return 0;
        }
    }
    
    return static_cast<size_t>(atoi(content_length.c_str()));
}

bool HttpRequest::is_chunked() const {
    std::string transfer_encoding = get_header("Transfer-Encoding");
    return transfer_encoding.find("chunked") != std::string::npos;
}

void HttpRequest::set_method(HttpMethod method) {
    this->method = method;
}

void HttpRequest::set_uri(const std::string& uri) {
    this->uri = uri;
    parse_uri();
}

void HttpRequest::set_http_version(const std::string& version) {
    this->http_version = version;
}

void HttpRequest::set_header(const std::string& name, const std::string& value) {
    // RFC 2616: Header names are case-insensitive
    std::string lower_name = name;
    for (size_t i = 0; i < lower_name.length(); ++i) {
        lower_name[i] = tolower(lower_name[i]);
    }
    headers[lower_name] = value;
}

void HttpRequest::set_body(const std::string& body) {
    this->body = body;
}

void HttpRequest::set_state(RequestState state) {
    this->state = state;
}

void HttpRequest::set_error(int code, const std::string& message) {
    error_code = code;
    error_message = message;
    state = ERROR;
}

void HttpRequest::clear() {
    method = UNKNOWN;
    uri.clear();
    http_version.clear();
    headers.clear();
    body.clear();
    state = PARSING_REQUEST_LINE;
    host.clear();
    port = 80;
    query_string.clear();
    path.clear();
    error_code = 0;
    error_message.clear();
}

bool HttpRequest::is_complete() const {
    return state == COMPLETE;
}

bool HttpRequest::has_error() const {
    return state == ERROR;
}

void HttpRequest::parse_uri() {
    if (uri.empty()) {
        return;
    }
    
    // Extract host and port from URI if present
    if (uri.substr(0, 7) == "http://") {
        size_t host_start = 7;
        size_t host_end = uri.find('/', host_start);
        if (host_end == std::string::npos) {
            host_end = uri.length();
        }
        
        std::string host_port = uri.substr(host_start, host_end - host_start);
        size_t colon_pos = host_port.find(':');
        
        if (colon_pos != std::string::npos) {
            host = host_port.substr(0, colon_pos);
            port = atoi(host_port.substr(colon_pos + 1).c_str());
        } else {
            host = host_port;
            port = 80;
        }
        
        // Extract path and query string
        if (host_end < uri.length()) {
            std::string path_query = uri.substr(host_end);
            size_t query_pos = path_query.find('?');
            if (query_pos != std::string::npos) {
                path = path_query.substr(0, query_pos);
                query_string = path_query.substr(query_pos + 1);
            } else {
                path = path_query;
            }
        }
    } else {
        // Relative URI
        size_t query_pos = uri.find('?');
        if (query_pos != std::string::npos) {
            path = uri.substr(0, query_pos);
            query_string = uri.substr(query_pos + 1);
        } else {
            path = uri;
        }
    }
}

bool HttpRequest::is_multipart() const {
    std::string content_type = get_content_type();
    return content_type.find("multipart/form-data") != std::string::npos;
}

std::string HttpRequest::get_content_type() const {
    return get_header("Content-Type");
} 