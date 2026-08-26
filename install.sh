#!/usr/bin/env bash
# ==============================================================================
# 🛸 SOLARIS OS // EXTRATERRESTRIAL COMMAND DECK INSTALLER 👽
# Automated Installer & Environment Synthesizer for Linux & Niri Wayland
# ==============================================================================

set -e

# ANSI Color Palette
CYAN='\033[38;2;0;240;255m'
BRIGHT_CYAN='\033[38;2;112;251;253m'
GREEN='\033[38;2;57;255;20m'
BLUE='\033[38;2;0;153;255m'
PURPLE='\033[38;2;200;80;255m'
YELLOW='\033[38;2;255;230;0m'
RED='\033[38;2;255;42;95m'
BOLD='\033[1m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BACKUP_DIR="${HOME}/.config/solaris_backup_$(date +%Y%m%d_%H%M%S)"
NON_INTERACTIVE=false

for arg in "$@"; do
    case "$arg" in
        -y|--yes|--non-interactive)
            NON_INTERACTIVE=true
            ;;
        -h|--help)
            echo "Usage: ./install.sh [OPTIONS]"
            echo "Options:"
            echo "  -y, --yes    Run installation in non-interactive automatic mode"
            echo "  -h, --help   Display this command matrix help manual"
            exit 0
            ;;
    esac
done

banner() {
    clear
    echo -e "${CYAN}${BOLD}"
    cat << "EOF_BANNER"
  ███████╗ ██████╗ ██╗      █████╗ ██████╗ ██╗███████╗     ██████╗ ███████╗
  ██╔════╝██╔═══██╗██║     ██╔══██╗██╔══██╗██║██╔════╝    ██╔═══██╗██╔════╝
  ███████╗██║   ██║██║     ███████║██████╔╝██║███████╗    ██║   ██║███████╗
  ╚════██║██║   ██║██║     ██╔══██║██╔══██╗██║╚════██║    ██║   ██║╚════██║
  ███████║╚██████╔╝███████╗██║  ██║██║  ██║██║███████║    ╚██████╔╝███████║
  ╚══════╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝╚══════╝     ╚═════╝ ╚══════╝
EOF_BANNER
    echo -e "${PURPLE}  🛸 EXTRATERRESTRIAL-INDUSTRIAL WAYLAND COMMAND DECK & RICE 👽${NC}"
    echo -e "${BLUE}  ─────────────────────────────────────────────────────────────────${NC}"
    echo -e "${BRIGHT_CYAN}  Author: Venkat Badhri (venkatengineer) | Repository: solaris-os${NC}\n"
}

log_info() {
    echo -e "${CYAN}[SOLARIS INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SOLARIS OK]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[SOLARIS WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[SOLARIS ERROR]${NC} $1" >&2
}

detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO=$ID
        DISTRO_LIKE=${ID_LIKE:-$ID}
    else
        DISTRO="unknown"
        DISTRO_LIKE="unknown"
    fi
    log_info "Detected OS Platform: ${BOLD}${DISTRO}${NC} (${DISTRO_LIKE})"
}

check_sudo() {
    if command -v sudo >/dev/null 2>&1; then
        SUDO_CMD="sudo"
    elif [ "$(id -u)" -eq 0 ]; then
        SUDO_CMD=""
    else
        log_warn "Neither sudo nor root access available; package installation will be skipped."
        SUDO_CMD=""
    fi
}

