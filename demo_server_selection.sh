#!/bin/bash

# Interactive demo for Step 10: Multiple Servers/Ports functionality
# This script shows real-time server logs alongside HTTP requests

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
CONFIG_FILE="configs/test.conf"
WEBSERV_PID=""

echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}  Interactive Server Selection Demo     ${NC}"
echo -e "${CYAN}=========================================${NC}"
echo

# Function to start webserver in background and tail logs
start_webserver() {
    echo -e "${YELLOW}Starting webserver with $CONFIG_FILE...${NC}"
    $WEBSERV_BIN $CONFIG_FILE > webserv_demo.log 2>&1 &
    WEBSERV_PID=$!

    # Wait for server to start
    sleep 2

    if kill -0 $WEBSERV_PID 2>/dev/null; then
        echo -e "${GREEN}✓ Webserver started successfully (PID: $WEBSERV_PID)${NC}"
        echo

        # Show configuration from log
        echo -e "${CYAN}Configuration loaded:${NC}"
        grep -E "(Successfully parsed|Server socket created|Successfully initialized)" webserv_demo.log
        echo
    else
        echo -e "${RED}✗ Failed to start webserver${NC}"
        exit 1
    fi
}

# Function to send request and show logs
send_request_with_logs() {
    local test_name="$1"
    local curl_cmd="$2"
    local description="$3"

    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${PURPLE}Demo: $test_name${NC}"
    echo -e "${YELLOW}Command: $curl_cmd${NC}"
    echo -e "${CYAN}Expected: $description${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    # Get current log size to track new entries
    local log_size_before=$(wc -l < webserv_demo.log)

    echo -e "\n${GREEN}Sending request...${NC}"

    # Send the request
    local response=$(eval $curl_cmd)

    # Wait a moment for logs to be written
    sleep 1

    # Show new log entries
    echo -e "\n${CYAN}Server Debug Output:${NC}"
    echo -e "${YELLOW}───────────────────${NC}"
    tail -n +$((log_size_before + 1)) webserv_demo.log | grep -E "(HTTP request completed|Host header|Selected server|Read.*bytes|Sent.*bytes)"

    # Show response
    echo -e "\n${CYAN}HTTP Response:${NC}"
    echo -e "${YELLOW}─────────────${NC}"
    echo -e "${GREEN}$response${NC}"

    echo
    echo -e "${BLUE}Press Enter to continue...${NC}"
    read
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
    rm -f webserv_demo.log
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

echo -e "${YELLOW}This demo will show you how the server selection logic works in real-time.${NC}"
echo -e "${YELLOW}You'll see the server debug output for each HTTP request.${NC}"
echo
echo -e "${BLUE}Press Enter to start the demo...${NC}"
read

# Start the webserver
start_webserver

# Demo 1: Basic Host header matching
send_request_with_logs \
    "Host Header Matching (localhost)" \
    "curl -s -H 'Host: localhost' http://localhost:8080/" \
    "Should match server_name 'localhost' and select first server"

# Demo 2: Second server
send_request_with_logs \
    "Second Server Selection (test.local)" \
    "curl -s -H 'Host: test.local' http://localhost:8081/" \
    "Should match server_name 'test.local' and select second server"

# Demo 3: Host header with port
send_request_with_logs \
    "Host Header with Port Number" \
    "curl -s -H 'Host: localhost:8080' http://localhost:8080/" \
    "Should strip port and match 'localhost', selecting first server"

# Demo 4: Non-matching host
send_request_with_logs \
    "Non-matching Host Header" \
    "curl -s -H 'Host: unknown.domain' http://localhost:8080/" \
    "Should NOT match any server_name, use default server (localhost)"

# Demo 5: No host header
send_request_with_logs \
    "Missing Host Header" \
    "curl -s http://localhost:8080/" \
    "Should use default server (localhost) when no Host header present"

# Demo 6: Case sensitivity
send_request_with_logs \
    "Case Sensitivity Test" \
    "curl -s -H 'Host: LOCALHOST' http://localhost:8080/" \
    "Should NOT match 'LOCALHOST' vs 'localhost' (case sensitive), use default"

# Final summary
echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}            Demo Complete!               ${NC}"
echo -e "${CYAN}=========================================${NC}"
echo
echo -e "${GREEN}✓ All server selection scenarios demonstrated${NC}"
echo -e "${YELLOW}Key observations:${NC}"
echo -e "  • Host header matching is case-sensitive"
echo -e "  • Port numbers are stripped from Host headers"
echo -e "  • Non-matching hosts use the default server"
echo -e "  • Each socket has its own default server"
echo
echo -e "${BLUE}Full server log available in: webserv_demo.log${NC}"

# Cleanup
cleanup
