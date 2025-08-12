/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   routing.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ksinn <ksinn@student.42heilbronn.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/04 15:45:00 by ksinn             #+#    #+#             */
/*   Updated: 2025/08/11 16:02:11 by ksinn            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ROUTING_HPP
#define ROUTING_HPP

#include "../structs/location_config.hpp"
#include "../structs/server_config.hpp"
#include "http_request.hpp"
#include <string>

enum RouteStatus {
  ROUTE_OK,                 // Route found and valid
  ROUTE_NOT_FOUND,          // No matching location (404)
  ROUTE_METHOD_NOT_ALLOWED, // Method not allowed for this location (405)
  ROUTE_INTERNAL_ERROR      // Internal routing error (500)
};

struct RouteResult {
  RouteStatus status;
  int http_status_code;           // HTTP status code to return
  const LocationConfig *location; // Matched location config
  std::string file_path;          // Resolved filesystem path
  std::string error_message;      // Error description for debugging
  bool is_directory;              // True if path points to directory
  bool should_list_directory;     // True if directory listing should be shown
  bool is_cgi_request;            // True if this should be handled by CGI
  bool is_redirect;               // True if this route should redirect
  std::string redirect_location;  // Location header target
};

class Router {
public:
  Router();
  ~Router();

  // Main routing method
  RouteResult route_request(const ServerConfig &server,
                            const HttpRequest &request);

private:
  // Location matching
  const LocationConfig *find_matching_location(const ServerConfig &server,
                                               const std::string &uri);

  // Method validation
  bool is_method_allowed(const LocationConfig &location, HttpMethod method);

  // Path resolution
  std::string resolve_file_path(const LocationConfig &location,
                                const std::string &uri);

  // Utility methods
  std::string normalize_path(const std::string &path);
  std::string join_paths(const std::string &root, const std::string &path);
  bool path_exists(const std::string &path);
  bool is_directory(const std::string &path);
  std::string method_to_string(HttpMethod method);

  // Error handling
  RouteResult create_error_result(RouteStatus status, int http_code,
                                  const std::string &message);
};

#endif // ROUTING_HPP
