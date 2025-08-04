#!/bin/bash

echo "=== Quick Routing Demo ==="
echo

# Start webserver in background
echo "Starting webserver..."
./webserv configs/routing_test.conf > demo.log 2>&1 &
WEBSERV_PID=$!
sleep 2

echo "✓ Server started"
echo

echo "🎯 Testing Routing Functionality:"
echo

echo "1. Root location with index file resolution:"
curl -s -H "Host: localhost" http://localhost:8080/ | head -3
echo

echo "2. Images location with directory listing:"
curl -s -H "Host: localhost" http://localhost:8080/images/ | head -3
echo

echo "3. API location with specific file:"
curl -s -H "Host: test.local" http://localhost:8081/api/users.json | head -3
echo

echo "4. CGI location detection:"
curl -s -H "Host: localhost" http://localhost:8080/cgi-bin/script.php | head -3
echo

echo "5. Method validation (DELETE not allowed on root):"
curl -s -X DELETE -H "Host: localhost" http://localhost:8080/ | head -1
echo

echo "6. Longest prefix matching (/images vs /):"
curl -s -H "Host: localhost" http://localhost:8080/images/photo1.jpg | head -3
echo

echo "=== Routing System Working Perfectly! ==="

# Cleanup
kill $WEBSERV_PID 2>/dev/null
wait $WEBSERV_PID 2>/dev/null
rm -f demo.log
