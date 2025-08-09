/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_response_handling.hpp                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ksinn <ksinn@student.42heilbronn.de>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/08 00:12:53 by hpehliva          #+#    #+#             */
/*   Updated: 2025/08/09 12:47:57 by ksinn            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_RESPONSE_HANDLING_HPP
#define HTTP_RESPONSE_HANDLING_HPP

#include "../structs/server_config.hpp"
#include "http_request.hpp"
#include "routing.hpp"

class HttpResponseHandling {
private:
  const ServerConfig *server_config;

public:
  explicit HttpResponseHandling(const ServerConfig *server_config);
  ~HttpResponseHandling();

  std::string handle_request(const HttpRequest &request,
                             const RouteResult &route_result);
  std::string build_error_response(int status_code, const std::string &message);

private:
  std::string handle_get_request(const HttpRequest &request,
                                 const RouteResult &route_result);
  std::string handle_post_request(const HttpRequest &request,
                                  const RouteResult &route_result);
  std::string handle_delete_request(const HttpRequest &request,
                                    const RouteResult &route_result);
  // put request is not implemented yet
  // head request is not implemented yet

  std::string serve_file(const std::string &file_path);
  std::string serve_directory_listing(const std::string &directory_path,
                                      const std::string &uri);
  // std::string serve_cgi(const HttpRequest& request);
  // std::string serve_redirect(const std::string& redirect_url);
  // std::string serve_default_page(const std::string& directory_path);

  std::string build_response(int status_code, const std::string &content_type,
                             const std::string &content);

  std::string get_mime_type(const std::string &file_path);
  std::string get_status_message(int status_code);
  bool file_exists(const std::string &path);
  bool is_directory(const std::string &path);
  std::string read_file(const std::string &path);
};
#endif
