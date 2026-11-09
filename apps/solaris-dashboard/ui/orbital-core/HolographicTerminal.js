// ==============================================================================
// 💻 HOLOGRAPHIC TERMINAL — State C: Floating xterm.js Interactive Console
// ==============================================================================

export class HolographicTerminal {
  constructor(container, options = {}) {
    this.parentContainer = container;
    this.options = options;
    this.onCloseCallback = options.onClose || null;

    this.isOpen = false;
    this.sessionId = 'solaris-term-' + Math.floor(Math.random() * 10000);
    this.sessionType = 'local'; // 'local' | 'ssh' | 'demo'
    this.connectionState = 'disconnected'; // 'connected' | 'connecting' | 'disconnected' | 'error'

    this.term = null;
    this.fitAddon = null;

    this.createDOM();
    this.initXterm();
    this.bindEvents();
  }

  createDOM() {
    // 1. Container Wrapper
    this.el = document.createElement('div');
    this.el.className = 'holo-terminal-container';
    this.el.id = 'holo-terminal-container';
    this.el.style.display = 'none';

    // 2. Targeting Reticle Frame (cinematic holographic projection arms)
    this.reticle = document.createElement('div');
    this.reticle.className = 'holo-projection-reticle';
    this.reticle.innerHTML = `
      <div class="reticle-bracket tl"></div>
      <div class="reticle-bracket tr"></div>
      <div class="reticle-bracket bl"></div>
      <div class="reticle-bracket br"></div>
      <div class="projection-beam left"></div>
      <div class="projection-beam right"></div>
    `;
    this.el.appendChild(this.reticle);

    // 3. Floating Terminal Window Glass
    this.windowEl = document.createElement('div');
    this.windowEl.className = 'holo-terminal-window';
    this.windowEl.innerHTML = `
      <!-- TOP HUD TITLEBAR -->
      <div class="holo-term-header">
        <div class="term-hdr-left">
          <span class="term-status-dot green" id="term-status-dot"></span>
          <span class="term-status-tag" id="term-status-text">INITIALIZING SUBSYSTEM...</span>
        </div>
        <div class="term-hdr-center">
          <span class="term-host-badge" id="term-host-badge">HOST: LOCALHOST</span>
          <span class="term-session-badge" id="term-session-badge">[ LOCAL PTY ]</span>
        </div>
        <div class="term-hdr-actions">
          <button class="term-action-btn" id="btn-term-ssh" title="Configure Remote SSH Connection">
            <svg viewBox="0 0 24 24" width="13" height="13" stroke="currentColor" stroke-width="2" fill="none"><rect x="2" y="2" width="20" height="8" rx="2" ry="2"></rect><rect x="2" y="14" width="20" height="8" rx="2" ry="2"></rect><line x1="6" y1="6" x2="6.01" y2="6"></line><line x1="6" y1="18" x2="6.01" y2="18"></line></svg>
            <span>SSH</span>
          </button>
          <button class="term-action-btn" id="btn-term-reconnect" title="Reconnect Shell Session">
            <svg viewBox="0 0 24 24" width="13" height="13" stroke="currentColor" stroke-width="2" fill="none"><polyline points="1 4 1 10 7 10"></polyline><path d="M3.51 15a9 9 0 1 0 2.13-9.36L1 10"></path></svg>
          </button>
          <button class="term-action-btn danger" id="btn-term-close" title="Close Terminal (Esc)">
            <svg viewBox="0 0 24 24" width="14" height="14" stroke="currentColor" stroke-width="2" fill="none"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg>
          </button>
        </div>
      </div>

      <!-- TERMINAL BODY WITH SCANLINE SHADER -->
      <div class="holo-term-body">
        <div class="term-scanlines-layer"></div>
        <div class="xterm-mount-point" id="xterm-mount-point"></div>
      </div>

      <!-- TERMINAL FOOTER HUD -->
      <div class="holo-term-footer">
        <span class="term-footer-meta" id="term-footer-meta">ENC: SECURE LOCAL SOCKET · BUFFER: 1000 LINES</span>
        <span class="term-footer-hint">PRESS ESC TO RETURN TO SERVER CORE</span>
      </div>

      <!-- SSH CONFIGURATION MODAL (Hidden by default) -->
      <div class="ssh-config-modal" id="ssh-config-modal" style="display: none;">
        <div class="ssh-modal-box">
          <div class="ssh-modal-header">
            <span>AUTHENTICATED SSH GATEWAY</span>
            <button class="modal-close-btn" id="btn-ssh-modal-close">✕</button>
          </div>
          <div class="ssh-modal-body">
            <div class="ssh-input-group">
              <label>HOST / IP</label>
              <input type="text" id="ssh-input-host" placeholder="e.g. 192.168.1.100 or server.orbital.net" value="127.0.0.1" />
            </div>
            <div class="ssh-input-row">
              <div class="ssh-input-group">
                <label>PORT</label>
                <input type="number" id="ssh-input-port" value="22" />
              </div>
              <div class="ssh-input-group">
                <label>USERNAME</label>
                <input type="text" id="ssh-input-user" placeholder="root / admin" value="venkatengineer" />
              </div>
            </div>
            <div class="ssh-input-group">
              <label>AUTHENTICATION MODE</label>
              <select id="ssh-input-auth-type">
                <option value="password">Password</option>
                <option value="key">Private Key (~/.ssh/id_ed25519)</option>
              </select>
            </div>
            <div class="ssh-input-group" id="ssh-pwd-group">
              <label>PASSWORD / PASSPHRASE</label>
              <input type="password" id="ssh-input-pwd" placeholder="Enter password (never stored)" />
            </div>
            <div class="ssh-input-group" id="ssh-key-group" style="display: none;">
              <label>PRIVATE KEY (PEM / OPENSSH)</label>
              <textarea id="ssh-input-key" rows="3" placeholder="Paste OpenSSH private key..."></textarea>
            </div>
            <div class="ssh-status-feedback" id="ssh-status-feedback"></div>
          </div>
          <div class="ssh-modal-footer">
            <button class="ssh-btn secondary" id="btn-ssh-local-fallback">USE LOCAL SHELL</button>
            <button class="ssh-btn primary" id="btn-ssh-connect-submit">CONNECT GATEWAY</button>
          </div>
        </div>
      </div>
    `;

    this.el.appendChild(this.windowEl);
    this.parentContainer.appendChild(this.el);
  }

