// ==============================================================================
// 📊 SERVER METRICS PANEL — Contextual Inspection & Service Control Interface
// ==============================================================================

export class ServerMetricsPanel {
  constructor(container) {
    this.parentContainer = container;
    this.currentHotspot = null;
    this.servicesData = null;
    this.telemetryCache = null;

    this.createDOM();
    this.bindEvents();
  }

  createDOM() {
    this.panelEl = document.createElement('div');
    this.panelEl.className = 'server-inspector-drawer';
    this.panelEl.id = 'server-inspector-drawer';
    this.panelEl.style.display = 'none';

    this.panelEl.innerHTML = `
      <div class="inspector-card">
        <!-- HEADER -->
        <div class="inspector-header">
          <div class="inspector-title-wrap">
            <span class="inspector-glyph">⌬</span>
            <div class="inspector-titles">
              <span class="inspector-main-title" id="inspector-title">SUBSYSTEM TELEMETRY</span>
              <span class="inspector-sub-title" id="inspector-subtitle">HARDWARE ARCHITECTURE MATRIX</span>
            </div>
          </div>
          <button class="inspector-close-btn" id="btn-inspector-close">✕</button>
        </div>

        <!-- BODY TABS: [METRICS DEEP-DIVE] [SYSTEM SERVICES] -->
        <div class="inspector-tabs">
          <button class="ins-tab-btn active" data-tab="metrics" id="tab-btn-metrics">SUBSYSTEM SENSORS</button>
          <button class="ins-tab-btn" data-tab="services" id="tab-btn-services">SERVICES & CONTAINERS</button>
        </div>

        <!-- TAB CONTENT 1: SENSOR DETAILS -->
        <div class="inspector-tab-pane active" id="pane-metrics">
          <div class="metrics-dynamic-grid" id="metrics-dynamic-grid">
            <!-- Populated dynamically on hotspot click -->
          </div>
        </div>

        <!-- TAB CONTENT 2: SERVICES & DOCKER -->
        <div class="inspector-tab-pane" id="pane-services">
          <div class="services-status-bar">
            <span>DAEMON SUPERVISOR (SYSTEMD & DOCKER)</span>
            <button class="services-refresh-btn" id="btn-services-refresh">↻ REFRESH</button>
          </div>
          <div class="services-list-container" id="services-list-container">
            <div class="service-loading-placeholder">Querying Linux system services...</div>
          </div>
          <div class="docker-status-section" id="docker-status-section">
            <div class="section-sub-title">DOCKER CONTAINER RUNTIME</div>
            <div id="docker-container-list"></div>
          </div>
        </div>

        <!-- CONFIRMATION MODAL FOR DESTRUCTIVE ACTIONS -->
        <div class="inspector-confirm-modal" id="inspector-confirm-modal" style="display: none;">
          <div class="confirm-box">
            <div class="confirm-title">⚠ CONFIRM SERVICE DISPATCH</div>
            <div class="confirm-msg" id="confirm-modal-msg">Are you sure you want to stop this service?</div>
            <div class="confirm-actions">
              <button class="confirm-btn cancel" id="btn-confirm-cancel">CANCEL</button>
              <button class="confirm-btn proceed danger" id="btn-confirm-proceed">CONFIRM ACTION</button>
            </div>
          </div>
        </div>

      </div>
    `;

    this.parentContainer.appendChild(this.panelEl);
  }

