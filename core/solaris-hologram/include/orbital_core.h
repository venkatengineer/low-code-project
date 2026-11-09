#ifndef SOLARIS_ORBITAL_CORE_H
#define SOLARIS_ORBITAL_CORE_H

#include "common.h"
#include "math3d.h"
#include "telemetry.h"

typedef struct {
    // 3D Planetary Simulation & Rotation
    double planet_yaw;
    double planet_pitch;
    double planet_pulse;
    double radar_angle;
    double scan_phase;

    // Live System Telemetry
    server_telemetry_t telemetry;
    double telemetry_timer;
} orbital_core_t;

// Lifecycle
void orbital_core_init(orbital_core_t *core);
void orbital_core_update(orbital_core_t *core, double dt);

#endif // SOLARIS_ORBITAL_CORE_H
