// ==============================================================================
// 🛸 ORBITAL CORE — Master Controller & Three-State Interaction Pipeline
// ==============================================================================
import { OrbitalScene } from './OrbitalScene.js';
import { OrbitalPlanet } from './OrbitalPlanet.js';
import { ServerModel } from './ServerModel.js';
import { CoreTransitionController } from './CoreTransitionController.js';
import { HolographicTerminal } from './HolographicTerminal.js';
import { ServerMetricsPanel } from './ServerMetricsPanel.js';

export class OrbitalCore {
  constructor(containerElement) {
    this.container = containerElement;
    this.container.classList.add('orbital-core-root');

    // 1. Initialize 3D Scene
    this.scene = new OrbitalScene(this.container);

    // 1b. Initialize Cinematic Computational Terrain Background Video
    this.initVideoBackground();

    // 2. Initialize State A: Orbital Planet
    this.planet = new OrbitalPlanet();
    this.scene.add(this.planet.group);

    // 3. Initialize State B: Server Model
    this.server = new ServerModel();
    this.scene.add(this.server.group);

    // 4. Initialize State C: Holographic Terminal
    this.terminal = new HolographicTerminal(this.container, {
      onClose: () => {
        this.transitionController.fromTerminalToServerCore();
      }
    });

    // 5. Initialize Subsystem Inspector Drawer
    this.metricsPanel = new ServerMetricsPanel(this.container);

    // 6. Create On-Screen Controls & Contextual Hint HUD
    this.initHUD();

    // 7. Transition Controller
    this.transitionController = new CoreTransitionController(
      this.scene,
      this.planet,
      this.server,
      this.terminal,
      {
        onStateChange: (newState) => {
          this.updateHUDModeSelector(newState);
          this.handleStateChange(newState);
        }
      }
    );

    // 8. Gesture Engine (2-Finger Swipe & Drag)
    this.activePointers = new Map();
    this.swipeStartX = 0;
    this.hasDiscoveredGesture = localStorage.getItem('orbital_gesture_discovered') === 'true';

    this.bindGestures();
    this.bindHotspotInteraction();

    // 9. Start Main 60FPS RAF Render Loop
    this.clock = { start: performance.now(), last: performance.now() };
    this.isPaused = false;
    this.startLoop();

    // 10. Page visibility throttling
    document.addEventListener('visibilitychange', () => {
      this.isPaused = document.hidden;
    });
  }

  initHUD() {
    this.hudContainer = document.createElement('div');
    this.hudContainer.className = 'orbital-core-hud-overlay';
    this.hudContainer.innerHTML = `
      <!-- TOP ACCESSIBLE MODE SELECTOR -->
      <div class="core-mode-switcher-bar">
        <button class="core-mode-pill-btn active" data-mode="ORBITAL" id="mode-btn-orbital">
          <span class="mode-glyph">⬡</span>
          <span class="mode-label">ORBITAL</span>
        </button>
        <button class="core-mode-pill-btn" data-mode="SERVER_CORE" id="mode-btn-server">
          <span class="mode-glyph">🖥️</span>
          <span class="mode-label">SERVER CORE</span>
        </button>
        <button class="core-mode-pill-btn" data-mode="TERMINAL" id="mode-btn-terminal">
          <span class="mode-glyph">&gt;_</span>
          <span class="mode-label">TERMINAL</span>
        </button>
      </div>

      <!-- CONTEXTUAL HINT -->
      <div class="orbital-context-hint" id="orbital-context-hint">
        <span class="hint-icon">⚡</span>
        <span class="hint-text">2-FINGER SWIPE // ACCESS SERVER CORE</span>
      </div>

      <!-- SERVER HARDWARE QUICK INSPECT BUTTON (Visible in Server Core mode) -->
      <div class="server-quick-action-bar" id="server-quick-action-bar" style="display: none;">
        <button class="core-hud-action-btn" id="btn-open-inspector">
          <span class="action-icon">⌬</span>
          <span>HARDWARE &amp; SERVICES</span>
        </button>
        <button class="core-hud-action-btn primary" id="btn-open-terminal-direct">
          <span class="action-icon">&gt;_</span>
          <span>PROJECT TERMINAL</span>
        </button>
      </div>
    `;

    this.container.appendChild(this.hudContainer);

    // Bind Mode Buttons
    this.hudContainer.querySelectorAll('.core-mode-pill-btn').forEach(btn => {
      btn.addEventListener('click', (e) => {
        e.stopPropagation();
        const mode = btn.dataset.mode;
        this.transitionController.switchState(mode);
        this.markGestureDiscovered();
      });
    });

    // Bind Server Quick Action Buttons
    const inspectBtn = this.hudContainer.querySelector('#btn-open-inspector');
    if (inspectBtn) {
      inspectBtn.addEventListener('click', (e) => {
        e.stopPropagation();
        this.metricsPanel.showHotspot('cpu', this.lastTelemetry);
      });
    }

    const termDirectBtn = this.hudContainer.querySelector('#btn-open-terminal-direct');
    if (termDirectBtn) {
      termDirectBtn.addEventListener('click', (e) => {
        e.stopPropagation();
        this.transitionController.toTerminal();
      });
    }

    if (this.hasDiscoveredGesture) {
      const hint = this.hudContainer.querySelector('#orbital-context-hint');
      if (hint) hint.classList.add('minimized');
    }
  }

