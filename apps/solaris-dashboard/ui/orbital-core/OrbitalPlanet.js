// ==============================================================================
// 🪐 ORBITAL PLANET — State A: 3D Geodesic Wireframe Planet & Radar System
// ==============================================================================
import * as THREE from '../../node_modules/three/build/three.module.js';

export class OrbitalPlanet {
  constructor() {
    this.group = new THREE.Group();
    this.radarAngle = 0;
    this.targetBlips = [];
    this.dissolveFactor = 0;

    // Station definition (1: CMD, 2: ENG, 3: NAV, 4: COM, 5: SCI, 6: AUX)
    this.stations = [
      { id: 1, code: '01 CMD', label: 'COMMAND', radius: 4.2, angle: 0.2, speed: 0.18, color: 0x00f0ff },
      { id: 2, code: '02 ENG', label: 'ENGINEERING', radius: 4.8, angle: 1.6, speed: 0.14, color: 0x00ff66, active: true },
      { id: 3, code: '03 NAV', label: 'NAVIGATION', radius: 5.4, angle: 3.1, speed: 0.11, color: 0x00f0ff },
      { id: 4, code: '04 COM', label: 'COMMS', radius: 4.2, angle: 4.2, speed: 0.16, color: 0x00f0ff },
      { id: 5, code: '05 SCI', label: 'SCIENCE', radius: 4.8, angle: 5.0, speed: 0.13, color: 0x00f0ff },
      { id: 6, code: '06 AUX', label: 'AUXILIARY', radius: 5.4, angle: 6.0, speed: 0.10, color: 0x00f0ff }
    ];

    this.initPlanet();
    this.initOrbitalRings();
    this.initStationNodes();
    this.initRadarGrid();
    this.initRadarSweep();
  }

  initPlanet() {
    this.planetGroup = new THREE.Group();
    this.group.add(this.planetGroup);

    // 1. Geodesic Icosahedron Wireframe
    const icoGeom = new THREE.IcosahedronGeometry(1.9, 1);
    const wireframeGeom = new THREE.WireframeGeometry(icoGeom);
    this.planetWireMat = new THREE.LineBasicMaterial({
      color: 0x00f0ff,
      transparent: true,
      opacity: 0.75,
      linewidth: 1.5
    });
    this.planetWire = new THREE.LineSegments(wireframeGeom, this.planetWireMat);
    this.planetGroup.add(this.planetWire);

    // 2. Second nested inner wireframe layer (Dual geodesic cage)
    const innerIcoGeom = new THREE.IcosahedronGeometry(1.4, 1);
    const innerWireGeom = new THREE.WireframeGeometry(innerIcoGeom);
    this.innerWireMat = new THREE.LineBasicMaterial({
      color: 0x00ff66,
      transparent: true,
      opacity: 0.45
    });
    this.innerWire = new THREE.LineSegments(innerWireGeom, this.innerWireMat);
    this.planetGroup.add(this.innerWire);

    // 3. Glowing Inner Plasma Sphere
    const sphereGeom = new THREE.SphereGeometry(0.95, 32, 32);
    this.plasmaMat = new THREE.MeshBasicMaterial({
      color: 0x00ffff,
      transparent: true,
      opacity: 0.35,
      wireframe: false
    });
    this.plasmaCore = new THREE.Mesh(sphereGeom, this.plasmaMat);
    this.planetGroup.add(this.plasmaCore);

    // 4. Quantum Vertices Points (glowing dots at vertices)
    const pointsMat = new THREE.PointsMaterial({
      color: 0xffffff,
      size: 0.12,
      transparent: true,
      opacity: 0.95
    });
    this.quantumVertices = new THREE.Points(icoGeom, pointsMat);
    this.planetGroup.add(this.quantumVertices);
  }