  bindEvents() {
    // Escape key closes inspector drawer
    window.addEventListener('keydown', (e) => {
      if (e.key === 'Escape' && this.panelEl.style.display !== 'none') {
        const confirmModal = this.panelEl.querySelector('#inspector-confirm-modal');
        if (confirmModal && confirmModal.style.display !== 'none') {
          confirmModal.style.display = 'none';
        } else {
          this.hide();
        }
      }
    });

    // Close button
    const closeBtn = this.panelEl.querySelector('#btn-inspector-close');
    if (closeBtn) {
      closeBtn.addEventListener('click', () => this.hide());
    }

    // Tab buttons
    this.panelEl.querySelectorAll('.ins-tab-btn').forEach(btn => {
      btn.addEventListener('click', () => {
        this.panelEl.querySelectorAll('.ins-tab-btn').forEach(b => b.classList.remove('active'));
        this.panelEl.querySelectorAll('.inspector-tab-pane').forEach(p => p.classList.remove('active'));

        btn.classList.add('active');
        const targetPane = this.panelEl.querySelector(`#pane-${btn.dataset.tab}`);
        if (targetPane) targetPane.classList.add('active');

        if (btn.dataset.tab === 'services') {
          this.fetchServices();
        }
      });
    });

    // Refresh services button
    const refreshBtn = this.panelEl.querySelector('#btn-services-refresh');
    if (refreshBtn) {
      refreshBtn.addEventListener('click', () => this.fetchServices());
    }

    // Confirmation Modal Cancel
    const cancelBtn = this.panelEl.querySelector('#btn-confirm-cancel');
    const confirmModal = this.panelEl.querySelector('#inspector-confirm-modal');
    if (cancelBtn && confirmModal) {
      cancelBtn.addEventListener('click', () => {
        confirmModal.style.display = 'none';
        this.pendingAction = null;
      });
    }

    // Confirmation Modal Proceed
    const proceedBtn = this.panelEl.querySelector('#btn-confirm-proceed');
    if (proceedBtn && confirmModal) {
      proceedBtn.addEventListener('click', () => {
        if (this.pendingAction) {
          confirmModal.style.display = 'none';
          this.executeServiceAction(this.pendingAction.service, this.pendingAction.action);
          this.pendingAction = null;
        }
      });
    }
  }

  showHotspot(hotspotId, telemetry) {
    this.currentHotspot = hotspotId;
    this.telemetryCache = telemetry;
    this.panelEl.style.display = 'flex';

    // Default to metrics pane
    this.panelEl.querySelector('#tab-btn-metrics').click();

    const titleEl = this.panelEl.querySelector('#inspector-title');
    const subtitleEl = this.panelEl.querySelector('#inspector-subtitle');
    const grid = this.panelEl.querySelector('#metrics-dynamic-grid');

    if (hotspotId === 'cpu') {
      titleEl.textContent = 'CENTRAL PROCESSOR REACTOR';
      subtitleEl.textContent = `AMD / INTEL X86_64 ARCHITECTURE · 16 CORES`;
      grid.innerHTML = this.renderCpuMetrics(telemetry);
    } else if (hotspotId === 'memory') {
      titleEl.textContent = 'QUANTUM RAM BUFFER';
      subtitleEl.textContent = 'HIGH-SPEED DDR5 VOLATILE MEMORY POOL';
      grid.innerHTML = this.renderMemoryMetrics(telemetry);
    } else if (hotspotId === 'storage') {
      titleEl.textContent = 'NVME STORAGE ARRAYS';
      subtitleEl.textContent = 'SOLID-STATE NON-VOLATILE FLASH CONDUITS';
      grid.innerHTML = this.renderStorageMetrics(telemetry);
    } else if (hotspotId === 'network') {
      titleEl.textContent = 'NETWORK & TELEMETRY BUS';
      subtitleEl.textContent = 'WI-FI, ETHERNET, AND SUBSPACE TUNNELS';
      grid.innerHTML = this.renderNetworkMetrics(telemetry);
    } else {
      titleEl.textContent = 'SERVER CORE DIAGNOSTICS';
      subtitleEl.textContent = 'GENERAL ARCHITECTURE MONITOR';
      grid.innerHTML = this.renderCpuMetrics(telemetry);
    }
  }

