#include "render_core.h"
#include "math3d.h"
#include <math.h>

#define LAT_STEPS 48
#define MERIDIAN_STEPS 32

// Primary 60FPS Holographic Planetary Core Renderer (Batched Cairo Operations)
void render_session_core(cairo_t *cr, const orbital_system_t *sys, double cx, double cy) {
    vec2_t center_sp;
    double scale, depth;
    if (!project_to_screen(vec3_create(0, 0, 0), &sys->camera, cx, cy, &center_sp, &scale, &depth)) {
        return;
    }

    gboolean conn = sys->is_connected;
    double pulse = sys->core_pulse;
    double base_r = 38.0 * scale;

    // 1. Atmospheric Volumetric Glow & Corona
    double corona_r = base_r * (2.8 + 0.35 * pulse);
    cairo_pattern_t *corona_pat = cairo_pattern_create_radial(center_sp.x, center_sp.y, 0,
                                                              center_sp.x, center_sp.y, corona_r);
    if (conn) {
        cairo_pattern_add_color_stop_rgba(corona_pat, 0.0, 1.0, 1.0, 1.0, 0.95);
        cairo_pattern_add_color_stop_rgba(corona_pat, 0.18, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.70);
        cairo_pattern_add_color_stop_rgba(corona_pat, 0.45, COLOR_TEAL_R, COLOR_TEAL_G, COLOR_TEAL_B, 0.28);
        cairo_pattern_add_color_stop_rgba(corona_pat, 0.75, COLOR_VIOLET_R, COLOR_VIOLET_G, COLOR_VIOLET_B, 0.10);
        cairo_pattern_add_color_stop_rgba(corona_pat, 1.0, 0.0, 0.0, 0.0, 0.0);
    } else {
        cairo_pattern_add_color_stop_rgba(corona_pat, 0.0, 1.0, 0.9, 0.9, 0.95);
        cairo_pattern_add_color_stop_rgba(corona_pat, 0.25, COLOR_ALERT_R, 0.4, 0.1, 0.75);
        cairo_pattern_add_color_stop_rgba(corona_pat, 0.65, COLOR_ALERT_R * 0.8, 0.1, 0.1, 0.20);
        cairo_pattern_add_color_stop_rgba(corona_pat, 1.0, 0.0, 0.0, 0.0, 0.0);
    }
    cairo_set_source(cr, corona_pat);
    cairo_arc(cr, center_sp.x, center_sp.y, corona_r, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(corona_pat);

    // 2. Translucent Planetary Core Sphere (Spherical 3D Shading)
    double core_radius = base_r * 0.72;
    double light_off_x = center_sp.x - (core_radius * 0.28);
    double light_off_y = center_sp.y - (core_radius * 0.28);
    cairo_pattern_t *sphere_pat = cairo_pattern_create_radial(light_off_x, light_off_y, 0,
                                                              center_sp.x, center_sp.y, core_radius);
    if (conn) {
        cairo_pattern_add_color_stop_rgba(sphere_pat, 0.0, 1.0, 1.0, 1.0, 0.98);
        cairo_pattern_add_color_stop_rgba(sphere_pat, 0.30, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.85);
        cairo_pattern_add_color_stop_rgba(sphere_pat, 0.70, 0.04, 0.16, 0.28, 0.82);
        cairo_pattern_add_color_stop_rgba(sphere_pat, 1.0, 0.01, 0.04, 0.10, 0.90);
    } else {
        cairo_pattern_add_color_stop_rgba(sphere_pat, 0.0, 1.0, 1.0, 1.0, 0.98);
        cairo_pattern_add_color_stop_rgba(sphere_pat, 0.40, COLOR_ALERT_R, 0.45, 0.1, 0.85);
        cairo_pattern_add_color_stop_rgba(sphere_pat, 1.0, 0.35, 0.05, 0.05, 0.90);
    }
    cairo_set_source(cr, sphere_pat);
    cairo_arc(cr, center_sp.x, center_sp.y, core_radius, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(sphere_pat);

    // 3. 3D Wireframe Graticule (Batched Back and Front Passes)
    double sphere_3d_r = 44.0;
    double p_pitch = sys->core_rot_pitch;
    double p_yaw = sys->core_rot_yaw;
    double cam_dist = sys->camera.distance;

    // A. Back-side Wireframe (Dimmed Depth Cue)
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.20);
    cairo_set_line_width(cr, 0.85 * scale);

    // 5 Latitudes: -60, -30, 0 (Equator), +30, +60
    static const double lats[] = { -1.05, -0.52, 0.0, 0.52, 1.05 };
    for (int k = 0; k < 5; k++) {
        double circle_r = sphere_3d_r * cos(lats[k]);
        double circle_y = sphere_3d_r * sin(lats[k]);

        gboolean in_p = FALSE;
        for (int i = 0; i < LAT_STEPS; i++) {
            int next = (i + 1) % LAT_STEPS;
            double a1 = (2.0 * M_PI * i) / (double)LAT_STEPS;
            double a2 = (2.0 * M_PI * next) / (double)LAT_STEPS;

            vec3_t p1 = vec3_rotate_euler(vec3_create(circle_r * cos(a1), circle_y, circle_r * sin(a1)), p_pitch, p_yaw, 0.0);
            vec3_t p2 = vec3_rotate_euler(vec3_create(circle_r * cos(a2), circle_y, circle_r * sin(a2)), p_pitch, p_yaw, 0.0);

            vec2_t s1, s2;
            double d1, d2;
            if (project_to_screen(p1, &sys->camera, cx, cy, &s1, NULL, &d1) &&
                project_to_screen(p2, &sys->camera, cx, cy, &s2, NULL, &d2)) {
                if (((d1 + d2) * 0.5) >= cam_dist) {
                    if (!in_p) { cairo_move_to(cr, s1.x, s1.y); in_p = TRUE; }
                    cairo_line_to(cr, s2.x, s2.y);
                } else {
                    in_p = FALSE;
                }
            } else {
                in_p = FALSE;
            }
        }
    }

    // 8 Longitude Meridians (Back)
    for (int m = 0; m < 8; m++) {
        double lon = (M_PI * m) / 8.0;
        gboolean in_p = FALSE;
        for (int i = 0; i < MERIDIAN_STEPS - 1; i++) {
            double phi1 = -M_PI * 0.5 + (M_PI * i) / (double)(MERIDIAN_STEPS - 1);
            double phi2 = -M_PI * 0.5 + (M_PI * (i + 1)) / (double)(MERIDIAN_STEPS - 1);

            vec3_t p1 = vec3_rotate_euler(vec3_create(sphere_3d_r * cos(phi1) * cos(lon), sphere_3d_r * sin(phi1), sphere_3d_r * cos(phi1) * sin(lon)), p_pitch, p_yaw, 0.0);
            vec3_t p2 = vec3_rotate_euler(vec3_create(sphere_3d_r * cos(phi2) * cos(lon), sphere_3d_r * sin(phi2), sphere_3d_r * cos(phi2) * sin(lon)), p_pitch, p_yaw, 0.0);

            vec2_t s1, s2;
            double d1, d2;
            if (project_to_screen(p1, &sys->camera, cx, cy, &s1, NULL, &d1) &&
                project_to_screen(p2, &sys->camera, cx, cy, &s2, NULL, &d2)) {
                if (((d1 + d2) * 0.5) >= cam_dist) {
                    if (!in_p) { cairo_move_to(cr, s1.x, s1.y); in_p = TRUE; }
                    cairo_line_to(cr, s2.x, s2.y);
                } else {
                    in_p = FALSE;
                }
            } else {
                in_p = FALSE;
            }
        }
    }
    cairo_stroke(cr);

    // B. Front-side Wireframe (Crisp Luminous Cyan)
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.85);
    cairo_set_line_width(cr, 1.35 * scale);

    for (int k = 0; k < 5; k++) {
        double circle_r = sphere_3d_r * cos(lats[k]);
        double circle_y = sphere_3d_r * sin(lats[k]);

        gboolean in_p = FALSE;
        for (int i = 0; i < LAT_STEPS; i++) {
            int next = (i + 1) % LAT_STEPS;
            double a1 = (2.0 * M_PI * i) / (double)LAT_STEPS;
            double a2 = (2.0 * M_PI * next) / (double)LAT_STEPS;

            vec3_t p1 = vec3_rotate_euler(vec3_create(circle_r * cos(a1), circle_y, circle_r * sin(a1)), p_pitch, p_yaw, 0.0);
            vec3_t p2 = vec3_rotate_euler(vec3_create(circle_r * cos(a2), circle_y, circle_r * sin(a2)), p_pitch, p_yaw, 0.0);

            vec2_t s1, s2;
            double d1, d2;
            if (project_to_screen(p1, &sys->camera, cx, cy, &s1, NULL, &d1) &&
                project_to_screen(p2, &sys->camera, cx, cy, &s2, NULL, &d2)) {
                if (((d1 + d2) * 0.5) < cam_dist) {
                    if (!in_p) { cairo_move_to(cr, s1.x, s1.y); in_p = TRUE; }
                    cairo_line_to(cr, s2.x, s2.y);
                } else {
                    in_p = FALSE;
                }
            } else {
                in_p = FALSE;
            }
        }
    }

    // 8 Longitude Meridians (Front)
    for (int m = 0; m < 8; m++) {
        double lon = (M_PI * m) / 8.0;
        gboolean in_p = FALSE;
        for (int i = 0; i < MERIDIAN_STEPS - 1; i++) {
            double phi1 = -M_PI * 0.5 + (M_PI * i) / (double)(MERIDIAN_STEPS - 1);
            double phi2 = -M_PI * 0.5 + (M_PI * (i + 1)) / (double)(MERIDIAN_STEPS - 1);

            vec3_t p1 = vec3_rotate_euler(vec3_create(sphere_3d_r * cos(phi1) * cos(lon), sphere_3d_r * sin(phi1), sphere_3d_r * cos(phi1) * sin(lon)), p_pitch, p_yaw, 0.0);
            vec3_t p2 = vec3_rotate_euler(vec3_create(sphere_3d_r * cos(phi2) * cos(lon), sphere_3d_r * sin(phi2), sphere_3d_r * cos(phi2) * sin(lon)), p_pitch, p_yaw, 0.0);

            vec2_t s1, s2;
            double d1, d2;
            if (project_to_screen(p1, &sys->camera, cx, cy, &s1, NULL, &d1) &&
                project_to_screen(p2, &sys->camera, cx, cy, &s2, NULL, &d2)) {
                if (((d1 + d2) * 0.5) < cam_dist) {
                    if (!in_p) { cairo_move_to(cr, s1.x, s1.y); in_p = TRUE; }
                    cairo_line_to(cr, s2.x, s2.y);
                } else {
                    in_p = FALSE;
                }
            } else {
                in_p = FALSE;
            }
        }
    }
    cairo_stroke(cr);

    // 4. Equatorial Navigation Reticle & Ticks (radius 66.0)
    double dial_r = 64.0;
    cairo_set_source_rgba(cr, 0.0, 1.0, 0.835, 0.50);
    cairo_set_line_width(cr, 1.1 * scale);

    const int TICKS = 24;
    for (int i = 0; i < TICKS; i++) {
        double angle = (2.0 * M_PI * i) / (double)TICKS;
        gboolean is_major = (i % 6 == 0);
        double tick_len = (is_major ? 7.0 : 3.5) * scale;

        vec3_t p1 = vec3_rotate_euler(vec3_create(dial_r * cos(angle), 0, dial_r * sin(angle)), p_pitch, p_yaw, 0.0);
        vec3_t p2 = vec3_rotate_euler(vec3_create((dial_r + tick_len) * cos(angle), 0, (dial_r + tick_len) * sin(angle)), p_pitch, p_yaw, 0.0);

        vec2_t s1, s2;
        double d1;
        if (project_to_screen(p1, &sys->camera, cx, cy, &s1, NULL, &d1) &&
            project_to_screen(p2, &sys->camera, cx, cy, &s2, NULL, NULL)) {
            if (d1 < cam_dist) {
                cairo_move_to(cr, s1.x, s1.y);
                cairo_line_to(cr, s2.x, s2.y);
            }
        }
    }
    cairo_stroke(cr);

    // 5. Rotating 360-Degree Radar / Laser Sweep Beam (eDEX-UI Signature)
    if (conn) {
        cairo_save(cr);
        double sweep_ang = sys->core.radar_angle;
        double sweep_span = M_PI * 0.25; // ~45 degrees fading trail
        const int SWEEP_STEPS = 12;

        for (int s = 0; s < SWEEP_STEPS; s++) {
            double frac1 = (double)s / (double)SWEEP_STEPS;
            double frac2 = (double)(s + 1) / (double)SWEEP_STEPS;
            double a1 = sweep_ang - (sweep_span * (1.0 - frac1));
            double a2 = sweep_ang - (sweep_span * (1.0 - frac2));

            vec3_t sp1 = vec3_rotate_euler(vec3_create(sphere_3d_r * 1.05 * cos(a1), 0, sphere_3d_r * 1.05 * sin(a1)), p_pitch, p_yaw, 0.0);
            vec3_t sp2 = vec3_rotate_euler(vec3_create(sphere_3d_r * 1.05 * cos(a2), 0, sphere_3d_r * 1.05 * sin(a2)), p_pitch, p_yaw, 0.0);

            vec2_t sc1, sc2;
            double d1, d2;
            if (project_to_screen(sp1, &sys->camera, cx, cy, &sc1, NULL, &d1) &&
                project_to_screen(sp2, &sys->camera, cx, cy, &sc2, NULL, &d2)) {
                if (((d1 + d2) * 0.5) < cam_dist) {
                    double sweep_alpha = frac2 * 0.40;
                    cairo_set_source_rgba(cr, 0.0, 1.0, 0.835, sweep_alpha);
                    cairo_move_to(cr, center_sp.x, center_sp.y);
                    cairo_line_to(cr, sc1.x, sc1.y);
                    cairo_line_to(cr, sc2.x, sc2.y);
                    cairo_close_path(cr);
                    cairo_fill(cr);
                }
            }
        }

        // Leading laser line
        vec3_t lead_p = vec3_rotate_euler(vec3_create(sphere_3d_r * 1.12 * cos(sweep_ang), 0, sphere_3d_r * 1.12 * sin(sweep_ang)), p_pitch, p_yaw, 0.0);
        vec2_t lead_sc;
        double lead_d;
        if (project_to_screen(lead_p, &sys->camera, cx, cy, &lead_sc, NULL, &lead_d)) {
            if (lead_d < cam_dist) {
                cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.90);
                cairo_set_line_width(cr, 1.6 * scale);
                cairo_move_to(cr, center_sp.x, center_sp.y);
                cairo_line_to(cr, lead_sc.x, lead_sc.y);
                cairo_stroke(cr);
            }
        }
        cairo_restore(cr);
    }

    // 6. North & South Polar Beacons (White-Hot Pivots)
    vec3_t np = vec3_rotate_euler(vec3_create(0, sphere_3d_r, 0), p_pitch, p_yaw, 0.0);
    vec3_t sp = vec3_rotate_euler(vec3_create(0, -sphere_3d_r, 0), p_pitch, p_yaw, 0.0);
    vec2_t np_sc, sp_sc;
    if (project_to_screen(np, &sys->camera, cx, cy, &np_sc, NULL, NULL)) {
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
        cairo_arc(cr, np_sc.x, np_sc.y, 2.5 * scale, 0, 2 * M_PI);
        cairo_fill(cr);
    }
    if (project_to_screen(sp, &sys->camera, cx, cy, &sp_sc, NULL, NULL)) {
        cairo_set_source_rgba(cr, 0.0, 0.94, 1.0, 0.80);
        cairo_arc(cr, sp_sc.x, sp_sc.y, 2.0 * scale, 0, 2 * M_PI);
        cairo_fill(cr);
    }

    // 7. Offline Status HUD Tag
    if (!conn) {
        cairo_save(cr);
        PangoLayout *layout = pango_cairo_create_layout(cr);
        PangoFontDescription *font_desc = pango_font_description_from_string("JetBrainsMono Nerd Font Bold 7");
        pango_layout_set_font_description(layout, font_desc);
        pango_font_description_free(font_desc);

        pango_layout_set_text(layout, "LINK OFFLINE // RETRYING", -1);
        cairo_set_source_rgba(cr, COLOR_ALERT_R, 0.6, 0.2, 0.95);

        int tw, th;
        pango_layout_get_pixel_size(layout, &tw, &th);
        cairo_move_to(cr, center_sp.x - (tw * 0.5), center_sp.y + core_radius + (14.0 * scale));
        pango_cairo_show_layout(cr, layout);
        g_object_unref(layout);
        cairo_restore(cr);
    }
}
