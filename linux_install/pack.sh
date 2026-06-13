#!/bin/bash

# Pratibimb Linux Packager
# Run this from the linux_install folder to generate the release tar.gz

set -e

echo "Packaging Pratibimb for Linux..."

# Navigate to project root
cd ..

# Define staging directory name
STAGING_DIR="Pratibimb-Linux"

# Remove any old staging dir or archive
rm -rf "$STAGING_DIR"
rm -f "$STAGING_DIR.tar.gz"

# Create fresh staging directory
mkdir "$STAGING_DIR"

# Copy necessary directories and files
echo "Copying files..."
cp -r host "$STAGING_DIR/"
cp -r linux_install "$STAGING_DIR/"
cp -r extension "$STAGING_DIR/"
cp extension.zip "$STAGING_DIR/" 2>/dev/null || true
cp README.md "$STAGING_DIR/" 2>/dev/null || true

# Clean up development/build artifacts from the staging directory
echo "Cleaning up artifacts..."
rm -f "$STAGING_DIR/host/pratibimb_host"
rm -f "$STAGING_DIR/host/com.suntzv.pratibimb.json"
rm -f "$STAGING_DIR/linux_install/pack.sh" # No need to ship the packer script to users

# Compress into a tarball
echo "Creating archive..."
tar -czvf "$STAGING_DIR.tar.gz" "$STAGING_DIR" > /dev/null

# Remove staging directory
rm -rf "$STAGING_DIR"

echo ""
echo "========================================="
echo "   Successfully created $STAGING_DIR.tar.gz 🎉"
echo "========================================="
echo "This file is located in the root of your project directory."
