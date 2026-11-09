const { ipcRenderer } = require('electron');
const path = require('path');
const os = require('os');

// ==============================================================================
// APPLICATION STATE
// ==============================================================================
const state = {
  currentPath: os.homedir(),
  history: [os.homedir()],
  historyIndex: 0,
  items: [],
  selectedItem: null,
  showHidden: false,
  viewMode: 'grid', // 'grid' or 'list'
  sortBy: 'name_asc',
  searchQuery: '',
  contextItem: null,
  sysStats: null
};

// ==============================================================================
// DOM ELEMENTS
// ==============================================================================
const el = {
  breadcrumbTrail: document.getElementById('breadcrumbTrail'),
  matrixViewport: document.getElementById('matrixViewport'),
  emptyState: document.getElementById('emptyState'),
  searchArchiveInput: document.getElementById('searchArchiveInput'),
  currentSectorLabel: document.getElementById('currentSectorLabel'),
  sortSelect: document.getElementById('sortSelect'),

  // Deck Buttons
  btnBack: document.getElementById('btnBack'),
  btnForward: document.getElementById('btnForward'),
  btnUp: document.getElementById('btnUp'),
  btnRefresh: document.getElementById('btnRefresh'),
  btnGridView: document.getElementById('btnGridView'),
  btnListView: document.getElementById('btnListView'),
  btnToggleHidden: document.getElementById('btnToggleHidden'),
  btnOpenTerminal: document.getElementById('btnOpenTerminal'),
  btnNewFolder: document.getElementById('btnNewFolder'),
  btnNewFile: document.getElementById('btnNewFile'),

  // Window Buttons
  winMin: document.getElementById('winMin'),
  winMax: document.getElementById('winMax'),
  winClose: document.getElementById('winClose'),

  // Right Inspector
  analysisPanel: document.getElementById('analysisPanel'),
  analysisBody: document.getElementById('analysisBody'),
  btnCloseAnalysis: document.getElementById('btnCloseAnalysis'),
  heroName: document.getElementById('heroName'),
  heroType: document.getElementById('heroType'),
  heroIcon: document.getElementById('heroIcon'),
  specSize: document.getElementById('specSize'),
  specPath: document.getElementById('specPath'),
  specModified: document.getElementById('specModified'),
  specPerms: document.getElementById('specPerms'),
  specOwner: document.getElementById('specOwner'),
  folderContentsGroup: document.getElementById('folderContentsGroup'),
  cntFiles: document.getElementById('cntFiles'),
  cntFolders: document.getElementById('cntFolders'),
  cntTotalSize: document.getElementById('cntTotalSize'),
  btnInspectorOpen: document.getElementById('btnInspectorOpen'),
  btnInspectorTerminal: document.getElementById('btnInspectorTerminal'),

  // 3D Canvas
  holoCoreCanvas: document.getElementById('holoCoreCanvas'),

  // Status Bar
  statObjectCount: document.getElementById('statObjectCount'),
  statSelectedInfo: document.getElementById('statSelectedInfo'),
  statDriveLabel: document.getElementById('statDriveLabel'),
  statCpu: document.getElementById('statCpu'),
  statMem: document.getElementById('statMem'),
  statDisk: document.getElementById('statDisk'),
  shipChrono: document.getElementById('shipChrono'),
  storagePillText: document.getElementById('storagePillText'),
  storagePillFill: document.getElementById('storagePillFill'),

  // Context Menu
  contextMenu: document.getElementById('contextMenu'),
  ctxHeader: document.getElementById('ctxHeader')
};

// ==============================================================================
// 1. ROUTE FORMATTER (SPACECRAFT BREADCRUMBS)
// ==============================================================================
function formatSpaceRoute(fullPath) {
  const home = os.homedir();
  let rel = fullPath;
  let prefix = 'SHIP ❯ ';

  if (fullPath === '/') {
    return [{ label: 'SHIP', path: '/' }, { label: 'ROOT_SECTOR', path: '/' }];
  }

  const tokens = [];
  tokens.push({ label: 'SHIP', path: '/' });

  if (fullPath.startsWith(home)) {
    tokens.push({ label: 'USER_CORE', path: home });
    const sub = fullPath.slice(home.length).split('/').filter(Boolean);
    let cur = home;
    for (const s of sub) {
      cur = path.join(cur, s);
      tokens.push({ label: s.toUpperCase(), path: cur });
    }
  } else {
    const parts = fullPath.split('/').filter(Boolean);
    let cur = '/';
    for (const p of parts) {
      cur = path.join(cur, p);
      tokens.push({ label: p.toUpperCase(), path: cur });
    }
  }

  return tokens;
}

