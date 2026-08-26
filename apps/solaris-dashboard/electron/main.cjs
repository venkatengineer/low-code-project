const { app, BrowserWindow, globalShortcut, ipcMain, screen } = require('electron');
const path = require('path');
const fs = require('fs');
const { exec } = require('child_process');
const telemetry = require('./system-telemetry.cjs');

// Configure application name for Wayland window identification
app.setName('solaris-dashboard');
app.setAppUserModelId('solaris-dashboard');

// Session data path
const sessionUserData = path.join(app.getPath('appData'), 'solaris_cockpit_data');
app.setPath('userData', sessionUserData);

// Clean up stale lock files if any
try {
  const singletonFiles = ['SingletonLock', 'SingletonCookie', 'SingletonSocket'];
  for (const sf of singletonFiles) {
    const sp = path.join(sessionUserData, sf);
    if (fs.existsSync(sp)) fs.unlinkSync(sp);
  }
} catch (e) {}

let mainWindow = null;
let telemetryInterval = null;
let currentWorkspaceId = null;
let solarisWindowId = null;

function createWindow() {
  const primaryDisplay = screen.getPrimaryDisplay();
  const { width: screenWidth, height: screenHeight } = primaryDisplay.workAreaSize;
  const iconPath = path.join(__dirname, 'icon.png');

  mainWindow = new BrowserWindow({
    title: 'Solaris - Spacecraft Command Bridge',
    width: screenWidth,
    height: screenHeight,
    x: 0,
    y: 0,
    frame: false,
    transparent: true,
    alwaysOnTop: false,
    focusable: false,
    resizable: false,
    hasShadow: false,
    show: true,
    backgroundColor: '#00000000',
    icon: fs.existsSync(iconPath) ? iconPath : undefined,
    webPreferences: {
      preload: path.join(__dirname, 'preload.cjs'),
      nodeIntegration: false,
      contextIsolation: true,
      webSecurity: false
    }
  });

  const indexPath = path.join(__dirname, '../ui/index.html');
  mainWindow.loadFile(indexPath);

  mainWindow.webContents.on('did-finish-load', () => {
    mainWindow.show();
  });

  // Global Shortcuts for Command Matrix & HUD Toggle
  try {
    globalShortcut.register('CommandOrControl+Space', () => {
      if (mainWindow && !mainWindow.isDestroyed()) {
        mainWindow.webContents.send('open-command-matrix');
      }
    });
    globalShortcut.register('CommandOrControl+Shift+D', () => {
      if (mainWindow && !mainWindow.isDestroyed()) {
        if (mainWindow.isVisible()) mainWindow.hide();
        else { mainWindow.show(); }
      }
    });
  } catch (e) {}

  // Telemetry stream & Workspace auto-follow loop (1000ms)
  if (telemetryInterval) clearInterval(telemetryInterval);
  telemetryInterval = setInterval(() => {
    if (mainWindow && !mainWindow.isDestroyed()) {
      try {
        const data = telemetry.getAllTelemetry();
        mainWindow.webContents.send('telemetry-update', data);

        // Auto-follow active workspace in Niri so the cockpit is persistent across all workspaces
        if (data.niri && data.niri.workspaces) {
          const activeWs = data.niri.workspaces.find(w => w.isActive || w.isFocused);
          if (activeWs && activeWs.id !== currentWorkspaceId) {
            currentWorkspaceId = activeWs.id;
            
            // Query for our window ID if not cached
            exec('niri msg -j windows 2>/dev/null', (err, stdout) => {
              if (!err && stdout) {
                try {
                  const wins = JSON.parse(stdout);
                  const myWin = wins.find(w => (w.title && w.title.includes('Solaris')) || w.app_id === 'solaris-dashboard');
                  if (myWin) {
                    solarisWindowId = myWin.id;
                    if (myWin.workspace_id !== activeWs.id) {
                      exec(`niri msg action move-window-to-workspace ${activeWs.idx} --window-id ${myWin.id} --focus false 2>/dev/null`);
                    }
                  }
                } catch (e) {}
              }
            });
          }
        }
      } catch (err) {
        console.error('[Telemetry Error]', err.message);
      }
    }
  }, 1000);

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

// IPC Handlers
ipcMain.handle('get-telemetry', () => {
  return telemetry.getAllTelemetry();
});

ipcMain.handle('perform-action', (event, action, payload) => {
  return telemetry.performAction(action, payload);
});

ipcMain.handle('get-applications', () => {
  return telemetry.getInstalledApplications();
});

ipcMain.on('hide-dashboard', () => {
  if (mainWindow) mainWindow.hide();
});

app.whenReady().then(() => {
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('will-quit', () => {
  globalShortcut.unregisterAll();
  if (telemetryInterval) clearInterval(telemetryInterval);
});
