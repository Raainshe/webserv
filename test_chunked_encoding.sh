#!/usr/bin/env bash
# Step 8: Chunked Encoding - Test Script

set -u -o pipefail

# Colors for output (same style as test_routing.sh)
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

# Configuration
ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"
WEBSERV_BIN="./webserv"
CONFIG_FILE="configs/default.conf"
LOG_FILE="webserv_chunked.log"
WEBSERV_PID=""

# Counters
TEST_COUNT=0         # Number of high-level test cases
ASSERT_COUNT=0       # Number of individual assertions
PASS_COUNT=0         # Number of passed assertions

# Header
echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}     Step 8: Chunked Encoding Tests      ${NC}"
echo -e "${CYAN}=========================================${NC}"
echo

print_test() {
  TEST_COUNT=$((TEST_COUNT + 1))
  echo -e "${BLUE}Test $TEST_COUNT: $1${NC}"
  [[ -n "${2:-}" ]] && echo -e "${YELLOW}Command: $2${NC}"
}

pass() {
  echo -e "${GREEN}✓ PASS${NC}: $1"
  PASS_COUNT=$((PASS_COUNT+1))
  ASSERT_COUNT=$((ASSERT_COUNT+1))
}

fail() {
  echo -e "${RED}✗ FAIL${NC}: $1"
  ASSERT_COUNT=$((ASSERT_COUNT+1))
}

assert_eq() {
  local got="$1"; shift
  local expect="$1"; shift
  local msg="$1"; shift || true
  if [[ "$got" == "$expect" ]]; then
    pass "$msg"
  else
    fail "$msg (expected '$expect', got '$got')"
  fi
}

assert_file_exists() {
  local path="$1"; shift
  local msg="$1"; shift || true
  if [[ -f "$path" ]]; then
    pass "$msg"
  else
    fail "$msg (missing: $path)"
  fi
}

assert_body_contains() {
  local body_file="$1"; shift
  local needle="$1"; shift
  local msg="$1"; shift || true
  if grep -q -- "$needle" "$body_file"; then
    pass "$msg"
  else
    echo -e "${PURPLE}--- response body ---${NC}"
    sed -n '1,200p' "$body_file" || true
    echo -e "${PURPLE}----------------------${NC}"
    fail "$msg (did not find: $needle)"
  fi
}

start_webserver() {
  echo -e "${YELLOW}Building server...${NC}"
  make -j >/dev/null || { echo -e "${RED}Build failed${NC}"; exit 1; }

  echo -e "${YELLOW}Starting webserver with $CONFIG_FILE...${NC}"
  : > "$LOG_FILE"
  $WEBSERV_BIN "$CONFIG_FILE" > "$LOG_FILE" 2>&1 &
  WEBSERV_PID=$!

  # Wait for server
  for i in {1..50}; do
    if curl -s -o /dev/null http://localhost:8080/; then
      echo -e "${GREEN}✓ Webserver started successfully (PID: $WEBSERV_PID)${NC}"
      echo
      return 0
    fi
    sleep 0.1
  done
  echo -e "${RED}✗ Webserver failed to start${NC}"
  cat "$LOG_FILE"
  exit 1
}

cleanup() {
  echo -e "\n${YELLOW}Cleaning up...${NC}"
  [[ -n "${WEBSERV_PID:-}" ]] && kill "$WEBSERV_PID" 2>/dev/null && wait "$WEBSERV_PID" 2>/dev/null || true
  rm -f /tmp/chunked_*.tmp /tmp/response_*.tmp
}

trap cleanup EXIT

# Helper function to send chunked request
send_chunked_request() {
  local chunks="$1"
  local endpoint="$2"
  local output_file="$3"

  # Build the complete chunked request
  local request="POST $endpoint HTTP/1.1\r\nHost: localhost:8080\r\nTransfer-Encoding: chunked\r\nContent-Type: text/plain\r\n\r\n$chunks"

  # Send using printf with proper timeout and error handling
  # Use bash's built-in timing to avoid nc timeout issues
  {
    printf "%b" "$request" | nc localhost 8080
  } > "$output_file" 2>/dev/null

  # Give the response time to complete
  sleep 0.1
}

# Start the webserver
start_webserver

# Test temp directory
TMPDIR="/tmp"

#################################################################
# Test 1: Simple single chunk request
#################################################################
print_test "Simple single chunk POST"