  initOrbitalRings() {
    this.ringsGroup = new THREE.Group();
    this.group.add(this.ringsGroup);

    this.ringMeshes = [];

    // Ring specifications: radius, inclination (pitch), roll, color
    const ringConfigs = [
      { r: 4.2, pitch: 0.45, roll: 0.15, color: 0x00f0ff, opacity: 0.4 },
      { r: 4.8, pitch: -0.55, roll: 0.35, color: 0x00ff66, opacity: 0.6 },
      { r: 5.4, pitch: 0.25, roll: -0.45, color: 0x00f0ff, opacity: 0.35 }
    ];

    ringConfigs.forEach((cfg) => {
      // Ring spline line
      const segments = 96;
      const points = [];
      for (let i = 0; i <= segments; i++) {
        const theta = (i / segments) * Math.PI * 2;
        points.push(new THREE.Vector3(Math.cos(theta) * cfg.r, 0, Math.sin(theta) * cfg.r));
      }
      const geom = new THREE.BufferGeometry().setFromPoints(points);
      const mat = new THREE.LineBasicMaterial({
        color: cfg.color,
        transparent: true,
        opacity: cfg.opacity,
        linewidth: 1.2
      });
      const ringLine = new THREE.Line(geom, mat);
      ringLine.rotation.x = cfg.pitch;
      ringLine.rotation.z = cfg.roll;

      this.ringsGroup.add(ringLine);
      this.ringMeshes.push({ line: ringLine, pitch: cfg.pitch, roll: cfg.roll, radius: cfg.r, rotSpeed: 0.05 });
    });
  }

  initStationNodes() {
    this.stationsGroup = new THREE.Group();
    this.group.add(this.stationsGroup);

    this.stationObjects = [];

    this.stations.forEach(st => {
      const nodeGroup = new THREE.Group();

      // Glowing anchor marker dot
      const dotGeom = new THREE.SphereGeometry(0.12, 12, 12);
      const dotMat = new THREE.MeshBasicMaterial({
        color: st.active ? 0x00ff66 : 0x00f0ff
      });
      const dot = new THREE.Mesh(dotGeom, dotMat);
      nodeGroup.add(dot);

      // Outer pulse halo for active station
      if (st.active) {
        const ringGeom = new THREE.RingGeometry(0.18, 0.28, 16);
        const ringMat = new THREE.MeshBasicMaterial({
          color: 0x00ff66,
          side: THREE.DoubleSide,
          transparent: true,
          opacity: 0.8
        });
        const halo = new THREE.Mesh(ringGeom, ringMat);
        halo.name = 'halo';
        nodeGroup.add(halo);
      }

      // Sprite text label (Canvas texture)
      const labelSprite = this.createStationSprite(st.code, st.label, st.active);
      labelSprite.position.set(0, 0.45, 0);
      nodeGroup.add(labelSprite);

      this.stationsGroup.add(nodeGroup);
      this.stationObjects.push({
        data: st,
        node: nodeGroup,
        sprite: labelSprite
      });
    });
  }

  createStationSprite(code, label, isActive) {
    if (typeof document === 'undefined') {
      return new THREE.Sprite();
    }
    const canvas = document.createElement('canvas');
    canvas.width = 256;
    canvas.height = 64;
    const ctx = canvas.getContext('2d');

    ctx.clearRect(0, 0, 256, 64);

    if (isActive) {
      // Active Station HUD pill (matches the screenshot: ◆ STATION 02 // ENGINEERING)
      ctx.fillStyle = 'rgba(4, 18, 22, 0.85)';
      ctx.strokeStyle = '#00ff66';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.roundRect(8, 10, 240, 44, 4);
      ctx.fill();
      ctx.stroke();

      // Diamond icon
      ctx.fillStyle = '#00ff66';
      ctx.font = 'bold 16px "JetBrains Mono", monospace';
      ctx.fillText('◆', 18, 38);

      // Label
      ctx.font = 'bold 15px "JetBrains Mono", monospace';
      ctx.fillStyle = '#00ff66';
      ctx.fillText(`STATION ${code.replace('02 ', '')} // ${label}`, 38, 38);
    } else {
      // Inactive station subtle tag (e.g. 05 SCI)
      ctx.fillStyle = 'rgba(0, 240, 255, 0.7)';
      ctx.font = 'bold 16px "JetBrains Mono", monospace';
      ctx.fillText(code, 20, 38);
    }

    const texture = new THREE.CanvasTexture(canvas);
    texture.minFilter = THREE.LinearFilter;
    const spriteMat = new THREE.SpriteMaterial({ map: texture, transparent: true });
    const sprite = new THREE.Sprite(spriteMat);
    sprite.scale.set(isActive ? 2.6 : 1.2, isActive ? 0.65 : 0.3, 1);
    return sprite;
  }

