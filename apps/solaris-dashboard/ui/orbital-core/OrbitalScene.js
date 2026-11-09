// ==============================================================================
// 🌌 ORBITAL SCENE — 3D Three.js Context & Rendering Pipeline
// ==============================================================================
import * as THREE from '../../node_modules/three/build/three.module.js';

export class OrbitalScene {
  constructor(container) {
    this.container = container;
    this.width = container.clientWidth || 400;
    this.height = container.clientHeight || 400;

    // 1. Scene
    this.scene = new THREE.Scene();
    this.scene.fog = new THREE.FogExp2(0x040912, 0.035);

    // 2. Camera
    this.camera = new THREE.PerspectiveCamera(45, this.width / this.height, 0.1, 100);
    this.camera.position.set(0, 3.2, 11.5);
    this.camera.lookAt(0, 0, 0);

    // Camera target for smooth tweening
    this.targetCameraPos = this.camera.position.clone();
    this.targetLookAt = new THREE.Vector3(0, 0, 0);
    this.currentLookAt = new THREE.Vector3(0, 0, 0);

    // 3. WebGL Renderer
    this.renderer = new THREE.WebGLRenderer({
      antialias: true,
      alpha: true,
      powerPreference: 'high-performance'
    });
    this.renderer.setSize(this.width, this.height);
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
    this.renderer.setClearColor(0x000000, 0);
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = 1.25;

    // Insert canvas into container
    this.canvas = this.renderer.domElement;
    this.canvas.className = 'orbital-3d-canvas';
    this.canvas.style.position = 'absolute';
    this.canvas.style.top = '0';
    this.canvas.style.left = '0';
    this.canvas.style.width = '100%';
    this.canvas.style.height = '100%';
    this.canvas.style.outline = 'none';
    this.canvas.style.touchAction = 'none';
    this.container.appendChild(this.canvas);

    // 4. Lighting System (Cyan, Green, Subtle Violet)
    this.initLights();

    // 5. Drag / Orbit Interaction
    this.isDragging = false;
    this.previousPointerPosition = { x: 0, y: 0 };
    this.rotationVelocity = { x: 0, y: 0 };
    this.targetRotation = { x: 0.15, y: 0 };
    this.currentRotation = { x: 0.15, y: 0 };
    this.userControlled = false;

    this.bindEvents();

    // 6. Reduced Motion Preference
    this.reducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;

    // 7. Raycaster for hotspots
    this.raycaster = new THREE.Raycaster();
    this.pointer = new THREE.Vector2();

    this.isDisposed = false;
  }

  initLights() {
    // Ambient Space Illumination
    this.ambientLight = new THREE.AmbientLight(0x061424, 1.8);
    this.scene.add(this.ambientLight);

    // Primary Neon Cyan Key Light
    this.keyLight = new THREE.DirectionalLight(0x00f0ff, 2.4);
    this.keyLight.position.set(5, 8, 8);
    this.scene.add(this.keyLight);

    // Green Reactor Core Point Light
    this.coreLight = new THREE.PointLight(0x00ff66, 3.0, 15);
    this.coreLight.position.set(0, 0, 0);
    this.scene.add(this.coreLight);

    // Subtle Violet Rim Light
    this.rimLight = new THREE.PointLight(0xa855f7, 2.0, 12);
    this.rimLight.position.set(-6, -4, -4);
    this.scene.add(this.rimLight);
  }

  bindEvents() {
    this.onResize = () => {
      if (this.isDisposed) return;
      this.width = this.container.clientWidth || 400;
      this.height = this.container.clientHeight || 400;
      this.camera.aspect = this.width / this.height;
      this.camera.updateProjectionMatrix();
      this.renderer.setSize(this.width, this.height);
    };
    window.addEventListener('resize', this.onResize);

    // Pointer events for smooth view tilt
    this.canvas.addEventListener('pointerdown', (e) => {
      if (e.button !== 0) return;
      this.isDragging = true;
      this.userControlled = true;
      this.previousPointerPosition = { x: e.clientX, y: e.clientY };
    });

    window.addEventListener('pointermove', (e) => {
      // Calculate normalized pointer for raycasting
      const rect = this.canvas.getBoundingClientRect();
      this.pointer.x = ((e.clientX - rect.left) / rect.width) * 2 - 1;
      this.pointer.y = -((e.clientY - rect.top) / rect.height) * 2 + 1;

      if (!this.isDragging) return;
      const deltaX = e.clientX - this.previousPointerPosition.x;
      const deltaY = e.clientY - this.previousPointerPosition.y;

      this.targetRotation.y += deltaX * 0.007;
      this.targetRotation.x = Math.max(-0.6, Math.min(0.6, this.targetRotation.x + deltaY * 0.007));

      this.previousPointerPosition = { x: e.clientX, y: e.clientY };
    });

    window.addEventListener('pointerup', () => {
      this.isDragging = false;
    });

    window.addEventListener('pointercancel', () => {
      this.isDragging = false;
    });
  }

  update(delta) {
    if (this.isDisposed) return;

    // Smooth damping for user tilt
    const damp = this.reducedMotion ? 0.95 : 0.08;
    this.currentRotation.x += (this.targetRotation.x - this.currentRotation.x) * damp;
    this.currentRotation.y += (this.targetRotation.y - this.currentRotation.y) * damp;

    // Apply rotation to scene root pivot if not reduced motion
    this.scene.rotation.x = this.currentRotation.x;
    this.scene.rotation.y = this.currentRotation.y;

    // Smooth camera lookAt interpolation
    this.currentLookAt.lerp(this.targetLookAt, 0.1);
    this.camera.lookAt(this.currentLookAt);

    this.renderer.render(this.scene, this.camera);
  }

  add(object) {
    this.scene.add(object);
  }

  remove(object) {
    this.scene.remove(object);
  }

  dispose() {
    this.isDisposed = true;
    window.removeEventListener('resize', this.onResize);
    if (this.canvas && this.canvas.parentElement) {
      this.canvas.parentElement.removeChild(this.canvas);
    }
    this.renderer.dispose();
  }
}
