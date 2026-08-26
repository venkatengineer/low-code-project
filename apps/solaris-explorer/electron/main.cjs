const { app, BrowserWindow, ipcMain, shell, dialog } = require('electron');
const path = require('path');
const fs = require('fs');
const os = require('os');
const { spawn, execSync } = require('child_process');

let mainWindow = null;

function createWindow() {
  mainWindow = new BrowserWindow({
    title: 'SOLARIS // DATA NAVIGATOR',
    width: 1440,
    height: 900,
    minWidth: 1024,
    minHeight: 650,
    backgroundColor: '#0e121a',
    frame: false,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
      enableRemoteModule: true,
      webSecurity: false
    }
  });

  mainWindow.loadFile(path.join(__dirname, '../src/index.html'));

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

app.whenReady().then(() => {
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});

// ==============================================================================
// FILESYSTEM IPC HANDLERS
// ==============================================================================

// Helper: classify file type
function getCategory(ext, isDir, isSymlink) {
  if (isDir) return 'directory';
  if (isSymlink) return 'symlink';
  const e = ext.toLowerCase();
  if (['.js', '.ts', '.py', '.c', '.cpp', '.h', '.rs', '.go', '.sh', '.bash', '.kdl', '.json', '.html', '.css', '.toml', '.yaml', '.yml'].includes(e)) return 'code';
  if (['.png', '.jpg', '.jpeg', '.gif', '.svg', '.webp', '.bmp', '.ico'].includes(e)) return 'image';
  if (['.mp4', '.mkv', '.webm', '.avi', '.mov', '.mp3', '.wav', '.flac', '.ogg'].includes(e)) return 'media';
  if (['.pdf', '.md', '.txt', '.doc', '.docx', '.odt', '.rtf'].includes(e)) return 'document';
  if (['.zip', '.tar', '.gz', '.bz2', '.xz', '.7z', '.rar'].includes(e)) return 'archive';
  if (['.bin', '.elf', '.so', '.dll', '.exe'].includes(e)) return 'system';
  return 'file';
}

function formatBytes(bytes) {
  if (bytes === 0) return '0 B';
  const k = 1024;
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i];
}

// 1. List directory
ipcMain.handle('fs:list-dir', async (event, dirPath) => {
  try {
    const targetPath = path.resolve(dirPath || os.homedir());
    const entries = await fs.promises.readdir(targetPath, { withFileTypes: true });
    
    const items = [];
    let totalSize = 0;
    let fileCount = 0;
    let dirCount = 0;

    for (const ent of entries) {
      const fullPath = path.join(targetPath, ent.name);
      try {
        const stat = await fs.promises.lstat(fullPath);
        const isDir = ent.isDirectory() || (ent.isSymbolicLink() && stat.isDirectory());
        const isSymlink = ent.isSymbolicLink();
        const ext = path.extname(ent.name);
        const category = getCategory(ext, isDir, isSymlink);
        
        if (isDir) {
          dirCount++;
        } else {
          fileCount++;
          totalSize += stat.size;
        }

        // Permissions string
        const mode = stat.mode;
        const perms = (stat.isDirectory() ? 'd' : '-') +
          ((mode & 0o400) ? 'r' : '-') + ((mode & 0o200) ? 'w' : '-') + ((mode & 0o100) ? 'x' : '-') +
          ((mode & 0o040) ? 'r' : '-') + ((mode & 0o020) ? 'w' : '-') + ((mode & 0o010) ? 'x' : '-') +
          ((mode & 0o004) ? 'r' : '-') + ((mode & 0o002) ? 'w' : '-') + ((mode & 0o001) ? 'x' : '-');

        items.push({
          name: ent.name,
          fullPath,
          isDir,
          isSymlink,
          isHidden: ent.name.startsWith('.'),
          size: stat.size,
          sizeFormatted: isDir ? '--' : formatBytes(stat.size),
          mtime: stat.mtime.toISOString(),
          mtimeFormatted: stat.mtime.toLocaleDateString() + ' ' + stat.mtime.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
          category,
          ext: ext.replace('.', '').toUpperCase(),
          perms,
          owner: stat.uid === 1000 ? 'user' : String(stat.uid)
        });
      } catch (err) {
        // Unreadable file / broken symlink
        items.push({
          name: ent.name,
          fullPath,
          isDir: ent.isDirectory(),
          isSymlink: true,
          isHidden: ent.name.startsWith('.'),
          size: 0,
          sizeFormatted: 'ERR',
          mtimeFormatted: '--',
          category: 'file',
          ext: 'SYS',
          perms: '---------',
          owner: 'system'
        });
      }
    }

    // Sort: directories first, then alphabetically
    items.sort((a, b) => {
      if (a.isDir !== b.isDir) return a.isDir ? -1 : 1;
      return a.name.localeCompare(b.name, undefined, { numeric: true, sensitivity: 'base' });
    });

    return {
      success: true,
      currentPath: targetPath,
      items,
      stats: {
        totalItems: items.length,
        fileCount,
        dirCount,
        totalSize: formatBytes(totalSize)
      }
    };
  } catch (err) {
    return { success: false, error: err.message };
  }
});

