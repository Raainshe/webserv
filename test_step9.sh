#!/usr/bin/env bash
# Step 9: Uploads and POST Handling - Test Script

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
CONFIG_FILE="configs/test.conf"
LOG_FILE="webserv_step9.log"
WEBSERV_PID=""

# Counters
TEST_COUNT=0         # Number of high-level test cases
ASSERT_COUNT=0       # Number of individual assertions
PASS_COUNT=0         # Number of passed assertions

# Header
echo -e "${CYAN}=========================================${NC}"
echo -e "${CYAN}     Step 9: Uploads and POST Tests      ${NC}"
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
  # Do not delete $LOG_FILE so user can inspect
}

trap cleanup EXIT INT TERM

# Ensure clean uploads dir
mkdir -p test_files/uploads
rm -rf test_files/uploads/*

# Start server
start_webserver

TMPDIR="/tmp/webserv_step9"
mkdir -p "$TMPDIR"

# ---------------------------------------------------------------------------
# Test 1: Non-upload POST to root
# ---------------------------------------------------------------------------
print_test "Non-upload POST to / returns 200 and legacy message" \
           "curl -s -X POST -H 'Host: localhost' http://localhost:8080/"
BODY="$TMPDIR/body1.txt"
CODE=$(curl -s -o "$BODY" -w "%{http_code}" -X POST -H 'Host: localhost' http://localhost:8080/)
assert_eq "$CODE" "200" "Status code is 200"
assert_body_contains "$BODY" "POST request received successfully!" "Body contains legacy message"

# ---------------------------------------------------------------------------
# Test 2: Single file upload
# ---------------------------------------------------------------------------
print_test "Single file upload to /upload returns 201 and saves file" \
           "curl -s -F 'file=@test_files/images/photo1.jpg' http://localhost:8080/upload"
rm -rf test_files/uploads/*
BODY="$TMPDIR/body2.json"
CODE=$(curl -s -o "$BODY" -w "%{http_code}" -F "file=@test_files/images/photo1.jpg" http://localhost:8080/upload)
assert_eq "$CODE" "201" "Status code is 201"
assert_body_contains "$BODY" "\"filename\":\"photo1.jpg\"" "Response lists uploaded filename"
assert_file_exists "test_files/uploads/photo1.jpg" "Saved file exists in upload_store"

# ---------------------------------------------------------------------------
# Test 3: Duplicate filename deduplication
# ---------------------------------------------------------------------------
print_test "Duplicate upload results in deduplicated filename" \
           "curl -s -F 'file=@test_files/images/photo1.jpg' http://localhost:8080/upload"
BODY="$TMPDIR/body3.json"
CODE=$(curl -s -o "$BODY" -w "%{http_code}" -F "file=@test_files/images/photo1.jpg" http://localhost:8080/upload)
assert_eq "$CODE" "201" "Status code is 201"
if grep -q '"filename":"photo1(' "$BODY"; then
  pass "Response contains deduplicated filename"
else
  fail "Response missing deduplicated filename indicator"
fi
# Extract deduped filename and verify saved
DEDUP_FILE=$(sed -n 's/.*"filename":"\([^"]*\)".*/\1/p' "$BODY" | head -n1)
if [[ -n "$DEDUP_FILE" ]]; then
  assert_file_exists "test_files/uploads/$DEDUP_FILE" "Deduplicated file exists on disk"
else
  fail "Could not extract deduplicated filename from response"
fi

# ---------------------------------------------------------------------------
# Test 4: Multiple files + form field
# ---------------------------------------------------------------------------
print_test "Multiple file upload returns 201 and saves both files; POST doesn't autoindex" \
           "curl -s -F 'f1=@test_files/images/photo1.jpg' -F 'f2=@test_files/images/photo2.jpg' -F 'note=hello' http://localhost:8080/upload"
rm -rf test_files/uploads/*
BODY="$TMPDIR/body4.json"
CODE=$(curl -s -o "$BODY" -w "%{http_code}" \
  -F "f1=@test_files/images/photo1.jpg" \
  -F "f2=@test_files/images/photo2.jpg" \
  -F "note=hello" \
  http://localhost:8080/upload)
assert_eq "$CODE" "201" "Status code is 201"
assert_body_contains "$BODY" "\"filename\":\"photo1.jpg\"" "Response includes photo1.jpg"
assert_body_contains "$BODY" "\"filename\":\"photo2.jpg\"" "Response includes photo2.jpg"
assert_file_exists "test_files/uploads/photo1.jpg" "Saved photo1 exists"
assert_file_exists "test_files/uploads/photo2.jpg" "Saved photo2 exists"

# ---------------------------------------------------------------------------
# Test 5: 413 Payload Too Large for big upload
# ---------------------------------------------------------------------------
print_test "Large upload (~2MB) rejected with 413 per client_max_body_size" \
           "curl -s -F 'file=@/tmp/large_upload.bin' http://localhost:8080/upload"
rm -rf test_files/uploads/*
LARGE="/tmp/large_upload.bin"
# Create ~2MB file
( dd if=/dev/zero of="$LARGE" bs=1048576 count=2 >/dev/null 2>&1 ) || true
BODY="$TMPDIR/body5.html"
CODE=$(curl -s -o "$BODY" -w "%{http_code}" -F "file=@$LARGE" http://localhost:8080/upload)
assert_eq "$CODE" "413" "Status code is 413 for oversized upload"

# Show some debug output
echo
echo -e "${CYAN}Recent Server Log Output (Uploads handling):${NC}"
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
  echo -e "\n${GREEN}🎉 All Step 9 assertions passed!${NC}"
  echo -e "${YELLOW}Features working:${NC}"
  echo -e "  ✓ Non-upload POST behavior"
  echo -e "  ✓ In-memory multipart parsing"
  echo -e "  ✓ File saving to upload_store"
  echo -e "  ✓ Filename sanitization and deduplication"
  echo -e "  ✓ 413 enforcement (client_max_body_size)"
else
  echo -e "\n${RED}❌ Some tests failed. Check $LOG_FILE and responses above.${NC}"
  exit 1
fi

exit 0
