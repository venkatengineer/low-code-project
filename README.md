# 🛸 SOLARIS OS // EXTRATERRESTRIAL COMMAND DECK

<div align="center">

[![License: MIT](https://img.shields.io/badge/License-MIT-00f0ff.svg?style=for-the-badge&logo=opensourceinitiative&logoColor=black)](LICENSE)
[![Platform: Linux](https://img.shields.io/badge/Platform-Linux%20%2F%20Wayland-00a6fb.svg?style=for-the-badge&logo=linux&logoColor=white)](https://wayland.freedesktop.org/)
[![Compositor: Niri](https://img.shields.io/badge/Compositor-Niri%20Scrolling-00ff88.svg?style=for-the-badge&logo=wayland&logoColor=black)](https://github.com/YaLTeR/niri)
[![Shell: Bash + Ble.sh](https://img.shields.io/badge/Shell-Bash%20%2B%20Ble.sh-a855f7.svg?style=for-the-badge&logo=gnubash&logoColor=white)](https://github.com/akinomyoga/ble.sh)
[![Audio: Vosk Voice ASR](https://img.shields.io/badge/Voice%20ASR-Vosk%20Kaldi%20Neural-ffb703.svg?style=for-the-badge&logo=soundcharts&logoColor=black)](https://alphacephei.com/vosk/)

<p align="center">
  <b>A complete, hyper-immersive extraterrestrial-industrial Linux desktop environment & rice.</b><br>
  Engineered for aerospace precision, high-readability telemetry, dynamic Wayland scrolling, and zero-compromise computing.
</p>

[✨ Features](#-key-subsystems--features) •
[⌨️ Keybindings](#%EF%B8%8F-spacecraft-flight-deck-keybindings) •
[🚀 Installation](#-installation--deployment) •
[📦 Architecture](#-subsystem-architecture) •
[🛠️ Manual Build](#%EF%B8%8F-manual-compilation--setup)

---

</div>

## 🌌 Overview

**Solaris OS** transforms your Linux workstation into an extraterrestrial starship bridge. Built primarily on the **Niri** Wayland scrollable-tiling compositor, it combines high-performance custom C background engines, real-time hardware telemetry HUDs, GLSL shader window animations, offline neural voice recognition, and dedicated sci-fi applications.

### 🎨 Design Philosophy & Color Matrix
* **Deep Bulkhead Obsidian (`#05080e` / `#0a1017`)**: Non-reflective deep carbon chassis preventing eye strain.
* **Ionized Cold Cyan (`#00f0ff` / `#00e5ff`)**: Primary telemetry readouts, radar sweeps, and laser focus rings.
* **Bioluminescent Emerald (`#00ff88` / `#39ff7a`)**: Operational nominal status, memory equilibrium, and network streams.
* **Psionic Plasma Purple (`#a855f7` / `#c084fc`)**: Warp drives, quantum buffer allocations, and git telemetry.
* **Containment Amber & Crimson (`#ffb703` / `#ff2a5f`)**: Thermal warnings, overclock governors, and critical alerts.

---

## ✨ Key Subsystems & Features

```
                                  🛸 SOLARIS OS BRIDGE ARCHITECTURE
  ┌───────────────────────────────────────────────────────────────────────────────────────────────────────┐
  │                                    TOP: Waybar Extraterrestrial HUD                                    │
  │  [⬡] [⬡ 1] [◈ 2] [⟡ 3] [◉ 4] [⌬ 5]   CHRONO // 2026.08.26 · 19:30   󰍛 4.2G   12%  󰋊 / 28%  󰤨 ⇡12M ⇣45M ⏻ │
  ├───────────────────────────────────────────────────────────────────────────────────────────────────────┤
  │ ┌──────────────────────────┐ ┌─────────────────────────────────────────┐ ┌──────────────────────────┐ │
  │ │  MISSION ACTIVITY STREAM │ │      3D HOLOGRAPHIC COMMAND MATRIX      │ │   AETHERIS AUDIO HUD     │ │
  │ │  • Download Complete     │ │      (C + OpenGL / Niri IPC HUD)        │ │   • Holographic Visualizer │ │
  │ │  • Package Updates (12)  │ │      Interactive Starship Node Orbit    │ │   • MPRIS Media Controller │ │
  │ │  • Git Commits (solaris) │ │      [Mod + Shift + H] / [Mod + W]      │ │   • Spectral Analyzer      │ │
  │ └──────────────────────────┘ └─────────────────────────────────────────┘ └──────────────────────────┘ │
  ├───────────────────────────────────────────────────────────────────────────────────────────────────────┤
  │                                 BOTTOM: Aerospace Instrument Deck (C + Cairo)                         │
  │  ┌───────────────────────┐ ┌───────────────────────┐ ┌───────────────────────┐ ┌────────────────────┐  │
  │  │  PROPULSION CPU TACH  │ │ QUANTUM RAM & STORAGE │ │ TENSOR GPU & TURBINE  │ │ AVIONICS BATTERY   │  │
  │  │  Speedometer + Plasma │ │ Hex Buffer + NVMe 0/1 │ │ Tachometer + Fan RPM  │ │ 10-Cell Core + WS  │  │
  │  └───────────────────────┘ └───────────────────────┘ └───────────────────────┘ └────────────────────┘  │
  │                           [  FILES ] [  TERMINAL ] [  NET ] [ 󰑈 RENDER ]                           │
  └───────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 1. 🌀 Dynamic Scrolling Window Matrix (Niri + Custom GLSL Shaders)
* **Infinite Horizontal Strip**: Infinite horizontal workstation column workflow with magnetic spring damping.
* **Custom GLSL Motion Shaders**: Window open, close, and resize shaders featuring laser scanline sweeps and clean geometry clipping.
* **Aerospace Chamfer & Border Armor**: 1.5px laser cyan focus rings with deep ambient occlusion drop shadows.

### 2. ⚡ Native C Background Instrument Deck (`solaris-bg`)
* Ultra-low-overhead C widgets running on the Wayland **Layer Shell** protocol below active windows.
* **Bay 1 (Propulsion Tachometer)**: Real-time CPU speedometer, 16-core plasma injectors, and historical load sparkline.
* **Bay 2 (Quantum Buffer & NVMe)**: Interactive RAM core, swap allocation, and NVMe partition monitors.
* **Bay 3 (Tensor GPU & Cooling Turbine)**: GPU utilization, VRAM gauge, and live rotating cooling fan RPM turbine.
* **Bay 4 (Avionics Power & Bridge)**: Boeing 10-cell battery readout and direct Niri workspace selectors (01 CMD, 02 ENG, 03 NAV, 04 COM, 05 SCI, 06 AUX).

### 3. 🌌 3D Holographic Workspace Command Center (`solaris-hologram`)
* Written in native **C + OpenGL / Cairo + GTK Layer Shell**.
* Direct **Niri IPC integration** displaying workspace nodes in an interactive orbital particle field.
* Toggle instantly via `Mod + Shift + H` or `Mod + W`.

### 4. 🎙️ Neural Voice-Activated Lockscreen (Hyprlock + Vosk Kaldi ASR)
* Speak **"ENGINES START"**, **"SOLARIS UNLOCK"**, or **"START IGNITION"** to unlock the desktop hands-free.
* Strict multi-word filtering prevents accidental single-word triggers.
* Live audio engine sound playback accompanied by **dynamic Edge-TTS live telemetry reports** (reporting current CPU temperature, battery capacity, and weather).
* PIN / Password fallback always available.

### 5. 🎬 Maximum RAM Liberation Rendering Mode (`solaris-render-mode`)
* Single keystroke (`Mod + Alt + R` or `Super + Alt + R`) to liberate over **1.2 GB+ of RAM**.
* Temporarily freezes non-critical background daemons (indexers, metadata trackers), purges system page caches, sets the CPU governor to `Performance`, and boosts render applications (Blender, Kdenlive, Godot, compilers) to `-10 Nice` and Real-Time I/O.
* Disengaging immediately restores all background services and previous power profiles.

### 6. 🚀 Dynamic Reactor Power Mode Governor (`solaris-power-mode`)
* Cycle power profiles (`Mod + P`):
  * **⚡ Warp Overclock (Performance)**: High clock governor with hyperdrive sound effect.
  * **🛸 Orbital Balanced (Balanced)**: Standard nominal operations vector.
  * **❄️ Cryo Power Saver (Power-Saver)**: Low-draw thermal conservation.
* System DBus listener daemon (`solaris-power-listener.service`) automatically triggers sound effects when AC power is plugged/unplugged or governor changes.

### 7. 🛸 Dedicated Spacecraft GUI Applications
* **Solaris Dashboard** (`solaris-dashboard`): Full-screen cockpit telemetry bridge with live hardware sensor arrays and Niri window auto-following.
* **Starship Data Navigator** (`solaris-explorer`): Sci-fi styled file manager with instant disk telemetry, categorized media classification, and terminal integration.
* **Aetheris Music HUD** (`aetheris`): Extraterrestrial holographic music player with spectral visualizer and Waybar MPRIS integration.

### 8. 👾 Terminal Subsystems & Shell Tools
* **Xenomon** (`xenomon`): Terminal-based aerospace task manager and process control HUD.
* **Alien Command Matrix** (`alien-cmd`): Interactive command suggester and hotkey manual (`Mod + /` or `Ctrl + G`).
* **Visualizers**: `alien-scan` (diagnostic scan), `alien-radar` (subspace RF sweep), `alien-matrix` (bioluminescent glyph rain), `alien-glitch`, `alien-say`.
* **Prompt HUD**: Real-time git telemetry prompt with `ble.sh` autosuggestions and FZF extraterrestrial HUD styling.

### 9. 🌟 Plymouth 3D Sci-Fi Boot Splash (`solaris-bootsplash`)
* 225-frame 60 FPS full HD boot splash animation rendered in C + Cairo, paired with a custom synthesized spacecraft ignition soundtrack.

---

## ⌨️ Spacecraft Flight Deck Keybindings

| Keybinding | Subsystem Action |
| :--- | :--- |
| **`Mod + T`** | Initialize Primary Ship Console (`Kitty`) |
| **`Mod + D`** | Application Launch Matrix (`Fuzzel`) |
| **`Mod + E`** | Starship Data Navigator (`Solaris Explorer`) |
| **`Mod + Alt + L`** | Engage Security Lockdown (Voice Unlock & PIN) |
| **`Ctrl + Shift + Esc`** | Process Telemetry Nexus (`Xenomon`) |
| **`Mod + /`** | Interactive Command Matrix & Manual (`Alien-CMD`) |
| **`Mod + Shift + H`** / **`Mod + W`** | 3D Holographic Workspace Command Center |
| **`Mod + P`** | Cycle Power Reactor (Overclock / Balanced / Cryo) |
| **`Mod + Alt + R`** | Engage / Disengage Rendering Mode (RAM Liberation) |
| **`Mod + 1 .. 6`** | Engage Station 01-06 (Workspaces 1-6) |
| **`Mod + Ctrl + 1 .. 6`** | Transfer Console to Station 01-06 |
| **`Mod + H / J / K / L`** | Vi Column Navigation (Left, Down, Up, Right) |
| **`Mod + ← / → / ↑ / ↓`** | Arrow Column Navigation |
| **`Mod + Ctrl + H / L`** | Move Console Column Left / Right |
| **`Mod + [ / ]`** | Consume / Expel Console from Column |
| **`Mod + R`** | Cycle Console Column Width Preset (1/3, 1/2, 2/3) |
| **`Mod + F`** | Maximize Column Viewport |
| **`Mod + Shift + F`** | Fullscreen Window |
| **`Mod + V`** | Toggle Window Float / Tile |
| **`Mod + O`** | Orbital Workspace Overview |
| **`Mod + Q`** | Close Window / Console |
| **`Mod + Shift + E`** | Exit Solaris Session |
| **`Print`** | Capture Interactive Screenshot |

> *Note: By default, `Mod` maps to the `Super` (Windows) key.*

---

## 🚀 Installation & Deployment

### Quick Automated Installation

```bash
# 1. Clone the repository
git clone https://github.com/venkatengineer/solaris-os.git
cd solaris-os

# 2. Run the automated installer
chmod +x install.sh
./install.sh
```

The installer automatically:
1. Detects your Linux distribution (**Arch, CachyOS, Fedora, Debian, Ubuntu, Pop!_OS**).
2. Installs required system packages, Wayland libraries, and build tools.
3. Downloads and caches **JetBrainsMono Nerd Font** and symbols.
4. Backs up existing configs to `~/.config/solaris_backup_<timestamp>`.
5. Compiles native C engines (`solaris-bg` and `solaris-hologram`).
6. Deploys dotfiles, shell scripts, wallpapers, and sound assets.
7. Sets up the Python voice unlock virtual environment and downloads the Vosk ASR model.
8. Enables the systemd user power telemetry daemon.

---

## 📦 Subsystem Architecture

```
solaris-os/
├── config/                          # Wayland & Desktop Configuration Trees
│   ├── niri/                        # Niri compositor config & GLSL motion shaders
│   ├── waybar/                      # Extraterrestrial Sci-Fi Waybar layout & CSS
│   ├── kitty/                       # Kitty terminal color matrix & glowing tabs
│   ├── fuzzel/                      # Fuzzel application launcher theme
│   ├── fastfetch/                   # Fastfetch configuration & ASCII alien art
│   ├── hypr/                        # Voice-controlled Hyprlock configuration
│   ├── yazi/                        # Yazi terminal file manager theme
│   ├── gtk-3.0/                     # GTK3 / Nautilus aerospace dark stylesheet
│   └── gtk-4.0/                     # GTK4 / Libadwaita dark stylesheet
├── shell/                           # Shell Environment & Prompt Architecture
│   ├── .bashrc                      # Extraterrestrial telemetry HUD prompt & aliases
│   ├── .blerc                       # Real-time autosuggestions & syntax highlighting
│   └── fzf_bash_integration.sh      # FZF fuzzy search aerospace HUD bindings
├── bin/                             # Custom CLI Binaries, Daemons & Tools
│   ├── solaris-bg                   # Background widget daemon manager
│   ├── solaris-render-mode          # Maximum RAM liberation controller
│   ├── solaris-power-mode           # Power governor cycler (Overclock/Balanced/Cryo)
│   ├── solaris-power-listener       # DBus system power change audio listener
│   ├── solaris-voice-unlock         # Vosk Kaldi neural voice recognition daemon
│   ├── solaris-lock                 # Lockscreen launcher with voice arming
│   ├── solaris-play-sound           # Low-latency PipeWire/ALSA sound dispatcher
│   ├── solaris-mem-optimizer        # System-wide memory trimmer
│   ├── xenomon                      # Extraterrestrial curses task manager
│   ├── alien-cmd                    # Interactive command matrix & hotkey HUD
│   ├── alien-scan                   # System telemetry diagnostic scanner
│   ├── alien-radar                  # Subspace RF sweep radar
│   ├── alien-matrix                 # Bioluminescent ASCII rain
│   └── pipes.sh                     # Extraterrestrial pipe visualizer
├── core/                            # Native C / C++ High-Performance Engines
│   ├── solaris-bg/                  # GTK Layer Shell live desktop instrumentation
│   │   ├── Makefile
│   │   ├── include/                 # Cairo graphics, telemetry headers
│   │   ├── src/                     # Tachometer, memory, GPU, battery engines
│   │   └── style/                   # Layer Shell stylesheet
│   └── solaris-hologram/            # 3D Holographic Workspace Controller
│       ├── Makefile
│       ├── include/                 # OpenGL, 3D math, Niri IPC headers
│       └── src/                     # Particle system, orbit renderer, HUD
├── apps/                            # Spacecraft Desktop GUI Applications
│   ├── solaris-dashboard/           # Cockpit bridge system telemetry overlay
│   ├── solaris-explorer/            # Starship data navigator file manager
│   └── aetheris/                    # Holographic music player & Waybar HUD
├── bootsplash/                      # Plymouth Boot Theme & Frame Generator
│   ├── generate_animation.sh        # 225-frame 3D animation builder
│   ├── plymouth-theme/              # Ready-to-install Plymouth theme
│   └── web-preview/                 # HTML5 preview player
├── systemd/                         # Systemd User Services
│   └── solaris-power-listener.service
├── assets/                          # Multimedia Assets
│   ├── wallpapers/                  # High-resolution spacecraft cockpit wallpapers
│   ├── sounds/                      # Spacecraft awakening, warp revs, engine start
│   └── desktop-entries/             # Standard XDG .desktop application files
├── install.sh                       # Universal automated installer
├── uninstall.sh                     # Clean system uninstaller & backup restorer
├── LICENSE                          # MIT License
└── README.md                        # Documentation
```

---

## 🛠️ Manual Compilation & Setup

If you prefer building and installing components manually:

### 1. Build C Background Widgets
```bash
cd core/solaris-bg
make
make install
```

### 2. Build 3D Hologram Engine
```bash
cd core/solaris-hologram
make
make install
```

### 3. Setup Neural Voice Environment
```bash
mkdir -p ~/.local/share/vosk-models ~/.local/share/solaris-voice-env
python3 -m venv ~/.local/share/solaris-voice-env
~/.local/share/solaris-voice-env/bin/pip install vosk sounddevice edge-tts psutil

# Download Vosk Model
curl -fsSL https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip -o /tmp/vosk.zip
unzip /tmp/vosk.zip -d ~/.local/share/vosk-models/
```

### 4. Enable Systemd Background Listener
```bash
cp systemd/solaris-power-listener.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now solaris-power-listener.service
```

---

## 🔊 Audio & Sound Suite

Solaris includes custom sound effects managed by `solaris-play-sound`:
* **`spaceship_awakening.wav`**: Sequential startup power-on sound when logging into the session.
* **`solaris_unlock.wav`**: Biometric authorization chime.
* **`spaceship_engine_start.wav`**: Hypercar-inspired sci-fi ignition sound on unlock.
* **`warp_overclock.wav`**: Power governor performance engagement.
* **`nominal_balanced.wav`**: Balanced profile transition chime.
* **`cryo_saver.wav`**: Low-power thermal conservation engagement.
* **`render_engage.wav` / `render_disengage.wav`**: RAM liberation mode transitions.

---

## 🗑️ Uninstallation

To restore your previous setup:
```bash
cd solaris-os
./uninstall.sh
```
This stops all active daemons, disables systemd user units, cleans up Solaris binaries, and automatically restores your original backed-up configurations.

---

## 👤 Author

**Venkat Badhri**
* GitHub: [@venkatengineer](https://github.com/venkatengineer)
* Repository: [https://github.com/venkatengineer/solaris-os](https://github.com/venkatengineer/solaris-os)

---

## 📄 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.