function renderBreadcrumbs() {
  el.breadcrumbTrail.innerHTML = '';
  const tokens = formatSpaceRoute(state.currentPath);

  tokens.forEach((t, i) => {
    const span = document.createElement('span');
    span.className = 'crumb-token' + (i === tokens.length - 1 ? ' active' : '');
    span.textContent = t.label;
    span.addEventListener('click', () => navigateTo(t.path));
    el.breadcrumbTrail.appendChild(span);

    if (i < tokens.length - 1) {
      const sep = document.createElement('span');
      sep.className = 'crumb-sep';
      sep.textContent = '❯';
      el.breadcrumbTrail.appendChild(sep);
    }
  });

  const base = path.basename(state.currentPath);
  el.currentSectorLabel.textContent = base ? base.toUpperCase() : 'ROOT';
}

// ==============================================================================
// 2. CATEGORY ICONS & EMBLEMS
// ==============================================================================
function getCategoryIcon(item) {
  if (item.isDir) {
    const n = item.name.toLowerCase();
    if (['projects', 'dev', 'code', 'workspace'].includes(n)) return '⌬';
    if (['documents', 'docs'].includes(n)) return '≡';
    if (['downloads'].includes(n)) return '⤓';
    if (['music', 'audio', 'sounds'].includes(n)) return '♬';
    if (['pictures', 'images', 'wallpapers', 'photos'].includes(n)) return '◫';
    if (['videos', 'movies'].includes(n)) return '▷';
    return '📁';
  }
  switch (item.category) {
    case 'code': return '⌨';
    case 'image': return '◫';
    case 'media': return '▶';
    case 'document': return '≡';
    case 'archive': return '◈';
    case 'system': return '⚙';
    default: return '📄';
  }
}

// ==============================================================================
// 3. LOAD & RENDER DIRECTORY
// ==============================================================================
async function loadDirectory(targetPath) {
  try {
    const res = await ipcRenderer.invoke('fs:list-dir', targetPath);
    if (!res.success) {
      console.error('[Solaris FS Error]', res.error);
      return;
    }

    state.currentPath = res.currentPath;
    state.items = res.items;
    state.selectedItem = null;

    renderBreadcrumbs();
    renderMatrixView();
    updateStatusBar(res.stats);
    updateInspector(null);
    updateNavButtons();
  } catch (err) {
    console.error('[Solaris Load Error]', err);
  }
}

function filterAndSortItems() {
  let list = state.items;

  // Filter hidden files
  if (!state.showHidden) {
    list = list.filter(i => !i.isHidden);
  }

  // Filter search query
  if (state.searchQuery.trim()) {
    const q = state.searchQuery.toLowerCase();
    list = list.filter(i => i.name.toLowerCase().includes(q));
  }

  // Sort
  const [field, dir] = state.sortBy.split('_');
  list.sort((a, b) => {
    if (a.isDir !== b.isDir) return a.isDir ? -1 : 1;
    if (field === 'name') {
      const cmp = a.name.localeCompare(b.name, undefined, { numeric: true, sensitivity: 'base' });
      return dir === 'asc' ? cmp : -cmp;
    }
    if (field === 'date') {
      const cmp = new Date(a.mtime) - new Date(b.mtime);
      return dir === 'asc' ? cmp : -cmp;
    }
    if (field === 'size') {
      const cmp = a.size - b.size;
      return dir === 'asc' ? cmp : -cmp;
    }
    if (field === 'type') {
      return a.category.localeCompare(b.category);
    }
    return 0;
  });

  return list;
}

