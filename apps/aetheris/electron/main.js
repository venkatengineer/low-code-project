const { app, BrowserWindow, globalShortcut, ipcMain, Tray, Menu, nativeImage } = require('electron');
const path = require('path');
const http = require('http');

console.log('====================================================');
console.log('   AETHERIS OS ELECTRON DESKTOP OVERLAY LAUNCHER    ');
console.log('====================================================');

let mainWindow = null;
let tray = null;

function checkUrlLive(url) {
  return new Promise((resolve) => {
    const req = http.get(url, (res) => {
      resolve(res.statusCode === 200);
    });
    req.on('error', () => resolve(false));
    req.setTimeout(800, () => {
      req.destroy();
      resolve(false);
    });
  });
}

async function createWindow() {
  console.log('[LOG] Creating BrowserWindow...');
  
  mainWindow = new BrowserWindow({
    width: 480,
    height: 720,
    x: 100,
    y: 100,
    frame: false,
    transparent: true,
    alwaysOnTop: true,
    resizable: true,
    hasShadow: false,
    show: true,
    backgroundColor: '#00000000',
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      nodeIntegration: false,
      contextIsolation: true,
      webSecurity: false,
    },
  });

  // Make sure it stays on top of full-screen apps on Windows
  mainWindow.setAlwaysOnTop(true, 'screen-saver');

  const devUrl = 'http://localhost:3000';
  const isDevLive = await checkUrlLive(devUrl);

  if (isDevLive) {
    console.log('[LOG] Connecting to Vite dev server at:', devUrl);
    mainWindow.loadURL(devUrl);
  } else {
    const indexPath = path.join(__dirname, '../dist/index.html');
    console.log('[LOG] Loading local production build at:', indexPath);
    mainWindow.loadFile(indexPath);
  }

  mainWindow.show();
  mainWindow.focus();

  mainWindow.webContents.on('did-finish-load', () => {
    console.log('[LOG] Window WebContents finished loading successfully!');
  });

  mainWindow.webContents.on('did-fail-load', (event, errorCode, errorDescription) => {
    console.error(`[ERROR] Window failed to load [${errorCode}]: ${errorDescription}`);
  });

  // Send Visibility Change Event to Renderer for Sleep Mode Architecture
  const sendVisibilityChange = (isVisible) => {
    if (mainWindow && !mainWindow.isDestroyed() && mainWindow.webContents) {
      mainWindow.webContents.send('window-visibility-changed', isVisible);
    }
  };

  // Toggle Visibility Function with Logging
  const toggleVisibility = (sourceKey) => {
    console.log(`[KEYBIND] Hotkey triggered by ${sourceKey}!`);
    if (!mainWindow) return;

    if (mainWindow.isVisible()) {
      console.log('[LOG] Hiding overlay window (entering SLEEP MODE)...');
      sendVisibilityChange(false);
      mainWindow.hide();
    } else {
      console.log('[LOG] Showing & focusing overlay window (WAKE SEQUENCE)...');
      sendVisibilityChange(true);
      mainWindow.show();
      mainWindow.focus();
      mainWindow.setAlwaysOnTop(true, 'screen-saver');
    }
  };

  // Window Show & Hide Event Hooks for System Tray or Manual Toggle Safety
  mainWindow.on('show', () => {
    sendVisibilityChange(true);
  });

  mainWindow.on('hide', () => {
    sendVisibilityChange(false);
  });

  mainWindow.on('closed', () => {
    console.log('[LOG] Window closed.');
    mainWindow = null;
  });

  // Register Native System-Wide Global Hotkeys with Validation Logs
  const keysToRegister = [
    { key: 'Alt+M', name: 'Alt+M' },
    { key: 'Alt+Space', name: 'Alt+Space' },
    { key: 'CommandOrControl+Space', name: 'Ctrl+Space' },
    { key: 'F9', name: 'F9 Key' },
    { key: 'CommandOrControl+Shift+M', name: 'Ctrl+Shift+M' },
  ];

  keysToRegister.forEach(({ key, name }) => {
    const success = globalShortcut.register(key, () => toggleVisibility(name));
    if (success) {
      console.log(`[LOG] Registered global shortcut: ${name} (${key}) - SUCCESS`);
    } else {
      console.warn(`[WARN] Failed to register global shortcut: ${name} (${key}) - MIGHT BE IN USE BY ANOTHER APP`);
    }
  });

  // System Tray Setup
  try {
    const icon = nativeImage.createFromNamedImage('NSActionTemplate');
    tray = new Tray(icon);
    const contextMenu = Menu.buildFromTemplate([
      { label: 'Aetheris HUD Overlay v1.0', enabled: false },
      { type: 'separator' },
      { label: 'Show Overlay (Alt+M / F9 / Ctrl+Space)', click: () => toggleVisibility('System Tray Menu') },
      { label: 'Hide Overlay', click: () => mainWindow && mainWindow.hide() },
      { label: 'Toggle DevTools', click: () => mainWindow && mainWindow.webContents.toggleDevTools() },
      { type: 'separator' },
      { label: 'Exit Aetheris', click: () => app.quit() },
    ]);
    tray.setToolTip('Aetheris OS Music Overlay');
    tray.setContextMenu(contextMenu);
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

app.on('will-quit', () => {
  console.log('[LOG] App quitting. Unregistering global shortcuts.');
  globalShortcut.unregisterAll();
});

ipcMain.handle('get-auto-start', () => {
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
    return false;
  }
});

ipcMain.handle('get-initial-visibility', () => {
  return mainWindow ? mainWindow.isVisible() : true;
});

ipcMain.on('toggle-always-on-top', (event, flag) => {
  if (mainWindow) mainWindow.setAlwaysOnTop(flag, 'screen-saver');
});

ipcMain.on('close-window', () => {
  if (mainWindow) {
    console.log('[LOG] Hide/Close window requested by renderer — placing overlay into SLEEP MODE...');
    mainWindow.hide();
  }
});
