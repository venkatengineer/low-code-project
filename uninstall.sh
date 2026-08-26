#!/usr/bin/env bash
# ==============================================================================
# 🛸 SOLARIS OS // SYSTEM UNINSTALLER & RESTORATION TOOL 👽
# ==============================================================================

set -e

RED='\033[38;2;255;42;95m'
CYAN='\033[38;2;0;240;255m'
GREEN='\033[38;2;57;255;20m'
YELLOW='\033[38;2;255;230;0m'
NC='\033[0m'

echo -e "${RED}==============================================================================${NC}"
echo -e "${RED}  🛸 SOLARIS OS // SYSTEM DECOMMISSION & UNINSTALLER${NC}"
echo -e "${RED}==============================================================================${NC}"

echo -ne "${YELLOW}Are you sure you want to uninstall Solaris OS and stop its daemons? [y/N]: ${NC}"
read -r resp
case "$resp" in
    [yY][eE][sS]|[yY]) ;;
    *) echo "Aborted."; exit 0 ;;
esac

echo -e "${CYAN}[1/5] Stopping active background daemons...${NC}"
solaris-bg stop >/dev/null 2>&1 || true
pkill -f solaris-hologram >/dev/null 2>&1 || true
pkill -f solaris-power-listener >/dev/null 2>&1 || true
pkill -f solaris-voice-unlock >/dev/null 2>&1 || true

echo -e "${CYAN}[2/5] Disabling systemd user services...${NC}"
systemctl --user disable --now solaris-power-listener.service >/dev/null 2>&1 || true
rm -f "${HOME}/.config/systemd/user/solaris-power-listener.service"

echo -e "${CYAN}[3/5] Removing Solaris binaries from ~/.local/bin...${NC}"
rm -f "${HOME}/.local/bin/solaris-"* \
      "${HOME}/.local/bin/alien-"* \
      "${HOME}/.local/bin/aetheris"* \
      "${HOME}/.local/bin/xenomon"

echo -e "${CYAN}[4/5] Removing desktop applications and sounds...${NC}"
rm -rf "${HOME}/.local/share/solaris" \
       "${HOME}/.local/share/solaris-dashboard" \
       "${HOME}/.local/share/solaris-explorer" \
       "${HOME}/.local/share/aetheris" \
       "${HOME}/.local/share/solaris-voice-env" \
       "${HOME}/.local/share/applications/solaris-"*.desktop \
       "${HOME}/.local/share/applications/aetheris.desktop" \
       "${HOME}/.local/share/applications/xenomon.desktop"

echo -e "${CYAN}[5/5] Checking for backups to restore...${NC}"
LATEST_BACKUP=$(ls -td "${HOME}/.config"/solaris_backup_* 2>/dev/null | head -n 1)
if [ -n "$LATEST_BACKUP" ] && [ -d "$LATEST_BACKUP" ]; then
    echo -e "${GREEN}Found backup at: $LATEST_BACKUP${NC}"
    echo -ne "${YELLOW}Do you want to restore this backup? [y/N]: ${NC}"
    read -r b_resp
    case "$b_resp" in
        [yY][eE][sS]|[yY])
            cp -rf "$LATEST_BACKUP/.config/"* "${HOME}/.config/" 2>/dev/null || true
            cp -rf "$LATEST_BACKUP/.bashrc" "${HOME}/.bashrc" 2>/dev/null || true
            cp -rf "$LATEST_BACKUP/.blerc" "${HOME}/.blerc" 2>/dev/null || true
            echo -e "${GREEN}Backup restored successfully.${NC}"
            ;;
    esac
fi

echo -e "${GREEN}Solaris OS uninstalled successfully.${NC}"