  initRadarGrid() {
    this.gridGroup = new THREE.Group();
    this.gridGroup.position.y = -2.4;
    this.group.add(this.gridGroup);

    // Concentric polar range rings on XZ plane
    const rings = [1.5, 3.0, 4.5, 6.0];
    rings.forEach((r, idx) => {
      const segs = 64;
      const pts = [];
      for (let i = 0; i <= segs; i++) {
        const th = (i / segs) * Math.PI * 2;
        pts.push(new THREE.Vector3(Math.cos(th) * r, 0, Math.sin(th) * r));
      }
      const geom = new THREE.BufferGeometry().setFromPoints(pts);
      const mat = new THREE.LineBasicMaterial({
        color: 0x00ff66,
        transparent: true,
        opacity: idx === rings.length - 1 ? 0.35 : 0.15
      });
      const ringLine = new THREE.Line(geom, mat);
      this.gridGroup.add(ringLine);
    });

    // Crosshairs & Polar spokes
    const spokeMat = new THREE.LineBasicMaterial({
      color: 0x00ff66,
      transparent: true,
      opacity: 0.12
    });
    const maxR = 6.0;
    for (let a = 0; a < Math.PI * 2; a += Math.PI / 4) {
      const pts = [
        new THREE.Vector3(0, 0, 0),
        new THREE.Vector3(Math.cos(a) * maxR, 0, Math.sin(a) * maxR)
      ];
      const geom = new THREE.BufferGeometry().setFromPoints(pts);
      this.gridGroup.add(new THREE.Line(geom, spokeMat));
    }
  }

  initRadarSweep() {
    // 3D Sweeping Beam Fan (Cone geometry segment)
    const sweepGeom = new THREE.CircleGeometry(6.0, 32, 0, 0.45);
    const sweepMat = new THREE.MeshBasicMaterial({
      color: 0x00ff66,
      transparent: true,
      opacity: 0.22,
      side: THREE.DoubleSide
    });
    this.sweepMesh = new THREE.Mesh(sweepGeom, sweepMat);
    this.sweepMesh.rotation.x = Math.PI / 2;
    this.gridGroup.add(this.sweepMesh);

    // Leading Sweep Ray
    const rayPts = [new THREE.Vector3(0, 0, 0), new THREE.Vector3(6.0, 0, 0)];
    const rayGeom = new THREE.BufferGeometry().setFromPoints(rayPts);
    const rayMat = new THREE.LineBasicMaterial({
      color: 0x00ff66,
      transparent: true,
      opacity: 0.85,
      linewidth: 2
    });
    this.sweepRay = new THREE.Line(rayGeom, rayMat);
    this.gridGroup.add(this.sweepRay);
  }

  updateTargets(wifiNetworks = [], btDevices = []) {
    // Clean old target blips
    this.targetBlips.forEach(b => {
      this.gridGroup.remove(b.mesh);
      if (b.halo) this.gridGroup.remove(b.halo);
    });
    this.targetBlips = [];

    const allNodes = [
      ...wifiNetworks.map((n, i) => ({ label: n.ssid, type: 'wifi', dist: 2.2 + (i % 3) * 1.4, angle: 0.6 + i * 1.5 })),
      ...btDevices.map((d, i) => ({ label: d.name, type: 'bt', dist: 2.5 + (i % 3) * 1.2, angle: 1.8 + i * 1.3 }))
    ].slice(0, 8);

    allNodes.forEach(item => {
      const geom = new THREE.SphereGeometry(0.09, 8, 8);
      const mat = new THREE.MeshBasicMaterial({
        color: item.type === 'wifi' ? 0x00ff66 : 0x00f0ff,
        transparent: true,
        opacity: 0.85
      });
      const mesh = new THREE.Mesh(geom, mat);
      const x = Math.cos(item.angle) * item.dist;
      const z = Math.sin(item.angle) * item.dist;
      mesh.position.set(x, 0.05, z);
      this.gridGroup.add(mesh);

      this.targetBlips.push({ mesh, angle: item.angle, dist: item.dist, type: item.type });
    });
  }

  setActiveStation(stationId) {
    this.stationObjects.forEach(obj => {
      const isAct = obj.data.id === stationId;
      obj.data.active = isAct;
      // Re-create sprite with new active state
      obj.node.remove(obj.sprite);
      obj.sprite = this.createStationSprite(obj.data.code, obj.data.label, isAct);
      obj.sprite.position.set(0, isAct ? 0.55 : 0.35, 0);
      obj.node.add(obj.sprite);
    });
  }

