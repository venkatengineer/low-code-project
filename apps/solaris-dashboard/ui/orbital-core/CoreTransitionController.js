// ==============================================================================
// 🎬 CORE TRANSITION CONTROLLER — Orchestrates Cinematic State Transitions
// ==============================================================================

export class CoreTransitionController {
  constructor(orbitalScene, orbitalPlanet, serverModel, terminalOverlay, hudElements = {}) {
    this.scene = orbitalScene;
    this.planet = orbitalPlanet;
    this.server = serverModel;
    this.terminal = terminalOverlay;
    this.hud = hudElements;

    this.currentState = 'ORBITAL'; // 'ORBITAL' | 'SERVER_CORE' | 'TERMINAL'
    this.isTransitioning = false;
  }

  getGsap() {
    return (typeof window !== 'undefined' && window.gsap) ? window.gsap : null;
  }

  /**
   * Transition: ORBITAL -> SERVER_CORE
   */
  async toServerCore() {
    if (this.isTransitioning || this.currentState === 'SERVER_CORE') return;
    this.isTransitioning = true;

    const gsap = this.getGsap();
    const duration = 1.2;

    this.server.group.visible = true;

    if (gsap) {
      const tl = gsap.timeline({
        onComplete: () => {
          this.planet.planetGroup.visible = false;
          this.planet.gridGroup.visible = false;
          this.planet.ringsGroup.visible = true;
          this.currentState = 'SERVER_CORE';
          this.isTransitioning = false;
          if (this.hud.onStateChange) this.hud.onStateChange('SERVER_CORE');
        }
      });

      // 1. Planet pitches toward camera
      tl.to(this.planet.planetGroup.rotation, {
        x: 0.35,
        duration: duration * 0.4,
        ease: 'power2.inOut'
      }, 0);

      // 2. Planet dissolves (dissolveFactor 0 -> 1)
      const dissolveObj = { factor: 0 };
      tl.to(dissolveObj, {
        factor: 1,
        duration: duration * 0.7,
        ease: 'power2.inOut',
        onUpdate: () => {
          this.planet.setDissolve(dissolveObj.factor);
        }
      }, duration * 0.1);

      // 3. Server chassis scales up & materializes
      tl.fromTo(this.server.group.scale, 
        { x: 0.05, y: 0.05, z: 0.05 },
        { x: 1.0, y: 1.0, z: 1.0, duration: duration * 0.75, ease: 'back.out(1.4)' },
        duration * 0.35
      );

      // 4. Camera adjusts slightly for server perspective
      tl.to(this.scene.camera.position, {
        y: 1.2,
        z: 9.8,
        duration: duration * 0.8,
        ease: 'power2.out'
      }, duration * 0.2);

    } else {
      // Fallback smooth transition without GSAP
      this.planet.setDissolve(1);
      this.planet.group.visible = false;
      this.server.group.scale.set(1, 1, 1);
      this.scene.camera.position.set(0, 1.2, 9.8);
      this.currentState = 'SERVER_CORE';
      this.isTransitioning = false;
      if (this.hud.onStateChange) this.hud.onStateChange('SERVER_CORE');
    }
  }

  /**
   * Transition: SERVER_CORE -> ORBITAL
   */
  async toOrbital() {
    if (this.isTransitioning || this.currentState === 'ORBITAL') return;
    this.isTransitioning = true;

    const gsap = this.getGsap();
    const duration = 1.1;

    this.planet.group.visible = true;
    this.planet.planetGroup.visible = true;
    this.planet.gridGroup.visible = true;
    this.planet.ringsGroup.visible = true;

    if (gsap) {
      const tl = gsap.timeline({
        onComplete: () => {
          this.server.group.visible = false;
          this.currentState = 'ORBITAL';
          this.isTransitioning = false;
          if (this.hud.onStateChange) this.hud.onStateChange('ORBITAL');
        }
      });

      // 1. Collapse server chassis
      tl.to(this.server.group.scale, {
        x: 0.05,
        y: 0.05,
        z: 0.05,
        duration: duration * 0.5,
        ease: 'back.in(1.2)'
      }, 0);

      // 2. Reverse dissolve planet
      const dissolveObj = { factor: 1 };
      tl.to(dissolveObj, {
        factor: 0,
        duration: duration * 0.7,
        ease: 'power2.out',
        onUpdate: () => {
          this.planet.setDissolve(dissolveObj.factor);
        }
      }, duration * 0.25);

      // 3. Camera resets to orbital vantage
      tl.to(this.scene.camera.position, {
        x: 0,
        y: 3.2,
        z: 11.5,
        duration: duration * 0.8,
        ease: 'power2.inOut'
      }, duration * 0.1);

    } else {
      this.server.group.visible = false;
      this.planet.setDissolve(0);
      this.planet.group.visible = true;
    this.planet.planetGroup.visible = true;
    this.planet.gridGroup.visible = true;
    this.planet.ringsGroup.visible = true;
      this.scene.camera.position.set(0, 3.2, 11.5);
      this.currentState = 'ORBITAL';
      this.isTransitioning = false;
      if (this.hud.onStateChange) this.hud.onStateChange('ORBITAL');
    }
  }

