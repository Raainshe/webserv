#!/bin/bash

# Setup script for system directories (requires sudo)
# Run this if you want to use default.conf which references /var/www/

echo "Setting up system directories for default.conf..."

# Create directories
sudo mkdir -p /var/www/html /var/www/uploads

# Copy test files to system locations
sudo cp test_files/www/index.html /var/www/html/
sudo cp test_files/www/404.html /var/www/html/

# Set permissions
sudo chown -R www-data:www-data /var/www/ 2>/dev/null || sudo chown -R $USER:$USER /var/www/
sudo chmod -R 755 /var/www/

echo "System directories created successfully!"
echo "You can now use configs/default.conf"
