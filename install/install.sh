#!/bin/bash
set -e

RED='\033[38;2;255;100;100m'
GREEN='\033[38;2;100;255;100m'
BLUE='\033[38;2;100;150;255m'
GRAY='\033[38;2;150;150;150m'
YELLOW='\033[38;2;255;200;100m'
NC='\033[0m'

clear
echo -e ""
echo -e "    ${RED}██████╗ ██████╗  █████╗ ████████╗██╗██████╗ ██╗███╗   ███╗██████╗ ${NC}"
echo -e "    ${RED}██╔══██╗██╔══██╗██╔══██╗╚══██╔══╝██║██╔══██╗██║████╗ ████║██╔══██╗${NC}"
echo -e "    ${RED}██████╔╝██████╔╝███████║   ██║   ██║██████╔╝██║██╔████╔██║██████╔╝${NC}"
echo -e "    ${RED}██╔═══╝ ██╔══██╗██╔══██║   ██║   ██║██╔══██╗██║██║╚██╔╝██║██╔══██╗${NC}"
echo -e "    ${RED}██║     ██║  ██║██║  ██║   ██║   ██║██████╔╝██║██║ ╚═╝ ██║██████╔╝${NC}"
echo -e "    ${RED}╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝╚═════╝ ╚═╝╚═╝     ╚═╝╚═════╝ ${NC}"
echo -e ""
echo -e "    ${GRAY}Sync your desktop wallpaper to your New Tab page.${NC}"
echo -e ""

echo -e "  ${BLUE}[1/4]${NC} Checking dependencies"
for cmd in g++ unzip curl; do
    if ! command -v $cmd &> /dev/null; then
        echo -e "    ${RED}✗${NC} Error: $cmd is not installed."
        echo "    Please install it (e.g. sudo apt install $cmd) and run this script again."
        exit 1
    fi
done
echo -e "    ${GREEN}✓${NC} Dependencies OK"

INSTALL_DIR="$HOME/.local/share/Pratibimb"
HOST_DIR="$INSTALL_DIR/host"
EXT_DIR="$INSTALL_DIR/extension"

mkdir -p "$HOST_DIR"
mkdir -p "$EXT_DIR"

echo -e "  ${BLUE}[2/4]${NC} Downloading latest stable release"
TMP_DIR=$(mktemp -d)
LATEST_URL=$(curl -s https://api.github.com/repos/SunTzv/Pratibimb/releases/latest | grep "zipball_url" | cut -d '"' -f 4)
curl -s -L -o "$TMP_DIR/release.zip" "$LATEST_URL"
unzip -q -o "$TMP_DIR/release.zip" -d "$TMP_DIR"
EXTRACTED_DIR=$(find "$TMP_DIR" -mindepth 1 -maxdepth 1 -type d | head -n 1)

cp -r "$EXTRACTED_DIR/extension/"* "$EXT_DIR/"
echo -e "    ${GREEN}✓${NC} Files downloaded and extracted"

echo -e "  ${BLUE}[3/4]${NC} Building host executable"
cd "$EXTRACTED_DIR/host"
g++ -O3 -Wall -Wno-ignored-attributes -std=c++14 main.cpp -o pratibimb_host
mv pratibimb_host "$HOST_DIR/"
cd ~
rm -rf "$TMP_DIR"
echo -e "    ${GREEN}✓${NC} Built and installed host"

echo -e "  ${BLUE}[4/4]${NC} Registering browsers"
EXT_ID="cbcdepgnlldcpbigcgjdkmnjcoekggji"
MANIFEST_PATH="$HOST_DIR/com.suntzv.pratibimb.json"

cat <<EOF > "$MANIFEST_PATH"
{
  "name": "com.suntzv.pratibimb",
  "description": "Pratibimb Native Host",
  "path": "$HOST_DIR/pratibimb_host",
  "type": "stdio",
  "allowed_origins": [
    "chrome-extension://$EXT_ID/"
  ]
}
EOF

install_manifest() {
    local browser_dir="$1"
    mkdir -p "$browser_dir"
    cp "$MANIFEST_PATH" "$browser_dir/com.suntzv.pratibimb.json"
}

install_manifest "$HOME/.config/google-chrome/NativeMessagingHosts"
install_manifest "$HOME/.config/chromium/NativeMessagingHosts"
install_manifest "$HOME/.config/BraveSoftware/Brave-Browser/NativeMessagingHosts"
install_manifest "$HOME/.config/microsoft-edge/NativeMessagingHosts"

echo -e "    ${GREEN}✓${NC} Registry updated"
echo -e ""
echo -e "  ✨ Installation Complete! ✨"
echo -e ""
echo -e "  ${NC}Final Steps:"
echo -e "  1. The extension folder is located at ${BLUE}$INSTALL_DIR${NC}"
echo -e "  2. Go to your browser's extensions page (e.g. ${BLUE}chrome://extensions${NC})"
echo -e "  3. Turn on ${YELLOW}'Developer mode'${NC}."
echo -e "  4. Drag and drop the ${YELLOW}'extension'${NC} folder into the browser."
echo -e ""
echo -e "  ${GREEN}Open a new tab to see the magic. 🎉${NC}"
echo -e ""

xdg-open "$INSTALL_DIR" 2>/dev/null || true
