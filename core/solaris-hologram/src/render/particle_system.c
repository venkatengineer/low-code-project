#include "particle_system.h"

static double rand_range(double min, double max) {
    return min + ((double)rand() / (double)RAND_MAX) * (max - min);
}

void particle_system_init(particle_system_t *ps) {
    ps->count = MAX_PARTICLES;
    for (int i = 0; i < ps->count; i++) {
        particle_t *p = &ps->particles[i];
        p->pos = vec3_create(rand_range(-350, 350), rand_range(-120, 120), rand_range(-350, 350));
        p->vel = vec3_create(rand_range(-6, 6), rand_range(-3, 3), rand_range(-6, 6));
        p->max_life = rand_range(4.0, 10.0);
        p->life = rand_range(0.0, p->max_life);
        p->size = rand_range(1.2, 2.8);
        p->brightness = rand_range(0.3, 0.9);
        p->type = (i % 4 == 0) ? 1 : 0; // 25% core sparks
    }
}

void particle_system_update(particle_system_t *ps, double dt, gboolean is_active_pulse) {
    for (int i = 0; i < ps->count; i++) {
        particle_t *p = &ps->particles[i];
        p->life += dt;

        if (p->life >= p->max_life) {
            // Respawn particle
            p->life = 0.0;
            p->max_life = rand_range(4.0, 9.0);
            if (p->type == 1 || is_active_pulse) {
                // Spawn near core and radiate outward
                p->pos = vec3_create(rand_range(-20, 20), rand_range(-15, 15), rand_range(-20, 20));
                double ang = rand_range(0, 2 * M_PI);
                double speed = rand_range(20, 50);
                p->vel = vec3_create(cos(ang) * speed, rand_range(-10, 10), sin(ang) * speed);
                p->size = rand_range(1.5, 3.0);
                p->brightness = 1.0;
            } else {
                p->pos = vec3_create(rand_range(-380, 380), rand_range(-120, 120), rand_range(-380, 380));
                p->vel = vec3_create(rand_range(-8, 8), rand_range(-4, 4), rand_range(-8, 8));
                p->size = rand_range(1.0, 2.2);
                p->brightness = rand_range(0.25, 0.75);
            }
        }

        // Apply velocity
        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;
        p->pos.z += p->vel.z * dt;

        // Gentle orbital swirl around Y-axis
        double dist = sqrt(p->pos.x * p->pos.x + p->pos.z * p->pos.z);
        if (dist > 10.0) {
            double swirl_speed = 0.08 / (1.0 + (dist * 0.005));
            double cur_ang = atan2(p->pos.z, p->pos.x) + (swirl_speed * dt);
            p->pos.x = cos(cur_ang) * dist;
            p->pos.z = sin(cur_ang) * dist;
        }
    }
}

void particle_system_render(const particle_system_t *ps, cairo_t *cr,
                            const camera3d_t *cam, double cx, double cy,
                            gboolean is_connected) {
    for (int i = 0; i < ps->count; i++) {
        const particle_t *p = &ps->particles[i];
        vec2_t sp;
        double scale, depth;
        if (!project_to_screen(p->pos, cam, cx, cy, &sp, &scale, &depth)) {
            continue;
        }

        // Fade with life cycle (sine window)
        double life_factor = sin((p->life / p->max_life) * M_PI);
        if (life_factor < 0.01) continue;

        // Depth fog factor
        double depth_fade = fmax(0.2, fmin(1.0, (500.0 - depth) / 600.0));
        double alpha = p->brightness * life_factor * depth_fade * 0.75;
        double r_pix = p->size * scale;

        if (r_pix < 0.5) continue;

        if (is_connected) {
            if (p->type == 1) {
                // High-energy core plasma spark (White core with cyan/gold bloom)
                if (i % 2 == 0) {
                    cairo_set_source_rgba(cr, 1.0, 0.85, 0.30, alpha);
                } else {
                    cairo_set_source_rgba(cr, 0.30, 0.95, 1.0, alpha);
                }
            } else {
                // Multi-spectrum cosmic nebula stardust (Station hues)
                int col_idx = (i % 6) + 1;
                color_rgb_t p_col = get_station_color(col_idx);
                cairo_set_source_rgba(cr, p_col.r, p_col.g, p_col.b, alpha * 0.85);
            }
        } else {
            // Disconnected alert amber/red dust
            cairo_set_source_rgba(cr, COLOR_ALERT_R, COLOR_ALERT_G * 0.6, COLOR_ALERT_B * 0.4, alpha);
        }

        cairo_arc(cr, sp.x, sp.y, r_pix, 0, 2 * M_PI);
        cairo_fill(cr);
    }
}
