/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_cgi_handler.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hpehliva <hpehliva@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/11 10:28:57 by hpehliva          #+#    #+#             */
/*   Updated: 2025/08/12 08:23:41 by hpehliva         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/http/http_cgi_handler.hpp"
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../includes/webserv.hpp"

#define BUFFER_SIZE 8192

CgiHandler::CgiHandler(){}
CgiHandler::~CgiHandler(){}
std::string CgiHandler::execute_cgi(const HttpRequest& request, const LocationConfig& location, const std::string& script_path){
    std::cout << "Executing CGI script: " << location.cgi_pass << " " << script_path << std::endl;
    // Check if script axists
    if(access(script_path.c_str(), F_OK) == -1) {
        std::cerr << "CGI script not found: " << script_path << std::endl;
        return create_cgi_errror(404, "CGI script not found");
    }
    // Check if script is executable
    if(access(script_path.c_str(), X_OK) == -1) {
        std::cerr << "CGI script is not executable: " << script_path << std::endl;
        return create_cgi_errror(403, "CGI script is not executable");
    }
    int input_pipe[2], output_pipe[2];
    if(pipe(input_pipe) == -1 || pipe(output_pipe) == -1) {
        std::cerr << "Failed to create pipes for CGI execution" << std::endl;
        return create_cgi_errror(500, "Failed to create pipes for CGI execution");
    }

    std::vector<std::string> env_vars = build_cgi_environment(request, location, script_path);
    char **env_array = create_env_array(env_vars);
    
    pid_t cgi_pid = fork_cgi_process(location.cgi_pass, script_path, env_array, input_pipe, output_pipe);
    if(cgi_pid == -1)
    {
        cleanup_env_array(env_array, env_vars.size());
        close(input_pipe[0]); close(input_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);
        return create_cgi_errror(500, "Failed to fork CGI process");
    }
    close(input_pipe[0]);
    close(output_pipe[1]);

    if(request.get_method() == POST && !request.get_body().empty()){
        write_cgi_input(input_pipe[1], request.get_body());
    }
    close(input_pipe[1]);
    
    std::string cgi_output = read_cgi_output(output_pipe[0], cgi_pid);
    close(output_pipe[0]);
    if(!wait_for_process(cgi_pid, CGI_TIMEOUT)) {
        std::cerr << "CGI process timed out, killing process" << std::endl;
        kill(cgi_pid, SIGKILL);
        waitpid(cgi_pid, NULL, 0);
        cleanup_env_array(env_array, env_vars.size());
        return create_cgi_errror(504, "CGI process timed out");
    }

    int status;
    waitpid(cgi_pid, &status, 0);
    cleanup_env_array(env_array, env_vars.size());
    if(WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return build_http_response(cgi_output); // It could be reversed
    } else {
        std::cerr << "CGI script exited with error" << std::endl;
        return create_cgi_errror(500, "CGI script execution failed");
    }
    
    if(cgi_output.empty()) {
        std::cerr << "CGI script produced no output" << std::endl;
        return create_cgi_errror(500, "CGI script produced no output");
    }

    return build_http_response(cgi_output);
}

std::vector<std::string> CgiHandler::build_cgi_environment(const HttpRequest& request, const LocationConfig& location, const std::string& script_path)
{
    (void)location; // Unused variable, but can be used for future extensions
    std::vector<std::string> env_vars;
    
    std::string method_str;
    if (request.get_method() == GET) {
        method_str = "GET";
    } else if (request.get_method() == POST) {
        method_str = "POST";
    } else if (request.get_method() == DELETE) {
        method_str = "DELETE";
    } else {
        method_str = "UNKNOWN";
    }
    env_vars.push_back("REQUEST_METHOD=" + method_str);
    
    env_vars.push_back("SERVER_SOFTWARE=webserv/1.0");
    env_vars.push_back("SERVER_NAME=" + request.get_header("host"));
    env_vars.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env_vars.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env_vars.push_back("REQUEST_URI=" + request.get_uri());
    env_vars.push_back("SCRIPT_NAME=" + script_path);
    env_vars.push_back("QUERY_STRING=" +request.get_query_string());
    
    // Content lenght for POST request
    if(request.get_method() == POST) {
        std::ostringstream oss;
        oss << request.get_content_length();
        env_vars.push_back("CONTENT_LENGTH=" + oss.str());
        std::string content_type = request.get_header("content-type");
        if(!content_type.empty()) {
            env_vars.push_back("CONTENT_TYPE=" + content_type);
        }
    }

    // HTTP headers as enviroment variables
    const std::map<std::string, std::string>& headers = request.get_headers();
    for(std::map<std::string, std::string>::const_iterator it = headers.begin();
        it != headers.end(); ++it) {
            std::string header_name = it->first;
            
            // Conver to uppercase and replace - with -
        for(size_t i = 0; i < header_name.length(); ++i) {
            if(header_name[i] == '-') {
                header_name[i] = '_';
            } else {
                header_name[i] = toupper(header_name[i]);
            }
        }
        env_vars.push_back("HTTP_" + header_name + "=" + it->second);    
    }
    // ADD path for script execution
    env_vars.push_back("PATH=/usr/local/bin:/usr/bin:/bin");
    return env_vars;
}


char** CgiHandler::create_env_array(const std::vector<std::string>& env_vars){
    char** env_array = new char*[env_vars.size() + 1];
    
    for(size_t i = 0; i < env_vars.size(); ++i){
        env_array[i] = new char[env_vars[i].length() + 1];
        std::strcpy(env_array[i], env_vars[i].c_str());
    }
    env_array[env_vars.size()] = NULL; // Null-terminate the array
    return env_array;
}
void CgiHandler::cleanup_env_array(char** env_array, size_t size){
    for(size_t i = 0; i < size; ++i) {
        delete[] env_array[i];
    }
    delete[] env_array;
}

pid_t CgiHandler::fork_cgi_process(const std::string& cgi_binary, const std::string& script_path, char ** env_array, int input_pipe[2], int output_pipe[2]){
    pid_t pid = fork();
    
    if(pid == 0)
    {
        dup2(input_pipe[0], STDIN_FILENO);
        dup2(output_pipe[1], STDOUT_FILENO);

        close(input_pipe[0]); close(input_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);
        
        std::string script_dir = script_path.substr(0, script_path.find_last_of('/'));
        if(!script_dir.empty()) {
            chdir(script_dir.c_str());
            // if(chdir(script_dir.c_str()) == -1) {
            //     std::cerr << "Failed to change directory to script directory: " << script_dir << std::endl;
            //     exit(1);
            // }
        }

        // Execute CGI binary with script as argument
        char* args[] = {
            const_cast<char*>(cgi_binary.c_str()),
            const_cast<char*>(script_path.c_str()),
            NULL
        };
        
        execve(cgi_binary.c_str(), args, env_array);

        // if execve fails
        std::cerr << "Failed to execute CGI binary: " << cgi_binary << std::endl;
        exit(1);
        
    }
    return pid;
}
std::string CgiHandler::read_cgi_output(int output_fd, pid_t cgi_pid){
    std::string output;
    char buffer[BUFFER_SIZE];
    
    // set none-blocking mode for reading
    int flags = fcntl(output_fd, F_GETFL, 0);
    fcntl(output_fd, F_SETFL, flags | O_NONBLOCK);
    time_t start_time = time(NULL);

    while(time(NULL) - start_time < CGI_TIMEOUT)
    {
        ssize_t bytes_read = read(output_fd, buffer, sizeof(buffer) - 1);
        
        if(bytes_read > 0)
        {
            buffer[bytes_read] = '\0'; // Null-terminate the buffer
            output += buffer;
            start_time = time(NULL);
        } else if(bytes_read == 0) {
            // EOF reached, break the loop
            break;
        } else if(errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Error reading CGI output: " << strerror(errno) << std::endl;
            break;
        }

        // Check if the process is still running
        if(kill(cgi_pid, 0) != 0){
            break;
        }
        usleep(10000); // 10ms
    }
    return output;
}

void CgiHandler::write_cgi_input(int input_fd, const std::string& input_data) {
    size_t total_written = 0;
    const char* data = input_data.c_str();
    size_t data_size = input_data.length();

    while(total_written < data_size){
        ssize_t bytes_written = write(input_fd, data + total_written, data_size - total_written);

        if(bytes_written < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                // Non-blocking write, wait and try again
                // usleep(10000); // 10ms
                continue;
            } else {
                std::cerr << "Error writing to CGI input: " << strerror(errno) << std::endl;
                return ;
            }
        } else if(bytes_written == 0) {
            // No more data to write
            break;
        }
        total_written += bytes_written;
    }
}

std::string CgiHandler::build_http_response(const std::string& cgi_output){
    // find the separator between headers and body
    size_t header_end = cgi_output.find("\r\n\r\n");
    if(header_end == std::string::npos) {
        header_end = cgi_output.find("\n\n");
        if(header_end == std::string::npos){
            std::ostringstream response;
            response << "HTTP/1.1 200 OK\r\n";
            response << "Content-Type: text/html\r\n";
            response << "Content-Lenght: " << cgi_output.length() << "\r\n";
            response << "\r\n";
            response << cgi_output;
            return response.str();
        }
        header_end += 2;
    } else {
        header_end += 4;
    }
    
    std::string headers = cgi_output.substr(0, header_end);
    std::string body = cgi_output.substr(header_end);
    
    // PArse CGI headers
    bool has_status = false;
    bool has_content_type = false;
    std::string status = "200 OK";

    std::istringstream header_stream(headers);
    std::string line;

    while(std::getline(header_stream, line)) {
        if(line.empty() || line == "\r") break;
        
        size_t colon_pos = line.find(':');
        if(colon_pos != std::string::npos){
            std::string header_name = line.substr(0, colon_pos);
            std::string header_value = line.substr(colon_pos + 1);

            //trim whitespace
            while(!header_value.empty() && isspace(header_value[0])){
                header_value.erase(0,1);
            }
            while(!header_value.empty() && isspace(header_value[header_value.length() - 1]))
            {
                header_value.erase(header_value.length() -1);
            }

            if(header_name == "Status") {
                status = header_value;
                has_status = true;
            } else if(header_name == "Content-Type" || header_name == "Content-type") {
                // content_type = header_value;
                has_content_type = true;
            }
        }
    }
    // Build HTTP response
    std::ostringstream response;
    if(!has_status) {
        response << "HTTP/1.1 " << status << "\r\n";
    } else {
        response << "HTTP/1.1 " << status << "\r\n";
    }
    
    // Add CGI headers
    std::istringstream header_stream2(headers);
    while(std::getline(header_stream2, line)){
        if(line.empty() || line == "\r") break;
        
        size_t colon_pos = line.find(':');
        if(colon_pos != std::string::npos){
            std::string header_name = line.substr(0, colon_pos);
            if(header_name != "Status") {
                response << line << "\r\n";
            }
        }
    }
    
    if(!has_content_type)
    {
        response << "Content-type: text/html\r\n";
    }
    
    response << "Content-Length: " << body.length() << "\r\n";
    response << "\r\n";
    response << body;
    return response.str();
}
bool CgiHandler::wait_for_process(pid_t pid, int timeout_seconds){
    time_t start_time = time(NULL);
    
    while(time(NULL) - start_time < timeout_seconds){
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);
        
        if(result == pid)
            return true;
        else if (result == -1)
            return false;
        usleep(100000);// 100 ms
    }
    return false;
}

std::string CgiHandler::create_cgi_errror(int error_code, const std::string& message){
    std::ostringstream response;
    std::string status_message;
    
    switch(error_code){
        case 404: status_message = "Not Found"; break;
        case 403: status_message = "Forbidden"; break;
        case 500: status_message = "Internal Server Error"; break;
        case 504: status_message = "Gateway Timeout"; break;
        default: status_message = "Error"; break;
    }
    std::ostringstream body;
    body << "<!DOCTYPE html>\n";
    body << "<html><head><title>" << error_code << " " << status_message << "      </title></head>\n";
    body << "<body><h1>" << error_code << " " << status_message << "</h1>\n";
    body << "<p>" << message << "</p>\n";
    body << "<hr><p>webserv/1.0 CGI</p></body></html>\n";
    response << "HTTP/1.1 " << error_code << " " << status_message << "\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << body.str().length() << "\r\n";
    response << "\r\n";
    response << body.str();
    return response.str();
}