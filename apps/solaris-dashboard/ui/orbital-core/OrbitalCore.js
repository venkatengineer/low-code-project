// ==============================================================================
// 🛸 ORBITAL CORE — Master 3D Holographic Planetary Command Deck
// Smooth 60FPS Planetary Orbit & Interactive Telemetry Emitter
// ==============================================================================
import { OrbitalScene } from './OrbitalScene.js';
import { OrbitalPlanet } from './OrbitalPlanet.js';
import { audioManager } from '../audio.js';

export class OrbitalCore {
  constructor(containerElement) {
    this.container = containerElement;
    this.container.classList.add('orbital-core-root');

    // 1. Initialize High-Performance 3D Scene
    this.scene = new OrbitalScene(this.container);

    // 2. Initialize Orbital Planet Hologram
    this.planet = new OrbitalPlanet();
    this.scene.add(this.planet.group);

    // 3. Create Clean Minimalist HUD Overlay (Station indicator & coordinates)
    this.initHUD();

    // 4. Interactive Raycasting & Station Selection
    this.hoveredStationId = null;
    this.bindInteractions();

    // 5. Start Main 60FPS RAF Render Loop
    this.clock = { start: performance.now(), last: performance.now() };
    this.isPaused = false;
    this.startLoop();

    // Page visibility throttling
    document.addEventListener('visibilitychange', () => {
      this.isPaused = document.hidden;
    });
  }

  initHUD() {
    this.hudContainer = document.createElement('div');
    this.hudContainer.className = 'orbital-core-hud-overlay';
    this.hudContainer.innerHTML = `
      <!-- TOP ORBITAL STATUS BADGE -->
      <div class="orbital-status-badge">
        <span class="status-pulse-dot"></span>
        <span class="status-title">ORBITAL CORE // PLANETARY RADAR ACTIVE</span>
        <span class="status-sep">|</span>
        <span class="status-fps" id="core-fps-counter">60 FPS</span>
      </div>

      <!-- BOTTOM COORDINATE TELEMETRY -->
      <div class="orbital-coords-hud">
        <div class="coord-block">
          <span class="coord-label">DECLINATION</span>
          <span class="coord-val" id="core-coord-dec">+14° 22' 08"</span>
        </div>
        <div class="coord-block">
          <span class="coord-label">RIGHT ASCENSION</span>
          <span class="coord-val" id="core-coord-ra">19h 50m 47s</span>
        </div>
        <div class="coord-block">
          <span class="coord-label">ORBIT VELOCITY</span>
          <span class="coord-val text-green" id="core-coord-vel">27,600 KM/H</span>
        </div>
      </div>
    `;

    this.container.appendChild(this.hudContainer);
  }

  bindInteractions() {
    const canvas = this.scene.canvas;
    const raycaster = this.scene.raycaster;

    // Station Hover & Click Raycasting
    canvas.addEventListener('pointermove', (e) => {
      const rect = canvas.getBoundingClientRect();
      const x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
      const y = -((e.clientY - rect.top) / rect.height) * 2 + 1;

      raycaster.setFromCamera({ x, y }, this.scene.camera);

      // Check station dots
      const targets = this.planet.stationObjects.map(o => o.dot);
      const intersects = raycaster.intersectObjects(targets, false);

      if (intersects.length > 0) {
        const hitDot = intersects[0].object;
        const stationId = hitDot.userData.stationId;
        canvas.style.cursor = 'pointer';

        if (this.hoveredStationId !== stationId) {
          this.hoveredStationId = stationId;
          audioManager.playScan();
        }
      } else {
        canvas.style.cursor = 'default';
        this.hoveredStationId = null;
      }
    });

    canvas.addEventListener('click', (e) => {
      const rect = canvas.getBoundingClientRect();
      const x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
      const y = -((e.clientY - rect.top) / rect.height) * 2 + 1;

      raycaster.setFromCamera({ x, y }, this.scene.camera);
      const targets = this.planet.stationObjects.map(o => o.dot);
      const intersects = raycaster.intersectObjects(targets, false);

      if (intersects.length > 0) {
        const hitDot = intersects[0].object;
        const stationId = hitDot.userData.stationId;
        this.planet.setActiveStation(stationId);
        audioManager.playPanels();
      }
    });
  }

  updateTelemetry(data) {
    if (!data) return;
    const fpsEl = document.getElementById('core-fps-counter');
    if (fpsEl && data.cpu) {
      // Dynamic coordinate jitter based on system telemetry
      const velEl = document.getElementById('core-coord-vel');
      if (velEl) {
        const speed = Math.round(27000 + (data.cpu.loadPercent || 15) * 45);
        velEl.textContent = `${speed.toLocaleString()} KM/H`;
      }
    }
  }

  startLoop() {
    let frameTimes = [];
    const fpsEl = document.getElementById('core-fps-counter');

    const animate = (timestamp) => {
      this.rafId = requestAnimationFrame(animate);

      if (this.isPaused) return;

      const now = performance.now();
      const delta = Math.min((now - this.clock.last) * 0.001, 0.1);
      const time = (now - this.clock.start) * 0.001;
      this.clock.last = now;

      // Track smoothed FPS
      frameTimes.push(now);
      if (frameTimes.length > 30) {
        frameTimes.shift();
        const fps = Math.round(1000 / ((now - frameTimes[0]) / frameTimes.length));
        if (fpsEl && (Math.random() < 0.05)) {
          fpsEl.textContent = `${fps} FPS`;
        }
      }

      // Update 3D Planetary Simulation
      this.planet.update(time, delta);

      // Render 3D Scene
      this.scene.render();
    };

    this.rafId = requestAnimationFrame(animate);
  }

  destroy() {
    if (this.rafId) cancelAnimationFrame(this.rafId);
    if (this.planet) this.planet.dispose();
    if (this.scene) this.scene.dispose();
  }
}
