const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('solarisAPI', {
  // Telemetry Stream Listener
  onTelemetry: (callback) => {
    ipcRenderer.on('telemetry-update', (event, data) => callback(data));
  },
  getInitialTelemetry: () => ipcRenderer.invoke('get-telemetry'),
  
  // Real Actions
  performAction: (action, payload) => ipcRenderer.invoke('perform-action', action, payload),
  getApplications: () => ipcRenderer.invoke('get-applications'),
  
  // Window & Overlay Controls
  setIgnoreMouseEvents: (ignore, options) => ipcRenderer.send('set-ignore-mouse-events', ignore, options),
  toggleFullscreen: () => ipcRenderer.send('toggle-fullscreen'),
  hideDashboard: () => ipcRenderer.send('hide-dashboard'),
  toggleDashboard: () => ipcRenderer.send('toggle-dashboard'),
  setWindowFocusable: (focusable) => ipcRenderer.send('set-window-focusable', focusable)
});
