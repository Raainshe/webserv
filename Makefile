NAME = webserv

SRC_DIR = src
OUT_DIR = out
INC_DIR = includes

# List source subdirectories here (add more as needed)
SRC_SUBDIRS = \
	$(SRC_DIR)

VPATH = $(SRC_SUBDIRS)

# List all source files (just filenames, no paths)
SRCS = \
	webserv.cpp \
	parsing/parser.cpp \
	parsing/tokenizer.cpp \
	parsing/parsing.cpp \
	networking/socket_manager.cpp \
	networking/client_connection.cpp \
	networking/event_loop.cpp \
	http/http_request.cpp \
	http/request_parser.cpp \
	http/routing.cpp \
	http/http_response_handling.cpp \
	http/http_cgi_handler.cpp

# Object files
OBJS = \
	$(OUT_DIR)/webserv.o \
	$(OUT_DIR)/parsing/parser.o \
	$(OUT_DIR)/parsing/tokenizer.o \
	$(OUT_DIR)/parsing/parsing.o \
	$(OUT_DIR)/networking/socket_manager.o \
	$(OUT_DIR)/networking/client_connection.o \
	$(OUT_DIR)/networking/event_loop.o \
	$(OUT_DIR)/http/http_request.o \
	$(OUT_DIR)/http/request_parser.o \
	$(OUT_DIR)/http/routing.o \
	$(OUT_DIR)/http/http_response_handling.o \
	$(OUT_DIR)/http/http_cgi_handler.o

# Compiler and flags
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++17 -I$(INC_DIR)
DEBUG_FLAGS = -g -O0

# Colors
GREEN = \033[0;32m
YELLOW = \033[0;33m
BLUE = \033[0;34m
PURPLE = \033[0;35m
CYAN = \033[0;36m
NC = \033[0m

all: $(NAME)

$(OUT_DIR):
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OUT_DIR)/parsing
	@mkdir -p $(OUT_DIR)/networking
	@mkdir -p $(OUT_DIR)/http
	@echo "$(BLUE)Created $(OUT_DIR) directory structure$(NC)"

$(OUT_DIR)/%.o: %.cpp | $(OUT_DIR)
	@echo "$(CYAN)Compiling $<...$(NC)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT_DIR)/parsing/%.o: src/parsing/%.cpp | $(OUT_DIR)
	@echo "$(CYAN)Compiling $<...$(NC)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT_DIR)/networking/%.o: src/networking/%.cpp | $(OUT_DIR)
	@echo "$(CYAN)Compiling $<...$(NC)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT_DIR)/http/%.o: src/http/%.cpp | $(OUT_DIR)
	@echo "$(CYAN)Compiling $<...$(NC)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	@echo "$(YELLOW)Linking $(NAME)...$(NC)"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)$(NAME) built successfully!$(NC)"
	@echo "$(GREEN)Run with ./webserv <config_file>$(NC)"

debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: fclean $(NAME)
	@echo "$(GREEN)$(NAME) built with debug flags!$(NC)"

clean:
	@echo "$(PURPLE)Cleaning object files...$(NC)"
	@rm -rf $(OUT_DIR)
	@echo "$(GREEN)Clean completed!$(NC)"

fclean: clean
	@echo "$(PURPLE)Removing $(NAME) and test binaries...$(NC)"
	@rm -f $(NAME)
	@rm -f test_tokenizer
	@rm -f test_parser
	@rm -f test_error_handling
	# Add any new test binaries here to ensure they are cleaned
	@echo "$(GREEN)Full clean completed!$(NC)"

re: fclean all

help:
	@echo "$(GREEN)Available targets:$(NC)"
	@echo "  all     - Build everything (webserv)"
	@echo "  debug   - Build with debug flags (-g -O0)"
	@echo "  clean   - Remove object files"
	@echo "  fclean  - Remove object files and executables"
	@echo "  re      - Rebuild everything"
	@echo "  help    - Show this help message"

# Test target for tokenizer
TEST_TOKENIZER_SRC = src/parsing/test_tokenizer.cpp src/parsing/tokenizer.cpp
TEST_TOKENIZER_OBJ = $(patsubst src/%.cpp,$(OUT_DIR)/%.o,$(TEST_TOKENIZER_SRC))

test_tokenizer: $(OUT_DIR) $(TEST_TOKENIZER_OBJ)
	@echo "$(YELLOW)Linking test_tokenizer...$(NC)"
	@$(CXX) $(CXXFLAGS) $(TEST_TOKENIZER_OBJ) -o test_tokenizer
	@echo "$(GREEN)test_tokenizer built successfully!$(NC)"

run_test_tokenizer: test_tokenizer
	@./test_tokenizer

# Test target for parser
TEST_PARSER_SRC = src/parsing/test_parser.cpp src/parsing/parser.cpp src/parsing/tokenizer.cpp
TEST_PARSER_OBJ = $(patsubst src/%.cpp,$(OUT_DIR)/%.o,$(TEST_PARSER_SRC))

test_parser: $(OUT_DIR) $(TEST_PARSER_OBJ)
	@echo "$(YELLOW)Linking test_parser...$(NC)"
	@$(CXX) $(CXXFLAGS) $(TEST_PARSER_OBJ) -o test_parser
	@echo "$(GREEN)test_parser built successfully!$(NC)"

run_test_parser: test_parser
	@./test_parser

# Test target for error handling
TEST_ERROR_HANDLING_SRC = src/parsing/test_error_handling.cpp src/parsing/parser.cpp src/parsing/tokenizer.cpp
TEST_ERROR_HANDLING_OBJ = $(patsubst src/%.cpp,$(OUT_DIR)/%.o,$(TEST_ERROR_HANDLING_SRC))

test_error_handling: $(OUT_DIR) $(TEST_ERROR_HANDLING_OBJ)
	@echo "$(YELLOW)Linking test_error_handling...$(NC)"
	@$(CXX) $(CXXFLAGS) $(TEST_ERROR_HANDLING_OBJ) -o test_error_handling
	@echo "$(GREEN)test_error_handling built successfully!$(NC)"

run_test_error_handling: test_error_handling
	@./test_error_handling

.PHONY: all clean fclean re debug help test_tokenizer run_test_tokenizer test_parser run_test_parser test_error_handling run_test_error_handling
