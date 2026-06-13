#!/bin/bash

# Pratibimb Linux Uninstaller
set -e

# Ensure we are in the script's directory so relative paths work
cd "$(dirname "$0")"

echo "========================================="
echo "       Pratibimb Linux Uninstaller"
echo "========================================="
echo ""

echo "Removing Native Messaging Manifests..."
rm -f "$HOME/.config/google-chrome/NativeMessagingHosts/com.suntzv.pratibimb.json"
rm -f "$HOME/.config/chromium/NativeMessagingHosts/com.suntzv.pratibimb.json"
rm -f "$HOME/.config/BraveSoftware/Brave-Browser/NativeMessagingHosts/com.suntzv.pratibimb.json"
rm -f "$HOME/.config/microsoft-edge/NativeMessagingHosts/com.suntzv.pratibimb.json"

echo "Removing host executable..."
INSTALL_DIR="$HOME/.local/share/Pratibimb"
rm -rf "$INSTALL_DIR"

if [ -d "../host" ]; then
    echo "Cleaning up local build files..."
    cd ../host
    make clean > /dev/null 2>&1 || true
fi

echo ""
echo "========================================="
echo "       Uninstallation Complete!"
echo "========================================="
echo "You can now safely remove the extension from your browser."
