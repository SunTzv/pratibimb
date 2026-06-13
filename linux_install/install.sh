#!/bin/bash

# Pratibimb Linux Installer
set -e

echo "========================================="
echo "        Pratibimb Linux Installer"
echo "========================================="
echo ""

# 1. Dependency Check
echo "[1/4] Checking dependencies..."
if ! command -v g++ &> /dev/null; then
    echo "Error: g++ is not installed. Please install it (e.g. sudo apt install g++) and try again."
    exit 1
fi
if ! command -v make &> /dev/null; then
    echo "Error: make is not installed. Please install it (e.g. sudo apt install make) and try again."
    exit 1
fi
echo "Dependencies OK."
echo ""

# 2. Build the host
echo "[2/4] Compiling native host..."
cd ../host
make clean > /dev/null 2>&1 || true
make > /dev/null
if [ ! -f "pratibimb_host" ]; then
    echo "Error: Compilation failed."
    exit 1
fi
echo "Compilation successful."
echo ""

# 3. Extension Configuration
echo "[3/4] Configuring Extension ID..."
EXT_ID="cbcdepgnlldcpbigcgjdkmnjcoekggji"
echo "Extension ID is permanently set to: $EXT_ID"
echo ""

# 4. Installation
echo "[4/4] Installing Native Messaging Manifest..."

INSTALL_DIR="$HOME/.local/share/Pratibimb"
mkdir -p "$INSTALL_DIR"
cp pratibimb_host "$INSTALL_DIR/pratibimb_host"
chmod +x "$INSTALL_DIR/pratibimb_host"

MANIFEST_CONTENT='{
  "name": "com.suntzv.pratibimb",
  "description": "Pratibimb Native Host",
  "path": "'$INSTALL_DIR'/pratibimb_host",
  "type": "stdio",
  "allowed_origins": [
    "chrome-extension://'$EXT_ID'/"
  ]
}'

install_manifest() {
    local browser_dir="$1"
    mkdir -p "$browser_dir"
    echo "$MANIFEST_CONTENT" > "$browser_dir/com.suntzv.pratibimb.json"
    echo "  -> Installed to $browser_dir"
}

echo ""
install_manifest "$HOME/.config/google-chrome/NativeMessagingHosts"
install_manifest "$HOME/.config/chromium/NativeMessagingHosts"
install_manifest "$HOME/.config/BraveSoftware/Brave-Browser/NativeMessagingHosts"
install_manifest "$HOME/.config/microsoft-edge/NativeMessagingHosts"

echo ""
echo "========================================="
echo "          Installation Complete"
echo "========================================="
echo "You can now open a new tab in your browser."
echo "If it doesn't work immediately, try reloading the extension in your browser."
echo "If it still doesnt work then fix it youself bro ur using lilnux"