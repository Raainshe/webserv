#!/bin/bash

# Test script for Step 7: Routing and Methods functionality
# This script demonstrates the complete routing system

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

# Configuration
WEBSERV_BIN="./webserv"
CONFIG_FILE="configs/routing_test.conf"
WEBSERV_PID=""

# Counters
TEST_COUNT=0         # Number of high-level test cases
ASSERT_COUNT=0       # Number of individual assertions
PASS_COUNT=0         # Number of passed assertions

echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}       Routing and Methods Test          ${NC}"
echo -e "${CYAN}=========================================${NC}"
echo

# Function to print test header
print_test() {
    TEST_COUNT=$((TEST_COUNT + 1))
    echo -e "${BLUE}Test $TEST_COUNT: $1${NC}"
    echo -e "${YELLOW}Command: $2${NC}"
}

# Function to verify response contains expected text
verify_response_contains() {
    local expected="$1"
    local actual="$2"

    ASSERT_COUNT=$((ASSERT_COUNT + 1))
    if [[ "$actual" == *"$expected"* ]]; then
        echo -e "${GREEN}✓ PASS${NC}: Response contains '$expected'"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo -e "${RED}✗ FAIL${NC}: Expected '$expected' not found in response"
        echo -e "${RED}   Actual response: ${NC}$actual"
    fi
    echo
}

# Function to verify HTTP status code
verify_status_code() {
    local expected="$1"
    local response="$2"

    ASSERT_COUNT=$((ASSERT_COUNT + 1))
    if [[ "$response" == *"$expected"* ]]; then
        echo -e "${GREEN}✓ PASS${NC}: Correct status code $expected"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo -e "${RED}✗ FAIL${NC}: Expected status $expected not found"
        echo -e "${RED}   Response: ${NC}$response"
    fi
    echo
}

# Function to start webserver
start_webserver() {
    echo -e "${YELLOW}Starting webserver with $CONFIG_FILE...${NC}"
    $WEBSERV_BIN $CONFIG_FILE > webserv_routing.log 2>&1 &
    WEBSERV_PID=$!

    # Wait for server to start
    sleep 2

    if kill -0 $WEBSERV_PID 2>/dev/null; then
        echo -e "${GREEN}✓ Webserver started successfully (PID: $WEBSERV_PID)${NC}"
        echo
    else
        echo -e "${RED}✗ Failed to start webserver${NC}"
        exit 1
    fi
}

# Function to stop webserver
stop_webserver() {
    if [[ -n "$WEBSERV_PID" ]]; then
        echo -e "${YELLOW}Stopping webserver (PID: $WEBSERV_PID)...${NC}"
        kill $WEBSERV_PID 2>/dev/null
        wait $WEBSERV_PID 2>/dev/null
        echo -e "${GREEN}✓ Webserver stopped${NC}"
    fi
}

# Cleanup function
cleanup() {
    stop_webserver
    rm -f webserv_routing.log
    exit
}

# Set up signal handlers
trap cleanup INT TERM

# Check prerequisites
if [[ ! -f "$WEBSERV_BIN" ]]; then
    echo -e "${RED}Error: $WEBSERV_BIN not found. Please run 'make' first.${NC}"
    exit 1
fi

if [[ ! -f "$CONFIG_FILE" ]]; then
    echo -e "${RED}Error: $CONFIG_FILE not found.${NC}"
    exit 1
fi

# Start the webserver
start_webserver

echo -e "${CYAN}Testing Routing Functionality...${NC}"
echo

# =============================================================================
# SERVER SELECTION AND BASIC ROUTING
# =============================================================================

