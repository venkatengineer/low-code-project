#include "render_core.h"
#include "math3d.h"
#include <math.h>

#define CIRCLE_STEPS 72

// Helper: draw a complete, continuous 3D circle at elevation y_off with radius r
static void draw_3d_wire_circle(cairo_t *cr, const camera3d_t *cam, double cx, double cy,
                                double radius, double y_offset, double pitch, double yaw,
                                double r, double g, double b, double alpha, double line_width) {
    vec2_t pts[CIRCLE_STEPS];
    gboolean valid[CIRCLE_STEPS];

    for (int i = 0; i < CIRCLE_STEPS; i++) {
        double theta = (2.0 * M_PI * i) / (double)CIRCLE_STEPS;
        vec3_t local_pt = vec3_create(radius * cos(theta), y_offset, radius * sin(theta));
        vec3_t rot_pt = vec3_rotate_euler(local_pt, pitch, yaw, 0.0);
        valid[i] = project_to_screen(rot_pt, cam, cx, cy, &pts[i], NULL, NULL);
    }

    cairo_set_source_rgba(cr, r, g, b, alpha);
    cairo_set_line_width(cr, line_width);

    gboolean in_path = FALSE;
    for (int i = 0; i < CIRCLE_STEPS; i++) {
        int next = (i + 1) % CIRCLE_STEPS;
        if (valid[i] && valid[next]) {
            if (!in_path) {
                cairo_move_to(cr, pts[i].x, pts[i].y);
                in_path = TRUE;
            }
            cairo_line_to(cr, pts[next].x, pts[next].y);
        } else {
            in_path = FALSE;
        }
    }
    cairo_stroke(cr);
}

// Helper: draw a complete, continuous 3D vertical meridian great circle at angle lon_rad
static void draw_3d_wire_meridian(cairo_t *cr, const camera3d_t *cam, double cx, double cy,
                                  double radius, double lon_rad, double pitch, double yaw,
                                  double r, double g, double b, double alpha, double line_width) {
    vec2_t pts[CIRCLE_STEPS];
    gboolean valid[CIRCLE_STEPS];

    double cos_lon = cos(lon_rad);
    double sin_lon = sin(lon_rad);

    for (int i = 0; i < CIRCLE_STEPS; i++) {
        double theta = (2.0 * M_PI * i) / (double)CIRCLE_STEPS;
        // Meridian in vertical plane at azimuth lon_rad
        vec3_t local_pt = vec3_create(
            radius * sin(theta) * cos_lon,
            radius * cos(theta),
            radius * sin(theta) * sin_lon
        );
        vec3_t rot_pt = vec3_rotate_euler(local_pt, pitch, yaw, 0.0);
        valid[i] = project_to_screen(rot_pt, cam, cx, cy, &pts[i], NULL, NULL);
    }

    cairo_set_source_rgba(cr, r, g, b, alpha);
    cairo_set_line_width(cr, line_width);

    gboolean in_path = FALSE;
    for (int i = 0; i < CIRCLE_STEPS; i++) {
        int next = (i + 1) % CIRCLE_STEPS;
        if (valid[i] && valid[next]) {
            if (!in_path) {
                cairo_move_to(cr, pts[i].x, pts[i].y);
                in_path = TRUE;
            }
            cairo_line_to(cr, pts[next].x, pts[next].y);
        } else {
            in_path = FALSE;
        }
    }
    cairo_stroke(cr);
}

