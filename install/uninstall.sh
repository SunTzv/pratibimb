#!/bin/bash
set -e

RED='\033[38;2;255;100;100m'
GRAY='\033[38;2;150;150;150m'
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
echo -e "    ${GRAY}Removing Pratibimb...${NC}"
echo -e ""

echo -e "  ${RED}[1/2]${NC} Removing browser registrations"
remove_manifest() {
    local browser_dir="$1"
    if [ -f "$browser_dir/com.suntzv.pratibimb.json" ]; then
        rm -f "$browser_dir/com.suntzv.pratibimb.json"
        echo -e "    ${RED}-${NC} Removed from $browser_dir"
    fi
}

remove_manifest "$HOME/.config/google-chrome/NativeMessagingHosts"
remove_manifest "$HOME/.config/chromium/NativeMessagingHosts"
remove_manifest "$HOME/.config/BraveSoftware/Brave-Browser/NativeMessagingHosts"
remove_manifest "$HOME/.config/microsoft-edge/NativeMessagingHosts"

echo -e "  ${RED}[2/2]${NC} Removing files from local share"
INSTALL_DIR="$HOME/.local/share/Pratibimb"
if [ -d "$INSTALL_DIR" ]; then
    rm -rf "$INSTALL_DIR"
    echo -e "    ${RED}-${NC} Removed $INSTALL_DIR"
fi

echo -e ""
echo -e "  ✨ Uninstallation Complete! ✨"
echo -e ""
echo -e "  ${NC}Don't forget to remove the extension from your browser!${NC}"
echo -e ""