function renderMatrixView() {
  const list = filterAndSortItems();
  el.matrixViewport.innerHTML = '';

  if (list.length === 0) {
    el.emptyState.style.display = 'flex';
    return;
  }
  el.emptyState.style.display = 'none';

  if (state.viewMode === 'grid') {
    el.matrixViewport.className = 'matrix-viewport grid-mode';
    list.forEach(item => {
      const card = document.createElement('div');
      card.className = 'data-module-card' + (state.selectedItem?.fullPath === item.fullPath ? ' selected' : '');
      card.dataset.cat = item.category;

      const emblem = item.isDir ? 'DIR' : item.ext || 'MOD';
      const icon = getCategoryIcon(item);

      card.innerHTML = `
        <div class="card-emblem-badge">${emblem}</div>
        <div class="card-icon-frame">${icon}</div>
        <div class="card-title" title="${item.name}">${item.name}</div>
        <div class="card-meta-line">${item.isDir ? item.owner : item.sizeFormatted}</div>
      `;

      card.addEventListener('click', (e) => {
        e.stopPropagation();
        selectItem(item);
      });

      card.addEventListener('dblclick', (e) => {
        e.stopPropagation();
        openItem(item);
      });

      card.addEventListener('contextmenu', (e) => {
        e.preventDefault();
        e.stopPropagation();
        selectItem(item);
        showContextMenu(e.clientX, e.clientY, item);
      });

      el.matrixViewport.appendChild(card);
    });
  } else {
    el.matrixViewport.className = 'matrix-viewport list-mode';
    list.forEach(item => {
      const row = document.createElement('div');
      row.className = 'data-list-row' + (state.selectedItem?.fullPath === item.fullPath ? ' selected' : '');

      const icon = getCategoryIcon(item);
      row.innerHTML = `
        <div class="list-col-icon">${icon}</div>
        <div class="list-col-name" title="${item.name}">${item.name}</div>
        <div class="list-col-size">${item.sizeFormatted}</div>
        <div class="list-col-mtime">${item.mtimeFormatted}</div>
        <div class="list-col-perms">${item.perms}</div>
      `;

      row.addEventListener('click', (e) => {
        e.stopPropagation();
        selectItem(item);
      });

      row.addEventListener('dblclick', (e) => {
        e.stopPropagation();
        openItem(item);
      });

      row.addEventListener('contextmenu', (e) => {
        e.preventDefault();
        e.stopPropagation();
        selectItem(item);
        showContextMenu(e.clientX, e.clientY, item);
      });

      el.matrixViewport.appendChild(row);
    });
  }
}

// ==============================================================================
// 4. NAVIGATION & SELECTION ACTIONS
// ==============================================================================
function navigateTo(targetPath) {
  if (targetPath === state.currentPath) return;
  if (typeof sfx !== 'undefined') sfx.playFolder();
  // Trim forward history if we navigated anew
  if (state.historyIndex < state.history.length - 1) {
    state.history = state.history.slice(0, state.historyIndex + 1);
  }
  state.history.push(targetPath);
  state.historyIndex = state.history.length - 1;
  loadDirectory(targetPath);
}

function goBack() {
  if (state.historyIndex > 0) {
    if (typeof sfx !== 'undefined') sfx.playPanels();
    state.historyIndex--;
    loadDirectory(state.history[state.historyIndex]);
  }
}

function goForward() {
  if (state.historyIndex < state.history.length - 1) {
    if (typeof sfx !== 'undefined') sfx.playPanels();
    state.historyIndex++;
    loadDirectory(state.history[state.historyIndex]);
  }
}

function goUp() {
  const parent = path.dirname(state.currentPath);
  if (parent && parent !== state.currentPath) {
    if (typeof sfx !== 'undefined') sfx.playExpand();
    navigateTo(parent);
  }
}

function updateNavButtons() {
  el.btnBack.style.opacity = state.historyIndex > 0 ? '1' : '0.4';
  el.btnForward.style.opacity = state.historyIndex < state.history.length - 1 ? '1' : '0.4';
}

function selectItem(item) {
  state.selectedItem = item;
  if (typeof sfx !== 'undefined') sfx.playScan();
  updateInspector(item);
  renderMatrixView();
  el.statSelectedInfo.textContent = item ? `1 OBJECT [ ${item.sizeFormatted} ]` : '0 SELECTED';
}

function openItem(item) {
  if (!item) return;
  if (item.isDir) {
    if (typeof sfx !== 'undefined') sfx.playFolder();
    navigateTo(item.fullPath);
  } else {
    if (typeof sfx !== 'undefined') sfx.playGranted();
    ipcRenderer.invoke('fs:open', item.fullPath);
  }
}

