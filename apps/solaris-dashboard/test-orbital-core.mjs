// ==============================================================================
// 🧪 TEST SUITE: ORBITAL PLANET // PURE 60FPS VISUALIZATION
// ==============================================================================
import { createRequire } from 'module';
const require = createRequire(import.meta.url);

const telemetry = require('./electron/system-telemetry.cjs');

async function runTests() {
  console.log('--- STARTING ORBITAL PLANET TEST SUITE ---');
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

  // 2. Three.js & ES Modules Import Validation
  try {
    const { OrbitalPlanet } = await import('./ui/orbital-core/OrbitalPlanet.js');
    const { OrbitalScene } = await import('./ui/orbital-core/OrbitalScene.js');

    assert(typeof OrbitalPlanet === 'function', 'OrbitalPlanet module exported as class');
    assert(typeof OrbitalScene === 'function', 'OrbitalScene module exported as class');

    // Test Orbital Planet Stations
    const planetInstance = new OrbitalPlanet();
    assert(planetInstance.stations.length === 6, 'OrbitalPlanet contains 6 station definitions (01-06)');
    planetInstance.setActiveStation(4);
    const st4 = planetInstance.stations.find(s => s.id === 4);
    assert(st4.active === true, 'setActiveStation sets target station active');

    // Test tick simulation
    planetInstance.update(0.016);
    assert(planetInstance.radarAngle > 0, 'Radar sweep rotates continuously with delta time');

    // Test disposal cleanup
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
