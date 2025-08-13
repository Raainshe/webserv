#!/bin/bash

# CGI Integration Test Suite

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

WEBSERV_BIN="./webserv"
CONFIG_FILE="configs/cgi_test.conf"
WEBSERV_PID=""

TESTS=0
ASSERTS=0
PASSES=0

print_test() {
  TESTS=$((TESTS+1))
  echo -e "${BLUE}Test $TESTS: $1${NC}"
  echo -e "${YELLOW}Command: $2${NC}"
}

expect_contains() {
  local expected="$1"
  local actual="$2"
  ASSERTS=$((ASSERTS+1))
  if [[ "$actual" == *"$expected"* ]]; then
    echo -e "${GREEN}✓ PASS${NC}: contains '$expected'"
    PASSES=$((PASSES+1))
  else
    echo -e "${RED}✗ FAIL${NC}: expected '$expected' not found"
    echo -e "${RED}   Actual:${NC} $actual"
  fi
  echo
}

start_server() {
  echo -e "${YELLOW}Starting webserver with $CONFIG_FILE...${NC}"
  $WEBSERV_BIN $CONFIG_FILE > webserv_cgi.log 2>&1 &
  WEBSERV_PID=$!
  sleep 2
  if kill -0 $WEBSERV_PID 2>/dev/null; then
    echo -e "${GREEN}✓ Webserver started (PID: $WEBSERV_PID)${NC}\n"
  else
    echo -e "${RED}✗ Failed to start webserver${NC}"
    exit 1
  fi
}

stop_server() {
  if [[ -n "$WEBSERV_PID" ]]; then
    echo -e "${YELLOW}Stopping webserver (PID: $WEBSERV_PID)...${NC}"
    kill $WEBSERV_PID 2>/dev/null
    wait $WEBSERV_PID 2>/dev/null
    echo -e "${GREEN}✓ Webserver stopped${NC}"
  fi
}

cleanup() {
  stop_server
  rm -f webserv_cgi.log
}

trap cleanup INT TERM EXIT

if [[ ! -f "$WEBSERV_BIN" ]]; then
  echo -e "${RED}Error: $WEBSERV_BIN not found. Run 'make' first.${NC}"
  exit 1
fi

chmod +x test_files/cgi-bin/*.py 2>/dev/null || true

echo -e "${CYAN}=================================${NC}"
echo -e "${CYAN}       CGI Integration Test       ${NC}"
echo -e "${CYAN}=================================${NC}\n"

start_server

# 1) GET to hello.py
print_test "GET hello.py returns text" "curl -s -H 'Host: localhost' http://localhost:8082/cgi-bin/hello.py"
resp=$(curl -s -H 'Host: localhost' http://localhost:8082/cgi-bin/hello.py)
expect_contains "hello from cgi" "$resp"

# 2) POST body echoed back
print_test "POST echoes body" "curl -s -X POST -H 'Host: localhost' --data 'abc123' http://localhost:8082/cgi-bin/echo_post.py"
resp=$(curl -s -X POST -H 'Host: localhost' --data 'abc123' http://localhost:8082/cgi-bin/echo_post.py)
expect_contains "abc123" "$resp"

# 3) Missing script → 404 from CGI layer
print_test "Missing script yields not found" "curl -s -H 'Host: localhost' http://localhost:8082/cgi-bin/missing.py"
resp=$(curl -s -H 'Host: localhost' http://localhost:8082/cgi-bin/missing.py)
expect_contains "CGI script not found" "$resp"

echo -e "${CYAN}=================================${NC}"
echo -e "${CYAN}           Test Summary           ${NC}"
echo -e "${CYAN}=================================${NC}"
echo -e "Tests:           ${BLUE}$TESTS${NC}"
echo -e "Assertions:      ${BLUE}$ASSERTS${NC}"
echo -e "Passed:          ${GREEN}$PASSES${NC}"
echo -e "Failed:          ${RED}$((ASSERTS - PASSES))${NC}"

exit 0


