// ==============================================================================
// 🪐 COMPUTATIONAL TERRAIN — State B: Alien Sci-Fi Computational Landscape
// Production Video-Driven Architecture for Solaris OS Dashboard
//
// The central visual rendering has been completely replaced by the high-budget
// Blender-rendered cinematic alien terrain video (solaris_terrain_master.mp4).
//
// All old JavaScript-generated wireframe lines, procedural cubes, glowing lasers,
// and floating particle motes have been completely removed to eliminate redundant
// CPU/GPU overhead and achieve cinematic AAA visual fidelity.
// ==============================================================================
import * as THREE from '../../node_modules/three/build/three.module.js';

export class ComputationalTerrain {
  constructor(orbitalScene) {
    this.scene = orbitalScene;
    this.group = new THREE.Group();
    this.group.visible = false;
    this.group.scale.set(0.001, 0.001, 0.001); // starts collapsed until transition

    // Authentic Tailscale SSH Remote Host State
    this.metrics = {
      hostname: 'venkat-server',
      tailscaleIp: '100.110.135.100',
      directEndpoint: '192.168.29.105:41641',
      pingMs: 3.2,
      isOnline: true,
      isLive: false,
      cpuUsage: 18,
      cpuTemp: 54,
      memPct: 41,
      diskUsage: 58,
      netKbs: 45,
      sshPort: 22
    };

    // Camera Navigation State
    this.cameraMode = 'CINEMATIC';
    this.cinematicTime = 0;
    this.focusedRegion = null;
    this.userMotionPaused = false;

    // Interactive Regions & Hotspots
    this.hotspots = [
      { id: 'cpu', name: 'COMPUTE NEXUS', bounds: { minX: -2, maxX: 2, minY: -1, maxY: 1 } },
      { id: 'ram', name: 'MEMORY MEGALITH', bounds: { minX: -5, maxX: -2, minY: -1, maxY: 1 } },
      { id: 'storage', name: 'STORAGE CANYONS', bounds: { minX: 2, maxX: 5, minY: -1, maxY: 1 } },
      { id: 'network', name: 'WIREguard MESH', bounds: { minX: -2, maxX: 2, minY: 1, maxY: 3 } }
    ];
    this.interactiveMeshes = [];
    this.hoveredRegion = null;

    // Create an invisible interaction proxy plane for hit-testing and click handling
    const hitGeo = new THREE.PlaneGeometry(8, 6);
    const hitMat = new THREE.MeshBasicMaterial({ visible: false });
    this.hitProxy = new THREE.Mesh(hitGeo, hitMat);
    this.hitProxy.name = 'server_click_proxy';
    this.group.add(this.hitProxy);
    this.interactiveMeshes.push(this.hitProxy);
  }

  // ---------------------------------------------------------------------------
  // REAL TELEMETRY MAPPING
  // ---------------------------------------------------------------------------
  setTelemetry(telemetry) {
    if (!telemetry) return;
    this.metrics.isLive = true;

    if (telemetry.system && telemetry.system.hostname) {
      this.metrics.hostname = telemetry.system.hostname;
    }
    if (telemetry.cpu) {
      if (telemetry.cpu.usage !== undefined) this.metrics.cpuUsage = telemetry.cpu.usage;
      if (telemetry.cpu.temp !== undefined) this.metrics.cpuTemp = telemetry.cpu.temp;
    }
    if (telemetry.memory && telemetry.memory.percentage !== undefined) {
      this.metrics.memPct = telemetry.memory.percentage;
    }
    if (telemetry.network && telemetry.network.rxKbs !== undefined) {
      this.metrics.netKbs = Math.round(telemetry.network.rxKbs);
    }
    if (telemetry.storage && telemetry.storage.percentage !== undefined) {
      this.metrics.diskUsage = telemetry.storage.percentage;
    }
  }

  updateTelemetry(data) {
    this.setTelemetry(data);
  }

  // ---------------------------------------------------------------------------
  // INTERACTION HANDLING
  // ---------------------------------------------------------------------------
  getHotspotAtScreenCoord(screenX, screenY, width, height) {
    const ndcX = (screenX / width) * 2 - 1;
    const ndcY = -(screenY / height) * 2 + 1;

    // Quick radial bounding check
    const r = Math.sqrt(ndcX * ndcX + ndcY * ndcY);
    if (r < 0.6) {
      return this.hotspots[0]; // Central Compute Nexus
    }
    return null;
  }

  focusRegion(regionId) {
    this.cameraMode = 'FOCUSED';
    this.focusedRegion = regionId;
  }

  resetCameraFocus() {
    this.cameraMode = 'CINEMATIC';
    this.focusedRegion = null;
  }

  // ---------------------------------------------------------------------------
  // LIGHTWEIGHT TICK (Zero redundant WebGL drawing)
  // ---------------------------------------------------------------------------
  animate(time) {
    this.cinematicTime = (time || performance.now()) * 0.001;
  }

  // ---------------------------------------------------------------------------
  // DISPOSAL & CLEANUP
  // ---------------------------------------------------------------------------
  dispose() {
    this.group.traverse(obj => {
      if (obj.geometry) obj.geometry.dispose();
      if (obj.material) {
        if (Array.isArray(obj.material)) obj.material.forEach(m => m.dispose());
        else obj.material.dispose();
      }
    });
  }
}
