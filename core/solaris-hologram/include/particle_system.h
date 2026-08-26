#ifndef SOLARIS_HOLOGRAM_PARTICLE_SYSTEM_H
#define SOLARIS_HOLOGRAM_PARTICLE_SYSTEM_H

#include "common.h"
#include "math3d.h"

typedef struct {
    vec3_t pos;
    vec3_t vel;
    double life;
    double max_life;
    double size;
    double brightness;
    int type; // 0 = ambient stardust, 1 = core spark, 2 = orbit trail
} particle_t;

typedef struct {
    particle_t particles[MAX_PARTICLES];
    int count;
} particle_system_t;

void particle_system_init(particle_system_t *ps);
void particle_system_update(particle_system_t *ps, double dt, gboolean is_active_pulse);
void particle_system_render(const particle_system_t *ps, cairo_t *cr,
                            const camera3d_t *cam, double cx, double cy,
                            gboolean is_connected);

#endif // SOLARIS_HOLOGRAM_PARTICLE_SYSTEM_H
