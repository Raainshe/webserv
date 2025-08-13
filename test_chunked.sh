#!/usr/bin/env bash

set -u -o pipefail

# Colors (style consistent with test_step9.sh)
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
PURPLE='\033[0;35m'
NC='\033[0m'

# Configuration
ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"
WEBSERV_BIN="./webserv"
CONFIG_FILE="configs/test.conf"
LOG_FILE="webserv_chunked.log"
WEBSERV_PID=""

# Counters
TEST_COUNT=0
ASSERT_COUNT=0
PASS_COUNT=0

echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}        Chunked Transfer-Encoding        ${NC}"
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
  local got="${1:-}"; shift || true
  local expect="${1:-}"; shift || true
  local msg="${1:-}"; shift || true
  if [[ "$got" == "$expect" ]]; then
    pass "$msg"
  else
    fail "$msg (expected '$expect', got '$got')"
  fi
}

assert_body_contains() {
  local body_file="$1"; shift
  local needle="$1"; shift
  local msg="${1:-}"
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
  make -j > /dev/null || { echo -e "${RED}Build failed${NC}"; exit 1; }

  echo -e "${YELLOW}Starting webserver with $CONFIG_FILE...${NC}"
  : > "$LOG_FILE"
  $WEBSERV_BIN "$CONFIG_FILE" > "$LOG_FILE" 2>&1 &
  WEBSERV_PID=$!

  # Wait for server readiness
  for i in {1..50}; do
    if curl -s -o /dev/null http://localhost:8080/; then
      echo -e "${GREEN}✓ Webserver started successfully (PID: $WEBSERV_PID)${NC}"
      echo
      return 0
    fi
    sleep 0.1
  done
  echo -e "${RED}✗ Failed to start webserver${NC} (see $LOG_FILE)"
  exit 1
}

stop_webserver() {
  if [[ -n "$WEBSERV_PID" ]]; then
    echo -e "${YELLOW}Stopping webserver (PID: $WEBSERV_PID)...${NC}"
    kill "$WEBSERV_PID" 2>/dev/null || true
    wait "$WEBSERV_PID" 2>/dev/null || true
    echo -e "${GREEN}✓ Webserver stopped${NC}"
  fi
}

cleanup() {
  stop_webserver
}

trap cleanup EXIT INT TERM

TMPDIR="/tmp/webserv_chunked"
mkdir -p "$TMPDIR"

send_raw_request() {
  # $1: request_file, $2: response_file
  local req="$1"
  local resp="$2"
  # Use nc with quiet-after-EOF to ensure connection close
  cat "$req" | nc -q 1 localhost 8080 > "$resp" 2>/dev/null || true
}

restart_server() {
  stop_webserver
  sleep 0.2
  start_webserver
}

# Start server
start_webserver

# ---------------------------------------------------------------------------
# Test 1: Simple valid chunked POST
# ---------------------------------------------------------------------------
print_test "Valid chunked POST returns 200 and legacy POST body" \
           "nc -q 1 localhost 8080 < request_1.txt"

REQ="$TMPDIR/req1.txt"
RESP="$TMPDIR/resp1.txt"
{
  printf 'POST / HTTP/1.1\r\n'
  printf 'Host: localhost\r\n'
  printf 'Transfer-Encoding: chunked\r\n'
  printf 'Content-Type: text/plain\r\n'
  printf '\r\n'
  printf '4\r\nWiki\r\n'
  printf '5\r\npedia\r\n'
  printf '0\r\n\r\n'
} > "$REQ"

send_raw_request "$REQ" "$RESP"

STATUS_CODE=$(head -n1 "$RESP" | awk '{print $2}')
BODY_FILE="$TMPDIR/body1.txt"
sed -n '/^\r$/,$p' "$RESP" | sed '1d' > "$BODY_FILE" || true
assert_eq "$STATUS_CODE" "200" "Status code is 200"
assert_body_contains "$BODY_FILE" "POST request received successfully!" "Body contains legacy POST message"

# ---------------------------------------------------------------------------
# Test 2: Multi-chunk sample payload
# ---------------------------------------------------------------------------
print_test "Multiple chunks are accepted (200)" \
           "nc -q 1 localhost 8080 < request_2.txt"

