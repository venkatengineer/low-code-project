// ==============================================================================
// 🔊 EDEX-UI HOLOGRAPHIC SOUND ENGINE — SOLARIS DASHBOARD
// Web Audio API Zero-Latency Sound Effects Engine
// ==============================================================================

class DashboardAudioManager {
  constructor() {
    this.ctx = null;
    this.buffers = new Map();
    this.muted = localStorage.getItem('solaris_dashboard_muted') === 'true';
    this.volume = 0.60;

    this.soundPaths = {
      folder: 'assets/audio/folder.wav',
      scan: 'assets/audio/scan.wav',
      expand: 'assets/audio/expand.wav',
      panels: 'assets/audio/panels.wav',
      keyboard: 'assets/audio/keyboard.wav',
      stdin: 'assets/audio/stdin.wav',
      stdout: 'assets/audio/stdout.wav',
      granted: 'assets/audio/granted.wav',
      denied: 'assets/audio/denied.wav',
      alarm: 'assets/audio/alarm.wav',
      theme: 'assets/audio/theme.wav'
    };

    this.lastPlayTime = {};
    this.init();
  }

  init() {
    const AudioContextClass = window.AudioContext || window.webkitAudioContext;
    if (!AudioContextClass) return;

    this.ctx = new AudioContextClass();

    const unlock = () => {
      if (this.ctx && this.ctx.state === 'suspended') {
        this.ctx.resume();
      }
      window.removeEventListener('click', unlock);
      window.removeEventListener('keydown', unlock);
    };
    window.addEventListener('click', unlock);
    window.addEventListener('keydown', unlock);

    // Preload all audio assets
    for (const [key, path] of Object.entries(this.soundPaths)) {
      this.loadSound(key, path);
    }
  }

  async loadSound(name, path) {
    try {
      const res = await fetch(path);
      const arrayBuffer = await res.arrayBuffer();
      if (this.ctx) {
        const audioBuf = await this.ctx.decodeAudioData(arrayBuffer);
        this.buffers.set(name, audioBuf);
      }
    } catch (err) {
      console.warn(`[Audio] Could not load ${name} (${path}):`, err.message);
    }
  }

  play(name, customVol = 1.0, debounceMs = 40) {
    if (this.muted || !this.ctx) return;

    const now = performance.now();
    if (this.lastPlayTime[name] && (now - this.lastPlayTime[name] < debounceMs)) {
      return; // prevent rapid buzzing / audio overlap
    }
    this.lastPlayTime[name] = now;

    if (this.ctx.state === 'suspended') {
      this.ctx.resume();
    }

    const buf = this.buffers.get(name);
    if (!buf) return;

    try {
      const src = this.ctx.createBufferSource();
      src.buffer = buf;

      const gain = this.ctx.createGain();
      gain.gain.value = Math.max(0, Math.min(1, this.volume * customVol));

      src.connect(gain);
      gain.connect(this.ctx.destination);

      src.start(0);
    } catch (e) {
      // Audio playback safety catch
    }
  }

  // Convenient methods matching eDEX-UI
  playScan() { this.play('scan', 0.55, 60); }
  playPanels() { this.play('panels', 0.75, 50); }
  playExpand() { this.play('expand', 0.80, 50); }
  playFolder() { this.play('folder', 0.90, 50); }
  playKey() { this.play('keyboard', 0.40, 25); }
  playGranted() { this.play('granted', 0.85, 80); }
  playDenied() { this.play('denied', 0.85, 80); }
  playTheme() { this.play('theme', 0.80, 200); }
  playAlarm() { this.play('alarm', 0.85, 200); }

  toggleMute() {
    this.muted = !this.muted;
    localStorage.setItem('solaris_dashboard_muted', String(this.muted));
    return this.muted;
  }
}

export const audioManager = new DashboardAudioManager();
window.solarisAudio = audioManager;