  updateHUDModeSelector(activeState) {
    this.hudContainer.querySelectorAll('.core-mode-pill-btn').forEach(b => {
      if (b.dataset.mode === activeState) b.classList.add('active');
      else b.classList.remove('active');
    });

    const serverBar = this.hudContainer.querySelector('#server-quick-action-bar');
    if (serverBar) {
      serverBar.style.display = (activeState === 'SERVER_CORE') ? 'flex' : 'none';
    }

    const hint = this.hudContainer.querySelector('#orbital-context-hint');
    if (hint) {
      if (activeState === 'ORBITAL') {
        hint.style.display = 'flex';
        hint.querySelector('.hint-text').textContent = '2-FINGER SWIPE // ACCESS SERVER CORE';
      } else if (activeState === 'SERVER_CORE') {
        hint.style.display = 'flex';
        hint.querySelector('.hint-text').textContent = 'CLICK SERVER // PROJECT TERMINAL';
      } else {
        hint.style.display = 'none';
      }
    }
  }

  markGestureDiscovered() {
    if (!this.hasDiscoveredGesture) {
      this.hasDiscoveredGesture = true;
      localStorage.setItem('orbital_gesture_discovered', 'true');
      const hint = this.hudContainer.querySelector('#orbital-context-hint');
      if (hint) {
        hint.classList.add('minimized');
      }
    }
  }

