const { app, BrowserWindow, globalShortcut, ipcMain, Tray, Menu, nativeImage, screen, shell } = require('electron');
const path = require('path');
const http = require('http');
const fs = require('fs');
const os = require('os');
const { spawn, execSync } = require('child_process');

// =========================================================================
// Spotify Background Audio Engine Lifecycle Manager
// =========================================================================
let spotifyProcess = null;
let spawnedSpotifyByAetheris = false;

function isSpotifyRunning() {
  try {
    const res = execSync('pgrep -x spotify || pgrep -f "spotify/spotify"', { encoding: 'utf8' }).trim();
    return Boolean(res);
  } catch (e) {
    return false;
  }
}

function startSpotifyDaemon() {
  if (isSpotifyRunning()) {
    console.log('[AETHERIS-SPOTIFY] Spotify background daemon is already running.');
    return;
  }

  console.log('[AETHERIS-SPOTIFY] Launching background Spotify audio daemon via spotify-launcher...');
  try {
    spotifyProcess = spawn('spotify-launcher', [], {
      detached: true,
      stdio: 'ignore',
      env: {
        ...process.env,
      },
    });

    spotifyProcess.unref();
    spawnedSpotifyByAetheris = true;
    console.log(`[AETHERIS-SPOTIFY] Spotify process spawned successfully (PID: ${spotifyProcess.pid}).`);

    spotifyProcess.on('exit', (code, signal) => {
      console.log(`[AETHERIS-SPOTIFY] Spotify process exited (code: ${code}, signal: ${signal})`);
      spotifyProcess = null;
    });
  } catch (err) {
    console.warn('[AETHERIS-SPOTIFY] Failed to spawn spotify-launcher:', err.message);
  }
}

function ensureSpotifyRunning() {
  if (!isSpotifyRunning()) {
    startSpotifyDaemon();
  }
}

function stopSpotifyDaemon() {
  console.log('[AETHERIS-SPOTIFY] Aetheris closing -- terminating background Spotify instance...');
  try {
    if (spotifyProcess && !spotifyProcess.killed) {
      try { process.kill(-spotifyProcess.pid, 'SIGKILL'); } catch (e) {}
      try { spotifyProcess.kill('SIGKILL'); } catch (e) {}
      spotifyProcess = null;
    }
  } catch (e) {}

  try {
    execSync('pkill -9 -f "/share/spotify/spotify" 2>/dev/null || pkill -9 -x spotify 2>/dev/null || true', { stdio: 'ignore' });
    console.log('[AETHERIS-SPOTIFY] Spotify instances closed.');
  } catch (e) {
    console.warn('[AETHERIS-SPOTIFY] Note while closing Spotify:', e.message);
  }
}

// Configure application name for Linux desktop integration and Wayland app_id
app.setName('aetheris');
app.setAppUserModelId('aetheris');

// Detect if launched automatically at login via --startup flag
const isStartup = process.argv.includes('--startup');

// Enforce single instance lock so duplicate processes don't conflict on ports or shortcuts
const gotTheLock = app.requestSingleInstanceLock();
if (!gotTheLock) {
  console.log('[LOG] Another instance of Aetheris OS is already running. Forwarding focus and exiting duplicate...');
  app.quit();
  process.exit(0);
}

// Set persistent user data path
const sessionUserData = path.join(app.getPath('appData'), 'aetheris_alien_music_overlay_data');
app.setPath('userData', sessionUserData);

console.log('====================================================');
console.log('   AETHERIS OS ELECTRON DESKTOP OVERLAY LAUNCHER    ');
console.log(`   Startup mode: ${isStartup ? 'SILENT BACKGROUND (SLEEP)' : 'NORMAL SHOW'}`);
console.log('====================================================');

let mainWindow = null;
let tray = null;
let oauthServer = null;

function toggleOverlayVisibility() {
  if (!mainWindow) return;
  if (mainWindow.isVisible()) {
    console.log('[LOG] Hiding overlay window (entering SLEEP MODE)...');
    mainWindow.hide();
    try { mainWindow.webContents.send('window-visibility-changed', false); } catch(e) {}
  } else {
    console.log('[LOG] Showing & focusing overlay window (WAKE SEQUENCE)...');
    ensureSpotifyRunning();
    mainWindow.show();
    mainWindow.focus();
    mainWindow.setAlwaysOnTop(true, 'screen-saver');
    try { mainWindow.webContents.send('window-visibility-changed', true); } catch(e) {}
  }
}