  initXterm() {
    const mountEl = this.el.querySelector('#xterm-mount-point');
    if (!mountEl) return;

    if (window.Terminal) {
      this.term = new window.Terminal({
        cursorBlink: true,
        cursorStyle: 'block',
        fontSize: 13,
        fontFamily: '"JetBrains Mono", "Courier New", monospace',
        lineHeight: 1.35,
        theme: {
          background: '#040b14f0',
          foreground: '#00f0ff',
          cursor: '#00ff66',
          cursorAccent: '#040b14',
          selectionBackground: 'rgba(0, 255, 102, 0.3)',
          black: '#08121e',
          red: '#ff5555',
          green: '#00ff66',
          yellow: '#f1fa8c',
          blue: '#00f0ff',
          magenta: '#bd93f9',
          cyan: '#8be9fd',
          white: '#f8f8f2',
          brightBlack: '#6272a4',
          brightRed: '#ff6e6e',
          brightGreen: '#69ff94',
          brightYellow: '#ffffa5',
          brightBlue: '#d6acff',
          brightMagenta: '#ff92df',
          brightCyan: '#a4ffff',
          brightWhite: '#ffffff'
        },
        allowTransparency: true,
        scrollback: 2000
      });

      if (window.FitAddon) {
        this.fitAddon = new window.FitAddon.FitAddon();
        this.term.loadAddon(this.fitAddon);
      }

      this.term.open(mountEl);

      // Attach custom key event handler for clipboard copy & paste
      this.term.attachCustomKeyEventHandler((event) => {
        if (event.type === 'keydown') {
          // Copy: Ctrl+Shift+C
          if ((event.ctrlKey || event.metaKey) && event.shiftKey && event.key.toLowerCase() === 'c') {
            const selection = this.term.getSelection();
            if (selection && navigator.clipboard) {
              navigator.clipboard.writeText(selection);
              return false;
            }
          }
          // Paste: Ctrl+Shift+V or Ctrl+V
          if ((event.ctrlKey || event.metaKey) && (event.key.toLowerCase() === 'v' || (event.shiftKey && event.key.toLowerCase() === 'v'))) {
            if (navigator.clipboard) {
              navigator.clipboard.readText().then(text => {
                if (text) {
                  if (this.sessionType === 'demo') {
                    this.handleDemoInput(text);
                  } else if (window.solarisAPI && window.solarisAPI.sendTerminalData) {
                    window.solarisAPI.sendTerminalData(this.sessionId, text);
                  }
                }
              }).catch(() => {});
              return false;
            }
          }
        }
        return true;
      });

      // Handle terminal keystroke input
      this.term.onData(data => {
        if (this.sessionType === 'demo') {
          this.handleDemoInput(data);
        } else if (window.solarisAPI && window.solarisAPI.sendTerminalData) {
          window.solarisAPI.sendTerminalData(this.sessionId, data);
        }
      });

    } else {
      mountEl.innerHTML = `<div style="color: #00f0ff; padding: 20px; font-family: monospace;">Loading xterm.js subsystem...</div>`;
    }
  }

