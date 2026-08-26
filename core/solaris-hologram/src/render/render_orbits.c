#include "render_orbits.h"
#include "math3d.h"

#define ORBIT_SEGMENTS 128

static void draw_orbit_arc(cairo_t *cr, const orbital_system_t *sys, const workspace_info_t *ws,
                           double cx, double cy, gboolean front_half) {
    vec2_t pts[ORBIT_SEGMENTS];
    double depths[ORBIT_SEGMENTS];
    gboolean valid[ORBIT_SEGMENTS];

    double r = ws->current_radius;
    double incl = ws->inclination;
    double y_h = ws->y_height;

    for (int i = 0; i < ORBIT_SEGMENTS; i++) {
        double angle = (2.0 * M_PI * i) / (double)ORBIT_SEGMENTS;
        vec3_t p3d = vec3_create(
            r * cos(angle),
            y_h + (sin(incl) * r * sin(angle)),
            r * sin(angle)
        );
        valid[i] = project_to_screen(p3d, &sys->camera, cx, cy, &pts[i], NULL, &depths[i]);
    }

    gboolean is_act = ws->is_active;
    gboolean is_hov = (sys->hovered_workspace_idx == ws->idx);
    gboolean is_empty = (ws->window_count == 0);

    color_rgb_t sc = get_station_color(ws->idx);
    double cam_dist = sys->camera.distance;

    double base_alpha = is_act ? 0.98 : (is_hov ? 0.85 : (is_empty ? 0.18 : 0.40));
    double line_w = (is_act ? 2.0 : (is_hov ? 1.6 : (is_empty ? 0.75 : 1.0))) * (sys->camera.fov / cam_dist);

    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    // 1. Multi-tier Neon Bloom for active / hovered orbits
    if (is_act || is_hov) {
        double halo_alpha = is_act ? 0.24 : 0.14;
        cairo_set_source_rgba(cr, sc.r, sc.g, sc.b, halo_alpha);
        cairo_set_line_width(cr, line_w * 4.0);
        gboolean in_path = FALSE;
        for (int i = 0; i < ORBIT_SEGMENTS; i++) {
            int next = (i + 1) % ORBIT_SEGMENTS;
            if (!valid[i] || !valid[next]) { in_path = FALSE; continue; }

            gboolean is_front = ((depths[i] + depths[next]) * 0.5 < cam_dist);
            if (is_front != front_half) { in_path = FALSE; continue; }

            if (!in_path) {
                cairo_move_to(cr, pts[i].x, pts[i].y);
                in_path = TRUE;
            }
            cairo_line_to(cr, pts[next].x, pts[next].y);
        }
        cairo_stroke(cr);

        // Secondary tight core bloom
        cairo_set_source_rgba(cr, sc.r, sc.g, sc.b, halo_alpha * 1.6);
        cairo_set_line_width(cr, line_w * 2.0);
        in_path = FALSE;
        for (int i = 0; i < ORBIT_SEGMENTS; i++) {
            int next = (i + 1) % ORBIT_SEGMENTS;
            if (!valid[i] || !valid[next]) { in_path = FALSE; continue; }

            gboolean is_front = ((depths[i] + depths[next]) * 0.5 < cam_dist);
            if (is_front != front_half) { in_path = FALSE; continue; }

            if (!in_path) {
                cairo_move_to(cr, pts[i].x, pts[i].y);
                in_path = TRUE;
            }
            cairo_line_to(cr, pts[next].x, pts[next].y);
        }
        cairo_stroke(cr);
    }

    // 2. Main Orbital Ring Track
    if (is_act) {
        cairo_set_source_rgba(cr, sc.r, sc.g, sc.b, base_alpha);
    } else if (is_hov) {
        cairo_set_source_rgba(cr, sc.r, sc.g, sc.b, base_alpha);
    } else if (is_empty) {
        cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, base_alpha);
    } else {
        cairo_set_source_rgba(cr, sc.r * 0.70, sc.g * 0.70, sc.b * 0.70, base_alpha);
    }
    cairo_set_line_width(cr, line_w);

    gboolean in_path = FALSE;
    for (int i = 0; i < ORBIT_SEGMENTS; i++) {
        int next = (i + 1) % ORBIT_SEGMENTS;
        if (!valid[i] || !valid[next]) { in_path = FALSE; continue; }

        gboolean is_front = ((depths[i] + depths[next]) * 0.5 < cam_dist);
        if (is_front != front_half) { in_path = FALSE; continue; }

        if (!in_path) {
            cairo_move_to(cr, pts[i].x, pts[i].y);
            in_path = TRUE;
        }
        cairo_line_to(cr, pts[next].x, pts[next].y);
    }
    cairo_stroke(cr);

    // 3. Traveling Photon Wave Packet (Station-Colored Comet with White Hot Nucleus)
    if (is_act || is_hov || !is_empty) {
        const int PULSE_STEPS = 24;
        double pulse_head_ang = ws->pulse_pos;
        double trail_span = M_PI * 0.35;

        for (int p = 0; p < PULSE_STEPS; p++) {
            double frac1 = (double)p / (double)PULSE_STEPS;
            double frac2 = (double)(p + 1) / (double)PULSE_STEPS;
            double a1 = pulse_head_ang - (trail_span * (1.0 - frac1));
            double a2 = pulse_head_ang - (trail_span * (1.0 - frac2));

            vec3_t p1_3d = vec3_create(r * cos(a1), y_h + sin(incl)*r*sin(a1), r * sin(a1));
            vec3_t p2_3d = vec3_create(r * cos(a2), y_h + sin(incl)*r*sin(a2), r * sin(a2));

            vec2_t s1, s2;
            double d1, d2;
            if (project_to_screen(p1_3d, &sys->camera, cx, cy, &s1, NULL, &d1) &&
                project_to_screen(p2_3d, &sys->camera, cx, cy, &s2, NULL, &d2)) {
                gboolean is_front = ((d1 + d2) * 0.5 < cam_dist);
                if (is_front == front_half) {
                    double pulse_alpha = frac2 * (is_act ? 1.0 : 0.65);
                    if (frac2 > 0.85) {
                        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, pulse_alpha);
                    } else {
                        cairo_set_source_rgba(cr, sc.r, sc.g, sc.b, pulse_alpha);
                    }
                    cairo_set_line_width(cr, line_w * (1.1 + frac2 * 2.0));
                    cairo_move_to(cr, s1.x, s1.y);
                    cairo_line_to(cr, s2.x, s2.y);
                    cairo_stroke(cr);
                }
            }
        }
    }
}

