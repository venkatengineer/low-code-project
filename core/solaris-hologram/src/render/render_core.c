#include "render_core.h"
#include "math3d.h"

static void draw_3d_wireframe_circle(cairo_t *cr, const camera3d_t *cam, double cx, double cy,
                                     double radius, double pitch, double yaw, double roll,
                                     double r, double g, double b, double alpha, double line_width) {
    const int STEPS = 96;
    vec2_t pts[96];
    gboolean valid[96];

    for (int i = 0; i < STEPS; i++) {
        double angle = (2.0 * M_PI * i) / STEPS;
        vec3_t local_pt = vec3_create(radius * cos(angle), 0, radius * sin(angle));
        vec3_t rot_pt = vec3_rotate_euler(local_pt, pitch, yaw, roll);
        valid[i] = project_to_screen(rot_pt, cam, cx, cy, &pts[i], NULL, NULL);
    }

    cairo_set_source_rgba(cr, r, g, b, alpha);
    cairo_set_line_width(cr, line_width);

    gboolean in_path = FALSE;
    for (int i = 0; i < STEPS; i++) {
        int next = (i + 1) % STEPS;
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

static void draw_3d_geodesic_shield(cairo_t *cr, const camera3d_t *cam, double cx, double cy,
                                    double radius, double pitch, double yaw, double roll,
                                    double r, double g, double b, double alpha, double scale) {
    const double t = (1.0 + sqrt(5.0)) / 2.0;
    const double norm = radius / sqrt(1.0 + t * t);

    // 12 vertices of regular icosahedron
    vec3_t v[12] = {
        vec3_create(-norm,  t * norm,  0),
        vec3_create( norm,  t * norm,  0),
        vec3_create(-norm, -t * norm,  0),
        vec3_create( norm, -t * norm,  0),
        vec3_create( 0, -norm,  t * norm),
        vec3_create( 0,  norm,  t * norm),
        vec3_create( 0, -norm, -t * norm),
        vec3_create( 0,  norm, -t * norm),
        vec3_create( t * norm,  0, -norm),
        vec3_create( t * norm,  0,  norm),
        vec3_create(-t * norm,  0, -norm),
        vec3_create(-t * norm,  0,  norm),
    };

    vec2_t pts[12];
    double depths[12];
    gboolean valid[12];

    for (int i = 0; i < 12; i++) {
        vec3_t rot_v = vec3_rotate_euler(v[i], pitch, yaw, roll);
        valid[i] = project_to_screen(rot_v, cam, cx, cy, &pts[i], NULL, &depths[i]);
    }

    // 30 edges of icosahedron
    static const int edges[30][2] = {
        {0,11}, {0,5}, {0,1}, {0,7}, {0,10},
        {1,5}, {5,11}, {11,10}, {10,7}, {7,1},
        {3,9}, {3,4}, {3,2}, {3,6}, {3,8},
        {2,4}, {4,9}, {9,8}, {8,6}, {6,2},
        {4,5}, {5,9}, {9,1}, {1,8}, {8,7},
        {7,6}, {6,10}, {10,2}, {2,11}, {11,4}
    };

    cairo_set_source_rgba(cr, r, g, b, alpha);
    cairo_set_line_width(cr, 1.1 * scale);

    for (int i = 0; i < 30; i++) {
        int e0 = edges[i][0];
        int e1 = edges[i][1];
        if (valid[e0] && valid[e1]) {
            cairo_move_to(cr, pts[e0].x, pts[e0].y);
            cairo_line_to(cr, pts[e1].x, pts[e1].y);
        }
    }
    cairo_stroke(cr);

    // Glowing quantum vertices
    for (int i = 0; i < 12; i++) {
        if (valid[i]) {
            double v_fade = (depths[i] < cam->distance) ? 1.0 : 0.55;
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha * v_fade);
            cairo_arc(cr, pts[i].x, pts[i].y, 2.5 * scale, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    }
}

void render_session_core(cairo_t *cr, const orbital_system_t *sys, double cx, double cy) {
    vec2_t center_sp;
    double scale, depth;
    if (!project_to_screen(vec3_create(0, 0, 0), &sys->camera, cx, cy, &center_sp, &scale, &depth)) {
        return;
    }

    gboolean conn = sys->is_connected;
    double pulse = sys->core_pulse;
    double base_r = 32.0 * scale;

    // 1. Outer Volumetric Plasma Corona (Radial Gradient)
    double glow_r = base_r * (2.8 + (0.4 * pulse));
    cairo_pattern_t *rad_pat = cairo_pattern_create_radial(center_sp.x, center_sp.y, 0,
                                                           center_sp.x, center_sp.y, glow_r);
    if (conn) {
        cairo_pattern_add_color_stop_rgba(rad_pat, 0.0, 1.0, 1.0, 1.0, 0.95);
        cairo_pattern_add_color_stop_rgba(rad_pat, 0.2, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.65);
        cairo_pattern_add_color_stop_rgba(rad_pat, 0.5, COLOR_TEAL_R, COLOR_TEAL_G, COLOR_TEAL_B, 0.25);
        cairo_pattern_add_color_stop_rgba(rad_pat, 0.8, COLOR_VIOLET_R, COLOR_VIOLET_G, COLOR_VIOLET_B, 0.08);
        cairo_pattern_add_color_stop_rgba(rad_pat, 1.0, 0.0, 0.0, 0.0, 0.0);
    } else {
        // Warning / Connection lost amber-red corona
        cairo_pattern_add_color_stop_rgba(rad_pat, 0.0, 1.0, 0.9, 0.9, 0.95);
        cairo_pattern_add_color_stop_rgba(rad_pat, 0.2, COLOR_ALERT_R, 0.4, 0.1, 0.70);
        cairo_pattern_add_color_stop_rgba(rad_pat, 0.6, COLOR_ALERT_R * 0.8, 0.1, 0.1, 0.20);
        cairo_pattern_add_color_stop_rgba(rad_pat, 1.0, 0.0, 0.0, 0.0, 0.0);
    }

    cairo_set_source(cr, rad_pat);
    cairo_arc(cr, center_sp.x, center_sp.y, glow_r, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(rad_pat);

    // 2. Rotating 3D Gyroscopic Gimbal Wireframes & Geodesic Shield
    if (conn) {
        // Outer 3D Geodesic Icosahedron Energy Cage (Cosmic Violet / Electric Cyan)
        draw_3d_geodesic_shield(cr, &sys->camera, cx, cy, 64.0,
                                sys->core_rot_pitch * 0.8, sys->core_rot_yaw * 0.6, sys->core_rot_yaw * -0.4,
                                0.0, 0.94, 1.0, 0.60, scale);

        // Nested Inner 3D Geodesic Core (Quantum Emerald)
        draw_3d_geodesic_shield(cr, &sys->camera, cx, cy, 48.0,
                                sys->core_rot_pitch * -0.7, sys->core_rot_yaw * -0.8, sys->core_rot_yaw * 0.5,
                                0.0, 1.0, 0.40, 0.45, scale);

        // Ring 1 (Equatorial Gyro - Electric Cyan)
        draw_3d_wireframe_circle(cr, &sys->camera, cx, cy, 38.0,
                                 sys->core_rot_pitch, sys->core_rot_yaw, 0.0,
                                 0.0, 0.94, 1.0, 0.90, 1.6 * scale);

        // Ring 2 (Polar Orbit Gyro - Quantum Emerald)
        draw_3d_wireframe_circle(cr, &sys->camera, cx, cy, 46.0,
                                 sys->core_rot_pitch + 1.57, 0.0, sys->core_rot_yaw * 0.8,
                                 0.0, 1.0, 0.835, 0.75, 1.4 * scale);

        // Ring 3 (Outer Containment Field - Amber Gold)
        draw_3d_wireframe_circle(cr, &sys->camera, cx, cy, 56.0,
                                 sys->core_rot_pitch * -0.4, sys->core_rot_yaw * 0.5, 1.2,
                                 1.0, 0.80, 0.15, 0.45, 1.0 * scale);
    } else {
        // Warning / Connection lost amber-red gyros
        draw_3d_wireframe_circle(cr, &sys->camera, cx, cy, 38.0,
                                 sys->core_rot_pitch, sys->core_rot_yaw, 0.0,
                                 COLOR_ALERT_R, 0.3, 0.1, 0.85, 1.5 * scale);
        draw_3d_wireframe_circle(cr, &sys->camera, cx, cy, 48.0,
                                 sys->core_rot_pitch + 1.57, 0.0, sys->core_rot_yaw * 0.8,
                                 1.0, 0.5, 0.0, 0.60, 1.2 * scale);
    }

    // 3. Inner Solid Fusion Reactor Core
    double core_radius = base_r * 0.68;
    cairo_pattern_t *inner_pat = cairo_pattern_create_radial(center_sp.x - (3.0 * scale),
                                                             center_sp.y - (3.0 * scale), 0,
                                                             center_sp.x, center_sp.y, core_radius);
    if (conn) {
        cairo_pattern_add_color_stop_rgba(inner_pat, 0.0, 1.0, 1.0, 1.0, 1.0);
        cairo_pattern_add_color_stop_rgba(inner_pat, 0.35, 0.0, 0.94, 1.0, 0.95);
        cairo_pattern_add_color_stop_rgba(inner_pat, 0.70, 0.45, 0.20, 0.90, 0.90);
        cairo_pattern_add_color_stop_rgba(inner_pat, 1.0, 0.02, 0.06, 0.20, 0.85);
    } else {
        cairo_pattern_add_color_stop_rgba(inner_pat, 0.0, 1.0, 1.0, 1.0, 1.0);
        cairo_pattern_add_color_stop_rgba(inner_pat, 0.5, COLOR_ALERT_R, 0.5, 0.1, 0.95);
        cairo_pattern_add_color_stop_rgba(inner_pat, 1.0, 0.6, 0.0, 0.1, 0.9);
    }
    cairo_set_source(cr, inner_pat);
    cairo_arc(cr, center_sp.x, center_sp.y, core_radius, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(inner_pat);

    // 4. Rotating Reactor Notches (Hexagonal aperture ticks)
    cairo_save(cr);
    cairo_translate(cr, center_sp.x, center_sp.y);
    cairo_rotate(cr, sys->core_rot_yaw * 1.5);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.85);
    cairo_set_line_width(cr, 1.5 * scale);
    for (int k = 0; k < 6; k++) {
        double a = k * (M_PI / 3.0);
        double r1 = core_radius * 0.7;
        double r2 = core_radius * 1.1;
        cairo_move_to(cr, cos(a) * r1, sin(a) * r1);
        cairo_line_to(cr, cos(a) * r2, sin(a) * r2);
    }
    cairo_stroke(cr);
    cairo_restore(cr);

    // 5. Offline Status HUD Tag (only when link is broken)
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
        cairo_move_to(cr, center_sp.x - (tw * 0.5), center_sp.y + core_radius + (12.0 * scale));
        pango_cairo_show_layout(cr, layout);
        g_object_unref(layout);
        cairo_restore(cr);
    }
}
