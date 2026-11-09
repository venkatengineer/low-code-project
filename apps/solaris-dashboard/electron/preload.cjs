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
  setWindowFocusable: (focusable) => ipcRenderer.send('set-window-focusable', focusable),

  // Interactive Terminal & SSH Subsystem
  startLocalTerminal: (sessionId, cols, rows) => ipcRenderer.invoke('terminal-start-local', { sessionId, cols, rows }),
  startSshTerminal: (sessionId, config, cols, rows) => ipcRenderer.invoke('terminal-start-ssh', { sessionId, config, cols, rows }),
  sendTerminalData: (sessionId, data) => ipcRenderer.send('terminal-send-data', { sessionId, data }),
  resizeTerminal: (sessionId, cols, rows) => ipcRenderer.send('terminal-resize', { sessionId, cols, rows }),
  closeTerminal: (sessionId) => ipcRenderer.send('terminal-close', { sessionId }),
  onTerminalData: (callback) => {
    ipcRenderer.on('terminal-data', (event, payload) => callback(payload));
  },
  onTerminalExit: (callback) => {
    ipcRenderer.on('terminal-exit', (event, payload) => callback(payload));
  },
  onTerminalError: (callback) => {
    ipcRenderer.on('terminal-error', (event, payload) => callback(payload));
  },

  // Service Management Subsystem
  getSystemServices: () => ipcRenderer.invoke('get-system-services'),
  manageService: (service, action) => ipcRenderer.invoke('manage-service', { service, action })
});