app.on('second-instance', (event, commandLine) => {
  console.log('[LOG] Second instance launched with commandLine:', commandLine);
  if (mainWindow) {
    if (commandLine.some(arg => arg.includes('--toggle'))) {
      toggleOverlayVisibility();
    } else if (commandLine.some(arg => arg.includes('--hide'))) {
      mainWindow.hide();
      try { mainWindow.webContents.send('window-visibility-changed', false); } catch(e) {}
    } else if (commandLine.some(arg => arg.includes('--show'))) {
      if (mainWindow.isMinimized()) mainWindow.restore();
      mainWindow.show();
      mainWindow.focus();
      mainWindow.setAlwaysOnTop(true, 'screen-saver');
      try { mainWindow.webContents.send('window-visibility-changed', true); } catch(e) {}
    } else {
      if (mainWindow.isMinimized()) mainWindow.restore();
      mainWindow.show();
      mainWindow.focus();
      mainWindow.setAlwaysOnTop(true, 'screen-saver');
      try { mainWindow.webContents.send('window-visibility-changed', true); } catch(e) {}
    }
  }
});

function startOAuthServer(portsToTry = [3000, 3001, 3002, 3003]) {
  if (oauthServer) return;

  const tryPort = (portIndex) => {
    if (portIndex >= portsToTry.length) {
      console.warn('[WARN] OAuth server: all ports in use, loopback redirect unavailable.');
      return;
    }
    const port = portsToTry[portIndex];

    try {
      oauthServer = http.createServer((req, res) => {
        const reqUrl = new URL(req.url, `http://127.0.0.1:${port}`);
        const code = reqUrl.searchParams.get('code');

        if (code && mainWindow) {
          console.log('[RUNTIME LOG] Electron received auth code:', code);
          console.log('[RUNTIME LOG] IPC event sent: spotify-auth-code');
          mainWindow.webContents.send('spotify-auth-code', code);
          res.writeHead(200, {
            'Content-Type': 'text/html',
            'Access-Control-Allow-Origin': '*',
          });
          res.end(`
            <!DOCTYPE html>
            <html>
              <head>
                <title>Aetheris OS - Spotify Connected</title>
                <style>
                  body { background: #040d12; color: #00ffaa; font-family: monospace; text-align: center; padding-top: 40px; }
                  .card { border: 1.5px solid #00ffaa; border-radius: 16px; padding: 25px; display: inline-block; box-shadow: 0 0 25px #00ffaa; background: #061822; }
                  h2 { margin: 0; font-size: 24px; letter-spacing: 2px; }
                  p { color: #00e5ff; }
                </style>
              </head>
              <body>
                <div class="card">
                  <h2>🛸 AETHERIS OS</h2>
                  <h4 style="color:#ffffff;">SPOTIFY AUTHORIZATION SUCCESSFUL!</h4>
                  <p>Syncing live music stream with HUD overlay...</p>
                </div>
                <script>
                  if (window.opener) {
                    try { window.opener.postMessage({ type: 'SPOTIFY_AUTH_CODE', code: ${JSON.stringify(code)} }, '*'); } catch(e) {}
                  }
                  setTimeout(() => window.close(), 1000);
                </script>
              </body>
            </html>
          `);
          return;
        }

        res.writeHead(200, { 'Content-Type': 'text/html', 'Access-Control-Allow-Origin': '*' });
        res.end('Aetheris OS OAuth Receiver Active');
      });

      oauthServer.listen(port, '127.0.0.1', () => {
        console.log(`[LOG] Spotify OAuth Loopback Receiver active at http://127.0.0.1:${port}/`);
      });

      oauthServer.on('error', (err) => {
        if (err.code === 'EADDRINUSE') {
          console.warn(`[WARN] Port ${port} is in use, trying next port...`);
          oauthServer = null;
          tryPort(portIndex + 1);
        } else {
          console.warn('[WARN] OAuth server note:', err.message);
        }
      });
    } catch (e) {
      console.warn('[WARN] Could not start OAuth loopback server:', e);
    }
  };

  tryPort(0);
}

