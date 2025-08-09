/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_response_handling.cpp                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ksinn <ksinn@student.42heilbronn.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/08 00:12:38 by hpehliva          #+#    #+#             */
/*   Updated: 2025/08/09 12:48:10 by ksinn            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/http/http_response_handling.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

HttpResponseHandling::HttpResponseHandling(const ServerConfig *server_config)
    : server_config(server_config) {}

HttpResponseHandling::~HttpResponseHandling() {}

std::string
HttpResponseHandling::handle_request(const HttpRequest &request,
                                     const RouteResult &route_result) {
  // If routing said directory listing is needed
  if (route_result.is_directory && route_result.should_list_directory) {
    return serve_directory_listing(route_result.file_path, request.get_path());
  }

  switch (request.get_method()) {
  case GET:
    return handle_get_request(request, route_result);
  case POST:
    return handle_post_request(request, route_result);
  case DELETE:
    return handle_delete_request(request, route_result);
  default:
    return build_error_response(405, "Method Not Allowed");
  }
}

std::string
HttpResponseHandling::handle_get_request(const HttpRequest & /*request*/,
                                         const RouteResult &route_result) {
  const std::string &file_path = route_result.file_path;
  if (file_path.empty()) {
    return build_error_response(404, "Not Found");
  }
  if (!file_exists(file_path)) {
    return build_error_response(404, "Not Found");
  }
  if (is_directory(file_path)) {
    // If we reach here with a directory, autoindex must be false; respond 403
    return build_error_response(403, "Forbidden");
  }
  return serve_file(file_path);
}

std::string
HttpResponseHandling::handle_post_request(const HttpRequest &request,
                                          const RouteResult &route_result) {
  (void)route_result;
  (void)request;
  std::string body = "POST request received successfully!";
  return build_response(200, "text/plain", body);
}
// put request is not implemented yet
// head request is not implemented yet

std::string
HttpResponseHandling::handle_delete_request(const HttpRequest &request,
                                            const RouteResult &route_result) {
  (void)request;
  std::string file_path = route_result.file_path;
  if (!file_exists(file_path))
    return build_error_response(404, "File not found");
  if (is_directory(file_path))
    return build_error_response(403, "Cannot delete a directory");

  if (unlink(file_path.c_str()) == 0) {
    std::string body = "File deleted successfully!";
    return build_response(200, "text/plain", body);
  } else {
    return build_error_response(500, "Failed to delete file");
  }
}

std::string HttpResponseHandling::serve_file(const std::string &file_path) {
  std::string content = read_file(file_path);
  if (content.empty())
    return build_error_response(500, "Failed to read file");

  std::string mime_type = get_mime_type(file_path);
  return build_response(200, mime_type, content);
}
std::string
HttpResponseHandling::serve_directory_listing(const std::string &directory_path,
                                              const std::string &uri) {
  std::string index_path = directory_path;
  if (index_path[index_path.length() - 1] != '/')
    index_path += "/";
  index_path += "index.html";
  if (file_exists(index_path)) {
    return serve_file(index_path);
  }
  // Basic autoindex-style listing placeholder for Step 6
  std::string body;
  body += "<html><body><h1>Index of ";
  body += uri;
  body += "</h1><p>Directory listing enabled</p></body></html>";
  return build_response(200, "text/html", body);
}
// std::string serve_error_page(int error_code);
// std::string serve_cgi(const HttpRequest& request);
// std::string serve_redirect(const std::string& redirect_url);
// std::string serve_autoindex(const std::string& directory_path);
// std::string serve_default_page(const std::string& directory_path);
// std::string serve_index_page(const std::string& directory_path);

std::string
HttpResponseHandling::build_response(int status_code,
                                     const std::string &content_type,
                                     const std::string &content) {
  std::ostringstream response_stream;
  response_stream << "HTTP/1.1 " << status_code << " "
                  << get_status_message(status_code) << "\r\n";
  response_stream << "Content-Type: " << content_type << "\r\n";
  response_stream << "Content-Length: " << content.size() << "\r\n";
  response_stream << "Server: webserv/1.0\r\n";
  response_stream << "\r\n";
  response_stream << content;
  return response_stream.str();
}
std::string
HttpResponseHandling::build_error_response(int status_code,
                                           const std::string &message) {
  std::ostringstream body;
  body << "<!DOCTYPE html>\n";
  body << "<html><head><title>" << status_code << " " << message
       << "</title></head>\n";
  body << "<body><h1>" << status_code << " " << message << "</h1>\n";
  body << "<hr><p>webserv/1.0</p></body></html>\n";
  return build_response(status_code, "text/html", body.str());
}
std::string HttpResponseHandling::get_mime_type(const std::string &file_path) {
  size_t dot_pos = file_path.find_last_of('.');
  if (dot_pos == std::string::npos)
    return "application/octet-stream"; // Default binary type

  std::string extension = file_path.substr(dot_pos + 1);

  if (extension == "html" || extension == "htm")
    return "text/html";
  if (extension == "css")
    return "text/css";
  if (extension == "js")
    return "application/javascript";
  if (extension == "jpg" || extension == "jpeg")
    return "image/jpeg";
  if (extension == "png")
    return "image/png";
  if (extension == "gif")
    return "image/gif";
  if (extension == "txt")
    return "text/plain";

  return "application/octet-stream"; // Default binary type
}
std::string HttpResponseHandling::get_status_message(int status_code) {
  switch (status_code) {
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 204:
    return "No Content";
  case 400:
    return "Bad Request";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 500:
    return "Internal Server Error";
  default:
    return "Unknown Status";
  }
}
bool HttpResponseHandling::file_exists(const std::string &path) {
  return access(path.c_str(), F_OK) == 0;
}
bool HttpResponseHandling::is_directory(const std::string &path) {
  struct stat path_status; // stat is used to get file status
  if (stat(path.c_str(), &path_status) != 0)
    return false;
  return S_ISDIR(path_status.st_mode);
}
std::string HttpResponseHandling::read_file(const std::string &path) {
  std::ifstream file(path.c_str(),
                     std::ios::binary); // std::ios::in | std::ios::binary
  if (!file.is_open())
    return "";

  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}