  /**
   * Transition: SERVER_CORE -> TERMINAL
   */
  async toTerminal() {
    if (this.isTransitioning || this.currentState === 'TERMINAL') return;
    this.isTransitioning = true;

    const gsap = this.getGsap();
    const duration = 0.85;

    // 1. Server briefly rotates to face camera squarely
    if (gsap) {
      const tl = gsap.timeline({
        onComplete: () => {
          this.currentState = 'TERMINAL';
          this.isTransitioning = false;
          if (this.terminal && this.terminal.focus) this.terminal.focus();
          if (this.hud.onStateChange) this.hud.onStateChange('TERMINAL');
        }
      });

      // Align server rotation
      tl.to(this.server.chassisGroup.rotation, {
        x: 0,
        y: 0,
        duration: 0.35,
        ease: 'power2.out'
      }, 0);

      // Dim and push server back slightly
      tl.to(this.server.group.position, {
        z: -2.5,
        y: -0.4,
        duration: duration * 0.7,
        ease: 'power2.out'
      }, 0.1);

      tl.to(this.server.frameMesh.material, {
        opacity: 0.45,
        duration: duration * 0.5
      }, 0.1);

      // Trigger holographic terminal unfold animation
      if (this.terminal && this.terminal.open) {
        this.terminal.open();
      }

    } else {
      this.server.group.position.set(0, -0.4, -2.5);
      if (this.terminal && this.terminal.open) this.terminal.open();
      this.currentState = 'TERMINAL';
      this.isTransitioning = false;
      if (this.hud.onStateChange) this.hud.onStateChange('TERMINAL');
    }
  }

  /**
   * Transition: TERMINAL -> SERVER_CORE
   */
  async fromTerminalToServerCore() {
    if (this.isTransitioning || this.currentState !== 'TERMINAL') return;
    this.isTransitioning = true;

    const gsap = this.getGsap();
    const duration = 0.65;

    if (this.terminal && this.terminal.close) {
      this.terminal.close();
    }

    if (gsap) {
      const tl = gsap.timeline({
        onComplete: () => {
          this.currentState = 'SERVER_CORE';
          this.isTransitioning = false;
          if (this.hud.onStateChange) this.hud.onStateChange('SERVER_CORE');
        }
      });

      // Restore server position and opacity
      tl.to(this.server.group.position, {
        z: 0,
        y: 0,
        duration: duration,
        ease: 'power2.out'
      }, 0);

      tl.to(this.server.frameMesh.material, {
        opacity: 1.0,
        duration: duration * 0.6
      }, 0);

    } else {
      this.server.group.position.set(0, 0, 0);
      this.currentState = 'SERVER_CORE';
      this.isTransitioning = false;
      if (this.hud.onStateChange) this.hud.onStateChange('SERVER_CORE');
    }
  }

  /**
   * Switch to any target state directly
   */
  async switchState(targetState) {
    if (targetState === this.currentState) return;

    if (targetState === 'ORBITAL') {
      if (this.currentState === 'TERMINAL') {
        await this.fromTerminalToServerCore();
      }
      await this.toOrbital();
    } else if (targetState === 'SERVER_CORE') {
      if (this.currentState === 'ORBITAL') {
        await this.toServerCore();
      } else if (this.currentState === 'TERMINAL') {
        await this.fromTerminalToServerCore();
      }
    } else if (targetState === 'TERMINAL') {
      if (this.currentState === 'ORBITAL') {
        await this.toServerCore();
      }
      await this.toTerminal();
    }
  }
}
