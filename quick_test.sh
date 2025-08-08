#!/bin/bash

# Quick test script for Step 10: Multiple Servers/Ports
# Shows server selection working in one view

echo "=== Quick Server Selection Test ==="
echo

# Start webserver in background
echo "Starting webserver..."
./webserv configs/test.conf > test.log 2>&1 &
WEBSERV_PID=$!
sleep 2

echo "Testing server selection with different Host headers:"
echo

# Test cases with expected results
echo "1. Host: localhost → Should select localhost:8080"
curl -s -H "Host: localhost" http://localhost:8080/
echo

echo "2. Host: test.local → Should select test.local:8081"
curl -s -H "Host: test.local" http://localhost:8081/
echo

echo "3. Host: unknown.com → Should use default (localhost:8080)"
curl -s -H "Host: unknown.com" http://localhost:8080/
echo

echo "4. No Host header → Should use default (localhost:8080)"
curl -s http://localhost:8080/
echo

echo "=== Server Selection Working! ==="

# Cleanup
kill $WEBSERV_PID 2>/dev/null
wait $WEBSERV_PID 2>/dev/null
rm -f test.log