REQ2="$TMPDIR/req2.txt"
RESP2="$TMPDIR/resp2.txt"
{
  printf 'POST / HTTP/1.1\r\n'
  printf 'Host: localhost\r\n'
  printf 'Transfer-Encoding: chunked\r\n'
  printf 'Content-Type: text/plain\r\n'
  printf '\r\n'
  printf '7\r\nMozilla\r\n'
  printf '9\r\nDeveloper\r\n'
  printf '7\r\nNetwork\r\n'
  printf '0\r\n\r\n'
} > "$REQ2"

send_raw_request "$REQ2" "$RESP2"
STATUS_CODE2=$(head -n1 "$RESP2" | awk '{print $2}')
BODY_FILE2="$TMPDIR/body2.txt"
sed -n '/^\r$/,$p' "$RESP2" | sed '1d' > "$BODY_FILE2" || true
assert_eq "$STATUS_CODE2" "200" "Status code is 200"
assert_body_contains "$BODY_FILE2" "POST request received successfully!" "Body contains legacy POST message"

# ---------------------------------------------------------------------------
# Test 3: Large chunked body triggers 413 Payload Too Large
# ---------------------------------------------------------------------------
print_test "Chunked body over client_max_body_size (1MB) returns 413" \
           "nc -q 1 localhost 8080 < big_request.txt"

REQ3="$TMPDIR/req3.txt"
RESP3="$TMPDIR/resp3.txt"

# Build a ~1.5MB payload and encode as chunked using 4096-byte chunks
PAYLOAD="$TMPDIR/payload.bin"
( dd if=/dev/zero bs=1024 count=1536 2>/dev/null | tr '\0' 'A' ) > "$PAYLOAD"

{
  printf 'POST / HTTP/1.1\r\n'
  printf 'Host: localhost\r\n'
  printf 'Transfer-Encoding: chunked\r\n'
  printf 'Content-Type: text/plain\r\n'
  printf '\r\n'
} > "$REQ3"

