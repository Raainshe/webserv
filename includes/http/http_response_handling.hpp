/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_response_handling.hpp                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hpehliva <hpehliva@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/08 00:12:53 by hpehliva          #+#    #+#             */
/*   Updated: 2025/08/08 02:40:14 by hpehliva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_RESPONSE_HANDLING_HPP
#define HTTP_RESPONSE_HANDLING_HPP

#include "http_request.hpp"
#include "request_parser.hpp"
#include "../structs/server_config.hpp"
#include "../structs/location_config.hpp"

class HttpResponseHandling {
private:
    // const ServerConfig* server_config;

public:
    HttpResponseHandling(const ServerConfig* server_config);
    ~HttpResponseHandling();

    std::string handle_request(const HttpRequest& request);
private:
    std::string handle_get_request(const HttpRequest& request);
    std::string handle_post_request(const HttpRequest& request);
    std::string handle_delete_request(const HttpRequest& request);
    // put request is not implemented yet
    // head request is not implemented yet

    std::string serve_file(const std::string& file_path);
    std::string serve_directory(const std::string& directory_path, const std::string& uri);
    // std::string serve_error_page(int error_code);
    // std::string serve_cgi(const HttpRequest& request);
    // std::string serve_redirect(const std::string& redirect_url);
    // std::string serve_autoindex(const std::string& directory_path);
    // std::string serve_default_page(const std::string& directory_path);
    // std::string serve_index_page(const std::string& directory_path);

    std::string build_response(int status_code, const std::string& content_type, const std::string& content);
    std::string build_error_response(int status_code, const std::string& message);

    std::string get_file_path(const std::string& uri);
    std::string get_mime_type(const std::string& file_path);
    std::string get_status_message(int status_code);
    bool file_exists(const std::string& path);
    bool is_directory(const std::string& path);
    std::string read_file(const std::string& path);  
};
#endif