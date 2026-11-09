#include "particle_system.h"
#include <math.h>

static double rand_range(double min, double max) {
    return min + ((double)rand() / (double)RAND_MAX) * (max - min);
}

void particle_system_init(particle_system_t *ps) {
    ps->count = MAX_PARTICLES;
    for (int i = 0; i < ps->count; i++) {
        particle_t *p = &ps->particles[i];
        p->pos = vec3_create(rand_range(-320, 320), rand_range(-100, 100), rand_range(-320, 320));
        p->vel = vec3_create(rand_range(-5, 5), rand_range(-2, 2), rand_range(-5, 5));
        p->max_life = rand_range(4.0, 9.0);
        p->life = rand_range(0.0, p->max_life);
        p->size = rand_range(1.1, 2.4);
        p->brightness = rand_range(0.3, 0.85);
        p->type = (i % 4 == 0) ? 1 : 0; // 25% sparks
    }
}

void particle_system_update(particle_system_t *ps, double dt, gboolean is_active_pulse) {
    for (int i = 0; i < ps->count; i++) {
        particle_t *p = &ps->particles[i];
        p->life += dt;

        if (p->life >= p->max_life) {
            p->life = 0.0;
            p->max_life = rand_range(4.0, 8.0);
            if (p->type == 1 || is_active_pulse) {
                p->pos = vec3_create(rand_range(-16, 16), rand_range(-12, 12), rand_range(-16, 16));
                double ang = rand_range(0, 2 * M_PI);
                double speed = rand_range(18, 40);
                p->vel = vec3_create(cos(ang) * speed, rand_range(-8, 8), sin(ang) * speed);
                p->size = rand_range(1.3, 2.6);
                p->brightness = 1.0;
            } else {
                p->pos = vec3_create(rand_range(-340, 340), rand_range(-100, 100), rand_range(-340, 340));
                p->vel = vec3_create(rand_range(-6, 6), rand_range(-3, 3), rand_range(-6, 6));
                p->size = rand_range(0.9, 1.8);
                p->brightness = rand_range(0.25, 0.70);
            }
        }

        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;
        p->pos.z += p->vel.z * dt;

        // Gentle orbital swirl around Y-axis
        double dist = sqrt(p->pos.x * p->pos.x + p->pos.z * p->pos.z);
        if (dist > 10.0) {
            double swirl_speed = 0.06 / (1.0 + (dist * 0.005));
            double cur_ang = atan2(p->pos.z, p->pos.x) + (swirl_speed * dt);
            p->pos.x = cos(cur_ang) * dist;
            p->pos.z = sin(cur_ang) * dist;
        }
    }
}

void particle_system_render(const particle_system_t *ps, cairo_t *cr,
                            const camera3d_t *cam, double cx, double cy,
                            gboolean is_connected) {
    cairo_save(cr);

    // Batch 1: White/Gold plasma sparks (Single fill)
    cairo_set_source_rgba(cr, 1.0, 0.95, 0.70, is_connected ? 0.85 : 0.40);
    gboolean has_spark = FALSE;
    for (int i = 0; i < ps->count; i++) {
        const particle_t *p = &ps->particles[i];
        if (p->type != 1) continue;
        vec2_t sp;
        double scale, depth;
        if (!project_to_screen(p->pos, cam, cx, cy, &sp, &scale, &depth)) continue;
        double life_factor = sin((p->life / p->max_life) * M_PI);
        if (life_factor < 0.05) continue;
        double r_pix = p->size * scale;
        if (r_pix < 0.5) continue;

        cairo_new_sub_path(cr);
        cairo_arc(cr, sp.x, sp.y, r_pix, 0, 2 * M_PI);
        has_spark = TRUE;
    }
    if (has_spark) cairo_fill(cr);

    // Batch 2: Cyan/Emerald cosmic stardust (Single fill)
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, is_connected ? 0.50 : 0.25);
    gboolean has_dust = FALSE;
    for (int i = 0; i < ps->count; i++) {
        const particle_t *p = &ps->particles[i];
        if (p->type == 1) continue;
        vec2_t sp;
        double scale, depth;
        if (!project_to_screen(p->pos, cam, cx, cy, &sp, &scale, &depth)) continue;
        double life_factor = sin((p->life / p->max_life) * M_PI);
        if (life_factor < 0.05) continue;
        double r_pix = p->size * scale;
        if (r_pix < 0.5) continue;

        cairo_new_sub_path(cr);
        cairo_arc(cr, sp.x, sp.y, r_pix, 0, 2 * M_PI);
        has_dust = TRUE;
    }
    if (has_dust) cairo_fill(cr);

    cairo_restore(cr);
}
