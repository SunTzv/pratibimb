#!/bin/bash
set -e

GREEN='\033[38;2;50;255;100m'
BLUE='\033[38;2;100;150;255m'
NC='\033[0m'

echo -e ""
echo -e "  \033[38;2;150;200;255mPratibimb Updater\033[0m"
echo -e "  \033[38;2;100;100;100m=================================\033[0m"
echo -e ""

INSTALL_DIR="$HOME/.local/share/Pratibimb"
EXT_DIR="$INSTALL_DIR/extension"

if [ ! -d "$INSTALL_DIR" ]; then
    echo -e "  \033[38;2;255;100;100mError:\033[0m Pratibimb is not installed! Run install.sh first."
    exit 1
fi

echo -e "  ${BLUE}[1/2]${NC} Downloading latest stable release"
TMP_DIR=$(mktemp -d)
LATEST_URL=$(curl -s https://api.github.com/repos/SunTzv/Pratibimb/releases/latest | grep "zipball_url" | cut -d '"' -f 4)
curl -s -L -o "$TMP_DIR/release.zip" "$LATEST_URL"
echo -e "    ${GREEN}✓${NC} Fetched latest stable release"

echo -e "  ${BLUE}[2/2]${NC} Updating files"
unzip -q -o "$TMP_DIR/release.zip" -d "$TMP_DIR"
EXTRACTED_DIR=$(find "$TMP_DIR" -mindepth 1 -maxdepth 1 -type d | head -n 1)

cp -r "$EXTRACTED_DIR/extension/"* "$EXT_DIR/"
rm -rf "$TMP_DIR"
echo -e "    ${GREEN}✓${NC} Files updated successfully"

echo -e ""
echo -e "  ✨ Update Complete! ✨"
echo -e "  Go to chrome://extensions and click the 'Update' or reload button for Pratibimb."
echo -e ""
