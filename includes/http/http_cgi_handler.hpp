/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_cgi_handler.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hpehliva <hpehliva@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/11 09:25:47 by hpehliva          #+#    #+#             */
/*   Updated: 2025/08/12 08:15:12 by hpehliva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_CGI_HANDLER_HPP
#define HTTP_CGI_HANDLER_HPP

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include "http_request.hpp"
#include "http_response_handling.hpp"
#include "../structs/location_config.hpp"

class CgiHandler {
    private:
        static const int PIPE_READ = 0;
        static const int PIPE_WRITE = 1;
        static const size_t MAX_ENV_SIZE = 8192;
        static const int CGI_TIMEOUT = 30; // seconds
    public:
        CgiHandler();
        ~CgiHandler();
        std::string execute_cgi(const HttpRequest& request, const LocationConfig& location, const std::string& script_path);
        
    private:
        std::vector<std::string> build_cgi_environment(const HttpRequest& request, const LocationConfig& location, const std::string& script_path);
        char** create_env_array(const std::vector<std::string>& env_vars);
        void cleanup_env_array(char** env_array, size_t size);
        
        pid_t fork_cgi_process(const std::string& cgi_binary, const std::string& script_path, char ** env_array, int input_pipe[2], int output_pipe[2]);
        std::string read_cgi_output(int output_fd, pid_t cgi_pid);
        void write_cgi_input(int input_fd, const std::string& input_data);

        std::string build_http_response(const std::string& cgi_output);
        bool wait_for_process(pid_t pid, int timeout_seconds);
        
        std::string create_cgi_errror(int error_code, const std::string& message);
};

#endif