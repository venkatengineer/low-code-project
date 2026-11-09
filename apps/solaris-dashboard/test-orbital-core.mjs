// ==============================================================================
// 🧪 TEST SUITE: ORBITAL // CORE UPGRADE VERIFICATION
// ==============================================================================
import { createRequire } from 'module';
const require = createRequire(import.meta.url);

const sessionManager = require('./electron/ssh-session-manager.cjs');
const telemetry = require('./electron/system-telemetry.cjs');

async function runTests() {
  console.log('--- STARTING ORBITAL // CORE VERIFICATION TEST SUITE ---');
  let passCount = 0;
  let failCount = 0;

  function assert(condition, testName) {
    if (condition) {
      console.log(`[PASS] ${testName}`);
      passCount++;
    } else {
      console.error(`[FAIL] ${testName}`);
      failCount++;
    }
  }

  // 1. Telemetry Verification
  try {
    const data = telemetry.getAllTelemetry();
    assert(data !== null && typeof data === 'object', 'Telemetry aggregator returns object');
    assert(data.system && typeof data.system.hostname === 'string', 'System telemetry contains real hostname');
    assert(data.cpu && typeof data.cpu.usage === 'number', 'CPU telemetry reports numeric usage');
    assert(data.memory && typeof data.memory.percentage === 'number', 'Memory telemetry reports numeric percentage');
    assert(data.storage && Array.isArray(data.storage.disks), 'Storage telemetry reports disk partitions');
    assert(data.wifi && Array.isArray(data.wifi.networks), 'Wi-Fi telemetry reports scan list');
    assert(data.bluetooth && Array.isArray(data.bluetooth.devices), 'Bluetooth telemetry reports peripheral list');
    console.log(`Telemetry Sample: Host=${data.system.hostname}, CPU=${data.cpu.usage}%, MEM=${data.memory.percentage}%`);
  } catch (err) {
    assert(false, `Telemetry test threw error: ${err.message}`);
  }

  // 2. Services Supervisor Verification
  try {
    const servicesRes = await sessionManager.getSystemServices();
    assert(servicesRes && Array.isArray(servicesRes.services), 'Services manager queries systemctl services');
    assert(servicesRes.services.length >= 5, `Monitored services count is sufficient (${servicesRes.services.length})`);
    
    const sshdSvc = servicesRes.services.find(s => s.name === 'sshd');
    assert(sshdSvc !== undefined, 'sshd.service is tracked in monitored services');
    
    assert(servicesRes.docker && typeof servicesRes.docker.active === 'boolean', 'Docker daemon status detected');
    console.log(`Services Tracked: ${servicesRes.services.map(s => s.name + ':' + s.status).join(', ')}`);
  } catch (err) {
    assert(false, `Services supervisor test threw error: ${err.message}`);
  }

  // 3. Local PTY / Shell Session Lifecycle
  try {
    let receivedData = '';
    const testSessionId = 'test-session-' + Date.now();
    
    await new Promise((resolve) => {
      const res = sessionManager.startLocalSession(
        testSessionId,
        80,
        24,
        (data) => {
          receivedData += data;
          if (receivedData.length > 10) {
            resolve();
          }
        },
        () => {}
      );
      assert(res && res.success, 'startLocalSession initializes shell process');
    });

    assert(receivedData.length > 0, 'Local shell session emitted terminal data');
    
    // Test sending command
    const sent = sessionManager.sendData(testSessionId, 'echo "ORBITAL_CORE_TEST_OK"\r');
    assert(sent === true, 'sendData dispatched keystrokes to shell process');

    // Test resize
    const resized = sessionManager.resizeSession(testSessionId, 100, 30);
    assert(resized === true, 'resizeSession adjusted terminal dimensions');

    // Test close
    const closed = sessionManager.closeSession(testSessionId);
    assert(closed === true, 'closeSession terminated shell process cleanly');
  } catch (err) {
    assert(false, `Session manager test threw error: ${err.message}`);
  }

  // 4. Three.js & ES Modules Import Validation
  try {
    const { OrbitalPlanet } = await import('./ui/orbital-core/OrbitalPlanet.js');
    const { ServerModel } = await import('./ui/orbital-core/ServerModel.js');
    const { CoreTransitionController } = await import('./ui/orbital-core/CoreTransitionController.js');

    assert(typeof OrbitalPlanet === 'function', 'OrbitalPlanet module exported as class');
    assert(typeof ServerModel === 'function', 'ServerModel module exported as class');
    assert(typeof CoreTransitionController === 'function', 'CoreTransitionController exported as class');

    // Test Server Model Telemetry Mapping
    const serverInstance = new ServerModel();
    assert(serverInstance.metrics.isLive === false, 'ServerModel initial state isLive=false (demo/offline)');
    
    serverInstance.setTelemetry({
      system: { hostname: 'alpha-node-01', kernel: 'Linux 6.13' },
      cpu: { usage: 42, temp: 58, fanSpeedRpm: 3200 },
      memory: { percentage: 68 },
      network: { rxKbs: 120 }
    });
    assert(serverInstance.metrics.isLive === true, 'setTelemetry marks isLive=true with real metrics');
    assert(serverInstance.metrics.hostname === 'alpha-node-01', 'setTelemetry updates hostname');
    assert(serverInstance.metrics.cpuUsage === 42, 'setTelemetry updates CPU usage');
    assert(serverInstance.metrics.cpuTemp === 58, 'setTelemetry updates CPU temperature');

    // Test Orbital Planet Stations
    const planetInstance = new OrbitalPlanet();
    assert(planetInstance.stations.length === 6, 'OrbitalPlanet contains 6 station definitions (01-06)');
    planetInstance.setActiveStation(4);
    const st4 = planetInstance.stations.find(s => s.id === 4);
    assert(st4.active === true, 'setActiveStation sets target station active');

    // Test Dissolve & Containment Field Transition
    planetInstance.setDissolve(1.0);
    assert(planetInstance.dissolveFactor === 1.0, 'setDissolve sets dissolveFactor to 1.0');
    assert(planetInstance.ringMeshes[0].line.position.y !== 0, 'Containment rings settle at vertical offsets around server');

    // Test Controller State Transitions
    let recordedState = null;
    const controller = new CoreTransitionController(
      { camera: { position: { set: () => {} } } },
      planetInstance,
      serverInstance,
      { open: () => {}, close: () => {}, focus: () => {} },
      { onStateChange: (s) => { recordedState = s; } }
    );
    assert(controller.currentState === 'ORBITAL', 'Initial state is ORBITAL');
    
    await controller.toServerCore();
    assert(controller.currentState === 'SERVER_CORE', 'toServerCore sets state to SERVER_CORE');
    
    await controller.toTerminal();
    assert(controller.currentState === 'TERMINAL', 'toTerminal sets state to TERMINAL');
    
    await controller.fromTerminalToServerCore();
    assert(controller.currentState === 'SERVER_CORE', 'fromTerminalToServerCore restores SERVER_CORE');

    await controller.toOrbital();
    assert(controller.currentState === 'ORBITAL', 'toOrbital returns to ORBITAL');

    // Test disposal cleanup
    serverInstance.dispose();
    planetInstance.dispose();
    assert(true, 'Three.js geometries and materials disposed cleanly without errors');

  } catch (err) {
    assert(false, `3D modules test threw error: ${err.message}`);
  }

  console.log(`\n--- TEST SUMMARY: ${passCount} PASSED, ${failCount} FAILED ---`);
  if (failCount === 0) {
    console.log('✨ ALL VERIFICATION TESTS PASSED SUCCESSFULLY! ✨');
    process.exit(0);
  } else {
    process.exit(1);
  }
}

runTests();