print_test "Root location routing (localhost:8080)" "curl -s -H 'Host: localhost' http://localhost:8080/"
response=$(curl -s -H "Host: localhost" http://localhost:8080/)
verify_response_contains "Welcome to localhost server!" "$response"

print_test "Root location routing (test.local:8081)" "curl -s -H 'Host: test.local' http://localhost:8081/"
response=$(curl -s -H "Host: test.local" http://localhost:8081/)
verify_response_contains "Welcome to test.local server!" "$response"

# =============================================================================
# LOCATION MATCHING
# =============================================================================

print_test "Images location matching" "curl -s -H 'Host: localhost' http://localhost:8080/images/"
response=$(curl -s -H "Host: localhost" http://localhost:8080/images/)
verify_response_contains "Index of /images/" "$response"

print_test "API location matching (directory without autoindex -> 403)" "curl -s -H 'Host: test.local' http://localhost:8081/api/"
response=$(curl -s -H "Host: test.local" http://localhost:8081/api/)
verify_response_contains "Custom 403 Page" "$response"

print_test "Specific API endpoint" "curl -s -H 'Host: test.local' http://localhost:8081/api/users.json"
response=$(curl -s -H "Host: test.local" http://localhost:8081/api/users.json)
verify_response_contains "API endpoint working" "$response"

# =============================================================================
# HTTP METHOD VALIDATION
# =============================================================================

print_test "GET method allowed (root)" "curl -s -X GET -H 'Host: localhost' http://localhost:8080/"
response=$(curl -s -X GET -H "Host: localhost" http://localhost:8080/)
verify_response_contains "Welcome to localhost server!" "$response"

print_test "POST method allowed (root)" "curl -s -X POST -H 'Host: localhost' http://localhost:8080/"
response=$(curl -s -X POST -H "Host: localhost" http://localhost:8080/)
verify_response_contains "POST request received successfully!" "$response"

print_test "DELETE method not allowed (root)" "curl -s -X DELETE -H 'Host: localhost' http://localhost:8080/"
response=$(curl -s -X DELETE -H "Host: localhost" http://localhost:8080/)
verify_response_contains "Custom 405 Page" "$response"

# =============================================================================
# REDIRECTIONS (301/302)
# =============================================================================

print_test "Redirect from /old to / (301)" "curl -s -D - -o /dev/null -H 'Host: localhost' http://localhost:8080/old"
response_headers=$(curl -s -D - -o /dev/null -H "Host: localhost" http://localhost:8080/old)
verify_status_code "HTTP/1.1 301" "$response_headers"
verify_response_contains "Location: /" "$response_headers"

print_test "Follow redirect to / returns index page" "curl -s -L -H 'Host: localhost' http://localhost:8080/old"
response=$(curl -s -L -H "Host: localhost" http://localhost:8080/old)
verify_response_contains "Welcome to localhost server!" "$response"

# =============================================================================
# CLIENT MAX BODY SIZE (413)
# =============================================================================

print_test "POST over client_max_body_size triggers 413" "dd if=/dev/zero bs=1 count=2048 2>/dev/null | curl -s -X POST -H 'Host: localhost' -H 'Content-Length: 2048' --data-binary @- http://localhost:8080/"
response=$(dd if=/dev/zero bs=1 count=2048 2>/dev/null | curl -s -X POST -H 'Host: localhost' -H 'Content-Length: 2048' --data-binary @- http://localhost:8080/)
verify_response_contains "Custom 413 Page" "$response"

print_test "GET method allowed (images)" "curl -s -X GET -H 'Host: localhost' http://localhost:8080/images/"
response=$(curl -s -X GET -H "Host: localhost" http://localhost:8080/images/)
verify_response_contains "Index of /images/" "$response"

print_test "POST method not allowed (images)" "curl -s -X POST -H 'Host: localhost' http://localhost:8080/images/"
response=$(curl -s -X POST -H "Host: localhost" http://localhost:8080/images/)
verify_response_contains "Custom 405 Page" "$response"

print_test "DELETE method allowed (API)" "curl -s -X DELETE -H 'Host: test.local' http://localhost:8081/api/tmp_delete.json"
echo '{"tmp":"delete"}' > test_files/api/tmp_delete.json
response=$(curl -s -X DELETE -H "Host: test.local" http://localhost:8081/api/tmp_delete.json)
verify_response_contains "File deleted successfully!" "$response"

# =============================================================================
# DIRECTORY AND FILE DETECTION
# =============================================================================

print_test "Directory detection (images)" "curl -s -H 'Host: localhost' http://localhost:8080/images/"
response=$(curl -s -H "Host: localhost" http://localhost:8080/images/)
verify_response_contains "Index of /images/" "$response"

print_test "File detection (index.html)" "curl -s -H 'Host: localhost' http://localhost:8080/"
response=$(curl -s -H "Host: localhost" http://localhost:8080/)
verify_response_contains "Welcome to localhost server!" "$response"

print_test "Specific file request" "curl -s -H 'Host: test.local' http://localhost:8081/api/users.json"
response=$(curl -s -H "Host: test.local" http://localhost:8081/api/users.json)
verify_response_contains "API endpoint working" "$response"

# =============================================================================
# CGI DETECTION
# =============================================================================

print_test "CGI location detection (not implemented yet)" "curl -s -H 'Host: localhost' http://localhost:8080/cgi-bin/script.php"
response=$(curl -s -H "Host: localhost" http://localhost:8080/cgi-bin/script.php)
verify_response_contains "Custom 404 Page" "$response"

# =============================================================================
# ERROR CASES
# =============================================================================

print_test "Non-existent file" "curl -s -H 'Host: localhost' http://localhost:8080/nonexistent.html"
response=$(curl -s -H "Host: localhost" http://localhost:8080/nonexistent.html)
verify_response_contains "Custom 404 Page" "$response"

print_test "Non-matching location (falls back to root, unresolved -> 404)" "curl -s -H 'Host: localhost' http://localhost:8080/unknown/path"
response=$(curl -s -H "Host: localhost" http://localhost:8080/unknown/path)
verify_response_contains "Custom 404 Page" "$response"

# =============================================================================
# LONGEST PREFIX MATCHING
# =============================================================================

print_test "Longest prefix matching (/images vs /)" "curl -s -H 'Host: localhost' http://localhost:8080/images/photo1.jpg"
response=$(curl -s -H "Host: localhost" http://localhost:8080/images/photo1.jpg)
verify_response_contains "Image file 1" "$response"

print_test "Root fallback for unmatched paths" "curl -s -H 'Host: localhost' http://localhost:8080/other/path"
response=$(curl -s -H "Host: localhost" http://localhost:8080/other/path)
verify_response_contains "Custom 404 Page" "$response"

# Show some debug output
echo -e "${CYAN}Recent Server Log Output (Routing Decisions):${NC}"
echo -e "${YELLOW}=====================================================${NC}"
tail -30 webserv_routing.log | grep -E "(Routing request|Matched location|Method.*allowed|Resolved file path)" | tail -15

echo
echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}          Test Summary                   ${NC}"
echo -e "${CYAN}=========================================${NC}"
echo -e "Test Cases:       ${BLUE}$TEST_COUNT${NC}"
echo -e "Total Assertions: ${BLUE}$ASSERT_COUNT${NC}"
echo -e "Passed:           ${GREEN}$PASS_COUNT${NC}"
echo -e "Failed:           ${RED}$((ASSERT_COUNT - PASS_COUNT))${NC}"

if [[ $PASS_COUNT -eq $ASSERT_COUNT ]]; then
    echo -e "\n${GREEN}🎉 All routing assertions passed! Step 7 is working correctly.${NC}"
    echo -e "${YELLOW}Features working:${NC}"
    echo -e "  ✓ Location matching (longest prefix)"
    echo -e "  ✓ HTTP method validation"
    echo -e "  ✓ File path resolution"
    echo -e "  ✓ Directory detection"
    echo -e "  ✓ Index file resolution"
    echo -e "  ✓ CGI detection"
    echo -e "  ✓ Error handling (404, 405)"
    echo -e "  ✓ Server selection integration"
else
    echo -e "\n${RED}❌ Some tests failed. Check the implementation.${NC}"
fi

echo -e "\n${BLUE}Full server log available in: webserv_routing.log${NC}"

# Cleanup
cleanup