  renderCpuMetrics(t) {
    const cpu = t?.cpu || {};
    const sys = t?.system || {};
    return `
      <div class="ins-metric-tile">
        <span class="ins-tile-label">CPU LOAD</span>
        <span class="ins-tile-val green">${cpu.usage || 18}%</span>
      </div>
      <div class="ins-metric-tile">
        <span class="ins-tile-label">FREQUENCY</span>
        <span class="ins-tile-val cyan">${cpu.avgFreqGhz || '3.42'} GHz</span>
      </div>
      <div class="ins-metric-tile">
        <span class="ins-tile-label">TEMPERATURE</span>
        <span class="ins-tile-val ${(cpu.temp || 46) > 70 ? 'danger' : 'green'}">${cpu.temp || 46}°C</span>
      </div>
      <div class="ins-metric-tile">
        <span class="ins-tile-label">TURBINE FAN</span>
        <span class="ins-tile-val">${cpu.fanSpeedRpm || 2400} RPM</span>
      </div>
      <div class="ins-metric-tile wide">
        <span class="ins-tile-label">HOST & KERNEL</span>
        <span class="ins-tile-val mono">${sys.hostname || 'solaris'} · ${sys.kernel || 'Linux 6.x'}</span>
      </div>
      <div class="ins-metric-tile wide">
        <span class="ins-tile-label">UPTIME</span>
        <span class="ins-tile-val green mono">ONLINE ${sys.uptime || '00:42:15'}</span>
      </div>
    `;
  }

  renderMemoryMetrics(t) {
    const mem = t?.memory || {};
    return `
      <div class="ins-metric-tile">
        <span class="ins-tile-label">RAM USAGE</span>
        <span class="ins-tile-val green">${mem.percentage || 33}%</span>
      </div>
      <div class="ins-metric-tile">
        <span class="ins-tile-label">USED MEMORY</span>
        <span class="ins-tile-val cyan">${mem.usedGb || '5.1'} GB</span>
      </div>
      <div class="ins-metric-tile">
        <span class="ins-tile-label">TOTAL CAPACITY</span>
        <span class="ins-tile-val">${mem.totalGb || '15.5'} GB</span>
      </div>
      <div class="ins-metric-tile">
        <span class="ins-tile-label">AVAILABLE</span>
        <span class="ins-tile-val">${(mem.totalGb - mem.usedGb).toFixed(1) || '10.4'} GB</span>
      </div>
      <div class="ins-metric-tile wide">
        <span class="ins-tile-label">ZRAM / SWAP BUFFER</span>
        <span class="ins-tile-val cyan mono">ALLOCATED: 4.0 GB · ACTIVE: 0.0 GB</span>
      </div>
    `;
  }

  renderStorageMetrics(t) {
    const stor = t?.storage?.root || {};
    return `
      <div class="ins-metric-tile">
        <span class="ins-tile-label">ROOT USAGE (/)</span>
        <span class="ins-tile-val green">${stor.pct || 48}%</span>
      </div>
      <div class="ins-metric-tile">
        <span class="ins-tile-label">USED DISK</span>
        <span class="ins-tile-val cyan">${stor.usedGb || '15.7'} GB</span>
      </div>
      <div class="ins-metric-tile">
        <span class="ins-tile-label">TOTAL CAPACITY</span>
        <span class="ins-tile-val">${stor.totalGb || '52.7'} GB</span>
      </div>
      <div class="ins-metric-tile">
        <span class="ins-tile-label">STATUS</span>
        <span class="ins-tile-val green">OPTIMAL NVME</span>
      </div>
      <div class="ins-metric-tile wide">
        <span class="ins-tile-label">MOUNT ARRAY</span>
        <span class="ins-tile-val mono">/dev/nvme0n1p2 -> / (btrfs/ext4)</span>
      </div>
    `;
  }

  renderNetworkMetrics(t) {
    const wifi = t?.wifi || {};
    return `
      <div class="ins-metric-tile">
        <span class="ins-tile-label">ACTIVE INTERFACE</span>
        <span class="ins-tile-val green">${wifi.currentSsid || 'SOLARIS_5G'}</span>
      </div>
      <div class="ins-metric-tile">
        <span class="ins-tile-label">SIGNAL LEVEL</span>
        <span class="ins-tile-val cyan">-38 dBm (96%)</span>
      </div>
      <div class="ins-metric-tile">
        <span class="ins-tile-label">THROUGHPUT</span>
        <span class="ins-tile-val green">↑ 14.8 kB/s · ↓ 34.8 kB/s</span>
      </div>
      <div class="ins-metric-tile">
        <span class="ins-tile-label">LATENCY PING</span>
        <span class="ins-tile-val cyan">12 ms (Cloudflare)</span>
      </div>
      <div class="ins-metric-tile wide">
        <span class="ins-tile-label">SSH GATEWAY STATUS</span>
        <span class="ins-tile-val green mono">PORT 22 · SSHD DAEMON: ACTIVE / AUTH READY</span>
      </div>
    `;
  }