void render_orbital_rings_back(cairo_t *cr, const orbital_system_t *sys, double cx, double cy) {
    for (int i = sys->workspace_count - 1; i >= 0; i--) {
        draw_orbit_arc(cr, sys, &sys->workspaces[i], cx, cy, FALSE);
    }
}

void render_orbital_rings_front(cairo_t *cr, const orbital_system_t *sys, double cx, double cy) {
    for (int i = 0; i < sys->workspace_count; i++) {
        draw_orbit_arc(cr, sys, &sys->workspaces[i], cx, cy, TRUE);
    }
}

static void draw_chamfer_rect(cairo_t *cr, double x, double y, double w, double h, double c) {
    cairo_move_to(cr, x + c, y);
    cairo_line_to(cr, x + w - c, y);
    cairo_line_to(cr, x + w, y + c);
    cairo_line_to(cr, x + w, y + h - c);
    cairo_line_to(cr, x + w - c, y + h);
    cairo_line_to(cr, x + c, y + h);
    cairo_line_to(cr, x, y + h - c);
    cairo_line_to(cr, x, y + c);
    cairo_close_path(cr);
}

void render_station_labels_and_controls(cairo_t *cr, const orbital_system_t *sys, double cx, double cy) {
    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *act_font = pango_font_description_from_string("JetBrainsMono Nerd Font Bold 8");
    PangoFontDescription *dim_font = pango_font_description_from_string("JetBrainsMono Nerd Font Bold 7");

    // 1. Render Station Name Indicators
    for (int i = 0; i < sys->workspace_count; i++) {
        const workspace_info_t *ws = &sys->workspaces[i];
        if (ws->label_scale <= 0.1) continue;

        double x = ws->label_screen_pos.x;
        double y = ws->label_screen_pos.y;
        double sc = ws->label_scale;
        color_rgb_t station_col = get_station_color(ws->idx);

        gboolean is_act = ws->is_active;
        gboolean is_hov = (sys->hovered_workspace_idx == ws->idx);
        gboolean is_empty = (ws->window_count == 0);

        cairo_save(cr);

        if (is_act || is_hov) {
            // Luminous Active / Hovered Station Glass Capsule
            pango_layout_set_font_description(layout, act_font);
            char title_buf[96];
            snprintf(title_buf, sizeof(title_buf), "◆ STATION %02d // %s", ws->idx, ws->raw_name);
            pango_layout_set_text(layout, title_buf, -1);

            int tw, th;
            pango_layout_get_pixel_size(layout, &tw, &th);

            double box_w = (tw + 18.0) * sc;
            double box_h = (th + 6.0) * sc;
            double box_x = x - (box_w * 0.5);
            double box_y = y - (box_h * 0.5);

            // Translucent glass body
            draw_chamfer_rect(cr, box_x, box_y, box_w, box_h, 3.5 * sc);
            cairo_set_source_rgba(cr, 0.02, 0.08, 0.16, 0.94);
            cairo_fill_preserve(cr);

            // Neon border with station color
            cairo_set_source_rgba(cr, station_col.r, station_col.g, station_col.b, is_act ? 1.0 : 0.85);
            cairo_set_line_width(cr, (is_act ? 1.5 : 1.2) * sc);
            cairo_stroke(cr);

            // Text
            cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 1.0);
            cairo_move_to(cr, box_x + (9.0 * sc), box_y + (3.0 * sc));
            pango_cairo_show_layout(cr, layout);
        } else {
            // Minimalist Inactive Station Etched Tag along orbit
            pango_layout_set_font_description(layout, dim_font);
            char mini_buf[48];
            snprintf(mini_buf, sizeof(mini_buf), "%02d %s", ws->idx, get_station_short_name(ws->idx));
            pango_layout_set_text(layout, mini_buf, -1);

            int tw, th;
            pango_layout_get_pixel_size(layout, &tw, &th);

            // Tiny station LED pip
            double led_x = x - ((tw + 12.0) * sc * 0.5);
            double led_y = y;
            cairo_set_source_rgba(cr, station_col.r, station_col.g, station_col.b, is_empty ? 0.40 : 0.75);
            cairo_arc(cr, led_x, led_y, 2.0 * sc, 0, 2 * M_PI);
            cairo_fill(cr);

            // Sleek translucent text
            cairo_set_source_rgba(cr, station_col.r * 0.8, station_col.g * 0.8, station_col.b * 0.8, is_empty ? 0.40 : 0.70);
            cairo_move_to(cr, led_x + (6.0 * sc), y - (th * 0.5 * sc));
            pango_cairo_show_layout(cr, layout);
        }

        cairo_restore(cr);
    }

    // 2. Render "+ New Station" Button (Only when hovered or in Command Center)
    if (sys->hovered_create_btn) {
        double bx = sys->create_btn_pos.x;
        double by = sys->create_btn_pos.y;
        double bsc = sys->create_btn_scale;

        cairo_save(cr);
        double btn_r = 14.0 * bsc;
        cairo_arc(cr, bx, by, btn_r, 0, 2 * M_PI);
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.25);
        cairo_fill_preserve(cr);

        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
        cairo_set_line_width(cr, 1.6 * bsc);
        cairo_stroke(cr);

        // Plus symbol
        cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 1.0);
        cairo_set_line_width(cr, 1.8 * bsc);
        cairo_move_to(cr, bx - (4.5 * bsc), by);
        cairo_line_to(cr, bx + (4.5 * bsc), by);
        cairo_move_to(cr, bx, by - (4.5 * bsc));
        cairo_line_to(cr, bx, by + (4.5 * bsc));
        cairo_stroke(cr);

        // Hover label
        pango_layout_set_font_description(layout, act_font);
        pango_layout_set_text(layout, "+ NEW STATION", -1);
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
        cairo_move_to(cr, bx + (18.0 * bsc), by - (6.0 * bsc));
        pango_cairo_show_layout(cr, layout);
        cairo_restore(cr);
    }

    pango_font_description_free(act_font);
    pango_font_description_free(dim_font);
    g_object_unref(layout);
}
