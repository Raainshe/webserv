# Test Files Structure

This directory contains all the test files and content needed for the webserv configuration files.

## Directory Structure

```
test_files/
├── www/                 # Main web directory (localhost server)
│   ├── index.html       # Welcome page
│   ├── 404.html         # Custom 404 error page
│   ├── 405.html         # Custom 405 error page
│   ├── 413.html         # Custom 413 error page
│   └── 500.html         # Custom 500 error page
├── www2/                # Secondary web directory (test.local server)
│   ├── index.html       # Test.local welcome page
│   └── 403.html         # Custom 403 error page
├── images/              # Images directory with autoindex
│   ├── photo1.jpg       # Mock image file 1
│   ├── photo2.jpg       # Mock image file 2
│   └── readme.txt       # Directory documentation
├── uploads/             # Upload directory (initially empty)
├── api/                 # API test files
│   └── users.json       # Sample API endpoint
└── cgi-bin/             # CGI scripts directory
    ├── hello.py         # Simple CGI hello world script
    └── echo_post.py     # POST data echo script
```

## Configuration Files Mapping

### test.conf / routing_test.conf
- Uses `test_files/` directories
- Multiple servers (localhost:8080, test.local:8081)
- Complete feature testing

### test_same_port.conf
- Uses `test_files/` directories
- Same port, different hostnames
- Server selection testing

### cgi_test.conf
- Uses `test_files/` directories
- CGI functionality testing
- Port 8082

### default.conf
- Uses `/var/www/` system directories
- Requires sudo to setup (run `./setup_system_dirs.sh`)
- Production-like configuration

## Testing

1. **Basic functionality**: Use `test.conf`
2. **CGI testing**: Use `cgi_test.conf`
3. **Server selection**: Use `test_same_port.conf`
4. **Routing tests**: Use `routing_test.conf`

## URLs to Test

### localhost:8080 (test.conf)
- `http://localhost:8080/` - Main page
- `http://localhost:8080/images/` - Directory listing
- `http://localhost:8080/upload` - Upload endpoint (POST)

### test.local:8081 (test.conf)
- `http://localhost:8081/` (Host: test.local) - Secondary server
- `http://localhost:8081/api/users.json` - API endpoint

### CGI (cgi_test.conf on port 8082)
- `http://localhost:8082/cgi-bin/hello.py` - Simple CGI
- `http://localhost:8082/cgi-bin/echo_post.py` - POST echo

All files are properly set up for comprehensive webserv testing!