  async fetchServices() {
    const container = this.panelEl.querySelector('#services-list-container');
    const dockerContainer = this.panelEl.querySelector('#docker-container-list');
    container.innerHTML = `<div class="service-loading-placeholder">Querying live systemctl units...</div>`;

    if (window.solarisAPI && window.solarisAPI.getSystemServices) {
      try {
        const res = await window.solarisAPI.getSystemServices();
        this.servicesData = res;
        this.renderServicesList(res.services);

        if (res.docker && res.docker.active && res.docker.containers.length > 0) {
          dockerContainer.innerHTML = res.docker.containers.map(c => `
            <div class="docker-row">
              <span class="doc-name green">${c.name}</span>
              <span class="doc-img">${c.image}</span>
              <span class="doc-status">${c.status}</span>
            </div>
          `).join('');
        } else {
          dockerContainer.innerHTML = `<div class="docker-empty-msg">No active Docker containers running. (Docker daemon: ${res.docker?.active ? 'ONLINE' : 'INACTIVE'})</div>`;
        }
      } catch (err) {
        container.innerHTML = `<div class="service-error-msg">Error querying services: ${err.message}</div>`;
      }
    } else {
      container.innerHTML = `<div class="service-error-msg">IPC service supervisor unavailable in demo environment.</div>`;
    }
  }

  renderServicesList(services = []) {
    const container = this.panelEl.querySelector('#services-list-container');
    if (!services || services.length === 0) {
      container.innerHTML = `<div class="service-error-msg">No monitored services found.</div>`;
      return;
    }

    container.innerHTML = services.map(s => `
      <div class="service-item-row" data-unit="${s.unit}">
        <div class="service-info-col">
          <span class="service-name">${s.name}</span>
          <span class="service-unit">${s.unit}</span>
        </div>
        <div class="service-status-col">
          <span class="service-badge ${s.active ? 'active' : 'inactive'}">${s.status}</span>
        </div>
        <div class="service-actions-col">
          ${s.active 
            ? `<button class="svc-btn restart" data-action="restart" data-service="${s.unit}" title="Restart Service">RESTART</button>
               <button class="svc-btn stop danger" data-action="stop" data-service="${s.unit}" title="Stop Service">STOP</button>`
            : `<button class="svc-btn start primary" data-action="start" data-service="${s.unit}" title="Start Service">START</button>`
          }
        </div>
      </div>
    `).join('');

    // Bind action buttons
    container.querySelectorAll('.svc-btn').forEach(btn => {
      btn.addEventListener('click', (e) => {
        const action = btn.dataset.action;
        const service = btn.dataset.service;

        if (action === 'stop' || action === 'restart') {
          // Confirm destructive actions
          this.confirmAction(service, action);
        } else {
          this.executeServiceAction(service, action);
        }
      });
    });
  }

  confirmAction(service, action) {
    const confirmModal = this.panelEl.querySelector('#inspector-confirm-modal');
    const msgEl = this.panelEl.querySelector('#confirm-modal-msg');
    this.pendingAction = { service, action };
    msgEl.textContent = `Are you sure you want to ${action.toUpperCase()} ${service}? This may temporarily affect system connectivity.`;
    confirmModal.style.display = 'flex';
  }

  async executeServiceAction(service, action) {
    const row = this.panelEl.querySelector(`.service-item-row[data-unit="${service}"]`);
    if (row) {
      row.style.opacity = '0.5';
    }

    if (window.solarisAPI && window.solarisAPI.manageService) {
      try {
        const res = await window.solarisAPI.manageService(service, action);
        if (res && res.success) {
          setTimeout(() => this.fetchServices(), 600);
        } else {
          alert(`Service operation failed: ${res ? res.error : 'Unknown error'}`);
          if (row) row.style.opacity = '1.0';
        }
      } catch (err) {
        alert(`Service error: ${err.message}`);
        if (row) row.style.opacity = '1.0';
      }
    }
  }

  hide() {
    this.panelEl.style.display = 'none';
  }
}