// 2. Open file with system default
ipcMain.handle('fs:open', async (event, fullPath) => {
  try {
    shell.openPath(fullPath);
    return { success: true };
  } catch (err) {
    return { success: false, error: err.message };
  }
});

// 3. Open in Terminal
ipcMain.handle('fs:open-terminal', async (event, targetDir) => {
  try {
    spawn('kitty', ['--directory', targetDir || os.homedir()], { detached: true, stdio: 'ignore' });
    return { success: true };
  } catch (err) {
    return { success: false, error: err.message };
  }
});

// 4. File operations (Create, Rename, Delete)
ipcMain.handle('fs:create-folder', async (event, { dirPath, folderName }) => {
  try {
    const full = path.join(dirPath, folderName);
    await fs.promises.mkdir(full, { recursive: true });
    return { success: true, path: full };
  } catch (err) {
    return { success: false, error: err.message };
  }
});

ipcMain.handle('fs:create-file', async (event, { dirPath, fileName }) => {
  try {
    const full = path.join(dirPath, fileName);
    await fs.promises.writeFile(full, '', { flag: 'wx' });
    return { success: true, path: full };
  } catch (err) {
    return { success: false, error: err.message };
  }
});

ipcMain.handle('fs:delete', async (event, fullPath) => {
  try {
    await fs.promises.rm(fullPath, { recursive: true, force: true });
    return { success: true };
  } catch (err) {
    return { success: false, error: err.message };
  }
});

ipcMain.handle('fs:rename', async (event, { oldPath, newName }) => {
  try {
    const dir = path.dirname(oldPath);
    const newPath = path.join(dir, newName);
    await fs.promises.rename(oldPath, newPath);
    return { success: true, newPath };
  } catch (err) {
    return { success: false, error: err.message };
  }
});

// 5. System Telemetry (Disk, CPU, Memory)
ipcMain.handle('sys:telemetry', async () => {
  try {
    const memTotal = os.totalmem();
    const memFree = os.freemem();
    const memUsed = memTotal - memFree;
    const memPct = Math.round((memUsed / memTotal) * 100);

    let diskTotal = '1000 GB';
    let diskUsed = '280 GB';
    let diskAvail = '720 GB';
    let diskPct = 28;

    try {
      const df = execSync("df -B1 / | tail -n 1").toString().trim().split(/\s+/);
      if (df.length >= 5) {
        const totalB = parseInt(df[1]);
        const usedB = parseInt(df[2]);
        const availB = parseInt(df[3]);
        diskTotal = formatBytes(totalB);
        diskUsed = formatBytes(usedB);
        diskAvail = formatBytes(availB);
        diskPct = Math.round((usedB / totalB) * 100);
      }
    } catch (e) {}

    const cpus = os.cpus();
    let cpuLoad = 12;
    try {
      const load = os.loadavg()[0];
      cpuLoad = Math.min(100, Math.round((load / cpus.length) * 100));
    } catch (e) {}

    return {
      success: true,
      cpu: cpuLoad,
      mem: {
        total: formatBytes(memTotal),
        used: formatBytes(memUsed),
        pct: memPct
      },
      disk: {
        total: diskTotal,
        used: diskUsed,
        avail: diskAvail,
        pct: diskPct
      },
      hostname: os.hostname(),
      uptime: Math.round(os.uptime())
    };
  } catch (err) {
    return { success: false, error: err.message };
  }
});

// Window controls
ipcMain.on('win:minimize', () => mainWindow?.minimize());
ipcMain.on('win:maximize', () => {
  if (mainWindow?.isMaximized()) {
    mainWindow.unmaximize();
  } else {
    mainWindow?.maximize();
  }
});
ipcMain.on('win:close', () => mainWindow?.close());
