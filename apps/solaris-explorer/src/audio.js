// ==============================================================================
// 🔊 EDEX-UI HOLOGRAPHIC SOUND ENGINE — SOLARIS EXPLORER
// Authentic Tron / Extraterrestrial Sci-Fi Audio Feedback Subsystem
// ==============================================================================

class SoundEngine {
  constructor() {
    this.ctx = null;
    this.buffers = new Map();
    this.muted = false;
    this.volume = 0.65;

    this.soundFiles = {
      folder: '../assets/audio/folder.wav',
      scan: '../assets/audio/scan.wav',
      expand: '../assets/audio/expand.wav',
      panels: '../assets/audio/panels.wav',
      keyboard: '../assets/audio/keyboard.wav',
      stdin: '../assets/audio/stdin.wav',
      stdout: '../assets/audio/stdout.wav',
      granted: '../assets/audio/granted.wav',
      denied: '../assets/audio/denied.wav',
      alarm: '../assets/audio/alarm.wav',
      theme: '../assets/audio/theme.wav'
    };

    this.initAudioContext();
  }

  initAudioContext() {
    const AudioContextClass = window.AudioContext || window.webkitAudioContext;
    if (!AudioContextClass) return;

    this.ctx = new AudioContextClass();

    // Unlock audio context on user interaction if suspended
    const unlock = () => {
      if (this.ctx && this.ctx.state === 'suspended') {
        this.ctx.resume();
      }
      window.removeEventListener('click', unlock);
      window.removeEventListener('keydown', unlock);
    };
    window.addEventListener('click', unlock);
    window.addEventListener('keydown', unlock);

    // Preload audio buffers for instant playback
    for (const [key, filePath] of Object.entries(this.soundFiles)) {
      this.loadBuffer(key, filePath);
    }
  }

  async loadBuffer(name, path) {
    try {
      const response = await fetch(path);
      const arrayBuffer = await response.arrayBuffer();
      if (this.ctx) {
        const audioBuffer = await this.ctx.decodeAudioData(arrayBuffer);
        this.buffers.set(name, audioBuffer);
      }
    } catch (err) {
      // Non-blocking fallback
      console.warn(`[SoundEngine] Failed to decode audio: ${name}`, err);
    }
  }

  play(name, customVolume = 1.0) {
    if (this.muted || !this.ctx) return;
    if (this.ctx.state === 'suspended') {
      this.ctx.resume();
    }

    const buffer = this.buffers.get(name);
    if (!buffer) return;

    try {
      const source = this.ctx.createBufferSource();
      source.buffer = buffer;

      const gainNode = this.ctx.createGain();
      gainNode.gain.value = Math.max(0, Math.min(1, this.volume * customVolume));

      source.connect(gainNode);
      gainNode.connect(this.ctx.destination);

      source.start(0);
    } catch (err) {
      console.warn(`[SoundEngine] Playback error: ${name}`, err);
    }
  }

  // Convenient semantic aliases
  playFolder() { this.play('folder', 1.0); }
  playScan() { this.play('scan', 0.6); }
  playExpand() { this.play('expand', 0.85); }
  playPanels() { this.play('panels', 0.85); }
  playKey() { this.play('keyboard', 0.45); }
  playGranted() { this.play('granted', 0.9); }
  playDenied() { this.play('denied', 0.9); }
  playTheme() { this.play('theme', 0.75); }
  playAlarm() { this.play('alarm', 0.9); }

  toggleMute() {
    this.muted = !this.muted;
    return this.muted;
  }
}

// Export singleton instance
const sfx = new SoundEngine();
if (typeof module !== 'undefined' && module.exports) {
  module.exports = { sfx };
}
