#!/bin/bash

echo "Testing HTTP Response Handler"
echo "============================="

# Start server
./webserv configs/default.conf &
SERVER_PID=$!
sleep 2

echo "1. GET index.html:"
curl -i http://localhost:8080/index.html
echo -e "\n\n2. GET text file:"
curl -i http://localhost:8080/test.txt
echo -e "\n\n3. GET 404:"
curl -i http://localhost:8080/notfound.html
echo -e "\n\n4. POST request:"
curl -i -X POST http://localhost:8080/
echo -e "\n\n5. DELETE request:"
curl -i -X DELETE http://localhost:8080/test.txt

kill $SERVER_PID
