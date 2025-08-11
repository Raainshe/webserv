/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request_parser.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ksinn <ksinn@student.42heilbronn.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/08/11 13:13:59 by ksinn            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../../includes/http/request_parser.hpp"
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
    if (current_pos == 0) {
        request.clear();
    }
    if (request.get_state() == PARSING_REQUEST_LINE) {
        if (!parse_request_line(request, data)) {
            return false;
        }
    }
    if (request.get_state() == PARSING_HEADERS) {
        if (!parse_headers(request, data)) {
            return false;
        }
    }
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
        return true;
    }
    if (line.length() > MAX_REQUEST_LINE_LENGTH) {
        set_parse_error(request, 414, "Request-URI Too Long");
        return false;
    }
    std::istringstream iss(line);
    std::string method_str, uri_str, version_str;
    if (!(iss >> method_str >> uri_str >> version_str)) {
        set_parse_error(request, 400, "Bad Request - Invalid request line");
        return false;
    }
    if (!parse_method(request, method_str)) return false;
    if (!parse_uri(request, uri_str)) return false;
    if (!parse_http_version(request, version_str)) return false;
    request.set_state(PARSING_HEADERS);
    return true;
}

bool RequestParser::parse_headers(HttpRequest& request, const std::string& data) {
    while (current_pos < data.length()) {
        std::string line = extract_line(data, current_pos);
        if (line.empty()) {
            return true;
        }
        if (line == "\r\n" || line == "\n") {
            if (request.get_method() == POST) {
                if (request.has_header("Content-Length") || request.is_chunked()) {
                    request.set_state(PARSING_BODY);
                    expected_body_length = request.get_content_length();
                    return true;
                }
            }
            request.set_state(COMPLETE);
            return true;
        }
        if (!parse_header_line(request, line)) {
            return false;
        }
        headers_count++;
        if (headers_count > MAX_HEADERS_COUNT) {
            set_parse_error(request, 431, "Request Header Fields Too Large");
            return false;
        }

        // Track presence of content-length
        if (to_lower(line.substr(0, line.find(':'))) == "content-length") {
            found_content_length = true;
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
    if (request.is_multipart()) {
        return parse_multipart_body(request, data);
    }
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
    if (method_str == "GET")      request.set_method(GET);
    else if (method_str == "POST") request.set_method(POST);
    else if (method_str == "DELETE") request.set_method(DELETE);
    else request.set_method(UNKNOWN);
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
    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos) {
        set_parse_error(request, 400, "Bad Request - Invalid header format");
        return false;
    }
    std::string name = line.substr(0, colon_pos);
    std::string value = line.substr(colon_pos + 1);
    trim_whitespace(name);
    trim_whitespace(value);
    name = to_lower(name);
    for (size_t i = 0; i < name.length(); ++i) {
        if (!isprint(name[i]) || name[i] == ' ' || name[i] == ':') {
            set_parse_error(request, 400, "Bad Request - Invalid header name");
            return false;
        }
    }
    if (name == "content-length") {
        found_content_length = true;
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
        return "";
    }
    std::string line = data.substr(pos, crlf_pos - pos + CRLF.length());
    pos = crlf_pos + CRLF.length();
    return line;
}

bool RequestParser::is_valid_http_version(const std::string& version) {
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
    for (size_t i = 0; i < major.length(); ++i) {
        if (!isdigit(major[i])) return false;
    }
    for (size_t i = 0; i < minor.length(); ++i) {
        if (!isdigit(minor[i])) return false;
    }
    return true;
}

bool RequestParser::is_valid_method(const std::string& method) {
    return (method == "GET" || method == "POST" || method == "DELETE");
}

bool RequestParser::is_valid_uri(const std::string& uri) {
    if (uri.empty()) return false;
    if (uri[0] != '/' && uri.substr(0, 7) != "http://") return false;
    return true;
}

void RequestParser::trim_whitespace(std::string& str) {
    while (!str.empty() && isspace(str[0])) {
        str.erase(0, 1);
    }
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

// ---------------- Multipart parsing -----------------

static std::string parse_boundary_param(const std::string& content_type) {
    // Expect: multipart/form-data; boundary=XYZ
    size_t bpos = content_type.find("boundary=");
    if (bpos == std::string::npos) return "";
    std::string b = content_type.substr(bpos + 9);
    // Trim possible quotes
    if (!b.empty() && (b[0] == '"' || b[0] == '\'')) {
        char q = b[0];
        size_t endq = b.find(q, 1);
        if (endq != std::string::npos) b = b.substr(1, endq - 1);
        else b = b.substr(1);
    } else {
        // Trim to end or semicolon
        size_t semi = b.find(';');
        if (semi != std::string::npos) b = b.substr(0, semi);
    }
    // Trim whitespace
    size_t start = 0; while (start < b.size() && isspace(b[start])) start++;
    size_t end = b.size(); while (end > start && isspace(b[end-1])) end--;
    return b.substr(start, end - start);
}

bool RequestParser::parse_multipart_body(HttpRequest& request, const std::string& data) {
    // Ensure we have the full body buffered based on Content-Length
    if (!found_content_length) {
        set_parse_error(request, 400, "Bad Request - Missing Content-Length for multipart");
        return false;
    }
    size_t remaining_data = data.length() - current_pos;
    size_t needed_bytes = expected_body_length - body_bytes_read;
    if (remaining_data < needed_bytes) {
        // Not all body arrived yet
        return true;
    }
    // Read the full body
    std::string body_chunk = data.substr(current_pos, needed_bytes);
    body_bytes_read += needed_bytes;
    current_pos += needed_bytes;

    // Parse boundary
    std::string content_type = request.get_content_type();
    std::string boundary = parse_boundary_param(content_type);
    if (boundary.empty()) {
        set_parse_error(request, 400, "Bad Request - Missing multipart boundary");
        return false;
    }
    std::string delimiter = "--" + boundary;
    std::string close_delimiter = delimiter + "--";

    // Split by boundary lines
    // Body format often starts with CRLF before first boundary, be tolerant
    size_t pos = 0;
    // Find first delimiter
    size_t first = body_chunk.find(delimiter, pos);
    if (first == std::string::npos) {
        set_parse_error(request, 400, "Bad Request - Boundary not found in body");
        return false;
    }
    pos = first + delimiter.size();
    // Expect CRLF after delimiter
    if (body_chunk.compare(pos, 2, "\r\n") == 0) pos += 2;

    while (pos < body_chunk.size()) {
        // Check for closing boundary
        if (body_chunk.compare(pos - 2, close_delimiter.size(), close_delimiter) == 0) {
            break;
        }
        // Parse part headers until empty line
        size_t headers_start = pos;
        size_t headers_end = body_chunk.find("\r\n\r\n", headers_start);
        if (headers_end == std::string::npos) {
            set_parse_error(request, 400, "Bad Request - Malformed multipart headers");
            return false;
        }
        std::string part_headers = body_chunk.substr(headers_start, headers_end - headers_start);
        pos = headers_end + 4; // skip CRLFCRLF
        // Find next boundary
        size_t next_delim = body_chunk.find(delimiter, pos);
        if (next_delim == std::string::npos) {
            set_parse_error(request, 400, "Bad Request - Next boundary not found");
            return false;
        }
        // Part body is everything up to the preceding CRLF before boundary
        size_t part_body_end = next_delim;
        // Trim trailing CRLF
        if (part_body_end >= 2 && body_chunk.substr(part_body_end - 2, 2) == "\r\n") {
            part_body_end -= 2;
        }
        std::string part_body = body_chunk.substr(pos, part_body_end - pos);
        // Parse this part
        if (!parse_multipart_part(request, part_headers, part_body)) {
            return false;
        }
        // Advance pos past boundary line
        pos = next_delim + delimiter.size();
        // If closing boundary
        if (body_chunk.compare(pos, 2, "--") == 0) {
            pos += 2;
            // Optionally there may be trailing CRLF
            if (body_chunk.compare(pos, 2, "\r\n") == 0) pos += 2;
            break;
        }
        // Skip CRLF after normal boundary
        if (body_chunk.compare(pos, 2, "\r\n") == 0) pos += 2;
    }

    request.set_state(COMPLETE);
    return true;
}

static std::string header_value(const std::string& headers, const std::string& key) {
    std::string k = key;
    for (size_t i = 0; i < k.size(); ++i) k[i] = tolower(k[i]);
    std::istringstream iss(headers);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line[line.size()-1] == '\r') line.erase(line.size()-1);
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string name = line.substr(0, colon);
        for (size_t i = 0; i < name.size(); ++i) name[i] = tolower(name[i]);
        std::string value = line.substr(colon + 1);
        // trim
        size_t s = 0; while (s < value.size() && isspace(value[s])) s++;
        size_t e = value.size(); while (e > s && isspace(value[e-1])) e--;
        value = value.substr(s, e - s);
        if (name == k) return value;
    }
    return "";
}

