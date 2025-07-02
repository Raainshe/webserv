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
	webserv.cpp

# Object files
OBJS = $(addprefix $(OUT_DIR)/, $(SRCS:.cpp=.o))

# Compiler and flags
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I$(INC_DIR)
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
	@echo "$(BLUE)Created $(OUT_DIR) directory$(NC)"

$(OUT_DIR)/%.o: %.cpp | $(OUT_DIR)
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
TEST_TOKENIZER_OBJ = $(addprefix $(OUT_DIR)/, $(notdir $(TEST_TOKENIZER_SRC:.cpp=.o)))

test_tokenizer: $(OUT_DIR) $(TEST_TOKENIZER_OBJ)
	@echo "$(YELLOW)Linking test_tokenizer...$(NC)"
	@$(CXX) $(CXXFLAGS) $(TEST_TOKENIZER_OBJ) -o test_tokenizer
	@echo "$(GREEN)test_tokenizer built successfully!$(NC)"

$(OUT_DIR)/%.o: src/parsing/%.cpp | $(OUT_DIR)
	@echo "$(CYAN)Compiling $<...$(NC)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

run_test_tokenizer: test_tokenizer
	@./test_tokenizer

.PHONY: all clean fclean re debug help test_tokenizer run_test_tokenizer