BODY="$TMPDIR/response_1.tmp"
# Single chunk: "5\r\nhello\r\n0\r\n\r\n"
CHUNKS="5\r\nhello\r\n0\r\n\r\n"
send_chunked_request "$CHUNKS" "/test" "$BODY"

# Check if we got a response (server didn't crash)
if [[ -s "$BODY" ]]; then
  pass "Server responded to simple chunked request"
  # Extract status code
  STATUS=$(head -n1 "$BODY" | grep -o '[0-9][0-9][0-9]' | head -n1)
  if [[ "$STATUS" =~ ^[12] ]]; then
    pass "Got successful status code ($STATUS)"
  else
    fail "Got error status code ($STATUS)"
  fi
else
  fail "No response from server for simple chunked request"
fi

#################################################################
# Test 2: Multiple chunks request
#################################################################
print_test "Multiple chunks POST"

BODY="$TMPDIR/response_2.tmp"
# Multiple chunks: "3\r\nfoo\r\n3\r\nbar\r\n4\r\nbaz!\r\n0\r\n\r\n"
CHUNKS="3\r\nfoo\r\n3\r\nbar\r\n4\r\nbaz!\r\n0\r\n\r\n"
send_chunked_request "$CHUNKS" "/multi" "$BODY"

if [[ -s "$BODY" ]]; then
  pass "Server responded to multi-chunk request"
  STATUS=$(head -n1 "$BODY" | grep -o '[0-9][0-9][0-9]' | head -n1)
  if [[ "$STATUS" =~ ^[12] ]]; then
    pass "Got successful status code for multi-chunk ($STATUS)"
  else
    fail "Got error status code for multi-chunk ($STATUS)"
  fi
else
  fail "No response from server for multi-chunk request"
fi

#################################################################
# Test 3: Empty chunked request (just final chunk)
#################################################################
print_test "Empty chunked request"

BODY="$TMPDIR/response_3.tmp"
# Empty body: just "0\r\n\r\n"
CHUNKS="0\r\n\r\n"
send_chunked_request "$CHUNKS" "/empty" "$BODY"

if [[ -s "$BODY" ]]; then
  pass "Server responded to empty chunked request"
  STATUS=$(head -n1 "$BODY" | grep -o '[0-9][0-9][0-9]' | head -n1)
  if [[ "$STATUS" =~ ^[12] ]]; then
    pass "Got successful status code for empty chunk ($STATUS)"
  else
    fail "Got error status code for empty chunk ($STATUS)"
  fi
else
  fail "No response from server for empty chunked request"
fi

#################################################################
# Test 4: Large chunk request
#################################################################
print_test "Large chunk request"

BODY="$TMPDIR/response_4.tmp"
# Large chunk (hex 64 = 100 bytes)
LARGE_DATA=$(printf '%*s' 100 '' | tr ' ' 'A')
CHUNKS="64\r\n$LARGE_DATA\r\n0\r\n\r\n"
send_chunked_request "$CHUNKS" "/large" "$BODY"

if [[ -s "$BODY" ]]; then
  pass "Server responded to large chunk request"
  STATUS=$(head -n1 "$BODY" | grep -o '[0-9][0-9][0-9]' | head -n1)
  if [[ "$STATUS" =~ ^[12] ]]; then
    pass "Got successful status code for large chunk ($STATUS)"
  else
    fail "Got error status code for large chunk ($STATUS)"
  fi
else
  fail "No response from server for large chunk request"
fi

#################################################################
# Test 5: Invalid chunked request (should be rejected)
#################################################################
print_test "Invalid chunked request (missing CRLF)"

BODY="$TMPDIR/response_5.tmp"
# Invalid: missing \r in chunk size line
CHUNKS="5\nhello\r\n0\r\n\r\n"
send_chunked_request "$CHUNKS" "/invalid" "$BODY"

if [[ -s "$BODY" ]]; then
  STATUS=$(head -n1 "$BODY" | grep -o '[0-9][0-9][0-9]' | head -n1)
  if [[ "$STATUS" == "400" ]]; then
    pass "Server correctly rejected invalid chunked request (400)"
  else
    fail "Server should reject invalid chunked with 400, got $STATUS"
  fi
else
  # Server might close connection on invalid request
  pass "Server handled invalid chunked request (connection closed)"
fi

