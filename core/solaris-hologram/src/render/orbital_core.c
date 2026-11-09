#include "orbital_core.h"
#include <math.h>
#include <string.h>

void orbital_core_init(orbital_core_t *core) {
    if (!core) return;
    memset(core, 0, sizeof(*core));
    core->planet_pitch = 0.26; // ~15 degrees axial tilt
    core->planet_yaw = 0.0;
    core->planet_pulse = 0.0;
    core->radar_angle = 0.0;
    core->scan_phase = 0.0;
    telemetry_init(&core->telemetry);
}

void orbital_core_update(orbital_core_t *core, double dt) {
    if (!core) return;

    // Smooth continuous planetary rotation (60FPS precision)
    core->planet_yaw += dt * 0.40;
    if (core->planet_yaw > 2.0 * M_PI) core->planet_yaw -= 2.0 * M_PI;

    // Gentle axial precession
    core->planet_pitch = 0.26 + 0.03 * sin(core->planet_yaw * 0.5);

    // Dynamic core pulsation
    core->planet_pulse += dt * 2.2;
    if (core->planet_pulse > 2.0 * M_PI) core->planet_pulse -= 2.0 * M_PI;

    // Fast 360-degree radar / scan sweep
    core->radar_angle += dt * 1.8;
    if (core->radar_angle > 2.0 * M_PI) core->radar_angle -= 2.0 * M_PI;
    core->scan_phase = fmod(core->scan_phase + (dt * 0.5), 1.0);

    // Periodic telemetry sampling
    core->telemetry_timer += dt;
    if (core->telemetry_timer >= 0.50) {
        core->telemetry_timer = 0.0;
        telemetry_update(&core->telemetry);
    }
}