install_packages() {
    log_info "Synchronizing subsystem packages and core dependencies..."

    if [[ "$DISTRO" =~ ^(arch|cachyos|manjaro|endeavouros|artix)$ ]] || [[ "$DISTRO_LIKE" =~ arch ]]; then
        local ARCH_PKGS=(
            niri waybar kitty fuzzel fastfetch hyprlock swaybg
            gtk3 gtk-layer-shell cairo pango json-c gdk-pixbuf2
            gcc make pkg-config git curl wget playerctl brightnessctl
            pipewire pipewire-pulse wireplumber alsa-utils libpulse
            python python-pip python-psutil python-sounddevice
            electron nodejs npm
        )
        if [ -n "$SUDO_CMD" ]; then
            $SUDO_CMD pacman -Sy --needed --noconfirm "${ARCH_PKGS[@]}" 2>/dev/null || {
                log_warn "Some pacman packages failed; continuing with available packages..."
            }
        fi

        # Optional AUR helper check for nerd fonts and extras
        if command -v yay >/dev/null 2>&1; then
            yay -S --needed --noconfirm ttf-jetbrains-mono-nerd blesh-git 2>/dev/null || true
        elif command -v paru >/dev/null 2>&1; then
            paru -S --needed --noconfirm ttf-jetbrains-mono-nerd blesh-git 2>/dev/null || true
        fi

    elif [[ "$DISTRO" =~ ^(fedora|rhel|centos)$ ]] || [[ "$DISTRO_LIKE" =~ fedora ]]; then
        local FEDORA_PKGS=(
            waybar kitty fuzzel fastfetch hyprlock swaybg
            gtk3-devel gtk-layer-shell-devel cairo-devel pango-devel json-c-devel gdk-pixbuf2-devel
            gcc make pkgconf git curl wget playerctl brightnessctl
            pipewire pipewire-pulseaudio wireplumber alsa-utils libpulse
            python3 python3-pip python3-devel nodejs npm
        )
        if [ -n "$SUDO_CMD" ]; then
            $SUDO_CMD dnf install -y "${FEDORA_PKGS[@]}" || log_warn "Non-critical dnf package warnings."
        fi

    elif [[ "$DISTRO" =~ ^(ubuntu|debian|pop|linuxmint)$ ]] || [[ "$DISTRO_LIKE" =~ (debian|ubuntu) ]]; then
        local DEBIAN_PKGS=(
            waybar kitty fuzzel fastfetch swaybg
            libgtk-3-dev libgtk-layer-shell-dev libcairo2-dev libpango1.0-dev libjson-c-dev libgdk-pixbuf-2.0-dev
            gcc make pkg-config git curl wget playerctl brightnessctl
            pipewire wireplumber alsa-utils libpulse0
            python3 python3-pip python3-venv nodejs npm
        )
        if [ -n "$SUDO_CMD" ]; then
            $SUDO_CMD apt update -y
            $SUDO_CMD apt install -y "${DEBIAN_PKGS[@]}" || log_warn "Non-critical apt package warnings."
        fi
    else
        log_warn "Unrecognized Linux distribution. Please ensure Niri, Waybar, Kitty, GTK3 and C compiler are installed."
    fi

    log_success "Subsystem package validation completed."
}

install_fonts() {
    log_info "Verifying JetBrainsMono Nerd Font & Icon typography..."
    mkdir -p "${HOME}/.local/share/fonts"

    if ! fc-list : family | grep -iq "JetBrainsMono Nerd Font"; then
        log_info "Downloading latest JetBrainsMono Nerd Font release..."
        local FONT_ZIP="/tmp/JetBrainsMono.zip"
        curl -fsSL "https://github.com/ryanoasis/nerd-fonts/releases/latest/download/JetBrainsMono.tar.xz" -o "/tmp/JetBrainsMono.tar.xz" 2>/dev/null && {
            tar -xf "/tmp/JetBrainsMono.tar.xz" -C "${HOME}/.local/share/fonts/" 2>/dev/null || true
            rm -f "/tmp/JetBrainsMono.tar.xz"
        } || {
            curl -fsSL "https://github.com/ryanoasis/nerd-fonts/releases/latest/download/JetBrainsMono.zip" -o "$FONT_ZIP" 2>/dev/null && {
                unzip -q -o "$FONT_ZIP" -d "${HOME}/.local/share/fonts/" 2>/dev/null || true
                rm -f "$FONT_ZIP"
            }
        }
        fc-cache -fv "${HOME}/.local/share/fonts" >/dev/null 2>&1 || true
        log_success "JetBrainsMono Nerd Font installed."
    else
        log_success "JetBrainsMono Nerd Font is already active."
    fi
}

backup_existing_configs() {
    log_info "Creating defensive backup of existing configurations..."
    mkdir -p "$BACKUP_DIR"

    local TARGETS=(
        "${HOME}/.config/niri"
        "${HOME}/.config/waybar"
        "${HOME}/.config/kitty"
        "${HOME}/.config/fuzzel"
        "${HOME}/.config/fastfetch"
        "${HOME}/.config/hypr/hyprlock.conf"
        "${HOME}/.config/yazi"
        "${HOME}/.config/gtk-3.0"
        "${HOME}/.config/gtk-4.0"
        "${HOME}/.bashrc"
        "${HOME}/.blerc"
    )

    for item in "${TARGETS[@]}"; do
        if [ -e "$item" ]; then
            cp -r --parents "$item" "$BACKUP_DIR" 2>/dev/null || cp -r "$item" "$BACKUP_DIR/" 2>/dev/null || true
        fi
    done

    log_success "Backup preserved at: ${BOLD}$BACKUP_DIR${NC}"
}

