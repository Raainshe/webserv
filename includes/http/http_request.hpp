/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_request.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ksinn <ksinn@student.42heilbronn.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/08/14 13:53:44 by ksinn            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <map>
#include <string>
#include <vector>

enum HttpMethod { GET, POST, DELETE, UNKNOWN };

enum RequestState {
  PARSING_REQUEST_LINE,
  PARSING_HEADERS,
  PARSING_BODY,
  COMPLETE,
  ERROR
};

class HttpRequest {
private:
  HttpMethod method;
  std::string uri;
  std::string http_version;
  std::map<std::string, std::string> headers;
  std::string body;
  RequestState state;
  std::string host;
  int port;
  std::string query_string;
  std::string path;

  int error_code;
  std::string error_message;

public:
  struct UploadedFile {
    std::string fieldName;
    std::string filename;
    std::string contentType;
    std::string data;
  };

private:
  std::vector<UploadedFile> uploaded_files;
  std::map<std::string, std::string> form_fields;

public:
  HttpRequest();
  ~HttpRequest();

  // Getters
  HttpMethod get_method() const;
  const std::string &get_uri() const;
  const std::string &get_http_version() const;
  const std::map<std::string, std::string> &get_headers() const;
  const std::string &get_body() const;
  RequestState get_state() const;
  const std::string &get_host() const;
  int get_port() const;
  const std::string &get_query_string() const;
  const std::string &get_path() const;
  int get_error_code() const;
  const std::string &get_error_message() const;

  // Header access (case-insensitive names)
  std::string get_header(const std::string &name) const;
  bool has_header(const std::string &name) const;
  size_t get_content_length() const;
  bool is_chunked() const;

  // Setters
  void set_method(HttpMethod method);
  void set_uri(const std::string &uri);
  void set_http_version(const std::string &version);
  void set_header(const std::string &name, const std::string &value);
  void set_body(const std::string &body);
  void set_state(RequestState state);
  void set_error(int code, const std::string &message);

  // Utility
  void clear();
  bool is_complete() const;
  bool has_error() const;
  void parse_uri();

  // Content type helpers
  bool is_multipart() const;
  std::string get_content_type() const;

  // Multipart/form-data results
  const std::vector<UploadedFile> &get_uploaded_files() const;
  const std::map<std::string, std::string> &get_form_fields() const;

  // Multipart/form-data mutators used by parser
  void add_uploaded_file(const std::string &fieldName,
                         const std::string &filename,
                         const std::string &contentType,
                         const std::string &data);
  void add_form_field(const std::string &name, const std::string &value);
};

#endif // HTTP_REQUEST_HPP
