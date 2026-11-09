const { spawn, exec } = require('child_process');
const { Client } = require('ssh2');
const os = require('os');

// Active sessions table: sessionId -> session object
const activeSessions = new Map();

/**
 * Start a local interactive shell session (Bash/Zsh)
 */
function startLocalSession(sessionId, cols = 80, rows = 24, onData, onExit) {
  closeSession(sessionId);

  const shellCmd = process.env.SHELL || '/bin/bash';
  // Fallback to interactive shell
  const child = spawn(shellCmd, ['-i'], {
    name: 'xterm-256color',
    cols: cols,
    rows: rows,
    cwd: process.env.HOME || '/home/venkatengineer',
    env: {
      ...process.env,
      TERM: 'xterm-256color',
      COLORTERM: 'truecolor',
      PS1: '\\[\\033[38;5;51m\\]solaris\\:\\[\\033[38;5;82m\\]\\w\\[\\033[0m\\]\\$ '
    }
  });

  child.stdout.on('data', (chunk) => {
    if (onData) onData(chunk.toString('utf8'));
  });

  child.stderr.on('data', (chunk) => {
    if (onData) onData(chunk.toString('utf8'));
  });

  child.on('close', (code) => {
    activeSessions.delete(sessionId);
    if (onExit) onExit(code);
  });

  child.on('error', (err) => {
    if (onData) onData(`\r\n\x1b[31m[Process Error]: ${err.message}\x1b[0m\r\n`);
  });

  activeSessions.set(sessionId, {
    type: 'local',
    child,
    write: (data) => {
      try {
        child.stdin.write(data);
      } catch (e) {}
    },
    resize: (newCols, newRows) => {
      // Best-effort resize for non-pty child process
      try {
        if (child.stdin && child.stdin.setWindowSize) {
          child.stdin.setWindowSize(newRows, newCols);
        }
      } catch (e) {}
    },
    close: () => {
      try {
        child.kill('SIGTERM');
      } catch (e) {}
    }
  });

  // Initial welcome greeting
  setTimeout(() => {
    if (onData) {
      onData(`\r\n\x1b[38;5;51m╔═══════════════════════════════════════════════════════════════╗\x1b[0m\r\n` +
             `\x1b[38;5;51m║  ORBITAL // CORE — LOCAL SUBSYSTEM TERMINAL EMULATOR v2.8.7   ║\x1b[0m\r\n` +
             `\x1b[38;5;51m║  Host: \x1b[38;5;82m${os.hostname()}\x1b[38;5;51m  · Kernel: \x1b[38;5;82m${os.release()}\x1b[38;5;51m  · Arch: \x1b[38;5;82m${os.arch()}\x1b[38;5;51m  ║\x1b[0m\r\n` +
             `\x1b[38;5;51m╚═══════════════════════════════════════════════════════════════╝\x1b[0m\r\n\r\n`);
    }
  }, 50);

  return { success: true, sessionId, type: 'local' };
}

/**
 * Start an authenticated remote SSH session
 */
function startSshSession(sessionId, config, cols = 80, rows = 24, onData, onExit, onError) {
  closeSession(sessionId);

  const conn = new Client();
  let streamRef = null;

  conn.on('ready', () => {
    if (onData) {
      onData(`\r\n\x1b[38;5;82m[SSH GATEWAY] Connection established to ${config.host}:${config.port || 22}\x1b[0m\r\n`);
      onData(`\x1b[38;5;51m[SSH GATEWAY] Authenticated as ${config.username}. Spawning remote shell...\x1b[0m\r\n\r\n`);
    }

    conn.shell({
      term: 'xterm-256color',
      cols: cols,
      rows: rows
    }, (err, stream) => {
      if (err) {
        if (onError) onError(err.message);
        conn.end();
        return;
      }

      streamRef = stream;

      stream.on('data', (chunk) => {
        if (onData) onData(chunk.toString('utf8'));
      });

      stream.stderr.on('data', (chunk) => {
        if (onData) onData(chunk.toString('utf8'));
      });

      stream.on('close', () => {
        conn.end();
        activeSessions.delete(sessionId);
        if (onExit) onExit();
      });
    });
  });

  conn.on('error', (err) => {
    activeSessions.delete(sessionId);
    if (onError) onError(err.message);
    if (onData) onData(`\r\n\x1b[31m[SSH Error]: ${err.message}\x1b[0m\r\n`);
  });

  conn.on('close', () => {
    activeSessions.delete(sessionId);
    if (onExit) onExit();
  });

  try {
    const connectOptions = {
      host: config.host,
      port: config.port ? parseInt(config.port, 10) : 22,
      username: config.username,
      readyTimeout: 10000,
      keepaliveInterval: 10000
    };

    if (config.privateKey) {
      connectOptions.privateKey = config.privateKey;
      if (config.passphrase) connectOptions.passphrase = config.passphrase;
    } else if (config.password) {
      connectOptions.password = config.password;
    }

    conn.connect(connectOptions);
  } catch (err) {
    if (onError) onError(err.message);
    return { success: false, error: err.message };
  }

  activeSessions.set(sessionId, {
    type: 'ssh',
    conn,
    write: (data) => {
      if (streamRef) {
        try { streamRef.write(data); } catch (e) {}
      }
    },
    resize: (newCols, newRows) => {
      if (streamRef && streamRef.setWindow) {
        try { streamRef.setWindow(newRows, newCols); } catch (e) {}
      }
    },
    close: () => {
      try {
        if (streamRef) streamRef.end();
        conn.end();
      } catch (e) {}
    }
  });

  return { success: true, sessionId, type: 'ssh' };
}

