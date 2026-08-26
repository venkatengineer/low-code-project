// ==============================================================================
// 🛸 SOLARIS SPACECRAFT COMMAND BRIDGE — DASHBOARD ENGINE
// 60FPS Holographic 3D Canvas Rendering · Real Linux Telemetry Stream
// ==============================================================================

(function() {
  'use strict';

  // State Buffers
  const historyCPU = new Array(30).fill(15);
  const historyGPU = new Array(20).fill(5);
  const historyMEM = new Array(20).fill(30);

  let currentTelemetry = null;
  let radarAngle = 0;
  let wifiRadarAngle = 0;
  let btSonarRadius = 0;
  let shipEngineThrusterTime = 0;
  let oscPhase = 0;

  // Radar target nodes (populated dynamically by Wi-Fi & Bluetooth devices)
  let radarTargets = [
    { angle: 0.8, dist: 0.45, label: 'SOLARIS_5G', type: 'wifi', ping: 1 },
    { angle: 2.1, dist: 0.65, label: 'WH-1000XM4', type: 'bt', ping: 0.8 },
    { angle: 3.7, dist: 0.35, label: 'Nexus_7_Pro', type: 'wifi', ping: 0.5 },
    { angle: 5.2, dist: 0.75, label: 'AirPods Pro', type: 'bt', ping: 0.3 }
  ];

  // ---------------------------------------------------------------------------
  // 1. CLOCK & DATE ENGINE
  // ---------------------------------------------------------------------------
  function updateClock() {
    const now = new Date();
    const timeEl = document.getElementById('top-clock-time');
    const dateEl = document.getElementById('top-clock-date');
    if (timeEl) {
      const hh = String(now.getHours()).padStart(2, '0');
      const mm = String(now.getMinutes()).padStart(2, '0');
      const ss = String(now.getSeconds()).padStart(2, '0');
      timeEl.textContent = `${hh}:${mm}:${ss}`;
    }
    if (dateEl) {
      const days = ['SUN', 'MON', 'TUE', 'WED', 'THU', 'FRI', 'SAT'];
      const months = ['JAN', 'FEB', 'MAR', 'APR', 'MAY', 'JUN', 'JUL', 'AUG', 'SEP', 'OCT', 'NOV', 'DEC'];
      const dName = days[now.getDay()];
      const dNum = now.getDate();
      const mName = months[now.getMonth()];
      const yNum = now.getFullYear();
      dateEl.textContent = `${dName}, ${dNum} ${mName} ${yNum}`;
    }
  }
  setInterval(updateClock, 1000);
  updateClock();

  // ---------------------------------------------------------------------------
  // 2. CANVAS ANIMATION ENGINE: 3D HOLOGRAPHIC RADAR
  // ---------------------------------------------------------------------------
  const canvasRadar = document.getElementById('canvas-3d-radar');
  const ctxRadar = canvasRadar ? canvasRadar.getContext('2d') : null;

  function draw3DRadar() {
    if (!ctxRadar) return;
    const w = canvasRadar.width;
    const h = canvasRadar.height;
    const cx = w / 2;
    const cy = h / 2;
    const maxR = w * 0.44;

    ctxRadar.clearRect(0, 0, w, h);

    // Save context for perspective tilt
    ctxRadar.save();
    
    // 3D Perspective Tilt Transformation
    ctxRadar.translate(cx, cy);
    ctxRadar.scale(1.0, 0.78); // Perspective compression for 3D deck look

    // Outer Glow Ring
    ctxRadar.beginPath();
    ctxRadar.arc(0, 0, maxR, 0, Math.PI * 2);
    ctxRadar.strokeStyle = 'rgba(0, 255, 102, 0.4)';
    ctxRadar.lineWidth = 2;
    ctxRadar.stroke();

    // Concentric Range Rings
    const rings = [0.25, 0.5, 0.75, 1.0];
    rings.forEach((rRatio, idx) => {
      const r = maxR * rRatio;
      ctxRadar.beginPath();
      ctxRadar.arc(0, 0, r, 0, Math.PI * 2);
      ctxRadar.strokeStyle = idx === 3 ? 'rgba(0, 255, 102, 0.5)' : 'rgba(0, 255, 102, 0.18)';
      ctxRadar.lineWidth = 1;
      ctxRadar.setLineDash(idx === 1 ? [4, 4] : []);
      ctxRadar.stroke();
      ctxRadar.setLineDash([]);

      // Distance tags
      ctxRadar.fillStyle = 'rgba(0, 255, 102, 0.45)';
      ctxRadar.font = '8px "JetBrains Mono"';
      ctxRadar.fillText(`${Math.round(rRatio * 50)}M`, 4, -r + 10);
    });

    // Crosshairs & Polar Grid Lines
    ctxRadar.beginPath();
    ctxRadar.moveTo(-maxR, 0);
    ctxRadar.lineTo(maxR, 0);
    ctxRadar.moveTo(0, -maxR);
    ctxRadar.lineTo(0, maxR);
    ctxRadar.strokeStyle = 'rgba(0, 255, 102, 0.15)';
    ctxRadar.lineWidth = 1;
    ctxRadar.stroke();

    // 45-degree diagonals
    for (let a = Math.PI / 4; a < Math.PI * 2; a += Math.PI / 2) {
      ctxRadar.beginPath();
      ctxRadar.moveTo(0, 0);
      ctxRadar.lineTo(Math.cos(a) * maxR, Math.sin(a) * maxR);
      ctxRadar.strokeStyle = 'rgba(0, 255, 102, 0.08)';
      ctxRadar.stroke();
    }

    // Sweeping Radar Beam with Phosphor Gradient Trail
    radarAngle += 0.025;
    if (radarAngle > Math.PI * 2) radarAngle -= Math.PI * 2;

    const trailSegments = 30;
    for (let i = 0; i < trailSegments; i++) {
      const a1 = radarAngle - (i * 0.02);
      const a2 = radarAngle - ((i + 1) * 0.02);
      const alpha = (1 - (i / trailSegments)) * 0.28;

      ctxRadar.beginPath();
      ctxRadar.moveTo(0, 0);
      ctxRadar.arc(0, 0, maxR, a2, a1);
      ctxRadar.closePath();
      ctxRadar.fillStyle = `rgba(0, 255, 102, ${alpha})`;
      ctxRadar.fill();
    }

    // Bright Leading Sweep Line
    ctxRadar.beginPath();
    ctxRadar.moveTo(0, 0);
    ctxRadar.lineTo(Math.cos(radarAngle) * maxR, Math.sin(radarAngle) * maxR);
    ctxRadar.strokeStyle = '#00ff66';
    ctxRadar.lineWidth = 1.5;
    ctxRadar.shadowColor = '#00ff66';
    ctxRadar.shadowBlur = 8;
    ctxRadar.stroke();
    ctxRadar.shadowBlur = 0;

    // Render Detected Target Blips
    radarTargets.forEach(target => {
      const tx = Math.cos(target.angle) * (maxR * target.dist);
      const ty = Math.sin(target.angle) * (maxR * target.dist);

      // Check angle distance to sweep line for flash ping
      let angleDiff = Math.abs(radarAngle - target.angle);
      if (angleDiff > Math.PI) angleDiff = (Math.PI * 2) - angleDiff;
      if (angleDiff < 0.15) {
        target.ping = 1.0;
      } else {
        target.ping = Math.max(0.2, target.ping - 0.01);
      }

      // Draw blip dot
      ctxRadar.beginPath();
      ctxRadar.arc(tx, ty, 3, 0, Math.PI * 2);
      ctxRadar.fillStyle = target.type === 'wifi' ? '#00ff66' : '#00f0ff';
      ctxRadar.shadowColor = target.type === 'wifi' ? '#00ff66' : '#00f0ff';
      ctxRadar.shadowBlur = 6 * target.ping;
      ctxRadar.fill();
      ctxRadar.shadowBlur = 0;

      // Pulsing Target Reticle
      if (target.ping > 0.4) {
        ctxRadar.beginPath();
        ctxRadar.arc(tx, ty, 6 + (1 - target.ping) * 8, 0, Math.PI * 2);
        ctxRadar.strokeStyle = target.type === 'wifi' ? `rgba(0, 255, 102, ${target.ping * 0.6})` : `rgba(0, 240, 255, ${target.ping * 0.6})`;
        ctxRadar.lineWidth = 1;
        ctxRadar.stroke();
      }

      // Target Label
      ctxRadar.fillStyle = `rgba(220, 245, 240, ${Math.min(1, target.ping + 0.3)})`;
      ctxRadar.font = '8px "JetBrains Mono"';
      ctxRadar.fillText(target.label, tx + 6, ty + 2);
    });

    // Center Coordinate Reticle
    ctxRadar.beginPath();
    ctxRadar.arc(0, 0, 4, 0, Math.PI * 2);
    ctxRadar.fillStyle = '#00ff66';
    ctxRadar.fill();

    ctxRadar.restore();
  }

  // ---------------------------------------------------------------------------
  // 3. CANVAS ANIMATION: WI-FI MINI RADAR
  // ---------------------------------------------------------------------------
  const canvasWifi = document.getElementById('canvas-wifi-radar');
  const ctxWifi = canvasWifi ? canvasWifi.getContext('2d') : null;

  function drawWifiRadar() {
    if (!ctxWifi) return;
    const w = canvasWifi.width;
    const h = canvasWifi.height;
    const cx = w / 2;
    const cy = h / 2;
    const maxR = w * 0.42;

    ctxWifi.clearRect(0, 0, w, h);

    // Rings
    [0.33, 0.66, 1.0].forEach(r => {
      ctxWifi.beginPath();
      ctxWifi.arc(cx, cy, maxR * r, 0, Math.PI * 2);
      ctxWifi.strokeStyle = 'rgba(0, 255, 102, 0.2)';
      ctxWifi.lineWidth = 1;
      ctxWifi.stroke();
    });

    // Crosshairs
    ctxWifi.beginPath();
    ctxWifi.moveTo(cx - maxR, cy);
    ctxWifi.lineTo(cx + maxR, cy);
    ctxWifi.moveTo(cx, cy - maxR);
    ctxWifi.lineTo(cx, cy + maxR);
    ctxWifi.strokeStyle = 'rgba(0, 255, 102, 0.15)';
    ctxWifi.stroke();

    // Sweeping beam
    wifiRadarAngle += 0.04;
    if (wifiRadarAngle > Math.PI * 2) wifiRadarAngle -= Math.PI * 2;

    ctxWifi.beginPath();
    ctxWifi.moveTo(cx, cy);
    ctxWifi.arc(cx, cy, maxR, wifiRadarAngle - 0.5, wifiRadarAngle);
    ctxWifi.closePath();
    ctxWifi.fillStyle = 'rgba(0, 255, 102, 0.18)';
    ctxWifi.fill();

    ctxWifi.beginPath();
    ctxWifi.moveTo(cx, cy);
    ctxWifi.lineTo(cx + Math.cos(wifiRadarAngle) * maxR, cy + Math.sin(wifiRadarAngle) * maxR);
    ctxWifi.strokeStyle = '#00ff66';
    ctxWifi.lineWidth = 1.5;
    ctxWifi.stroke();

    // Target blips
    ctxWifi.fillStyle = '#00ff66';
    ctxWifi.beginPath();
    ctxWifi.arc(cx + 16, cy - 20, 2.5, 0, Math.PI * 2);
    ctxWifi.arc(cx - 24, cy + 12, 2.5, 0, Math.PI * 2);
    ctxWifi.arc(cx + 10, cy + 26, 2, 0, Math.PI * 2);
    ctxWifi.fill();
  }

  // ---------------------------------------------------------------------------
  // 4. CANVAS ANIMATION: BLUETOOTH SONAR RADAR
  // ---------------------------------------------------------------------------
  const canvasBt = document.getElementById('canvas-bt-radar');
  const ctxBt = canvasBt ? canvasBt.getContext('2d') : null;

  function drawBtRadar() {
    if (!ctxBt) return;
    const w = canvasBt.width;
    const h = canvasBt.height;
    const cx = w / 2;
    const cy = h / 2;
    const maxR = w * 0.42;

    ctxBt.clearRect(0, 0, w, h);

    // Sonar pulse wave
    btSonarRadius += 0.4;
    if (btSonarRadius > maxR) btSonarRadius = 0;

    ctxBt.beginPath();
    ctxBt.arc(cx, cy, btSonarRadius, 0, Math.PI * 2);
    ctxBt.strokeStyle = `rgba(0, 240, 255, ${1 - (btSonarRadius / maxR)})`;
    ctxBt.lineWidth = 1.5;
    ctxBt.stroke();

    // Outer boundary
    ctxBt.beginPath();
    ctxBt.arc(cx, cy, maxR, 0, Math.PI * 2);
    ctxBt.strokeStyle = 'rgba(0, 240, 255, 0.25)';
    ctxBt.lineWidth = 1;
    ctxBt.stroke();

    // Bluetooth Center Glyph
    ctxBt.strokeStyle = '#00f0ff';
    ctxBt.lineWidth = 1.5;
    ctxBt.beginPath();
    ctxBt.moveTo(cx, cy - 14);
    ctxBt.lineTo(cx, cy + 14);
    ctxBt.lineTo(cx + 8, cy + 7);
    ctxBt.lineTo(cx - 8, cy - 7);
    ctxBt.moveTo(cx - 8, cy + 7);
    ctxBt.lineTo(cx + 8, cy - 7);
    ctxBt.lineTo(cx, cy - 14);
    ctxBt.stroke();

    // Device dots
    ctxBt.fillStyle = '#00f0ff';
    ctxBt.beginPath();
    ctxBt.arc(cx - 20, cy - 16, 2.5, 0, Math.PI * 2);
    ctxBt.arc(cx + 25, cy + 14, 2.5, 0, Math.PI * 2);
    ctxBt.fill();
  }

  // ---------------------------------------------------------------------------
  // 5. CANVAS ANIMATION: 3D SPACESHIP WIREFRAME WITH PARTICLE THRUSTER
  // ---------------------------------------------------------------------------
  const canvasShip = document.getElementById('canvas-ship-thruster');
  const ctxShip = canvasShip ? canvasShip.getContext('2d') : null;

  function drawSpaceship() {
    if (!ctxShip) return;
    const w = canvasShip.width;
    const h = canvasShip.height;
    ctxShip.clearRect(0, 0, w, h);

    shipEngineThrusterTime += 0.05;

    ctxShip.save();
    ctxShip.translate(w / 2 - 10, h / 2);
    ctxShip.rotate(-Math.PI / 12); // Sleek angle

    // Glowing Purple/Violet Plasma Thruster Plume
    const plumeLength = 28 + Math.sin(shipEngineThrusterTime * 6) * 6;
    const grad = ctxShip.createLinearGradient(0, 0, -plumeLength, 0);
    grad.addColorStop(0, '#00ffff');
    grad.addColorStop(0.3, '#a855f7');
    grad.addColorStop(1, 'rgba(168, 85, 247, 0)');

    ctxShip.beginPath();
    ctxShip.moveTo(-22, -6);
    ctxShip.lineTo(-22 - plumeLength, 0);
    ctxShip.lineTo(-22, 6);
    ctxShip.closePath();
    ctxShip.fillStyle = grad;
    ctxShip.shadowColor = '#a855f7';
    ctxShip.shadowBlur = 10;
    ctxShip.fill();
    ctxShip.shadowBlur = 0;

    // Spaceship Hull Wireframe (Aerospace Interceptor Geometry)
    ctxShip.strokeStyle = '#00f0ff';
    ctxShip.lineWidth = 1.2;
    ctxShip.fillStyle = 'rgba(10, 25, 40, 0.7)';

    ctxShip.beginPath();
    // Nose tip
    ctxShip.moveTo(35, 0);
    // Upper cockpit & wing
    ctxShip.lineTo(8, -8);
    ctxShip.lineTo(-12, -22);
    ctxShip.lineTo(-18, -20);
    ctxShip.lineTo(-14, -8);
    ctxShip.lineTo(-22, -6);
    // Engine base
    ctxShip.lineTo(-22, 6);
    // Lower wing
    ctxShip.lineTo(-14, 8);
    ctxShip.lineTo(-18, 20);
    ctxShip.lineTo(-12, 22);
    ctxShip.lineTo(8, 8);
    ctxShip.closePath();
    ctxShip.fill();
    ctxShip.stroke();

    // Cockpit canopy glass
    ctxShip.beginPath();
    ctxShip.moveTo(18, 0);
    ctxShip.lineTo(6, -3);
    ctxShip.lineTo(-2, 0);
    ctxShip.lineTo(6, 3);
    ctxShip.closePath();
    ctxShip.fillStyle = 'rgba(0, 255, 102, 0.4)';
    ctxShip.strokeStyle = '#00ff66';
    ctxShip.fill();
    ctxShip.stroke();

    ctxShip.restore();
  }

  // ---------------------------------------------------------------------------
  // 6. CANVAS ANIMATION: TELEMETRY OSCILLOSCOPE WAVEFORM
  // ---------------------------------------------------------------------------
  const canvasWave = document.getElementById('canvas-telemetry-waveform');
  const ctxWave = canvasWave ? canvasWave.getContext('2d') : null;

  function drawOscilloscope() {
    if (!ctxWave) return;
    const w = canvasWave.width;
    const h = canvasWave.height;
    const cy = h / 2;

    ctxWave.clearRect(0, 0, w, h);
    oscPhase += 0.08;

    // Grid lines
    ctxWave.strokeStyle = 'rgba(0, 255, 102, 0.1)';
    ctxWave.lineWidth = 1;
    ctxWave.beginPath();
    ctxWave.moveTo(0, cy);
    ctxWave.lineTo(w, cy);
    ctxWave.stroke();

    // Primary Green Telemetry Wave
    ctxWave.beginPath();
    ctxWave.moveTo(0, cy);
    for (let x = 0; x < w; x++) {
      const y = cy + Math.sin(x * 0.08 + oscPhase) * 16 * Math.cos(x * 0.03);
      ctxWave.lineTo(x, y);
    }
    ctxWave.strokeStyle = '#00ff66';
    ctxWave.lineWidth = 1.5;
    ctxWave.shadowColor = '#00ff66';
    ctxWave.shadowBlur = 4;
    ctxWave.stroke();
    ctxWave.shadowBlur = 0;
  }

  // ---------------------------------------------------------------------------
  // 7. SPARKLINE GRAPH RENDERERS
  // ---------------------------------------------------------------------------
  function drawSparkline(canvasId, data, color) {
    const canvas = document.getElementById(canvasId);
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const w = canvas.width;
    const h = canvas.height;

    ctx.clearRect(0, 0, w, h);
    if (data.length < 2) return;

    const step = w / (data.length - 1);
    ctx.beginPath();
    ctx.moveTo(0, h - (data[0] / 100) * (h - 4) - 2);

    for (let i = 1; i < data.length; i++) {
      const x = i * step;
      const y = h - (data[i] / 100) * (h - 4) - 2;
      ctx.lineTo(x, y);
    }

    ctx.strokeStyle = color;
    ctx.lineWidth = 1.5;
    ctx.stroke();
  }

  // Global 60FPS Render Loop
  function animationLoop() {
    draw3DRadar();
    drawWifiRadar();
    drawBtRadar();
    drawSpaceship();
    drawOscilloscope();
    requestAnimationFrame(animationLoop);
  }
  requestAnimationFrame(animationLoop);

  // ---------------------------------------------------------------------------
  // 8. REAL TELEMETRY STREAM HANDLER
  // ---------------------------------------------------------------------------
  function updateTelemetryUI(data) {
    if (!data) return;
    currentTelemetry = data;

    // --- CPU TELEMETRY ---
    if (data.cpu) {
      const cpuVal = data.cpu.usage || 0;
      historyCPU.shift();
      historyCPU.push(cpuVal);
      drawSparkline('canvas-cpu-sparkline', historyCPU, '#00ff66');

      const pctEl = document.getElementById('val-cpu-pct');
      if (pctEl) pctEl.textContent = `${cpuVal}%`;

      const ghzEl = document.getElementById('val-cpu-ghz');
      if (ghzEl) ghzEl.textContent = `${data.cpu.avgFreqGhz || '3.42'} GHz`;

      // Speedometer needle and circular arc
      const arcEl = document.getElementById('svg-cpu-arc');
      if (arcEl) {
        // max dasharray is 270
        const offset = 270 - (cpuVal / 100) * 270;
        arcEl.style.strokeDashoffset = offset;
      }

      // Center bottom speedometer
      const engineGhzEl = document.getElementById('val-engine-ghz');
      if (engineGhzEl) engineGhzEl.textContent = data.cpu.avgFreqGhz || '3.42';

      const speedFillEl = document.getElementById('svg-speed-fill');
      if (speedFillEl) {
        const ghzNum = parseFloat(data.cpu.avgFreqGhz) || 3.42;
        const fillOffset = 220 - Math.min(1, ghzNum / 5.0) * 220;
        speedFillEl.style.strokeDashoffset = fillOffset;
      }

      // Environment Panel
      const envTempEl = document.getElementById('val-env-temp');
      if (envTempEl) envTempEl.textContent = `${data.cpu.temp || 46}°C`;

      const envFanEl = document.getElementById('val-env-fan');
      if (envFanEl) envFanEl.textContent = `${data.cpu.fanSpeedRpm || 2400} RPM`;
    }

    // --- MEMORY TELEMETRY ---
    if (data.memory) {
      const memPct = data.memory.percentage || 0;
      historyMEM.shift();
      historyMEM.push(memPct);
      drawSparkline('canvas-mem-sparkline', historyMEM, '#a855f7');

      const memPctEl = document.getElementById('val-mem-pct');
      if (memPctEl) memPctEl.textContent = `${memPct}%`;

      const memSubPctEl = document.getElementById('val-mem-sub-pct');
      if (memSubPctEl) memSubPctEl.textContent = `${memPct}%`;

      const memGbEl = document.getElementById('val-mem-gb');
      if (memGbEl) memGbEl.textContent = `${data.memory.usedGb} / ${data.memory.totalGb} GB`;

      const memArc = document.getElementById('svg-mem-arc');
      if (memArc) {
        const offset = 240 - (memPct / 100) * 240;
        memArc.style.strokeDashoffset = offset;
      }
    }

    // --- GPU TELEMETRY ---
    if (data.gpu) {
      const gpuPct = data.gpu.utilization || 7;
      historyGPU.shift();
      historyGPU.push(gpuPct);
      drawSparkline('canvas-gpu-sparkline', historyGPU, '#00ff66');

      const gpuPctEl = document.getElementById('val-gpu-pct');
      if (gpuPctEl) gpuPctEl.textContent = `${gpuPct}%`;

      const gpuSubPctEl = document.getElementById('val-gpu-sub-pct');
      if (gpuSubPctEl) gpuSubPctEl.textContent = `${gpuPct}%`;

      const gpuModelEl = document.getElementById('val-gpu-model');
      if (gpuModelEl) gpuModelEl.textContent = data.gpu.name || 'NVIDIA RTX 5050';

      const gpuTempEl = document.getElementById('val-gpu-temp');
      if (gpuTempEl) gpuTempEl.textContent = `${data.gpu.temp || 42}°C`;

      const gpuVramEl = document.getElementById('val-gpu-vram');
      if (gpuVramEl) gpuVramEl.textContent = `${data.gpu.memUsedGb || '1.3'} / ${data.gpu.memTotalGb || '8.0'} GB`;

      const gpuArc = document.getElementById('svg-gpu-arc');
      if (gpuArc) {
        const offset = 240 - (gpuPct / 100) * 240;
        gpuArc.style.strokeDashoffset = offset;
      }
    }

    // --- STORAGE DISKS ---
    if (data.storage && data.storage.disks) {
      const rootDisk = data.storage.root || data.storage.disks[0];
      if (rootDisk) {
        const pctEl = document.getElementById('val-disk1-pct');
        if (pctEl) pctEl.textContent = `${rootDisk.pct}%`;

        const subEl = document.getElementById('val-disk1-sub');
        if (subEl) subEl.textContent = `${rootDisk.usedGb} GB / ${rootDisk.totalGb} GB`;

        const barEl = document.getElementById('bar-disk1');
        if (barEl) barEl.style.width = `${rootDisk.pct}%`;
      }
    }

    // --- WI-FI SCANNER NETWORKS ---
    if (data.wifi && data.wifi.networks) {
      const container = document.getElementById('wifi-networks-container');
      if (container) {
        container.innerHTML = '';
        data.wifi.networks.slice(0, 4).forEach((net, idx) => {
          const row = document.createElement('div');
          row.className = `holo-net-row ${idx === 0 ? 'active' : ''}`;
          row.innerHTML = `
            <span class="net-ssid">${net.ssid}</span>
            <span class="net-signal ${idx === 0 ? 'green' : ''}">${net.dbm}</span>
          `;
          container.appendChild(row);
        });

        // Update Radar Targets with live SSIDs
        radarTargets = data.wifi.networks.slice(0, 4).map((net, i) => ({
          angle: 0.5 + i * 1.5,
          dist: 0.35 + (100 - net.signal) * 0.005,
          label: net.ssid,
          type: 'wifi',
          ping: 1.0
        }));
      }
    }

    // --- BLUETOOTH DEVICES ---
    if (data.bluetooth && data.bluetooth.devices) {
      const container = document.getElementById('bt-devices-container');
      if (container) {
        container.innerHTML = '';
        data.bluetooth.devices.slice(0, 4).forEach((dev, idx) => {
          const row = document.createElement('div');
          row.className = `holo-net-row ${idx === 0 ? 'active' : ''}`;
          row.innerHTML = `
            <span class="net-ssid">${dev.name}</span>
            <span class="net-signal ${idx === 0 ? 'green' : ''}">${dev.dbm}</span>
          `;
          container.appendChild(row);

          // Append to radar
          radarTargets.push({
            angle: 1.2 + idx * 1.4,
            dist: 0.55 + idx * 0.08,
            label: dev.name,
            type: 'bt',
            ping: 0.8
          });
        });
      }
    }

    // --- POWER CORE ---
    if (data.power) {
      const pwrPct = data.power.capacity || 72;
      const pwrPctEl = document.getElementById('val-power-pct');
      if (pwrPctEl) pwrPctEl.textContent = `${pwrPct}%`;

      const pwrStateEl = document.getElementById('val-power-state');
      if (pwrStateEl) pwrStateEl.textContent = data.power.status || 'Discharging';

      const acEl = document.getElementById('val-ac-status');
      if (acEl) acEl.textContent = `AC Adapter: ${data.power.acOnline ? 'Connected' : 'Disconnected'}`;

      const batHealthEl = document.getElementById('val-bat-health');
      if (batHealthEl) batHealthEl.textContent = data.power.health || 'GOOD';

      const pwrArc = document.getElementById('svg-power-arc');
      if (pwrArc) {
        const offset = 270 - (pwrPct / 100) * 270;
        pwrArc.style.strokeDashoffset = offset;
      }

      // Active Profile Pill
      const curProfile = (data.power.currentProfile || 'BALANCED').toLowerCase();
      document.querySelectorAll('.prof-pill-btn').forEach(btn => {
        if (btn.dataset.profile === curProfile) {
          btn.classList.add('active');
        } else {
          btn.classList.remove('active');
        }
      });
    }

    // --- NIRI WORKSPACES ---
    if (data.niri && data.niri.workspaces) {
      const wsContainer = document.getElementById('workspaces-list-container');
      if (wsContainer) {
        wsContainer.innerHTML = '';
        data.niri.workspaces.slice(0, 5).forEach(ws => {
          const row = document.createElement('div');
          row.className = `workspace-channel-row ${ws.isActive ? 'active' : ''}`;
          row.dataset.wsId = ws.id;
          row.innerHTML = `
            <span class="ws-num">0${ws.idx}</span>
            <span class="ws-name">${ws.label}</span>
            <span class="ws-indicator ${ws.isActive ? 'active' : ''}">${ws.isActive ? '●' : '○'}</span>
          `;
          row.addEventListener('click', () => {
            if (window.solarisAPI) {
              window.solarisAPI.performAction('focus-workspace', { id: ws.id });
            }
          });
          wsContainer.appendChild(row);
        });

        const winCountEl = document.getElementById('val-niri-windows');
        if (winCountEl) winCountEl.textContent = `ACTIVE: ${data.niri.totalWindows || 1} WINDOWS`;
      }
    }

    // --- SYSTEM UPTIME ---
    if (data.system && data.system.uptime) {
      const upEl = document.getElementById('dock-stat-uptime');
      if (upEl) upEl.textContent = `UPTIME ${data.system.uptime}`;
    }
  }

  // Hook into Electron Telemetry Stream
  if (window.solarisAPI) {
    window.solarisAPI.onTelemetry((data) => {
      updateTelemetryUI(data);
    });
    window.solarisAPI.getInitialTelemetry().then(updateTelemetryUI);
  }

  // ---------------------------------------------------------------------------
  // 9. ACTION EVENT BINDINGS
  // ---------------------------------------------------------------------------
  function bindAction(btnId, action, payload = {}) {
    const el = document.getElementById(btnId);
    if (el) {
      el.addEventListener('click', () => {
        if (window.solarisAPI) {
          window.solarisAPI.performAction(action, payload);
        }
      });
    }
  }

  // Top Dock Launcher Buttons
  bindAction('dock-btn-files', 'launch-app', { exec: 'kitty -e yazi || nautilus || thunar' });
  bindAction('dock-btn-terminal', 'launch-app', { exec: 'kitty' });
  bindAction('dock-btn-browser', 'launch-app', { exec: 'vivaldi-stable || firefox || chromium' });
  bindAction('dock-btn-settings', 'launch-app', { exec: 'fuzzel' });

  // Quick Access Buttons
  bindAction('qa-btn-lock', 'lock-screen');
  bindAction('qa-btn-reboot', 'system-reboot');
  bindAction('qa-btn-shutdown', 'system-shutdown');
  bindAction('qa-btn-logout', 'system-logout');
  bindAction('qa-btn-notes', 'launch-app', { exec: 'kitty -e nano ~/notes.txt' });
  bindAction('qa-btn-calc', 'launch-app', { exec: 'fuzzel || kitty' });
  bindAction('qa-btn-screenshot', 'take-screenshot');
  bindAction('qa-btn-files', 'launch-app', { exec: 'nautilus || thunar || kitty -e yazi' });

  // Top Rail Buttons
  bindAction('btn-top-power', 'lock-screen');
  bindAction('btn-top-audio', 'toggle-mute');

  // Power Profile Buttons
  document.querySelectorAll('.prof-pill-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const prof = btn.dataset.profile;
      if (window.solarisAPI && prof) {
        window.solarisAPI.performAction('set-power-profile', { profile: prof });
        document.querySelectorAll('.prof-pill-btn').forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
      }
    });
  });

  // Speedometer Rev Interactivity
  const cpuSpeedo = document.getElementById('mod-system-engine');
  if (cpuSpeedo) {
    cpuSpeedo.style.cursor = 'pointer';
    cpuSpeedo.addEventListener('click', () => {
      const arcEl = document.getElementById('svg-cpu-arc');
      if (arcEl) {
        arcEl.style.transition = 'stroke-dashoffset 0.4s ease';
        arcEl.style.strokeDashoffset = '0';
        setTimeout(() => {
          arcEl.style.strokeDashoffset = '200';
          setTimeout(() => {
            arcEl.style.transition = '';
          }, 400);
        }, 500);
      }
    });
  }

})();