  bindEvents() {
    // Escape key closes terminal
    window.addEventListener('keydown', (e) => {
      if (this.isOpen && e.key === 'Escape') {
        const sshModal = this.el.querySelector('#ssh-config-modal');
        if (sshModal && sshModal.style.display !== 'none') {
          sshModal.style.display = 'none';
        } else {
          this.handleClose();
        }
      }
    });

    // Clicking backdrop / projection reticle outside the terminal window closes it
    this.el.addEventListener('click', (e) => {
      if (this.isOpen && !this.windowEl.contains(e.target) && !e.target.closest('#ssh-config-modal')) {
        this.handleClose();
      }
    });

    // Close button
    const closeBtn = this.el.querySelector('#btn-term-close');
    if (closeBtn) {
      closeBtn.addEventListener('click', () => this.handleClose());
    }

    // Reconnect button
    const reconnectBtn = this.el.querySelector('#btn-term-reconnect');
    if (reconnectBtn) {
      reconnectBtn.addEventListener('click', () => this.reconnect());
    }

    // SSH Modal buttons
    const sshBtn = this.el.querySelector('#btn-term-ssh');
    const sshModal = this.el.querySelector('#ssh-config-modal');
    const sshCloseBtn = this.el.querySelector('#btn-ssh-modal-close');

    if (sshBtn && sshModal) {
      sshBtn.addEventListener('click', () => {
        sshModal.style.display = 'flex';
      });
    }

    if (sshCloseBtn && sshModal) {
      sshCloseBtn.addEventListener('click', () => {
        sshModal.style.display = 'none';
      });
    }

    // Auth type change
    const authTypeSelect = this.el.querySelector('#ssh-input-auth-type');
    const pwdGroup = this.el.querySelector('#ssh-pwd-group');
    const keyGroup = this.el.querySelector('#ssh-key-group');
    if (authTypeSelect) {
      authTypeSelect.addEventListener('change', () => {
        if (authTypeSelect.value === 'key') {
          pwdGroup.style.display = 'none';
          keyGroup.style.display = 'block';
        } else {
          pwdGroup.style.display = 'block';
          keyGroup.style.display = 'none';
        }
      });
    }

    // SSH Connect Submit
    const connectSubmit = this.el.querySelector('#btn-ssh-connect-submit');
    if (connectSubmit) {
      connectSubmit.addEventListener('click', () => this.connectSSH());
    }

    // Local Shell fallback button
    const localFallback = this.el.querySelector('#btn-ssh-local-fallback');
    if (localFallback) {
      localFallback.addEventListener('click', () => {
        if (sshModal) sshModal.style.display = 'none';
        this.startLocalSession();
      });
    }

    // Listen to IPC terminal streams if available
    if (window.solarisAPI) {
      window.solarisAPI.onTerminalData(payload => {
        if (this.term && payload.data) {
          this.term.write(payload.data);
        }
      });

      window.solarisAPI.onTerminalExit(() => {
        this.setConnectionStatus('disconnected', 'SESSION TERMINATED');
      });

      window.solarisAPI.onTerminalError(payload => {
        this.setConnectionStatus('error', `ERROR: ${payload.error}`);
        if (this.term) {
          this.term.write(`\r\n\x1b[31m[GATEWAY ERROR]: ${payload.error}\x1b[0m\r\n`);
        }
      });
    }

    // Window resize handler
    window.addEventListener('resize', () => {
      if (this.isOpen && this.fitAddon) {
        this.fitAddon.fit();
        if (window.solarisAPI && this.term) {
          window.solarisAPI.resizeTerminal(this.sessionId, this.term.cols, this.term.rows);
        }
      }
    });
  }