// ==============================================================================
// 5. DATA ANALYSIS & OBJECT INFORMATION INSPECTOR
// ==============================================================================
function updateInspector(item) {
  if (!item) {
    el.heroName.textContent = path.basename(state.currentPath).toUpperCase();
    el.heroType.textContent = 'CURRENT DIRECTORY // SECTOR';
    el.heroIcon.textContent = '⬡';
    el.specSize.textContent = '--';
    el.specPath.textContent = state.currentPath;
    el.specModified.textContent = '--';
    el.specPerms.textContent = 'rwxr-xr-x';
    el.specOwner.textContent = 'user:user';
    el.folderContentsGroup.style.display = 'block';
    return;
  }

  el.heroName.textContent = item.name.toUpperCase();
  el.heroType.textContent = item.isDir ? 'STORAGE CONTAINER // DIR' : `${item.category.toUpperCase()} MODULE // .${item.ext}`;
  el.heroIcon.textContent = getCategoryIcon(item);

  el.specSize.textContent = item.sizeFormatted;
  el.specPath.textContent = item.fullPath;
  el.specModified.textContent = item.mtimeFormatted;
  el.specPerms.textContent = item.perms;
  el.specOwner.textContent = `${item.owner}:user`;

  if (item.isDir) {
    el.folderContentsGroup.style.display = 'block';
    el.cntFiles.textContent = 'SCANNING';
    el.cntFolders.textContent = '--';
    el.cntTotalSize.textContent = item.sizeFormatted;
  } else {
    el.folderContentsGroup.style.display = 'none';
  }
}

function updateStatusBar(stats) {
  if (!stats) return;
  el.statObjectCount.textContent = `${stats.totalItems} DATA MODULES (${stats.fileCount} Files, ${stats.dirCount} Folders)`;
  el.cntFiles.textContent = stats.fileCount;
  el.cntFolders.textContent = stats.dirCount;
  el.cntTotalSize.textContent = stats.totalSize;
}

// ==============================================================================
// 6. 3D HOLOGRAPHIC STORAGE CORE VISUALIZER (CANVAS)
// ==============================================================================
let holoAngle = 0;
function renderHoloCore() {
  const canvas = el.holoCoreCanvas;
  if (!canvas) return;
  const ctx = canvas.getContext('2d');
  const w = canvas.width;
  const h = canvas.height;
  const cx = w / 2;
  const cy = h / 2;

  ctx.clearRect(0, 0, w, h);

  holoAngle += 0.015;

  // Background grid lines in holo box
  ctx.strokeStyle = 'rgba(56, 189, 248, 0.08)';
  ctx.lineWidth = 1;
  for (let x = 0; x < w; x += 20) {
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, h);
    ctx.stroke();
  }

  // Draw 3D Rotating Cylinder / Storage Disk Array
  const layers = 5;
  const rx = 80;
  const ry = 28;

  for (let l = 0; l < layers; l++) {
    const yOff = (l - 2) * 16;
    const alpha = 0.4 + (l / layers) * 0.5;

    // Cylinder ring
    ctx.beginPath();
    ctx.ellipse(cx, cy + yOff, rx, ry, 0, 0, Math.PI * 2);
    ctx.strokeStyle = l === 2 ? '#38bdf8' : `rgba(56, 189, 248, ${alpha})`;
    ctx.lineWidth = l === 2 ? 1.5 : 1;
    ctx.stroke();

    // Rotating sector nodes on ring
    const nodes = 6;
    for (let n = 0; n < nodes; n++) {
      const a = holoAngle + (n * (Math.PI * 2 / nodes)) + (l * 0.3);
      const px = cx + Math.cos(a) * rx;
      const py = cy + yOff + Math.sin(a) * ry;

      ctx.beginPath();
      ctx.arc(px, py, l === 2 ? 2.5 : 1.5, 0, Math.PI * 2);
      ctx.fillStyle = (n % 2 === 0) ? '#10b981' : '#38bdf8';
      ctx.fill();
    }
  }

  // Central Plasma Data Core
  const corePulse = Math.sin(holoAngle * 3) * 4;
  ctx.beginPath();
  ctx.arc(cx, cy, 10 + corePulse, 0, Math.PI * 2);
  ctx.fillStyle = 'rgba(56, 189, 248, 0.2)';
  ctx.fill();
  ctx.beginPath();
  ctx.arc(cx, cy, 4, 0, Math.PI * 2);
  ctx.fillStyle = '#00f0ff';
  ctx.fill();

  requestAnimationFrame(renderHoloCore);
}

