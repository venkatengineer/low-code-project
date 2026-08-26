# ==============================================================================
# 🛸 EXTRATERRESTRIAL-INDUSTRIAL TELEMETRY & COMMAND SHELL 👽
# Precise · Engineered · Mysterious · Technological
# ==============================================================================

# Sourcing ble.sh at the top for real-time command autosuggestions
[[ $- == *i* ]] && [[ -f "$HOME/.local/share/blesh/ble.sh" ]] && source "$HOME/.local/share/blesh/ble.sh" --attach=none

# If not running interactively, don't do anything
[[ $- != *i* ]] && return

# Android & Java Development
export JAVA_HOME="$HOME/.local/opt/jdk-17"
export ANDROID_HOME="$HOME/development/android-sdk"

# User Binary Matrix
export PATH="$JAVA_HOME/bin:$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools:$HOME/development/flutter/bin:$HOME/.local/bin:$PATH"

# Environment Variables & Terminal Capabilities
export TERM=xterm-256color
export CLICOLOR=1
export EDITOR="nano"
export VISUAL="nano"

# Extraterrestrial Color Palette: Deep Graphite, Ionized Cyan, Electric Blue, Psionic Violet
export LS_COLORS='rs=0:di=01;36:ln=01;35:mh=00:pi=40;33:so=01;35:do=01;35:bd=40;33;01:cd=40;33;01:or=40;31;01:mi=00:su=37;41:sg=30;43:ca=30;41:tw=30;42:ow=34;42:st=37;44:ex=01;32:*.tar=01;31:*.zip=01;31:*.gz=01;31:*.png=01;35:*.jpg=01;35:*.svg=01;35:*.mp4=01;35:*.sh=01;32:*.py=01;36:'

# FZF Extraterrestrial Industrial HUD Styling
export FZF_DEFAULT_OPTS="--height 45% --reverse --border=rounded \
--color=bg+:#0d141e,fg+:#dff8f6,hl:#00f0ff,hl+:#39ff7a,pointer:#00f0ff,border:#0077cc,header:#00e5ff,prompt:#00f0ff,info:#5d7a8c"

# Source FZF Shell Integration
if [[ -f "$HOME/.local/share/fzf_bash_integration.sh" ]]; then
    source "$HOME/.local/share/fzf_bash_integration.sh"
fi

# Git Telemetry Helper
__xeno_git_branch() {
    local branch
    if branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null); then
        echo -e " \033[38;2;140;80;250m⌬ ($branch)\033[0m"
    fi
}

# Extraterrestrial-Industrial HUD Prompt
__set_xeno_prompt() {
    local C_BORDER="\[\033[38;2;40;60;80m\]"
    local C_BRACKET="\[\033[38;2;0;160;220m\]"
    local C_TAG="\[\033[38;2;120;140;160m\]"
    local C_HOST="\[\033[38;2;0;229;255m\]"
    local C_PATH="\[\033[38;2;0;153;255m\]"
    local C_CHEVRON="\[\033[38;2;0;240;255m\]"
    local C_RESET="\[\033[0m\]"

    PS1="${C_BORDER}⟦ ${C_BRACKET}0x7F ${C_BORDER}· ${C_TAG}node://${C_HOST}\u@\h ${C_BORDER}⟧ ${C_BORDER}⟨ ${C_PATH}\w ${C_BORDER}⟩\$(__xeno_git_branch)\n${C_CHEVRON}❯${C_RESET} "
}

PROMPT_COMMAND=__set_xeno_prompt

# Core Aliases: Alien Subsystems & Telemetry Suite
alias ls='ls --color=auto -F'
alias ll='ls -la --color=auto -F'
alias grep='grep --color=auto'

# Task Manager & Process Nexus
alias top='xenomon'
alias taskmanager='xenomon'
alias taskmgr='xenomon'
alias xeno-top='xenomon'
alias xenomon='xenomon'

# Command Recommender & Interactive Matrix
alias cmd='alien-cmd'
alias suggest='alien-cmd'
alias xeno-cmd='alien-cmd'

# Telemetry Diagnostics & Scanners
alias scan='alien-scan'
alias mothership='alien-scan'
alias radar='alien-radar'
alias alien='fastfetch'
alias xmatrix='alien-matrix'
alias glitch='alien-glitch'

# Quick Hotkey: Alt+S or Ctrl+G for Alien Command Suggester
bind -x '"\C-g": alien-cmd'

# Welcome Telemetry on Interactive Terminal Launch
if [[ -z "$_ALIEN_WELCOME_SHOWN" ]] && command -v fastfetch &>/dev/null; then
    export _ALIEN_WELCOME_SHOWN=1
    fastfetch
fi

# Attach ble.sh at the bottom of bashrc for real-time autosuggestions
[[ ${BLE_VERSION-} ]] && ble-attach

