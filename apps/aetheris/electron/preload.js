const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electronAPI', {
  toggleAlwaysOnTop: (flag) => ipcRenderer.send('toggle-always-on-top', flag),
  setIgnoreMouseEvents: (ignore) => ipcRenderer.send('set-ignore-mouse-events', ignore),
  closeWindow: () => ipcRenderer.send('close-window'),
  getAutoStart: () => ipcRenderer.invoke('get-auto-start'),
  setAutoStart: (enable) => ipcRenderer.invoke('set-auto-start', enable),
  onWindowVisibilityChanged: (callback) => {
    ipcRenderer.on('window-visibility-changed', (_event, isVisible) => callback(isVisible));
  },
  getInitialVisibility: () => ipcRenderer.invoke('get-initial-visibility'),
  onSpotifyAuthCode: (callback) => {
    ipcRenderer.on('spotify-auth-code', (_event, code) => callback(code));
  },
});