async function createWindow() {
  console.log('[LOG] Creating Full Desktop Screen BrowserWindow with Transparency...');
  const primaryDisplay = screen.getPrimaryDisplay();
  const { width: screenWidth, height: screenHeight } = primaryDisplay.workAreaSize;
  const iconPath = path.join(__dirname, 'icon.png');

  mainWindow = new BrowserWindow({
    title: 'Aetheris - Alien Holographic Desktop HUD',
    width: screenWidth,
    height: screenHeight,
    x: 0,
    y: 0,
    frame: false,
    transparent: true,
    alwaysOnTop: true,
    resizable: true,
    hasShadow: false,
    show: !isStartup,
    backgroundColor: '#00000000',
    icon: iconPath,
    webPreferences: {
      preload: path.join(__dirname, 'preload.cjs'),
      nodeIntegration: false,
      contextIsolation: true,
      webSecurity: false,
    },
  });

  mainWindow.setAlwaysOnTop(true, 'screen-saver');

  mainWindow.on('show', () => {
    try { mainWindow.webContents.send('window-visibility-changed', true); } catch (e) {}
  });

  mainWindow.on('hide', () => {
    try { mainWindow.webContents.send('window-visibility-changed', false); } catch (e) {}
  });

  mainWindow.webContents.on('console-message', (_event, _level, message) => {
    if (message.includes('Third-party cookie') || message.includes('Electron Security Warning')) return;
    console.log('[RENDERER LOG]', message);
  });

  // Configure popup handler so Spotify Auth popup floats ON TOP of screen-saver overlay
  mainWindow.webContents.setWindowOpenHandler(({ url }) => {
    console.log('[LOG] Intercepted window.open request for URL:', url);
    return {
      action: 'allow',
      overrideBrowserWindowOptions: {
        alwaysOnTop: true,
        autoHideMenuBar: true,
        center: true,
        width: 550,
        height: 750,
        resizable: true,
        frame: true,
        title: 'Spotify Authorization',
      },
    };
  });

  mainWindow.webContents.on('did-create-window', (childWindow) => {
    console.log('[LOG] Child popup window created! Forcing to front above overlay...');
    childWindow.setAlwaysOnTop(true, 'screen-saver', 1);
    childWindow.show();
    childWindow.focus();

    if (mainWindow) {
      mainWindow.setIgnoreMouseEvents(false);
    }
  });

  // Clear Chromium disk cache so local dist bundle updates always load fresh code!
  try {
    mainWindow.webContents.session.clearCache();
  } catch (e) {}

  const indexPath = path.join(__dirname, '../dist/index.html');
  console.log('[LOG] Loading fresh production build at:', indexPath);
  mainWindow.loadFile(indexPath, { search: `v=${Date.now()}` });

  if (!isStartup) {
    mainWindow.show();
    mainWindow.focus();
  } else {
    console.log('[LOG] Startup launch detected -- overlay initialized SILENTLY in background sleep mode.');
  }

  mainWindow.webContents.on('did-finish-load', () => {
    console.log('[LOG] Window WebContents finished loading successfully!');
    try {
      mainWindow.webContents.send('window-visibility-changed', !isStartup && mainWindow.isVisible());
    } catch (e) {}
  });

  mainWindow.webContents.on('did-fail-load', (event, errorCode, errorDescription) => {
    console.error(`[ERROR] Window failed to load [${errorCode}]: ${errorDescription}`);
  });

  globalShortcut.unregisterAll();

  // Register common fallback shortcuts
  const keysToTry = [
    { key: 'CommandOrControl+Alt+M', name: 'Ctrl+Alt+M' },
    { key: 'CommandOrControl+Shift+M', name: 'Ctrl+Shift+M' },
    { key: 'Alt+M', name: 'Alt+M' },
    { key: 'F9', name: 'F9' },
    { key: 'F8', name: 'F8' },
    { key: 'CommandOrControl+Alt+Space', name: 'Ctrl+Alt+Space' },
    { key: 'Alt+Shift+M', name: 'Alt+Shift+M' },
  ];

  let registeredCount = 0;
  keysToTry.forEach(({ key, name }) => {
    try {
      const ok = globalShortcut.register(key, () => toggleOverlayVisibility());
      if (ok) {
        console.log(`[LOG] Registered global shortcut: ${name} (${key}) ✓`);
        registeredCount++;
      }
    } catch (e) {
      console.warn(`[WARN] Error registering ${name}:`, e.message);
    }
  });

  try {
    const trayIconPath = path.join(__dirname, 'tray-icon.png');
    tray = new Tray(trayIconPath);
    const contextMenu = Menu.buildFromTemplate([
      { label: '🛸 Aetheris HUD Overlay v1.0', enabled: false },
      { type: 'separator' },
      { label: 'Toggle Overlay (Mod+Shift+M / Alt+M)', click: () => toggleOverlayVisibility() },
      { label: 'Show Overlay', click: () => { mainWindow && mainWindow.show(); mainWindow && mainWindow.focus(); } },
      { label: 'Hide Overlay', click: () => { mainWindow && mainWindow.hide(); } },
      { label: 'Toggle Fullscreen (F11)', click: () => mainWindow && mainWindow.setFullScreen(!mainWindow.isFullScreen()) },
      { label: 'Toggle DevTools', click: () => mainWindow && mainWindow.webContents.toggleDevTools() },
      { type: 'separator' },
      { label: 'Exit Aetheris', click: () => app.quit() },
    ]);
    tray.setToolTip('Aetheris OS Music Overlay');
    tray.setContextMenu(contextMenu);
    tray.on('click', () => toggleOverlayVisibility());
    console.log('[LOG] System Tray created successfully.');
  } catch (err) {
    console.warn('[WARN] System tray note:', err);
  }

  mainWindow.on('closed', () => {
    console.log('[LOG] Window closed.');
    mainWindow = null;
  });
}