  open() {
    this.isOpen = true;
    this.el.style.display = 'flex';

    // Focus window in Electron so user can type
    if (window.solarisAPI && window.solarisAPI.setWindowFocusable) {
      window.solarisAPI.setWindowFocusable(true);
    }

    const gsap = window.gsap;
    if (gsap) {
      // Reticle targeting animation
      gsap.fromTo(this.reticle,
        { opacity: 0, scale: 0.7 },
        { opacity: 1, scale: 1.0, duration: 0.35, ease: 'power2.out' }
      );

      // Terminal window unfolding
      gsap.fromTo(this.windowEl,
        { opacity: 0, scaleY: 0.05, scaleX: 0.4 },
        {
          opacity: 1,
          scaleY: 1.0,
          scaleX: 1.0,
          duration: 0.65,
          ease: 'power3.out',
          onComplete: () => {
            if (this.fitAddon) this.fitAddon.fit();
            if (this.term) this.term.focus();
          }
        }
      );
    } else {
      this.reticle.style.opacity = '1';
      this.windowEl.style.opacity = '1';
      if (this.fitAddon) this.fitAddon.fit();
      if (this.term) this.term.focus();
    }

    // Initialize session if first time or disconnected
    if (this.connectionState === 'disconnected') {
      this.startLocalSession();
    }
  }

  close() {
    if (!this.isOpen) return;
    this.isOpen = false;

    // Relinquish window focus so user can type in desktop terminal (kitty/alacritty) without interference
    if (window.solarisAPI && window.solarisAPI.setWindowFocusable) {
      window.solarisAPI.setWindowFocusable(false);
    }

    const gsap = window.gsap;
    if (gsap) {
      gsap.to(this.windowEl, {
        opacity: 0,
        scaleY: 0.1,
        scaleX: 0.4,
        duration: 0.4,
        ease: 'power2.in',
        onComplete: () => {
          this.el.style.display = 'none';
        }
      });
      gsap.to(this.reticle, {
        opacity: 0,
        duration: 0.3
      });
    } else {
      this.el.style.display = 'none';
    }
  }

  handleClose() {
    if (this.onCloseCallback) {
      this.onCloseCallback();
    } else {
      this.close();
    }
  }

  startLocalSession() {
    this.sessionType = 'local';
    this.setConnectionStatus('connecting', 'CONNECTING LOCAL PTY...');

    const cols = this.term ? this.term.cols : 80;
    const rows = this.term ? this.term.rows : 24;

    if (window.solarisAPI && window.solarisAPI.startLocalTerminal) {
      window.solarisAPI.startLocalTerminal(this.sessionId, cols, rows).then(res => {
        if (res && res.success) {
          this.setConnectionStatus('connected', 'CONNECTED // LOCAL HOST INTERFACE');
          this.updateSessionBadge('LOCAL PTY', 'solaris (local)');
        } else {
          this.fallbackToDemo();
        }
      }).catch(() => {
        this.fallbackToDemo();
      });
    } else {
      this.fallbackToDemo();
    }
  }

  async connectSSH() {
    const host = this.el.querySelector('#ssh-input-host').value.trim();
    const port = this.el.querySelector('#ssh-input-port').value.trim();
    const username = this.el.querySelector('#ssh-input-user').value.trim();
    const authType = this.el.querySelector('#ssh-input-auth-type').value;
    const password = this.el.querySelector('#ssh-input-pwd').value;
    const key = this.el.querySelector('#ssh-input-key').value;
    const feedback = this.el.querySelector('#ssh-status-feedback');

    if (!host || !username) {
      if (feedback) feedback.textContent = 'Please provide host and username.';
      return;
    }

    if (feedback) feedback.textContent = 'Negotiating cryptographic handshake...';

    const config = { host, port, username };
    if (authType === 'key') config.privateKey = key;
    else config.password = password;

    const cols = this.term ? this.term.cols : 80;
    const rows = this.term ? this.term.rows : 24;

    this.sessionType = 'ssh';
    this.setConnectionStatus('connecting', `DIALING ${host}:${port}...`);

    if (window.solarisAPI && window.solarisAPI.startSshTerminal) {
      const res = await window.solarisAPI.startSshTerminal(this.sessionId, config, cols, rows);
      if (res && res.success) {
        this.setConnectionStatus('connected', `SSH ONLINE // ${username}@${host}`);
        this.updateSessionBadge(`SSH: ${host}`, `${username}@${host}`);
        const modal = this.el.querySelector('#ssh-config-modal');
        if (modal) modal.style.display = 'none';
      } else {
        if (feedback) feedback.textContent = `SSH Error: ${res ? res.error : 'Connection failed'}`;
        this.setConnectionStatus('error', 'SSH HANDSHAKE FAILED');
      }
    } else {
      if (feedback) feedback.textContent = 'Electron backend SSH gateway unavailable.';
      this.fallbackToDemo();
    }
  }