/**
 * Send keyboard input or command chunk to active session
 */
function sendData(sessionId, data) {
  const session = activeSessions.get(sessionId);
  if (session && session.write) {
    session.write(data);
    return true;
  }
  return false;
}

/**
 * Resize active terminal session
 */
function resizeSession(sessionId, cols, rows) {
  const session = activeSessions.get(sessionId);
  if (session && session.resize) {
    session.resize(cols, rows);
    return true;
  }
  return false;
}

/**
 * Cleanly close session
 */
function closeSession(sessionId) {
  const session = activeSessions.get(sessionId);
  if (session) {
    if (session.close) session.close();
    activeSessions.delete(sessionId);
    return true;
  }
  return false;
}

/**
 * Query real system services & Docker containers
 */
function getSystemServices() {
  return new Promise((resolve) => {
    // 1. Check core services status via systemctl
    const targetServices = [
      'sshd.service',
      'docker.service',
      'NetworkManager.service',
      'bluetooth.service',
      'pipewire.service',
      'systemd-resolved.service',
      'tailscaled.service',
      'wireplumber.service'
    ];

    exec('systemctl is-active ' + targetServices.join(' '), (err, stdout) => {
      const statuses = stdout ? stdout.trim().split('\n') : [];
      const serviceList = targetServices.map((name, i) => {
        const state = (statuses[i] || 'unknown').trim();
        return {
          name: name.replace('.service', ''),
          unit: name,
          active: state === 'active',
          status: state.toUpperCase()
        };
      });

      // 2. Query Docker containers if docker is active
      const dockerService = serviceList.find(s => s.name === 'docker');
      if (dockerService && dockerService.active) {
        exec('docker ps --format "{{.ID}}|{{.Names}}|{{.Image}}|{{.Status}}" 2>/dev/null', (dErr, dStdout) => {
          const containers = [];
          if (!dErr && dStdout) {
            dStdout.trim().split('\n').filter(Boolean).forEach(line => {
              const parts = line.split('|');
              containers.push({
                id: parts[0] || '',
                name: parts[1] || '',
                image: parts[2] || '',
                status: parts[3] || ''
              });
            });
          }
          resolve({
            services: serviceList,
            docker: {
              active: true,
              containers: containers
            }
          });
        });
      } else {
        resolve({
          services: serviceList,
          docker: {
            active: false,
            containers: []
          }
        });
      }
    });
  });
}

/**
 * Perform safe service operations
 */
function manageService(serviceName, action) {
  return new Promise((resolve) => {
    const allowedActions = ['start', 'stop', 'restart', 'status'];
    if (!allowedActions.includes(action)) {
      return resolve({ success: false, error: 'Action not permitted' });
    }

    const unit = serviceName.endsWith('.service') ? serviceName : `${serviceName}.service`;
    // We execute systemctl commands
    exec(`systemctl ${action} ${unit}`, (err, stdout, stderr) => {
      if (err) {
        resolve({ success: false, error: stderr || err.message });
      } else {
        resolve({ success: true, message: `Service ${unit} ${action} successful` });
      }
    });
  });
}

module.exports = {
  startLocalSession,
  startSshSession,
  sendData,
  resizeSession,
  closeSession,
  getSystemServices,
  manageService
};