// Complete, Refined 60FPS Holographic 3D Planet
void render_session_core(cairo_t *cr, const orbital_system_t *sys, double cx, double cy) {
    vec2_t center_sp;
    double scale, depth;
    if (!project_to_screen(vec3_create(0, 0, 0), &sys->camera, cx, cy, &center_sp, &scale, &depth)) {
        return;
    }

    gboolean conn = sys->is_connected;
    double pulse = sys->core_pulse;
    double base_r = 46.0 * scale;

    // 1. Soft Volumetric Atmospheric Glow Corona (Layered radial gradients)
    double corona_r = base_r * (2.4 + (0.25 * pulse));
    cairo_pattern_t *corona_pat = cairo_pattern_create_radial(center_sp.x, center_sp.y, 0,
                                                              center_sp.x, center_sp.y, corona_r);
    if (conn) {
        cairo_pattern_add_color_stop_rgba(corona_pat, 0.0, 1.0, 1.0, 1.0, 0.90);
        cairo_pattern_add_color_stop_rgba(corona_pat, 0.20, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.55);
        cairo_pattern_add_color_stop_rgba(corona_pat, 0.45, COLOR_TEAL_R, COLOR_TEAL_G, COLOR_TEAL_B, 0.22);
        cairo_pattern_add_color_stop_rgba(corona_pat, 0.70, COLOR_VIOLET_R, COLOR_VIOLET_G, COLOR_VIOLET_B, 0.08);
        cairo_pattern_add_color_stop_rgba(corona_pat, 1.0, 0.0, 0.0, 0.0, 0.0);
    } else {
        cairo_pattern_add_color_stop_rgba(corona_pat, 0.0, 1.0, 0.9, 0.9, 0.90);
        cairo_pattern_add_color_stop_rgba(corona_pat, 0.25, COLOR_ALERT_R, 0.4, 0.1, 0.60);
        cairo_pattern_add_color_stop_rgba(corona_pat, 0.65, COLOR_ALERT_R * 0.8, 0.1, 0.1, 0.15);
        cairo_pattern_add_color_stop_rgba(corona_pat, 1.0, 0.0, 0.0, 0.0, 0.0);
    }
    cairo_set_source(cr, corona_pat);
    cairo_arc(cr, center_sp.x, center_sp.y, corona_r, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(corona_pat);

    // 2. Solid Shaded Holographic Core Sphere (Smooth Specular Lighting)
    double core_radius = base_r * 0.82;
    double light_x = center_sp.x - (core_radius * 0.22);
    double light_y = center_sp.y - (core_radius * 0.22);
    cairo_pattern_t *core_pat = cairo_pattern_create_radial(light_x, light_y, 0,
                                                            center_sp.x, center_sp.y, core_radius);
    if (conn) {
        cairo_pattern_add_color_stop_rgba(core_pat, 0.0, 1.0, 1.0, 1.0, 0.98);
        cairo_pattern_add_color_stop_rgba(core_pat, 0.25, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.80);
        cairo_pattern_add_color_stop_rgba(core_pat, 0.65, 0.02, 0.12, 0.22, 0.78);
        cairo_pattern_add_color_stop_rgba(core_pat, 1.0, 0.01, 0.03, 0.08, 0.85);
    } else {
        cairo_pattern_add_color_stop_rgba(core_pat, 0.0, 1.0, 1.0, 1.0, 0.98);
        cairo_pattern_add_color_stop_rgba(core_pat, 0.35, COLOR_ALERT_R, 0.45, 0.1, 0.85);
        cairo_pattern_add_color_stop_rgba(core_pat, 1.0, 0.25, 0.04, 0.04, 0.90);
    }
    cairo_set_source(cr, core_pat);
    cairo_arc(cr, center_sp.x, center_sp.y, core_radius, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(core_pat);

    // 3. Complete, Unbroken 3D Graticule (Spherical Latitude Parallels)
    double sphere_r = 46.0;
    double pitch = sys->core_rot_pitch;
    double yaw = sys->core_rot_yaw;

    // Equator (Prominent Electric Cyan)
    draw_3d_wire_circle(cr, &sys->camera, cx, cy, sphere_r, 0.0, pitch, yaw,
                        0.0, 0.94, 1.0, 0.92, 1.6 * scale);

    // Latitude Parallels: +/- 25 deg, +/- 50 deg, +/- 70 deg (Complete closed circles)
    static const double lat_angles[] = { 0.436, 0.873, 1.222 }; // ~25 deg, 50 deg, 70 deg
    for (int k = 0; k < 3; k++) {
        double r_lat = sphere_r * cos(lat_angles[k]);
        double y_lat = sphere_r * sin(lat_angles[k]);

        // Northern Hemisphere
        draw_3d_wire_circle(cr, &sys->camera, cx, cy, r_lat, y_lat, pitch, yaw,
                            COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.65, 1.1 * scale);

        // Southern Hemisphere
        draw_3d_wire_circle(cr, &sys->camera, cx, cy, r_lat, -y_lat, pitch, yaw,
                            COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.65, 1.1 * scale);
    }

    // 4. Complete, Unbroken 3D Longitude Meridians (6 Great Circles = 12 Visible Halves)
    const int N_MERIDIANS = 6;
    for (int m = 0; m < N_MERIDIANS; m++) {
        double lon = (M_PI * m) / (double)N_MERIDIANS;
        draw_3d_wire_meridian(cr, &sys->camera, cx, cy, sphere_r, lon, pitch, yaw,
                              COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.60, 1.0 * scale);
    }

    // 5. Clean Concentric Planetary Rings (Saturn-Style Smooth Floating Rings, No Spikes)
    if (conn) {
        // Inner Ring
        draw_3d_wire_circle(cr, &sys->camera, cx, cy, sphere_r * 1.48, 0.0, pitch, yaw,
                            0.0, 1.0, 0.835, 0.55, 1.3 * scale);

        // Middle Glowing Ring
        draw_3d_wire_circle(cr, &sys->camera, cx, cy, sphere_r * 1.72, 0.0, pitch, yaw,
                            0.0, 0.94, 1.0, 0.70, 1.5 * scale);

        // Outer Containment Ring
        draw_3d_wire_circle(cr, &sys->camera, cx, cy, sphere_r * 1.95, 0.0, pitch, yaw,
                            COLOR_VIOLET_R, COLOR_VIOLET_G, COLOR_VIOLET_B, 0.40, 1.0 * scale);
    }

    // 6. Offline Status HUD Tag
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