#################################################################
# Test 6: Chunked with hex chunk sizes
#################################################################
print_test "Chunked with various hex sizes"

BODY="$TMPDIR/response_6.tmp"
# Mix of hex sizes: a=10, f=15, 1a=26
CHUNKS="a\r\n0123456789\r\nf\r\nabcdefghijklmno\r\n1a\r\nabcdefghijklmnopqrstuvwxyz\r\n0\r\n\r\n"
send_chunked_request "$CHUNKS" "/hex" "$BODY"

if [[ -s "$BODY" ]]; then
  pass "Server responded to hex chunk sizes"
  STATUS=$(head -n1 "$BODY" | grep -o '[0-9][0-9][0-9]' | head -n1)
  if [[ "$STATUS" =~ ^[12] ]]; then
    pass "Got successful status code for hex chunks ($STATUS)"
  else
    fail "Got error status code for hex chunks ($STATUS)"
  fi
else
  fail "No response from server for hex chunk request"
fi

#################################################################
# Test 7: CGI with chunked encoding (if CGI is available)
#################################################################
if [[ -f "test_files/cgi-bin/echo_post.py" ]]; then
  print_test "Chunked request to CGI script"

  BODY="$TMPDIR/response_7.tmp"
  CHUNKS="c\r\nHello CGI!\r\n0\r\n\r\n"
  send_chunked_request "$CHUNKS" "/cgi-bin/echo_post.py" "$BODY"

  if [[ -s "$BODY" ]]; then
    pass "Server responded to chunked CGI request"
    STATUS=$(head -n1 "$BODY" | grep -o '[0-9][0-9][0-9]' | head -n1)
    if [[ "$STATUS" =~ ^[12] ]]; then
      pass "Got successful status code for chunked CGI ($STATUS)"
    else
      fail "Got error status code for chunked CGI ($STATUS)"
    fi
  else
    fail "No response from server for chunked CGI request"
  fi
else
  echo -e "${YELLOW}Skipping CGI test (echo_post.py not found)${NC}"
fi

#################################################################
# Test 8: Server stability check
#################################################################
print_test "Server stability after chunked requests"

# Test that server is still responsive after all chunked requests
BODY="$TMPDIR/response_stability.tmp"
CODE=$(curl -s -o "$BODY" -w "%{http_code}" http://localhost:8080/ 2>/dev/null)

if [[ "$CODE" =~ ^[12] ]]; then
  pass "Server still responsive after chunked tests ($CODE)"
else
  fail "Server not responsive after chunked tests ($CODE)"
fi

# Check if server process is still running
if kill -0 "$WEBSERV_PID" 2>/dev/null; then
  pass "Server process still running (no crashes)"
else
  fail "Server process died during chunked tests"
fi

# Show some debug output
echo
echo -e "${CYAN}Recent Server Log Output (Chunked handling):${NC}"
echo -e "${YELLOW}=====================================================${NC}"
tail -30 "$LOG_FILE" | tail -30 || true

# Summary
echo
echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}              Test Summary                ${NC}"
echo -e "${CYAN}=========================================${NC}"
echo -e "Test Cases:       ${BLUE}$TEST_COUNT${NC}"
echo -e "Total Assertions: ${BLUE}$ASSERT_COUNT${NC}"
echo -e "Passed:           ${GREEN}$PASS_COUNT${NC}"
echo -e "Failed:           ${RED}$((ASSERT_COUNT - PASS_COUNT))${NC}"

if [[ $PASS_COUNT -eq $ASSERT_COUNT ]]; then
  echo -e "\n${GREEN}🎉 All chunked encoding assertions passed!${NC}"
  echo -e "${YELLOW}Features working:${NC}"
  echo -e "  ✓ Single chunk parsing"
  echo -e "  ✓ Multiple chunk parsing"
  echo -e "  ✓ Empty chunked requests"
  echo -e "  ✓ Large chunk handling"
  echo -e "  ✓ Invalid chunk rejection"
  echo -e "  ✓ Hexadecimal chunk sizes"
  echo -e "  ✓ Server stability"
  [[ -f "test_files/cgi-bin/echo_post.py" ]] && echo -e "  ✓ CGI with chunked encoding"
else
  echo -e "\n${RED}❌ Some tests failed. Check $LOG_FILE and responses above.${NC}"
  exit 1
fi

exit 0