// ==============================================================================
// 7. REAL-TIME SYSTEM TELEMETRY
// ==============================================================================
async function updateTelemetry() {
  try {
    const res = await ipcRenderer.invoke('sys:telemetry');
    if (res.success) {
      state.sysStats = res;
      el.statCpu.textContent = `CPU: ${res.cpu}%`;
      el.statMem.textContent = `MEM: ${res.mem.used} (${res.mem.pct}%)`;
      el.statDisk.textContent = `STORAGE: ${res.disk.pct}%`;
      el.storagePillText.textContent = `${res.disk.used} / ${res.disk.total}`;
      el.storagePillFill.style.width = `${res.disk.pct}%`;
    }
  } catch (e) {}

  // Update Clock
  const now = new Date();
  el.shipChrono.textContent = `UTC ${now.toTimeString().split(' ')[0]}`;
}

// ==============================================================================
// 8. CONTEXT MENU & MODALS
// ==============================================================================
function showContextMenu(x, y, item) {
  state.contextItem = item;
  el.ctxHeader.textContent = item ? item.name.toUpperCase() : 'SECTOR ACTIONS';
  el.contextMenu.style.display = 'block';
  el.contextMenu.style.left = `${Math.min(x, window.innerWidth - 210)}px`;
  el.contextMenu.style.top = `${Math.min(y, window.innerHeight - 240)}px`;
}

function hideContextMenu() {
  el.contextMenu.style.display = 'none';
  state.contextItem = null;
}