bool RequestParser::parse_multipart_part(HttpRequest& request,
                              const std::string& part_headers,
                              const std::string& part_body) {
    std::string disp = header_value(part_headers, "Content-Disposition");
    if (disp.empty()) {
        set_parse_error(request, 400, "Bad Request - Missing Content-Disposition in part");
        return false;
    }
    // Expect: form-data; name="field"; filename="file" (optional)
    // Parse name
    std::string name;
    std::string filename;
    size_t name_pos = disp.find("name=");
    if (name_pos != std::string::npos) {
        size_t start = name_pos + 5;
        if (start < disp.size() && (disp[start] == '"' || disp[start] == '\'')) {
            char q = disp[start];
            size_t endq = disp.find(q, start + 1);
            if (endq != std::string::npos) name = disp.substr(start + 1, endq - start - 1);
        } else {
            size_t end = disp.find(';', start);
            name = disp.substr(start, end == std::string::npos ? std::string::npos : end - start);
        }
    }
    if (name.empty()) {
        set_parse_error(request, 400, "Bad Request - multipart field name missing");
        return false;
    }
    size_t fn_pos = disp.find("filename=");
    if (fn_pos != std::string::npos) {
        size_t start = fn_pos + 9;
        if (start < disp.size() && (disp[start] == '"' || disp[start] == '\'')) {
            char q = disp[start];
            size_t endq = disp.find(q, start + 1);
            if (endq != std::string::npos) filename = disp.substr(start + 1, endq - start - 1);
        } else {
            size_t end = disp.find(';', start);
            filename = disp.substr(start, end == std::string::npos ? std::string::npos : end - start);
        }
    }
    std::string ctype = header_value(part_headers, "Content-Type");

    if (!filename.empty()) {
        request.add_uploaded_file(name, filename, ctype, part_body);
    } else {
        request.add_form_field(name, part_body);
    }
    return true;
}