deploy_configs() {
    log_info "Deploying Solaris OS dotfiles and Wayland compositor configurations..."

    mkdir -p "${HOME}/.config/niri" \
             "${HOME}/.config/waybar" \
             "${HOME}/.config/kitty" \
             "${HOME}/.config/fuzzel" \
             "${HOME}/.config/fastfetch" \
             "${HOME}/.config/hypr" \
             "${HOME}/.config/yazi" \
             "${HOME}/.config/gtk-3.0" \
             "${HOME}/.config/gtk-4.0" \
             "${HOME}/.config/systemd/user" \
             "${HOME}/.local/bin" \
             "${HOME}/.local/share/applications" \
             "${HOME}/.local/share/icons/hicolor/512x512/apps" \
             "${HOME}/.local/share/icons/hicolor/256x256/apps" \
             "${HOME}/.local/share/solaris/sounds" \
             "${HOME}/.local/share/solaris/style" \
             "${HOME}/.local/share/sounds" \
             "${HOME}/Pictures/Wallpapers"

    # Copy configs
    cp -rf "${SCRIPT_DIR}/config/niri/"* "${HOME}/.config/niri/"
    cp -rf "${SCRIPT_DIR}/config/waybar/"* "${HOME}/.config/waybar/"
    cp -rf "${SCRIPT_DIR}/config/kitty/"* "${HOME}/.config/kitty/"
    cp -rf "${SCRIPT_DIR}/config/fuzzel/"* "${HOME}/.config/fuzzel/"
    cp -rf "${SCRIPT_DIR}/config/fastfetch/"* "${HOME}/.config/fastfetch/"
    cp -rf "${SCRIPT_DIR}/config/hypr/"* "${HOME}/.config/hypr/"
    cp -rf "${SCRIPT_DIR}/config/yazi/"* "${HOME}/.config/yazi/"
    cp -rf "${SCRIPT_DIR}/config/gtk-3.0/"* "${HOME}/.config/gtk-3.0/"
    cp -rf "${SCRIPT_DIR}/config/gtk-4.0/"* "${HOME}/.config/gtk-4.0/"

    # Replace wallpaper path in hyprlock to match current user's home
    sed -i "s|\\\$HOME|${HOME}|g" "${HOME}/.config/hypr/hyprlock.conf" 2>/dev/null || true

    # Copy Shell
    cp -rf "${SCRIPT_DIR}/shell/.bashrc" "${HOME}/.bashrc"
    cp -rf "${SCRIPT_DIR}/shell/.blerc" "${HOME}/.blerc"
    if [ -f "${SCRIPT_DIR}/shell/fzf_bash_integration.sh" ]; then
        cp -rf "${SCRIPT_DIR}/shell/fzf_bash_integration.sh" "${HOME}/.local/share/fzf_bash_integration.sh"
    fi

    # Copy Style for background widgets
    cp -rf "${SCRIPT_DIR}/core/solaris-bg/style/solaris.css" "${HOME}/.local/share/solaris/style/" 2>/dev/null || true

    log_success "Dotfiles deployed successfully."
}

compile_and_install_core() {
    log_info "Compiling high-performance native C engines (solaris-bg & solaris-hologram)..."

    # Compile solaris-bg
    if [ -d "${SCRIPT_DIR}/core/solaris-bg" ]; then
        log_info "Building solaris-bg Layer Shell engine..."
        make -C "${SCRIPT_DIR}/core/solaris-bg" clean >/dev/null 2>&1 || true
        make -C "${SCRIPT_DIR}/core/solaris-bg" all
        make -C "${SCRIPT_DIR}/core/solaris-bg" install
        log_success "solaris-bg widgets built & installed."
    fi

    # Compile solaris-hologram
    if [ -d "${SCRIPT_DIR}/core/solaris-hologram" ]; then
        log_info "Building solaris-hologram 3D matrix engine..."
        make -C "${SCRIPT_DIR}/core/solaris-hologram" clean >/dev/null 2>&1 || true
        make -C "${SCRIPT_DIR}/core/solaris-hologram" all
        make -C "${SCRIPT_DIR}/core/solaris-hologram" install
        log_success "solaris-hologram 3D matrix built & installed."
    fi
}

