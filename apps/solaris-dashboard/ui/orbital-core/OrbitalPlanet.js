// ==============================================================================
// 🪐 ORBITAL PLANET — 3D Geodesic Hologram & Radar System
// Butter-Smooth 60FPS Planetary Visualization with Interactive Station Orbits
// ==============================================================================
import * as THREE from '../../node_modules/three/build/three.module.js';

export class OrbitalPlanet {
  constructor() {
    this.group = new THREE.Group();
    this.radarAngle = 0;
    this.targetBlips = [];

    // Stations orbiting the core
    this.stations = [
      { id: 1, code: '01 CMD', label: 'COMMAND', radius: 4.3, angle: 0.2, speed: 0.16, color: 0x00f0ff },
      { id: 2, code: '02 ENG', label: 'ENGINEERING', radius: 4.9, angle: 1.6, speed: 0.12, color: 0x00ff66, active: true },
      { id: 3, code: '03 NAV', label: 'NAVIGATION', radius: 5.5, angle: 3.1, speed: 0.09, color: 0x00f0ff },
      { id: 4, code: '04 COM', label: 'COMMS', radius: 4.3, angle: 4.2, speed: 0.14, color: 0x00f0ff },
      { id: 5, code: '05 SCI', label: 'SCIENCE', radius: 4.9, angle: 5.0, speed: 0.11, color: 0x00f0ff },
      { id: 6, code: '06 AUX', label: 'AUXILIARY', radius: 5.5, angle: 6.0, speed: 0.08, color: 0x00f0ff }
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

    // 1. Primary Geodesic Wireframe (Subdivision Level 2 for refined sphere look)
    const icoGeom = new THREE.IcosahedronGeometry(2.0, 2);
    const wireframeGeom = new THREE.WireframeGeometry(icoGeom);
    this.planetWireMat = new THREE.LineBasicMaterial({
      color: 0x00f0ff,
      transparent: true,
      opacity: 0.65,
      linewidth: 1.5
    });
    this.planetWire = new THREE.LineSegments(wireframeGeom, this.planetWireMat);
    this.planetGroup.add(this.planetWire);

    // 2. Nested Counter-Rotating Inner Geodesic Core
    const innerIcoGeom = new THREE.IcosahedronGeometry(1.45, 1);
    const innerWireGeom = new THREE.WireframeGeometry(innerIcoGeom);
    this.innerWireMat = new THREE.LineBasicMaterial({
      color: 0x00ff66,
      transparent: true,
      opacity: 0.40,
      linewidth: 1.0
    });
    this.innerWire = new THREE.LineSegments(innerWireGeom, this.innerWireMat);
    this.planetGroup.add(this.innerWire);

    // 3. Volumetric Glowing Inner Plasma Sphere
    const sphereGeom = new THREE.SphereGeometry(1.0, 32, 32);
    this.plasmaMat = new THREE.MeshBasicMaterial({
      color: 0x00f0ff,
      transparent: true,
      opacity: 0.22,
      wireframe: false
    });
    this.plasmaCore = new THREE.Mesh(sphereGeom, this.plasmaMat);
    this.planetGroup.add(this.plasmaCore);

    // 4. Quantum Vertices Points (sharp bright nodes at icosahedron points)
    const pointsMat = new THREE.PointsMaterial({
      color: 0xffffff,
      size: 0.09,
      transparent: true,
      opacity: 0.85
    });
    this.quantumVertices = new THREE.Points(icoGeom, pointsMat);
    this.planetGroup.add(this.quantumVertices);

    // 5. Equatorial Tachyon Ring
    const eqGeom = new THREE.RingGeometry(2.35, 2.45, 64);
    const eqMat = new THREE.MeshBasicMaterial({
      color: 0x00f0ff,
      side: THREE.DoubleSide,
      transparent: true,
      opacity: 0.35
    });
    this.equatorialRing = new THREE.Mesh(eqGeom, eqMat);
    this.equatorialRing.rotation.x = Math.PI / 2;
    this.planetGroup.add(this.equatorialRing);
  }

  initOrbitalRings() {
    this.ringsGroup = new THREE.Group();
    this.group.add(this.ringsGroup);

    this.ringMeshes = [];

    // Ring specifications: radius, inclination (pitch), roll, color, opacity
    const ringConfigs = [
      { r: 4.3, pitch: 0.40, roll: 0.15, color: 0x00f0ff, opacity: 0.45, rotSpeed: 0.002 },
      { r: 4.9, pitch: -0.50, roll: 0.30, color: 0x00ff66, opacity: 0.55, rotSpeed: -0.0025 },
      { r: 5.5, pitch: 0.20, roll: -0.40, color: 0x00f0ff, opacity: 0.35, rotSpeed: 0.0018 }
    ];

    ringConfigs.forEach((cfg) => {
      const segments = 128;
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
        linewidth: 1.5
      });
      const ringLine = new THREE.Line(geom, mat);
      ringLine.rotation.x = cfg.pitch;
      ringLine.rotation.z = cfg.roll;

      this.ringsGroup.add(ringLine);
      this.ringMeshes.push({
        line: ringLine,
        pitch: cfg.pitch,
        roll: cfg.roll,
        radius: cfg.r,
        rotSpeed: cfg.rotSpeed
      });
    });
  }

  initStationNodes() {
    this.stationsGroup = new THREE.Group();
    this.group.add(this.stationsGroup);

    this.stationObjects = [];

    this.stations.forEach(st => {
      const nodeGroup = new THREE.Group();

      // Glowing anchor marker dot
      const dotGeom = new THREE.SphereGeometry(0.12, 16, 16);
      const dotMat = new THREE.MeshBasicMaterial({
        color: st.active ? 0x00ff66 : 0x00f0ff
      });
      const dot = new THREE.Mesh(dotGeom, dotMat);
      dot.userData = { stationId: st.id, isStation: true };
      nodeGroup.add(dot);

      // Outer pulse halo for active station
      if (st.active) {
        const ringGeom = new THREE.RingGeometry(0.18, 0.30, 24);
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
        dot: dot,
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
      // Futuristic pill badge
      ctx.fillStyle = 'rgba(4, 18, 22, 0.90)';
      ctx.strokeStyle = '#00ff66';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.roundRect(8, 10, 240, 44, 4);
      ctx.fill();
      ctx.stroke();

      // Diamond icon & Text
      ctx.fillStyle = '#00ff66';
      ctx.font = 'bold 15px "JetBrains Mono", monospace';
      ctx.fillText('◆', 18, 38);
      ctx.fillText(`STATION ${code.replace('02 ', '')} // ${label}`, 38, 38);
    } else {
      ctx.fillStyle = 'rgba(0, 240, 255, 0.75)';
      ctx.font = 'bold 16px "JetBrains Mono", monospace';
      ctx.fillText(code, 20, 38);
    }

    const texture = new THREE.CanvasTexture(canvas);
    texture.minFilter = THREE.LinearFilter;
    const spriteMat = new THREE.SpriteMaterial({ map: texture, transparent: true });
    const sprite = new THREE.Sprite(spriteMat);
    sprite.scale.set(isActive ? 2.5 : 1.2, isActive ? 0.62 : 0.3, 1);
    return sprite;
  }

  initRadarGrid() {
    this.gridGroup = new THREE.Group();
    this.gridGroup.position.y = -2.5;
    this.group.add(this.gridGroup);

    // Concentric polar range rings on XZ plane
    const rings = [1.6, 3.2, 4.8, 6.2];
    rings.forEach((r, idx) => {
      const segs = 96;
      const pts = [];
      for (let i = 0; i <= segs; i++) {
        const th = (i / segs) * Math.PI * 2;
        pts.push(new THREE.Vector3(Math.cos(th) * r, 0, Math.sin(th) * r));
      }
      const geom = new THREE.BufferGeometry().setFromPoints(pts);
      const mat = new THREE.LineBasicMaterial({
        color: 0x00ff66,
        transparent: true,
        opacity: idx === rings.length - 1 ? 0.30 : 0.12
      });
      const ringLine = new THREE.Line(geom, mat);
      this.gridGroup.add(ringLine);
    });

    // Crosshairs & Polar spokes
    const spokeMat = new THREE.LineBasicMaterial({
      color: 0x00ff66,
      transparent: true,
      opacity: 0.10
    });
    const maxR = 6.2;
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
    const sweepGeom = new THREE.CircleGeometry(6.2, 48, 0, 0.40);
    const sweepMat = new THREE.MeshBasicMaterial({
      color: 0x00ff66,
      transparent: true,
      opacity: 0.16,
      side: THREE.DoubleSide
    });
    this.sweepMesh = new THREE.Mesh(sweepGeom, sweepMat);
    this.sweepMesh.rotation.x = Math.PI / 2;
    this.sweepMesh.position.y = -2.48;
    this.group.add(this.sweepMesh);

    // Leading edge beam ray
    const rayPts = [
      new THREE.Vector3(0, -2.48, 0),
      new THREE.Vector3(6.2, -2.48, 0)
    ];
    const rayGeom = new THREE.BufferGeometry().setFromPoints(rayPts);
    const rayMat = new THREE.LineBasicMaterial({
      color: 0x00ff66,
      transparent: true,
      opacity: 0.75,
      linewidth: 1.5
    });
    this.sweepRay = new THREE.Line(rayGeom, rayMat);
    this.group.add(this.sweepRay);
  }

  setActiveStation(stationId) {
    this.stations.forEach(st => {
      st.active = (st.id === stationId);
    });

    this.stationObjects.forEach(obj => {
      const isAct = (obj.data.id === stationId);
      obj.data.active = isAct;

      obj.dot.material.color.setHex(isAct ? 0x00ff66 : 0x00f0ff);

      const existingHalo = obj.node.getObjectByName('halo');
      if (isAct && !existingHalo) {
        const ringGeom = new THREE.RingGeometry(0.18, 0.30, 24);
        const ringMat = new THREE.MeshBasicMaterial({
          color: 0x00ff66,
          side: THREE.DoubleSide,
          transparent: true,
          opacity: 0.8
        });
        const halo = new THREE.Mesh(ringGeom, ringMat);
        halo.name = 'halo';
        obj.node.add(halo);
      } else if (!isAct && existingHalo) {
        obj.node.remove(existingHalo);
      }

      obj.node.remove(obj.sprite);
      obj.sprite = this.createStationSprite(obj.data.code, obj.data.label, isAct);
      obj.sprite.position.set(0, isAct ? 0.50 : 0.35, 0);
      obj.node.add(obj.sprite);
    });
  }

  update(time, delta) {
    // 1. Butter-smooth planetary idle rotation
    this.planetWire.rotation.y += 0.005;
    this.planetWire.rotation.x += 0.002;
    this.innerWire.rotation.y -= 0.008;
    this.innerWire.rotation.z += 0.003;

    // 2. Pulse plasma core gently
    const pulse = 1.0 + Math.sin(time * 3.0) * 0.05;
    this.plasmaCore.scale.set(pulse, pulse, pulse);

    // 3. Counter-rotate orbital rings smoothly
    this.ringMeshes.forEach(r => {
      r.line.rotation.y += r.rotSpeed;
    });

    // 4. Update orbiting station positions
    this.stationObjects.forEach(obj => {
      const st = obj.data;
      st.angle += st.speed * delta;
      if (st.angle > Math.PI * 2) st.angle -= Math.PI * 2;

      const x = Math.cos(st.angle) * st.radius;
      const z = Math.sin(st.angle) * (st.radius * 0.80);
      const y = Math.sin(st.angle * 2.0) * 0.35;
      obj.node.position.set(x, y, z);

      const halo = obj.node.getObjectByName('halo');
      if (halo) {
        const s = 1.0 + Math.sin(time * 4) * 0.18;
        halo.scale.set(s, s, s);
      }
    });

    // 5. Radar sweep beam rotation
    this.radarAngle += 0.030;
    if (this.radarAngle > Math.PI * 2) this.radarAngle -= Math.PI * 2;

    this.sweepMesh.rotation.z = -this.radarAngle;
    this.sweepRay.rotation.y = this.radarAngle;
  }

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
