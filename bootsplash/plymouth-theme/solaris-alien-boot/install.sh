#!/bin/bash
set -e
THEME_NAME="solaris-alien-boot"
TARGET_DIR="/usr/share/plymouth/themes/$THEME_NAME"

echo "Installing $THEME_NAME to $TARGET_DIR..."
sudo mkdir -p "$TARGET_DIR"
sudo cp -r ./* "$TARGET_DIR/"
sudo plymouth-set-default-theme -R "$THEME_NAME"
echo "Successfully installed and configured $THEME_NAME as default Plymouth boot splash!"
