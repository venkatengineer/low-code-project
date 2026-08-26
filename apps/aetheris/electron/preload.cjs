const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electronAPI', {
  toggleAlwaysOnTop: (flag) => ipcRenderer.send('toggle-always-on-top', flag),
  toggleFullscreen: () => ipcRenderer.send('toggle-fullscreen'),
  closeWindow: () => ipcRenderer.send('close-window'),
  setIgnoreMouseEvents: (ignore, options) => ipcRenderer.send('set-ignore-mouse-events', ignore, options),
  getAutoStart: () => ipcRenderer.invoke('get-auto-start'),
  setAutoStart: (enable) => ipcRenderer.invoke('set-auto-start', enable),
  getInitialVisibility: () => ipcRenderer.invoke('get-initial-visibility'),
  onSpotifyAuthCode: (callback) => {
    ipcRenderer.on('spotify-auth-code', (event, code) => callback(code));
  },
  onWindowVisibilityChanged: (callback) => {
    ipcRenderer.on('window-visibility-changed', (event, isVisible) => callback(isVisible));
  },
});