install_binaries_and_apps() {
    log_info "Installing command matrix CLI tools, scripts, and spacecraft applications..."

    # Install CLI binaries
    cp -rf "${SCRIPT_DIR}/bin/"* "${HOME}/.local/bin/"
    chmod +x "${HOME}/.local/bin/"*

    # Install Spacecraft GUI Applications
    mkdir -p "${HOME}/.local/share/solaris-dashboard" \
             "${HOME}/.local/share/solaris-explorer" \
             "${HOME}/.local/share/aetheris/app"

    cp -rf "${SCRIPT_DIR}/apps/solaris-dashboard/"* "${HOME}/.local/share/solaris-dashboard/" 2>/dev/null || true
    cp -rf "${SCRIPT_DIR}/apps/solaris-explorer/"* "${HOME}/.local/share/solaris-explorer/" 2>/dev/null || true
    cp -rf "${SCRIPT_DIR}/apps/aetheris/"* "${HOME}/.local/share/aetheris/app/" 2>/dev/null || true

    # Install Assets (Wallpapers, Sounds, Desktop entries, Icons)
    cp -rf "${SCRIPT_DIR}/assets/wallpapers/"* "${HOME}/Pictures/Wallpapers/" 2>/dev/null || true
    cp -rf "${SCRIPT_DIR}/assets/sounds/solaris/"* "${HOME}/.local/share/solaris/sounds/" 2>/dev/null || true
    cp -rf "${SCRIPT_DIR}/assets/sounds/spaceship_engine_start.wav" "${HOME}/.local/share/sounds/" 2>/dev/null || true
    cp -rf "${SCRIPT_DIR}/assets/sounds/solaris_nominal_telemetry.wav" "${HOME}/.local/share/sounds/" 2>/dev/null || true
    cp -rf "${SCRIPT_DIR}/assets/sounds/play_unlock_sequence.py" "${HOME}/.local/share/sounds/" 2>/dev/null || true
    chmod +x "${HOME}/.local/share/sounds/play_unlock_sequence.py" 2>/dev/null || true

    cp -rf "${SCRIPT_DIR}/assets/desktop-entries/"* "${HOME}/.local/share/applications/" 2>/dev/null || true
    cp -rf "${SCRIPT_DIR}/assets/icons/512x512/apps/"* "${HOME}/.local/share/icons/hicolor/512x512/apps/" 2>/dev/null || true
    cp -rf "${SCRIPT_DIR}/assets/icons/256x256/apps/"* "${HOME}/.local/share/icons/hicolor/256x256/apps/" 2>/dev/null || true

    # Update desktop database
    update-desktop-database "${HOME}/.local/share/applications" >/dev/null 2>&1 || true

    log_success "CLI binaries, apps, and assets synchronized."
}

setup_voice_engine() {
    log_info "Configuring Solaris Neural Voice Unlock Environment & Kaldi ASR..."

    local VENV_DIR="${HOME}/.local/share/solaris-voice-env"
    local VOSK_DIR="${HOME}/.local/share/vosk-models"
    local MODEL_DIR="${VOSK_DIR}/vosk-model-small-en-us-0.15"

    mkdir -p "$VOSK_DIR"

    if [ ! -d "$VENV_DIR" ]; then
        log_info "Creating dedicated Python virtual environment at ${VENV_DIR}..."
        python3 -m venv "$VENV_DIR"
    fi

    log_info "Installing Python dependencies (vosk, sounddevice, edge-tts, psutil)..."
    "${VENV_DIR}/bin/pip" install --quiet --upgrade pip 2>/dev/null || true
    "${VENV_DIR}/bin/pip" install --quiet vosk sounddevice edge-tts psutil requests tabulate 2>/dev/null || {
        log_warn "Python package install had warnings; continuing..."
    }

    if [ ! -d "$MODEL_DIR" ]; then
        log_info "Downloading Vosk speech acoustic model (small English 40MB)..."
        local MODEL_ZIP="/tmp/vosk-model-small.zip"
        if curl -fsSL "https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip" -o "$MODEL_ZIP"; then
            unzip -q "$MODEL_ZIP" -d "$VOSK_DIR"
            rm -f "$MODEL_ZIP"
            log_success "Vosk acoustic model installed."
        else
            log_warn "Could not download Vosk model automatically. Voice unlock will use PIN fallback until downloaded."
        fi
    else
        log_success "Vosk acoustic model is verified and ready."
    fi
}

setup_systemd_services() {
    log_info "Registering and enabling user systemd background services..."

    cp -rf "${SCRIPT_DIR}/systemd/solaris-power-listener.service" "${HOME}/.config/systemd/user/"
    systemctl --user daemon-reload >/dev/null 2>&1 || true
    systemctl --user enable --now solaris-power-listener.service >/dev/null 2>&1 || true

    log_success "solaris-power-listener.service is armed and active."
}

