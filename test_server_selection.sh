#!/bin/bash

# Test script for Step 10: Multiple Servers/Ports functionality
# This script demonstrates server selection based on Host headers

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
WEBSERV_BIN="./webserv"
CONFIG_FILE="configs/test.conf"
WEBSERV_PID=""

# Test counter
TEST_COUNT=0
PASS_COUNT=0

echo -e "${CYAN}=================================${NC}"
echo -e "${CYAN}  Multiple Servers/Ports Test   ${NC}"
echo -e "${CYAN}=================================${NC}"
echo

# Function to print test header
print_test() {
    TEST_COUNT=$((TEST_COUNT + 1))
    echo -e "${BLUE}Test $TEST_COUNT: $1${NC}"
    echo -e "${YELLOW}Command: $2${NC}"
}

# Function to verify response
verify_response() {
    local expected="$1"
    local actual="$2"
    local test_name="$3"

    if [[ "$actual" == *"$expected"* ]]; then
        echo -e "${GREEN}✓ PASS${NC}: Response contains '$expected'"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo -e "${RED}✗ FAIL${NC}: Expected '$expected', got '$actual'"
    fi
    echo
}

# Function to start webserver
start_webserver() {
    echo -e "${YELLOW}Starting webserver with $CONFIG_FILE...${NC}"
    $WEBSERV_BIN $CONFIG_FILE > webserv.log 2>&1 &
    WEBSERV_PID=$!

    # Wait for server to start
    sleep 2

    if kill -0 $WEBSERV_PID 2>/dev/null; then
        echo -e "${GREEN}✓ Webserver started successfully (PID: $WEBSERV_PID)${NC}"

        # Show server configuration
        echo -e "${CYAN}Server Configuration:${NC}"
        echo "  - Server 1: localhost:8080"
        echo "  - Server 2: test.local:8081"
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
    rm -f webserv.log
    exit
}

# Set up signal handlers
trap cleanup INT TERM

# Check if webserv binary exists
if [[ ! -f "$WEBSERV_BIN" ]]; then
    echo -e "${RED}Error: $WEBSERV_BIN not found. Please run 'make' first.${NC}"
    exit 1
fi

# Check if config file exists
if [[ ! -f "$CONFIG_FILE" ]]; then
    echo -e "${RED}Error: $CONFIG_FILE not found.${NC}"
    exit 1
fi

# Start the webserver
start_webserver

echo -e "${CYAN}Running Server Selection Tests...${NC}"
echo

# Test 1: Host header matches first server
print_test "Host header matches first server (localhost)" "curl -s -H 'Host: localhost' http://localhost:8080/"
response=$(curl -s -H "Host: localhost" http://localhost:8080/)
verify_response "Server: localhost (port 8080)" "$response"

# Test 2: Host header matches second server
print_test "Host header matches second server (test.local)" "curl -s -H 'Host: test.local' http://localhost:8081/"
response=$(curl -s -H "Host: test.local" http://localhost:8081/)
verify_response "Server: test.local (port 8081)" "$response"

# Test 3: Host header with port number (should strip port)
print_test "Host header with port number" "curl -s -H 'Host: localhost:8080' http://localhost:8080/"
response=$(curl -s -H "Host: localhost:8080" http://localhost:8080/)
verify_response "Server: localhost (port 8080)" "$response"

# Test 4: Non-matching Host header (should use default server)
print_test "Non-matching Host header (should use default)" "curl -s -H 'Host: nonexistent.com' http://localhost:8080/"
response=$(curl -s -H "Host: nonexistent.com" http://localhost:8080/)
verify_response "Server: localhost (port 8080)" "$response"

# Test 5: No Host header (should use default server)
print_test "No Host header (should use default)" "curl -s http://localhost:8080/"
response=$(curl -s http://localhost:8080/)
verify_response "Server: localhost (port 8080)" "$response"

# Test 6: Different path on first server
print_test "Different path on first server" "curl -s -H 'Host: localhost' http://localhost:8080/test/path"
response=$(curl -s -H "Host: localhost" http://localhost:8080/test/path)
verify_response "Server: localhost (port 8080)" "$response"

# Test 7: Different path on second server
print_test "Different path on second server" "curl -s -H 'Host: test.local' http://localhost:8081/api/endpoint"
response=$(curl -s -H "Host: test.local" http://localhost:8081/api/endpoint)
verify_response "Server: test.local (port 8081)" "$response"

# Test 8: Case sensitivity test
print_test "Case sensitivity test (LOCALHOST)" "curl -s -H 'Host: LOCALHOST' http://localhost:8080/"
response=$(curl -s -H "Host: LOCALHOST" http://localhost:8080/)
# This should NOT match (case sensitive), so should use default
verify_response "Server: localhost (port 8080)" "$response"

# Test 9: Host header with different case for second server
print_test "Case sensitivity test (TEST.LOCAL)" "curl -s -H 'Host: TEST.LOCAL' http://localhost:8081/"
response=$(curl -s -H "Host: TEST.LOCAL" http://localhost:8081/)
# This should NOT match (case sensitive), so should use default
verify_response "Server: test.local (port 8081)" "$response"

# Test 10: Multiple rapid requests to test stability
print_test "Multiple rapid requests (5 requests)" "for i in {1..5}; do curl -s -H 'Host: localhost' http://localhost:8080/; done"
echo "Sending 5 rapid requests..."
for i in {1..5}; do
    response=$(curl -s -H "Host: localhost" http://localhost:8080/)
    if [[ "$response" == *"Server: localhost (port 8080)"* ]]; then
        echo -e "  Request $i: ${GREEN}✓${NC}"
    else
        echo -e "  Request $i: ${RED}✗${NC}"
    fi
done
PASS_COUNT=$((PASS_COUNT + 1))
echo

# Show server logs for the last few requests
echo -e "${CYAN}Recent Server Log Output:${NC}"
echo -e "${YELLOW}========================${NC}"
tail -20 webserv.log | grep -E "(Host header|Selected server|HTTP request completed)" | tail -10
echo

# Summary
echo -e "${CYAN}=================================${NC}"
echo -e "${CYAN}         Test Summary            ${NC}"
echo -e "${CYAN}=================================${NC}"
echo -e "Total Tests: ${BLUE}$TEST_COUNT${NC}"
echo -e "Passed:      ${GREEN}$PASS_COUNT${NC}"
echo -e "Failed:      ${RED}$((TEST_COUNT - PASS_COUNT))${NC}"

if [[ $PASS_COUNT -eq $TEST_COUNT ]]; then
    echo -e "\n${GREEN}🎉 All tests passed! Multiple servers/ports is working correctly.${NC}"
else
    echo -e "\n${RED}❌ Some tests failed. Check the implementation.${NC}"
fi

# Cleanup
cleanup