  bindGestures() {
    const target = this.scene.canvas;

    // Track pointers for touch multi-pointer detection and desktop mouse drag
    target.addEventListener('pointerdown', (e) => {
      this.activePointers.set(e.pointerId, {
        x: e.clientX,
        y: e.clientY,
        startX: e.clientX,
        startY: e.clientY
      });
    });

    target.addEventListener('pointermove', (e) => {
      if (!this.activePointers.has(e.pointerId)) return;
      const pt = this.activePointers.get(e.pointerId);
      pt.x = e.clientX;
      pt.y = e.clientY;

      // Check if 2 pointers are active (2-Finger Touchscreen Swipe)
      if (this.activePointers.size === 2) {
        const ptrs = Array.from(this.activePointers.values());
        const avgDeltaX = ((ptrs[0].x - ptrs[0].startX) + (ptrs[1].x - ptrs[1].startX)) / 2;

        if (Math.abs(avgDeltaX) > 50) {
          // Trigger switch
          this.markGestureDiscovered();
          if (avgDeltaX < 0) {
            // Swipe Left -> Advance: ORBITAL -> SERVER_CORE -> TERMINAL
            if (this.transitionController.currentState === 'ORBITAL') {
              this.transitionController.toServerCore();
            } else if (this.transitionController.currentState === 'SERVER_CORE') {
              this.transitionController.toTerminal();
            }
          } else {
            // Swipe Right -> Retreat: TERMINAL -> SERVER_CORE -> ORBITAL
            if (this.transitionController.currentState === 'TERMINAL') {
              this.transitionController.fromTerminalToServerCore();
            } else if (this.transitionController.currentState === 'SERVER_CORE') {
              this.transitionController.toOrbital();
            }
          }
          // Reset start coords to prevent repeated firing
          ptrs[0].startX = ptrs[0].x;
          ptrs[1].startX = ptrs[1].x;
        }
      }
    });

    const handlePointerRelease = (e) => {
      // Desktop mouse horizontal drag gesture detection
      if (this.activePointers.has(e.pointerId) && this.activePointers.size === 1) {
        const pt = this.activePointers.get(e.pointerId);
        const deltaX = e.clientX - pt.startX;
        const deltaY = e.clientY - pt.startY;

        if (Math.abs(deltaX) > 60 && Math.abs(deltaX) > Math.abs(deltaY) * 1.2) {
          this.markGestureDiscovered();
          if (deltaX < 0) {
            // Dragged left -> Advance: ORBITAL -> SERVER_CORE -> TERMINAL
            if (this.transitionController.currentState === 'ORBITAL') {
              this.transitionController.toServerCore();
            } else if (this.transitionController.currentState === 'SERVER_CORE') {
              this.transitionController.toTerminal();
            }
          } else {
            // Dragged right -> Retreat: TERMINAL -> SERVER_CORE -> ORBITAL
            if (this.transitionController.currentState === 'TERMINAL') {
              this.transitionController.fromTerminalToServerCore();
            } else if (this.transitionController.currentState === 'SERVER_CORE') {
              this.transitionController.toOrbital();
            }
          }
        }
      }
      this.activePointers.delete(e.pointerId);
    };

    target.addEventListener('pointerup', handlePointerRelease);
    target.addEventListener('pointercancel', handlePointerRelease);

    // Trackpad 2-finger horizontal swipe detection via wheel event
    let wheelAccumX = 0;
    let wheelDebounceTimer = null;
    let lastWheelTime = 0;
    target.addEventListener('wheel', (e) => {
      if (Math.abs(e.deltaX) > Math.abs(e.deltaY) && Math.abs(e.deltaX) > 8) {
        e.preventDefault();
        wheelAccumX += e.deltaX;
        const now = performance.now();
        if (now - lastWheelTime < 350) return; // gesture cooldown

        clearTimeout(wheelDebounceTimer);
        wheelDebounceTimer = setTimeout(() => {
          if (Math.abs(wheelAccumX) > 35) {
            lastWheelTime = performance.now();
            this.markGestureDiscovered();
            if (wheelAccumX > 0) {
              // 2-finger swipe left / scroll right -> Advance
              if (this.transitionController.currentState === 'ORBITAL') {
                this.transitionController.toServerCore();
              } else if (this.transitionController.currentState === 'SERVER_CORE') {
                this.transitionController.toTerminal();
              }
            } else {
              // 2-finger swipe right / scroll left -> Retreat
              if (this.transitionController.currentState === 'TERMINAL') {
                this.transitionController.fromTerminalToServerCore();
              } else if (this.transitionController.currentState === 'SERVER_CORE') {
                this.transitionController.toOrbital();
              }
            }
          }
          wheelAccumX = 0;
        }, 40);
      }
    }, { passive: false });

    // Keyboard navigation (Arrow keys) when focused on workspace
    window.addEventListener('keydown', (e) => {
      if (document.activeElement && (document.activeElement.tagName === 'INPUT' || document.activeElement.tagName === 'TEXTAREA')) return;
      if (this.transitionController.currentState === 'TERMINAL') return;

      if (e.key === 'ArrowRight' && this.transitionController.currentState === 'ORBITAL') {
        this.transitionController.toServerCore();
      } else if (e.key === 'ArrowLeft' && this.transitionController.currentState === 'SERVER_CORE') {
        this.transitionController.toOrbital();
      }
    });
  }

  bindHotspotInteraction() {
    // Click on canvas
    this.scene.canvas.addEventListener('click', (e) => {
      const curState = this.transitionController.currentState;

      if (curState === 'SERVER_CORE') {
        // Raycast against server hotspots
        this.scene.raycaster.setFromCamera(this.scene.pointer, this.scene.camera);
        const intersects = this.scene.raycaster.intersectObjects(this.server.hotspots);

        if (intersects.length > 0) {
          const hit = intersects[0].object;
          const hotspotId = hit.userData.id;
          this.metricsPanel.showHotspot(hotspotId, this.lastTelemetry);
        } else {
          // Clicking anywhere on the server core triggers terminal!
          const frameIntersects = this.scene.raycaster.intersectObject(this.server.frameMesh);
          if (frameIntersects.length > 0) {
            this.transitionController.toTerminal();
          }
        }
      } else if (curState === 'ORBITAL') {
        // Clicking center planet in orbital state
        this.scene.raycaster.setFromCamera(this.scene.pointer, this.scene.camera);
        const hit = this.scene.raycaster.intersectObject(this.planet.plasmaCore);
        if (hit.length > 0) {
          this.transitionController.toServerCore();
        }
      } else if (curState === 'TERMINAL') {
        // Clicking 3D server / canvas outside terminal window returns to SERVER_CORE
        this.transitionController.fromTerminalToServerCore();
      }
    });

    // Hover effect on server hotspots
    window.addEventListener('pointermove', () => {
      if (this.transitionController.currentState !== 'SERVER_CORE') {
        this.scene.canvas.style.cursor = 'default';
        return;
      }

      this.scene.raycaster.setFromCamera(this.scene.pointer, this.scene.camera);
      const intersects = this.scene.raycaster.intersectObjects(this.server.hotspots);

      if (intersects.length > 0) {
        this.scene.canvas.style.cursor = 'pointer';
        this.server.hotspots.forEach(h => {
          h.material.opacity = (h === intersects[0].object) ? 0.45 : 0.0;
        });
      } else {
        const frameIntersects = this.scene.raycaster.intersectObject(this.server.frameMesh);
        this.scene.canvas.style.cursor = frameIntersects.length > 0 ? 'pointer' : 'default';
        this.server.hotspots.forEach(h => h.material.opacity = 0.0);
      }
    });
  }