setup_plymouth_bootsplash() {
    if [ -d "${SCRIPT_DIR}/bootsplash/plymouth-theme/solaris-alien-boot" ] && command -v plymouth-set-default-theme >/dev/null 2>&1; then
        if [ "$NON_INTERACTIVE" = true ]; then
            INSTALL_BOOT=true
        else
            echo -ne "${YELLOW}Do you want to install Solaris Alien 3D Plymouth Boot Splash? [y/N]: ${NC}"
            read -r response
            case "$response" in
                [yY][eE][sS]|[yY]) INSTALL_BOOT=true ;;
                *) INSTALL_BOOT=false ;;
            esac
        fi

        if [ "$INSTALL_BOOT" = true ] && [ -n "$SUDO_CMD" ]; then
            log_info "Installing Plymouth 3D Alien Boot Splash Theme..."
            $SUDO_CMD mkdir -p "/usr/share/plymouth/themes/solaris-alien-boot"
            $SUDO_CMD cp -rf "${SCRIPT_DIR}/bootsplash/plymouth-theme/solaris-alien-boot/"* "/usr/share/plymouth/themes/solaris-alien-boot/"
            $SUDO_CMD plymouth-set-default-theme -R solaris-alien-boot 2>/dev/null || true
            log_success "Plymouth boot splash configured."
        fi
    fi
}

summary() {
    echo -e "\n${GREEN}${BOLD}==============================================================================${NC}"
    echo -e "${GREEN}${BOLD}  🛸 SOLARIS OS // INITIALIZATION COMPLETE // ALL SYSTEMS OPERATIONAL 👽${NC}"
    echo -e "${GREEN}${BOLD}==============================================================================${NC}"
    echo -e "${CYAN}  Core Architecture:${NC}"
    echo -e "    • ${BOLD}Compositor:${NC}     Niri (Wayland Dynamic Scrolling Layout)"
    echo -e "    • ${BOLD}Status Bar:${NC}     Waybar with Extraterrestrial Sci-Fi HUD Modules"
    echo -e "    • ${BOLD}Terminal:${NC}       Kitty (Deep Obsidian Carbon + Ionized Cyan)"
    echo -e "    • ${BOLD}Launcher:${NC}       Fuzzel Application Matrix (${BOLD}Mod + D${NC})"
    echo -e "    • ${BOLD}Lockscreen:${NC}     Voice-Controlled Hyprlock (${BOLD}Mod + Alt + L${NC})"
    echo -e "    • ${BOLD}3D Hologram:${NC}    Interactive Hologram HUD (${BOLD}Mod + Shift + H${NC} or ${BOLD}Mod + W${NC})"
    echo -e "    • ${BOLD}Power Reactor:${NC}  Governor Cycler Overclock/Balanced/Cryo (${BOLD}Mod + P${NC})"
    echo -e "    • ${BOLD}Render Mode:${NC}    Maximum RAM Liberation & CPU Prioritization (${BOLD}Mod + Alt + R${NC})"
    echo -e "    • ${BOLD}Task Nexus:${NC}     Xenomon Interactive Process Manager (${BOLD}Ctrl + Shift + Esc${NC})"
    echo -e "    • ${BOLD}File Navigator:${NC} Solaris Explorer (${BOLD}Mod + E${NC})"
    echo -e "    • ${BOLD}Audio HUD:${NC}      Aetheris Music HUD Overlay"
    echo -e "    • ${BOLD}Command Matrix:${NC} Alien-CMD Manual & Quick Suggester (${BOLD}Mod + /${NC})"
    echo -e "\n${BRIGHT_CYAN}  Key Commands:${NC}"
    echo -e "    solaris-bg status      Check background widget daemons"
    echo -e "    solaris-bg wake        Trigger awakening sound & gauge sweep"
    echo -e "    solaris-render-mode    Toggle heavy 3D rendering maximum RAM mode"
    echo -e "    xenomon                Launch futuristic process monitor"
    echo -e "    alien-scan             Run extraterrestrial telemetry diagnostic"
    echo -e "    alien-radar            Engage tactical subspace RF radar"
    echo -e "${GREEN}==============================================================================${NC}\n"
}

main() {
    banner
    detect_distro
    check_sudo
    install_packages
    install_fonts
    backup_existing_configs
    deploy_configs
    compile_and_install_core
    install_binaries_and_apps
    setup_voice_engine
    setup_systemd_services
    setup_plymouth_bootsplash
    summary
}

main "$@"