// ==============================================================================
// 9. EVENT LISTENERS & SETUP
// ==============================================================================
function setupEventListeners() {
  // Navigation
  el.btnBack.addEventListener('click', goBack);
  el.btnForward.addEventListener('click', goForward);
  el.btnUp.addEventListener('click', goUp);
  el.btnRefresh.addEventListener('click', () => loadDirectory(state.currentPath));

  // Sidebar Links
  document.querySelectorAll('.nav-item').forEach(item => {
    item.addEventListener('click', () => {
      document.querySelectorAll('.nav-item').forEach(n => n.classList.remove('active'));
      item.classList.add('active');

      const p = item.dataset.path;
      const home = os.homedir();
      switch (p) {
        case 'HOME': navigateTo(home); break;
        case 'DESKTOP': navigateTo(path.join(home, 'Desktop')); break;
        case 'DOCUMENTS': navigateTo(path.join(home, 'Documents')); break;
        case 'DOWNLOADS': navigateTo(path.join(home, 'Downloads')); break;
        case 'PROJECTS': navigateTo(path.join(home, 'Projects')); break;
        case 'MEDIA': navigateTo(path.join(home, 'Music')); break;
        case 'ARCHIVE': navigateTo(path.join(home, '.local/share')); break;
        case 'ROOT': navigateTo('/'); break;
        case 'BOOT': navigateTo('/boot'); break;
        default: break;
      }
    });
  });

  // View Mode Toggles
  el.btnGridView.addEventListener('click', () => {
    if (typeof sfx !== 'undefined') sfx.playExpand();
    state.viewMode = 'grid';
    el.btnGridView.classList.add('active');
    el.btnListView.classList.remove('active');
    renderMatrixView();
  });

  el.btnListView.addEventListener('click', () => {
    if (typeof sfx !== 'undefined') sfx.playExpand();
    state.viewMode = 'list';
    el.btnListView.classList.add('active');
    el.btnGridView.classList.remove('active');
    renderMatrixView();
  });

  // Toggle Hidden Files
  el.btnToggleHidden.addEventListener('click', () => {
    if (typeof sfx !== 'undefined') sfx.playPanels();
    state.showHidden = !state.showHidden;
    el.btnToggleHidden.classList.toggle('active', state.showHidden);
    renderMatrixView();
  });

  // Search Filter
  el.searchArchiveInput.addEventListener('input', (e) => {
    if (typeof sfx !== 'undefined') sfx.playKey();
    state.searchQuery = e.target.value;
    renderMatrixView();
  });

  // Sort Selector
  el.sortSelect.addEventListener('change', (e) => {
    if (typeof sfx !== 'undefined') sfx.playPanels();
    state.sortBy = e.target.value;
    renderMatrixView();
  });

  // Quick Operations
  el.btnNewFolder.addEventListener('click', async () => {
    if (typeof sfx !== 'undefined') sfx.playPanels();
    const folderName = prompt('CREATE NEW DATA MODULE // ENTER FOLDER NAME:');
    if (folderName) {
      await ipcRenderer.invoke('fs:create-folder', { dirPath: state.currentPath, folderName });
      if (typeof sfx !== 'undefined') sfx.playGranted();
      loadDirectory(state.currentPath);
    }
  });

  el.btnNewFile.addEventListener('click', async () => {
    if (typeof sfx !== 'undefined') sfx.playPanels();
    const fileName = prompt('CREATE NEW DATA OBJECT // ENTER FILE NAME:');
    if (fileName) {
      await ipcRenderer.invoke('fs:create-file', { dirPath: state.currentPath, fileName });
      if (typeof sfx !== 'undefined') sfx.playGranted();
      loadDirectory(state.currentPath);
    }
  });

  el.btnOpenTerminal.addEventListener('click', () => {
    if (typeof sfx !== 'undefined') sfx.playPanels();
    ipcRenderer.invoke('fs:open-terminal', state.currentPath);
  });

  // Inspector Actions
  el.btnInspectorOpen.addEventListener('click', () => {
    if (state.selectedItem) openItem(state.selectedItem);
  });

  el.btnInspectorTerminal.addEventListener('click', () => {
    if (typeof sfx !== 'undefined') sfx.playPanels();
    const p = state.selectedItem?.isDir ? state.selectedItem.fullPath : state.currentPath;
    ipcRenderer.invoke('fs:open-terminal', p);
  });

  // Window Controls
  el.winMin.addEventListener('click', () => ipcRenderer.send('win:minimize'));
  el.winMax.addEventListener('click', () => ipcRenderer.send('win:maximize'));
  el.winClose.addEventListener('click', () => ipcRenderer.send('win:close'));

  // Context Menu Actions
  document.querySelectorAll('.ctx-item').forEach(btn => {
    btn.addEventListener('click', async () => {
      const act = btn.dataset.action;
      const item = state.contextItem;
      hideContextMenu();
      if (!item) return;

      switch (act) {
        case 'open': openItem(item); break;
        case 'terminal': ipcRenderer.invoke('fs:open-terminal', item.isDir ? item.fullPath : state.currentPath); break;
        case 'rename': {
          const newName = prompt(`RENAME ${item.name} TO:`, item.name);
          if (newName && newName !== item.name) {
            await ipcRenderer.invoke('fs:rename', { oldPath: item.fullPath, newName });
            if (typeof sfx !== 'undefined') sfx.playGranted();
            loadDirectory(state.currentPath);
          }
          break;
        }
        case 'delete': {
          if (confirm(`PURGE DATA OBJECT: ${item.name}? THIS CANNOT BE UNDONE.`)) {
            await ipcRenderer.invoke('fs:delete', item.fullPath);
            if (typeof sfx !== 'undefined') sfx.playDenied();
            loadDirectory(state.currentPath);
          }
          break;
        }
        default: break;
      }
    });
  });

  // Global Clicks & Shortcuts
  document.addEventListener('click', hideContextMenu);
  document.addEventListener('keydown', (e) => {
    if (e.target.tagName !== 'INPUT') {
      if (typeof sfx !== 'undefined' && e.key.length === 1) sfx.playKey();
    }
    if (e.key === 'Escape') hideContextMenu();
    if (e.ctrlKey && e.key === 'f') {
      e.preventDefault();
      el.searchArchiveInput.focus();
    }
    if (e.ctrlKey && e.key === 'h') {
      e.preventDefault();
      el.btnToggleHidden.click();
    }
    if (e.key === 'F5') {
      e.preventDefault();
      loadDirectory(state.currentPath);
    }
    if (e.key === 'F4') {
      e.preventDefault();
      ipcRenderer.invoke('fs:open-terminal', state.currentPath);
    }
    if (e.altKey && e.key === 'ArrowLeft') goBack();
    if (e.altKey && e.key === 'ArrowRight') goForward();
    if (e.altKey && e.key === 'ArrowUp') goUp();
    if (e.key === 'Enter' && state.selectedItem) {
      openItem(state.selectedItem);
    }
  });
}

// ==============================================================================
// INITIALIZATION
// ==============================================================================
window.addEventListener('DOMContentLoaded', () => {
  setupEventListeners();
  loadDirectory(state.currentPath);
  setTimeout(() => {
    if (typeof sfx !== 'undefined') sfx.playTheme();
  }, 400);
  setupEventListeners();
  loadDirectory(state.currentPath);
  renderHoloCore();
  updateTelemetry();
  setInterval(updateTelemetry, 2500);
});