  initVideoBackground() {
    this.video = document.createElement('video');
    this.video.className = 'computational-terrain-video';
    this.video.loop = true;
    this.video.muted = true;
    this.video.playsInline = true;
    this.video.preload = 'auto';
    this.video.src = '../../assets/computational-terrain/solaris_terrain_master.mp4';
    this.video.poster = '../../assets/computational-terrain/solaris_terrain_poster.jpg';
    this.video.style.cssText = `
      position: absolute;
      inset: 0;
      width: 100%;
      height: 100%;
      object-fit: cover;
      pointer-events: none;
      opacity: 0;
      transition: opacity 0.8s cubic-bezier(0.16, 1, 0.3, 1);
      z-index: 1;
      border-radius: 50%;
      mask-image: radial-gradient(circle, black 64%, transparent 96%);
      -webkit-mask-image: radial-gradient(circle, black 64%, transparent 96%);
    `;

    if (this.scene && this.scene.canvas) {
      this.container.insertBefore(this.video, this.scene.canvas);
    } else {
      this.container.appendChild(this.video);
    }

    this.video.addEventListener('error', () => {
      console.warn('[OrbitalCore] Video playback error, falling back to static poster.');
      this.video.style.backgroundImage = "url('../../assets/computational-terrain/solaris_terrain_poster.jpg')";
      this.video.style.backgroundSize = 'cover';
    });
  }

  handleStateChange(newState) {
    if (newState === 'SERVER_CORE') {
      if (this.video) {
        this.video.style.opacity = '0.92';
        this.video.play().catch(err => console.debug('Video auto-play handled:', err));
      }
    } else if (newState === 'TERMINAL') {
      if (this.video) {
        this.video.style.opacity = '0.25';
        this.video.pause();
      }
      this.scene.userControlled = false;
    } else {
      if (this.video) {
        this.video.style.opacity = '0';
        this.video.pause();
      }
    }
  }

  updateTelemetry(data) {
    this.lastTelemetry = data;
    if (this.planet) {
      const wifi = data.wifi?.networks || [];
      const bt = data.bluetooth?.devices || [];
      this.planet.updateTargets(wifi, bt);

      // Map active Niri workspace to active station
      if (data.niri && data.niri.workspaces) {
        const activeWs = data.niri.workspaces.find(w => w.isActive || w.isFocused);
        if (activeWs) {
          this.planet.setActiveStation(activeWs.idx || 2);
        }
      }
    }

    if (this.server) {
      this.server.setTelemetry(data);
    }
  }

  startLoop() {
    const loop = (timestamp) => {
      if (this.isDestroyed) return;

      const now = timestamp * 0.001;
      const delta = Math.min(0.1, now - (this.clock.last || now));
      this.clock.last = now;

      if (!this.isPaused) {
        this.planet.update(now, delta);
        this.server.update(now, delta);
        this.scene.update(delta);
      }

      requestAnimationFrame(loop);
    };
    requestAnimationFrame(loop);
  }

  destroy() {
    this.isDestroyed = true;
    if (this.terminal && this.terminal.dispose) {
      this.terminal.dispose();
    }
    if (this.metricsPanel && this.metricsPanel.panelEl) {
      this.metricsPanel.panelEl.remove();
    }
    this.scene.dispose();
    this.planet.dispose();
    this.server.dispose();
  }
}
