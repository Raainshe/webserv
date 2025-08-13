#!/usr/bin/python3
import os, sys

length = int(os.environ.get('CONTENT_LENGTH', '0') or '0')
data = sys.stdin.read(length) if length > 0 else ''

print("Status: 200 OK")
print("Content-Type: text/plain")
print("")
print(data)