app.whenReady().then(() => {
  console.log('[LOG] Electron app is ready.');
  startSpotifyDaemon();
  startOAuthServer();
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', () => {
  console.log('[LOG] All windows closed.');
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('before-quit', () => {
  stopSpotifyDaemon();
});

app.on('will-quit', () => {
  console.log('[LOG] App quitting. Unregistering global shortcuts.');
  globalShortcut.unregisterAll();
  if (oauthServer) oauthServer.close();
  stopSpotifyDaemon();
});

process.on('SIGINT', () => {
  stopSpotifyDaemon();
  process.exit(0);
});

process.on('SIGTERM', () => {
  stopSpotifyDaemon();
  process.exit(0);
});

process.on('exit', () => {
  stopSpotifyDaemon();
});

const autostartDir = path.join(os.homedir(), '.config', 'autostart');
const autostartFile = path.join(autostartDir, 'aetheris.desktop');

function getLinuxAutoStart() {
  try {
    return fs.existsSync(autostartFile);
  } catch (e) {
    return false;
  }
}

function setLinuxAutoStart(enable) {
  try {
    if (enable) {
      if (!fs.existsSync(autostartDir)) {
        fs.mkdirSync(autostartDir, { recursive: true });
      }
      const desktopContent = `[Desktop Entry]
Type=Application
Name=Aetheris OS
Comment=Alien Holographic Music Overlay HUD
Exec=${process.env.HOME}/.local/bin/aetheris --startup
Icon=aetheris
Terminal=false
Categories=AudioVideo;Audio;Player;
StartupWMClass=aetheris
X-GNOME-Autostart-enabled=true
`;
      fs.writeFileSync(autostartFile, desktopContent, 'utf8');
      return true;
    } else {
      if (fs.existsSync(autostartFile)) {
        fs.unlinkSync(autostartFile);
      }
      return false;
    }
  } catch (e) {
    console.warn('[WARN] Failed to configure Linux autostart:', e.message);
    return false;
  }
}

ipcMain.handle('get-auto-start', () => {
  if (process.platform === 'linux') {
    return getLinuxAutoStart();
  }
  try {
    const settings = app.getLoginItemSettings({
      path: process.execPath,
      args: ['--startup'],
    });
    return settings.openAtLogin;
  } catch (e) {
    return false;
  }
});

ipcMain.handle('set-auto-start', (event, enable) => {
  console.log(`[IPC] Configuring Auto-Start: ${enable}`);
  if (process.platform === 'linux') {
    return setLinuxAutoStart(Boolean(enable));
  }
  try {
    app.setLoginItemSettings({
      openAtLogin: Boolean(enable),
      path: process.execPath,
      args: ['--startup'],
    });
    const settings = app.getLoginItemSettings({
      path: process.execPath,
      args: ['--startup'],
    });
    return settings.openAtLogin;
  } catch (e) {
    console.warn('[WARN] Failed to configure login item settings:', e.message);
    return false;
  }
});

ipcMain.handle('get-initial-visibility', () => {
  return mainWindow ? (!isStartup && mainWindow.isVisible()) : true;
});

ipcMain.on('toggle-always-on-top', (event, flag) => {
  if (mainWindow) mainWindow.setAlwaysOnTop(flag, 'screen-saver');
});

ipcMain.on('toggle-fullscreen', () => {
  if (mainWindow) {
    const isFS = mainWindow.isFullScreen();
    mainWindow.setFullScreen(!isFS);
  }
});

ipcMain.on('close-window', () => {
  if (mainWindow) {
    console.log('[LOG] Close/hide requested by renderer -- hiding overlay into SLEEP MODE...');
    mainWindow.hide();
  }
});

ipcMain.on('set-ignore-mouse-events', (event, ignore, options) => {
  const win = BrowserWindow.fromWebContents(event.sender);
  if (win) {
    win.setIgnoreMouseEvents(ignore, options || { forward: true });
  }
});