  setDissolve(factor) {
    this.dissolveFactor = factor;
    // Scale and fade planet wireframes
    const scale = 1.0 + factor * 0.8;
    this.planetGroup.scale.set(scale, scale, scale);
    this.planetWireMat.opacity = Math.max(0, 0.75 * (1 - factor));
    this.innerWireMat.opacity = Math.max(0, 0.45 * (1 - factor));
    this.plasmaMat.opacity = Math.max(0, 0.35 * (1 - factor));

    // Dissolve spherical geometry into fine particles and wireframe fragments
    if (this.quantumVertices && this.quantumVertices.material) {
      const pScale = 1.0 + factor * 1.8;
      this.quantumVertices.scale.set(pScale, pScale, pScale);
      this.quantumVertices.material.opacity = Math.max(0, 0.95 * (1 - factor));
    }

    // Separate orbital rings into independent layers, settling into containment field
    const containmentY = [-1.4, 0.3, 1.8];
    this.ringMeshes.forEach((r, idx) => {
      // Interpolate from planetary tilt/level to horizontal containment rings around server
      const targetY = containmentY[idx] || 0;
      r.line.position.y = (idx - 1) * factor * 1.6 + factor * (targetY - (idx - 1) * 1.6);
      r.line.rotation.x = r.pitch * (1 - factor);
      r.line.rotation.z = r.roll * (1 - factor);
      
      const s = (1.0 + factor * 0.15);
      r.line.scale.set(s, s, s);

      if (r.line.material) {
        r.line.material.opacity = (1 - factor) * 0.5 + factor * 0.38;
      }
    });

    // Fade radar sweep and grid
    this.sweepMesh.material.opacity = Math.max(0, 0.22 * (1 - factor));
    this.sweepRay.material.opacity = Math.max(0, 0.85 * (1 - factor));

    // Fade station labels
    if (this.stationsGroup) {
      this.stationsGroup.visible = factor < 0.65;
    }
  }

  update(time, delta) {
    // 1. Slow idle rotation of planet
    this.planetWire.rotation.y += 0.006;
    this.planetWire.rotation.x += 0.003;
    this.innerWire.rotation.y -= 0.009;
    this.innerWire.rotation.z += 0.004;

    // 2. Pulse plasma core
    const pulse = 1.0 + Math.sin(time * 3.5) * 0.06;
    this.plasmaCore.scale.set(pulse, pulse, pulse);

    // 3. Rotate orbital rings slowly
    this.ringMeshes.forEach((r, idx) => {
      r.line.rotation.y += (idx % 2 === 0 ? 1 : -1) * 0.003;
    });

    // 4. Update station positions orbiting on their rings
    this.stationObjects.forEach(obj => {
      const st = obj.data;
      st.angle += st.speed * delta;
      if (st.angle > Math.PI * 2) st.angle -= Math.PI * 2;

      // Position along elliptical orbit
      const x = Math.cos(st.angle) * st.radius;
      const z = Math.sin(st.angle) * (st.radius * 0.78);
      const y = Math.sin(st.angle * 2.0) * 0.4;
      obj.node.position.set(x, y, z);

      // Pulse halo on active station
      const halo = obj.node.getObjectByName('halo');
      if (halo) {
        const s = 1.0 + Math.sin(time * 4) * 0.2;
        halo.scale.set(s, s, s);
      }
    });

    // 5. Radar sweep beam rotation
    this.radarAngle += 0.035;
    if (this.radarAngle > Math.PI * 2) this.radarAngle -= Math.PI * 2;

    this.sweepMesh.rotation.z = -this.radarAngle;
    this.sweepRay.rotation.y = this.radarAngle;

    // 6. Blip flash when radar sweep crosses
    this.targetBlips.forEach(b => {
      let diff = Math.abs(this.radarAngle - b.angle);
      if (diff > Math.PI) diff = (Math.PI * 2) - diff;
      if (diff < 0.2) {
        b.mesh.scale.set(1.8, 1.8, 1.8);
      } else {
        b.mesh.scale.lerp(new THREE.Vector3(1, 1, 1), 0.1);
      }
    });
  }

  dispose() {
    // Cleanup geometries and materials
    this.group.traverse(obj => {
      if (obj.geometry) obj.geometry.dispose();
      if (obj.material) {
        if (Array.isArray(obj.material)) obj.material.forEach(m => m.dispose());
        else obj.material.dispose();
      }
    });
  }
}