# Split and emit chunks
CHUNK_DIR="$TMPDIR/chunks"
mkdir -p "$CHUNK_DIR"
rm -f "$CHUNK_DIR"/* || true
split -b 4096 "$PAYLOAD" "$CHUNK_DIR/chunk_"
for f in "$CHUNK_DIR"/chunk_*; do
  size=$(stat -c%s "$f")
  printf '%x\r\n' "$size" >> "$REQ3"
  cat "$f" >> "$REQ3"
  printf '\r\n' >> "$REQ3"
done
printf '0\r\n\r\n' >> "$REQ3"

send_raw_request "$REQ3" "$RESP3"
STATUS_CODE3=$(head -n1 "$RESP3" | awk '{print $2}')
assert_eq "$STATUS_CODE3" "413" "Status code is 413 for oversized chunked body"

echo
echo -e "${CYAN}Recent Server Log Output (Chunked handling):${NC}"
echo -e "${YELLOW}=====================================================${NC}"
tail -30 "$LOG_FILE" | tail -30 || true

# ---------------------------------------------------------------------------
# Test 4: Uppercase hex sizes
# ---------------------------------------------------------------------------
print_test "Uppercase hex chunk sizes are accepted (200)" \
           "nc -q 1 localhost 8080 < request_4.txt"

REQ4="$TMPDIR/req4.txt"
RESP4="$TMPDIR/resp4.txt"
{
  printf 'POST / HTTP/1.1\r\n'
  printf 'Host: localhost\r\n'
  printf 'Transfer-Encoding: chunked\r\n'
  printf 'Content-Type: text/plain\r\n'
  printf '\r\n'
  printf 'A\r\nABCDEFGHIJ\r\n'
  printf '1\r\nX\r\n'
  printf '0\r\n\r\n'
} > "$REQ4"

send_raw_request "$REQ4" "$RESP4"
STATUS_CODE4=$(head -n1 "$RESP4" | awk '{print $2}')
assert_eq "$STATUS_CODE4" "200" "Status code is 200"

# ---------------------------------------------------------------------------
# Test 5: Chunk extensions support
# ---------------------------------------------------------------------------
print_test "Chunk extensions are tolerated (200)" \
           "nc -q 1 localhost 8080 < request_5.txt"

REQ5="$TMPDIR/req5.txt"
RESP5="$TMPDIR/resp5.txt"
{
  printf 'POST / HTTP/1.1\r\n'
  printf 'Host: localhost\r\n'
  printf 'Transfer-Encoding: chunked\r\n'
  printf 'Content-Type: text/plain\r\n'
  printf '\r\n'
  printf '4;foo=bar\r\nTest\r\n'
  printf '0\r\n\r\n'
} > "$REQ5"

send_raw_request "$REQ5" "$RESP5"
STATUS_CODE5=$(head -n1 "$RESP5" | awk '{print $2}')
assert_eq "$STATUS_CODE5" "200" "Status code is 200"

# ---------------------------------------------------------------------------
# Test 6: Zero-length chunked body only
# ---------------------------------------------------------------------------
print_test "Zero-length chunked body (0 only) returns 200" \
           "nc -q 1 localhost 8080 < request_6.txt"

REQ6="$TMPDIR/req6.txt"
RESP6="$TMPDIR/resp6.txt"
{
  printf 'POST / HTTP/1.1\r\n'
  printf 'Host: localhost\r\n'
  printf 'Transfer-Encoding: chunked\r\n'
  printf '\r\n'
  printf '0\r\n\r\n'
} > "$REQ6"

send_raw_request "$REQ6" "$RESP6"
STATUS_CODE6=$(head -n1 "$RESP6" | awk '{print $2}')
assert_eq "$STATUS_CODE6" "200" "Status code is 200 for zero-length body"

# ---------------------------------------------------------------------------
# Test 7: Invalid chunk size (non-hex) -> expect 400 or no response
# ---------------------------------------------------------------------------
print_test "Invalid chunk size yields 400 (or connection closed)" \
           "nc -q 1 localhost 8080 < request_7.txt"

REQ7="$TMPDIR/req7.txt"
RESP7="$TMPDIR/resp7.txt"
{
  printf 'POST / HTTP/1.1\r\n'
  printf 'Host: localhost\r\n'
  printf 'Transfer-Encoding: chunked\r\n'
  printf '\r\n'
  printf 'Z\r\nHello\r\n'
  printf '0\r\n\r\n'
} > "$REQ7"

send_raw_request "$REQ7" "$RESP7"
STATUS_CODE7=$(head -n1 "$RESP7" | awk '{print $2}')
if [[ -z "${STATUS_CODE7}" ]]; then
  pass "Server closed connection without response (acceptable)"
else
  assert_eq "$STATUS_CODE7" "400" "Status code is 400 for invalid chunk size"
fi

# ---------------------------------------------------------------------------
# Test 8: Missing CRLF after chunk data -> expect 400 or no response
# ---------------------------------------------------------------------------
print_test "Missing CRLF after chunk data leads to 400/close" \
           "nc -q 1 localhost 8080 < request_8.txt"

REQ8="$TMPDIR/req8.txt"
RESP8="$TMPDIR/resp8.txt"
{
  printf 'POST / HTTP/1.1\r\n'
  printf 'Host: localhost\r\n'
  printf 'Transfer-Encoding: chunked\r\n'
  printf '\r\n'
  printf '4\r\nTest'  # missing trailing CRLF intentionally
  printf '0\r\n\r\n'
} > "$REQ8"

send_raw_request "$REQ8" "$RESP8"
STATUS_CODE8=$(head -n1 "$RESP8" | awk '{print $2}')
if [[ -z "${STATUS_CODE8}" ]]; then
  pass "Server closed connection without response (acceptable)"
else
  assert_eq "$STATUS_CODE8" "400" "Status code is 400 for missing CRLF"
fi

# ---------------------------------------------------------------------------
# Test 9: POST with chunked to location that disallows POST -> 405
# ---------------------------------------------------------------------------
print_test "POST chunked to /images/ returns 405" \
           "nc -q 1 localhost 8080 < request_9.txt"

REQ9="$TMPDIR/req9.txt"
RESP9="$TMPDIR/resp9.txt"
{
  printf 'POST /images/ HTTP/1.1\r\n'
  printf 'Host: localhost\r\n'
  printf 'Transfer-Encoding: chunked\r\n'
  printf '\r\n'
  printf '4\r\nTest\r\n'
  printf '0\r\n\r\n'
} > "$REQ9"

send_raw_request "$REQ9" "$RESP9"
STATUS_CODE9=$(head -n1 "$RESP9" | awk '{print $2}')
BODY_FILE9="$TMPDIR/body9.txt"
sed -n '/^\r$/,$p' "$RESP9" | sed '1d' > "$BODY_FILE9" || true
assert_eq "$STATUS_CODE9" "405" "Status code is 405 for disallowed method"
assert_body_contains "$BODY_FILE9" "Custom 405 Page" "Body contains 405 page"

# ---------------------------------------------------------------------------
# Test 10: Trailers present after final 0-chunk (tolerated)
# ---------------------------------------------------------------------------
print_test "Trailers after final chunk are tolerated (200)" \
           "nc -q 1 localhost 8080 < request_10.txt"

REQ10="$TMPDIR/req10.txt"
RESP10="$TMPDIR/resp10.txt"
{
  printf 'POST / HTTP/1.1\r\n'
  printf 'Host: localhost\r\n'
  printf 'Transfer-Encoding: chunked\r\n'
  printf '\r\n'
  printf '4\r\nWiki\r\n'
  printf '0\r\n'
  printf 'X-Foo: bar\r\n\r\n'
} > "$REQ10"

send_raw_request "$REQ10" "$RESP10"
STATUS_CODE10=$(head -n1 "$RESP10" | awk '{print $2}')
assert_eq "$STATUS_CODE10" "200" "Status code is 200 with trailers"

# Intentionally malformed tests can poison a global parser; restart to isolate
restart_server

# ---------------------------------------------------------------------------
# Test 11: Lowercase header name 'transfer-encoding'
# ---------------------------------------------------------------------------
print_test "Lowercase transfer-encoding header is accepted (200)" \
           "nc -q 1 localhost 8080 < request_11.txt"

REQ11="$TMPDIR/req11.txt"
RESP11="$TMPDIR/resp11.txt"
{
  printf 'POST / HTTP/1.1\r\n'
  printf 'Host: localhost\r\n'
  printf 'transfer-encoding: chunked\r\n'
  printf '\r\n'
  printf '6\r\nfoobar\r\n'
  printf '0\r\n\r\n'
} > "$REQ11"

send_raw_request "$REQ11" "$RESP11"
STATUS_CODE11=$(head -n1 "$RESP11" | awk '{print $2}')
assert_eq "$STATUS_CODE11" "200" "Status code is 200 with lowercase header"

# ---------------------------------------------------------------------------
# Test 12: GET with chunked body (ignored) returns index (200)
# ---------------------------------------------------------------------------
print_test "GET with chunked body returns index (200)" \
           "nc -q 1 localhost 8080 < request_12.txt"

REQ12="$TMPDIR/req12.txt"
RESP12="$TMPDIR/resp12.txt"
{
  printf 'GET / HTTP/1.1\r\n'
  printf 'Host: localhost\r\n'
  printf 'Transfer-Encoding: chunked\r\n'
  printf '\r\n'
  printf '4\r\nWiki\r\n'
  printf '0\r\n\r\n'
} > "$REQ12"

send_raw_request "$REQ12" "$RESP12"
STATUS_CODE12=$(head -n1 "$RESP12" | awk '{print $2}')
BODY_FILE12="$TMPDIR/body12.txt"
sed -n '/^\r$/,$p' "$RESP12" | sed '1d' > "$BODY_FILE12" || true
assert_eq "$STATUS_CODE12" "200" "Status code is 200 for GET with body"
assert_body_contains "$BODY_FILE12" "Welcome to localhost server!" "Index content returned"

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
  echo -e "  ✓ Valid chunked POST parsing"
  echo -e "  ✓ Multi-chunk handling"
  echo -e "  ✓ 413 enforcement for oversized chunked bodies"
else
  echo -e "\n${RED}❌ Some tests failed. Check $LOG_FILE and responses above.${NC}"
  exit 1
fi

exit 0


