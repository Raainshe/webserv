/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   routing.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ksinn <ksinn@student.42heilbronn.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/04 15:45:00 by ksinn             #+#    #+#             */
/*   Updated: 2025/08/14 14:02:24 by ksinn            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/http/routing.hpp"
#include <iostream>
#include <sys/stat.h>

Router::Router() {}

Router::~Router() {}

RouteResult Router::route_request(const ServerConfig &server,
                                  const HttpRequest &request) {
  std::string uri = request.get_uri();
  HttpMethod method = request.get_method();

  std::cout << "Routing request: " << method_to_string(method) << " " << uri
            << std::endl;

  // Find matching location
  const LocationConfig *location = find_matching_location(server, uri);
  if (!location) {
    std::cout << "No matching location found for URI: " << uri << std::endl;
    return create_error_result(ROUTE_NOT_FOUND, 404,
                               "No matching location found");
  }

  std::cout << "Matched location: " << location->path
            << " (root: " << location->root << ")" << std::endl;

  // Redirect handling: if location defines a return 3xx, short-circuit
  if (location->return_code >= 300 && location->return_code <= 399 &&
      !location->return_url.empty()) {
    RouteResult rr;
    rr.status = ROUTE_OK;
    rr.http_status_code = location->return_code;
    rr.location = location;
    rr.file_path = "";
    rr.error_message = "";
    rr.is_directory = false;
    rr.should_list_directory = false;
    rr.is_cgi_request = false;
    rr.is_redirect = true;
    rr.redirect_location = location->return_url;
    return rr;
  }

  // Check if method is allowed
  if (!is_method_allowed(*location, method)) {
    std::cout << "Method " << method_to_string(method)
              << " not allowed for location " << location->path << std::endl;
    return create_error_result(ROUTE_METHOD_NOT_ALLOWED, 405,
                               "Method " + method_to_string(method) +
                                   " not allowed");
  }

  std::cout << "Method " << method_to_string(method) << " is allowed"
            << std::endl;

  // Create successful route result
  RouteResult result;
  result.status = ROUTE_OK;
  result.http_status_code = 200;
  result.location = location;
  result.error_message = "";
  result.is_cgi_request = !location->cgi_pass.empty();
  result.is_redirect = false;
  result.redirect_location.clear();

  // Resolve file path
  result.file_path = resolve_file_path(*location, uri);
  std::cout << "Resolved file path: " << result.file_path << std::endl;

  // For CGI requests, don't require file existence
  if (result.is_cgi_request) {
    std::cout << "CGI request detected - skipping file existence check"
              << std::endl;
    result.is_directory = false;
    result.should_list_directory = false;
    return result;
  }

  // Check if path exists and determine type
  if (path_exists(result.file_path)) {
    result.is_directory = is_directory(result.file_path);

    if (result.is_directory) {
      std::cout << "Path is a directory" << std::endl;

      // Try to find index file
      if (!location->index.empty()) {
        for (std::vector<std::string>::const_iterator it =
                 location->index.begin();
             it != location->index.end(); ++it) {
          std::string index_path = join_paths(result.file_path, *it);
          std::cout << "Checking index file: " << index_path << std::endl;

          if (path_exists(index_path) && !is_directory(index_path)) {
            std::cout << "Found index file: " << index_path << std::endl;
            result.file_path = index_path;
            result.is_directory = false;
            break;
          }
        }
      }

      // If still a directory, check autoindex
      if (result.is_directory) {
        result.should_list_directory = location->autoindex;
        std::cout << "Directory listing "
                  << (location->autoindex ? "enabled" : "disabled")
                  << std::endl;

        if (!location->autoindex) {
          // Directory access forbidden without autoindex
          return create_error_result(ROUTE_NOT_FOUND, 403,
                                     "Directory listing disabled");
        }
      }
    } else {
      std::cout << "Path is a file" << std::endl;
      result.is_directory = false;
      result.should_list_directory = false;
    }
  } else {
    std::cout << "Path does not exist: " << result.file_path << std::endl;
    // For non-existent files, let HTTP Response Handling decide
    // This allows for dynamic content, custom 404 pages, etc.
    result.is_directory = false;
    result.should_list_directory = false;
    std::cout << "Non-existent file - letting response handler decide"
              << std::endl;
  }

  return result;
}

const LocationConfig *Router::find_matching_location(const ServerConfig &server,
                                                     const std::string &uri) {
  const LocationConfig *best_match = NULL;
  size_t longest_match = 0;

  // Implement longest prefix matching (like nginx)
  for (std::vector<LocationConfig>::const_iterator it =
           server.locations.begin();
       it != server.locations.end(); ++it) {

    const std::string &location_path = it->path;

    // Check if URI starts with this location path
    if (uri.substr(0, location_path.length()) == location_path) {
      // For exact match or if location ends with / or URI has / after location
      bool is_valid_match = false;

      if (uri.length() == location_path.length()) {
        // Exact match
        is_valid_match = true;
      } else if (location_path == "/") {
        // Root location matches everything
        is_valid_match = true;
      } else if (location_path[location_path.length() - 1] == '/') {
        // Location ends with slash
        is_valid_match = true;
      } else if (uri[location_path.length()] == '/') {
        // Next character in URI is slash
        is_valid_match = true;
      }

      if (is_valid_match && location_path.length() > longest_match) {
        longest_match = location_path.length();
        best_match = &(*it);
      }
    }
  }

  return best_match;
}

bool Router::is_method_allowed(const LocationConfig &location,
                               HttpMethod method) {
  std::string method_str = method_to_string(method);

  // Check if method is in allow_methods list
  for (std::vector<std::string>::const_iterator it =
           location.allow_methods.begin();
       it != location.allow_methods.end(); ++it) {
    if (*it == method_str) {
      return true;
    }
  }

  return false;
}

std::string Router::resolve_file_path(const LocationConfig &location,
                                      const std::string &uri) {
  std::string location_path = location.path;
  std::string root = location.root;

  // Remove location path from URI to get relative path
  std::string relative_path;
  if (uri.length() >= location_path.length()) {
    relative_path = uri.substr(location_path.length());
  }

  // Handle root location special case
  if (location_path == "/") {
    relative_path = uri;
  }

  // Join root with relative path
  return join_paths(root, relative_path);
}

std::string Router::normalize_path(const std::string &path) {
  std::string normalized = path;

  // Remove duplicate slashes
  size_t pos = 0;
  while ((pos = normalized.find("//", pos)) != std::string::npos) {
    normalized.replace(pos, 2, "/");
  }

  return normalized;
}

std::string Router::join_paths(const std::string &root,
                               const std::string &path) {
  std::string result = root;

  // Ensure root ends with /
  if (!result.empty() && result[result.length() - 1] != '/') {
    result += "/";
  }

  // Remove leading / from path
  std::string clean_path = path;
  if (!clean_path.empty() && clean_path[0] == '/') {
    clean_path = clean_path.substr(1);
  }

  result += clean_path;
  return normalize_path(result);
}

bool Router::path_exists(const std::string &path) {
  struct stat st;
  return stat(path.c_str(), &st) == 0;
}

bool Router::is_directory(const std::string &path) {
  struct stat st;
  if (stat(path.c_str(), &st) == 0) {
    return S_ISDIR(st.st_mode);
  }
  return false;
}

std::string Router::method_to_string(HttpMethod method) {
  switch (method) {
  case GET:
    return "GET";
  case POST:
    return "POST";
  case DELETE:
    return "DELETE";
  default:
    return "UNKNOWN";
  }
}

RouteResult Router::create_error_result(RouteStatus status, int http_code,
                                        const std::string &message) {
  RouteResult result;
  result.status = status;
  result.http_status_code = http_code;
  result.location = NULL;
  result.file_path = "";
  result.error_message = message;
  result.is_directory = false;
  result.should_list_directory = false;
  result.is_cgi_request = false;
  return result;
}