  fallbackToDemo() {
    this.sessionType = 'demo';
    this.setConnectionStatus('connected', 'OFFLINE DEMO SHELL // BACKEND UNCONFIGURED');
    this.updateSessionBadge('DEMO MODE', 'solaris (simulated)');

    if (this.term) {
      this.term.reset();
      this.term.writeln('\x1b[38;5;51m╔═══════════════════════════════════════════════════════════════╗\x1b[0m');
      this.term.writeln('\x1b[38;5;51m║  ORBITAL // CORE — DEMO INTERACTIVE DIAGNOSTIC SHELL          ║\x1b[0m');
      this.term.writeln('\x1b[38;5;51m║  Status: Backend daemon disconnected. Demo emulator active.   ║\x1b[0m');
      this.term.writeln('\x1b[38;5;51m║  Type "help", "telemetry", "status", or "clear"               ║\x1b[0m');
      this.term.writeln('\x1b[38;5;51m╚═══════════════════════════════════════════════════════════════╝\x1b[0m');
      this.term.write('\r\nsolaris:demo$ ');
      this.demoLine = '';
    }
  }

  handleDemoInput(char) {
    if (!this.term) return;

    if (char === '\r') { // Enter
      this.term.write('\r\n');
      const cmd = (this.demoLine || '').trim().toLowerCase();
      this.demoLine = '';

      if (cmd === 'help') {
        this.term.writeln('Available demo commands:');
        this.term.writeln('  status      Display server core operational status');
        this.term.writeln('  telemetry   Print live telemetry readings');
        this.term.writeln('  ping        Test link ping latency');
        this.term.writeln('  ssh         Show SSH gateway configuration guidance');
        this.term.writeln('  clear       Clear terminal screen');
      } else if (cmd === 'status') {
        this.term.writeln('\x1b[38;5;82mCORE STATUS: NOMINAL (STATION 02 // ENGINEERING)\x1b[0m');
        this.term.writeln('CHASSIS: AEROSPACE CARBON-ALLOY BLADE');
        this.term.writeln('FIRMWARE: SOLARIS-CORE-V2.8.7');
      } else if (cmd === 'telemetry') {
        this.term.writeln('CPU LOAD: 18% · TEMP: 46°C · RAM: 5.1/15.5 GB (33%)');
        this.term.writeln('GPU: NVIDIA RTX 5050 · FAN: 2400 RPM');
      } else if (cmd === 'ping') {
        this.term.writeln('64 bytes from 127.0.0.1: icmp_seq=1 ttl=64 time=0.042 ms');
      } else if (cmd === 'clear') {
        this.term.clear();
      } else if (cmd !== '') {
        this.term.writeln(`command not found: ${cmd}. Type "help" for list.`);
      }
      this.term.write('solaris:demo$ ');
    } else if (char === '\u007F') { // Backspace
      if (this.demoLine && this.demoLine.length > 0) {
        this.demoLine = this.demoLine.slice(0, -1);
        this.term.write('\b \b');
      }
    } else {
      this.demoLine = (this.demoLine || '') + char;
      this.term.write(char);
    }
  }

  setConnectionStatus(state, message) {
    this.connectionState = state;
    const dot = this.el.querySelector('#term-status-dot');
    const text = this.el.querySelector('#term-status-text');

    if (dot) {
      dot.className = `term-status-dot ${state === 'connected' ? 'green' : state === 'connecting' ? 'cyan' : 'red'}`;
    }
    if (text) {
      text.textContent = message;
    }
  }

  updateSessionBadge(typeStr, hostStr) {
    const sessionBadge = this.el.querySelector('#term-session-badge');
    const hostBadge = this.el.querySelector('#term-host-badge');
    if (sessionBadge) sessionBadge.textContent = `[ ${typeStr} ]`;
    if (hostBadge) hostBadge.textContent = `HOST: ${hostStr.toUpperCase()}`;
  }

  reconnect() {
    if (this.term) this.term.reset();
    if (this.sessionType === 'ssh') {
      this.connectSSH();
    } else {
      this.startLocalSession();
    }
  }

  focus() {
    if (this.term) {
      this.term.focus();
    }
  }

  dispose() {
    this.close();
    if (this.term) {
      try { this.term.dispose(); } catch (e) {}
      this.term = null;
    }
    if (this.el && this.el.parentElement) {
      this.el.parentElement.removeChild(this.el);
    }
  }
}
