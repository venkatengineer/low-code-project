#include "render_hud.h"
#include "math3d.h"

void render_hud_background_and_grid(cairo_t *cr, const orbital_system_t *sys, double w, double h) {
    double cx = w * 0.5;
    double cy = h * 0.48;

    // 1. Atmospheric Deep-Space Hologram Volumetric Aura
    cairo_save(cr);
    double aura_r = (w < h ? w : h) * 0.52;
    cairo_pattern_t *aura_pat = cairo_pattern_create_radial(cx, cy, 0, cx, cy, aura_r);
    cairo_pattern_add_color_stop_rgba(aura_pat, 0.0, 0.0, 0.85, 1.0, 0.08);
    cairo_pattern_add_color_stop_rgba(aura_pat, 0.35, 0.45, 0.15, 0.85, 0.045);
    cairo_pattern_add_color_stop_rgba(aura_pat, 0.70, 0.0, 0.35, 0.65, 0.02);
    cairo_pattern_add_color_stop_rgba(aura_pat, 1.0, 0.0, 0.0, 0.0, 0.0);
    cairo_set_source(cr, aura_pat);
    cairo_arc(cr, cx, cy, aura_r, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(aura_pat);
    cairo_restore(cr);

    // 2. 3D Volumetric Holographic Projector Emitter Base
    cairo_save(cr);
    const double EMITTER_Y = -60.0;
    const int RAYS = 16;
    double rot_emitter = sys->time_sec * 0.15;

    // Upward Volumetric Holographic Light Columns
    for (int r = 0; r < RAYS; r++) {
        double a = rot_emitter + (2.0 * M_PI * r) / (double)RAYS;
        double rad_base = (r % 2 == 0) ? 90.0 : 180.0;
        vec3_t p_bottom = vec3_create(rad_base * cos(a), EMITTER_Y, rad_base * sin(a));
        vec3_t p_top = vec3_create((rad_base * 0.4) * cos(a + 0.3), EMITTER_Y + 140.0, (rad_base * 0.4) * sin(a + 0.3));

        vec2_t s_b, s_t;
        if (project_to_screen(p_bottom, &sys->camera, cx, cy, &s_b, NULL, NULL) &&
            project_to_screen(p_top, &sys->camera, cx, cy, &s_t, NULL, NULL)) {
            cairo_pattern_t *beam = cairo_pattern_create_linear(s_b.x, s_b.y, s_t.x, s_t.y);
            cairo_pattern_add_color_stop_rgba(beam, 0.0, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.22);
            cairo_pattern_add_color_stop_rgba(beam, 0.5, COLOR_TEAL_R, COLOR_TEAL_G, COLOR_TEAL_B, 0.08);
            cairo_pattern_add_color_stop_rgba(beam, 1.0, 0.0, 0.0, 0.0, 0.0);
            cairo_set_source(cr, beam);
            cairo_set_line_width(cr, (r % 2 == 0 ? 1.4 : 0.8));
            cairo_move_to(cr, s_b.x, s_b.y);
            cairo_line_to(cr, s_t.x, s_t.y);
            cairo_stroke(cr);
            cairo_pattern_destroy(beam);
        }
    }

    // 3D Perspective Plane Grid (Horizontal XZ ground plane at y = -60)
    cairo_set_line_width(cr, 0.7);
    cairo_set_source_rgba(cr, COLOR_TEAL_R, COLOR_TEAL_G, COLOR_TEAL_B, 0.10);

    const int GRID_LINES = 13;
    const double GRID_SPAN = 640.0;
    const double GRID_STEP = GRID_SPAN / (GRID_LINES - 1);

    for (int i = 0; i < GRID_LINES; i++) {
        double offset = -GRID_SPAN * 0.5 + (i * GRID_STEP);
        // Lines parallel to X
        vec3_t p1_x = vec3_create(-GRID_SPAN * 0.5, EMITTER_Y, offset);
        vec3_t p2_x = vec3_create(GRID_SPAN * 0.5, EMITTER_Y, offset);
        vec2_t s1_x, s2_x;
        if (project_to_screen(p1_x, &sys->camera, cx, cy, &s1_x, NULL, NULL) &&
            project_to_screen(p2_x, &sys->camera, cx, cy, &s2_x, NULL, NULL)) {
            cairo_move_to(cr, s1_x.x, s1_x.y);
            cairo_line_to(cr, s2_x.x, s2_x.y);
        }

        // Lines parallel to Z
        vec3_t p1_z = vec3_create(offset, EMITTER_Y, -GRID_SPAN * 0.5);
        vec3_t p2_z = vec3_create(offset, EMITTER_Y, GRID_SPAN * 0.5);
        vec2_t s1_z, s2_z;
        if (project_to_screen(p1_z, &sys->camera, cx, cy, &s1_z, NULL, NULL) &&
            project_to_screen(p2_z, &sys->camera, cx, cy, &s2_z, NULL, NULL)) {
            cairo_move_to(cr, s1_z.x, s1_z.y);
            cairo_line_to(cr, s2_z.x, s2_z.y);
        }
    }
    cairo_stroke(cr);
    cairo_restore(cr);

    // 3. Minimal Sweeping Scanline
    cairo_save(cr);
    double scan_y = h * sys->scanline_phase;
    cairo_pattern_t *scan_pat = cairo_pattern_create_linear(0, scan_y - 24.0, 0, scan_y + 24.0);
    cairo_pattern_add_color_stop_rgba(scan_pat, 0.0, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.0);
    cairo_pattern_add_color_stop_rgba(scan_pat, 0.5, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.06);
    cairo_pattern_add_color_stop_rgba(scan_pat, 1.0, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.0);
    cairo_set_source(cr, scan_pat);
    cairo_rectangle(cr, 0, scan_y - 24.0, w, 48.0);
    cairo_fill(cr);
    cairo_pattern_destroy(scan_pat);
    cairo_restore(cr);
}

static void draw_tech_frame(cairo_t *cr, double x, double y, double w, double h) {
    // Chamfered technical tooltip frame
    double c = 6.0;
    cairo_move_to(cr, x + c, y);
    cairo_line_to(cr, x + w - c, y);
    cairo_line_to(cr, x + w, y + c);
    cairo_line_to(cr, x + w, y + h - c);
    cairo_line_to(cr, x + w - c, y + h);
    cairo_line_to(cr, x + c, y + h);
    cairo_line_to(cr, x, y + h - c);
    cairo_line_to(cr, x, y + c);
    cairo_close_path(cr);

    cairo_set_source_rgba(cr, 0.02, 0.07, 0.13, 0.94);
    cairo_fill_preserve(cr);

    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.85);
    cairo_set_line_width(cr, 1.2);
    cairo_stroke(cr);

    // Subtle header accent line
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.4);
    cairo_move_to(cr, x + 8.0, y + 22.0);
    cairo_line_to(cr, x + w - 8.0, y + 22.0);
    cairo_stroke(cr);
}

void render_hud_hover_card(cairo_t *cr, const orbital_system_t *sys, double mx, double my, double w, double h) {
    if (sys->hovered_window_id == 0 && sys->hovered_workspace_idx == -1 && !sys->hovered_core) {
        return;
    }

    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *title_font = pango_font_description_from_string("JetBrainsMono Nerd Font Bold 8");
    PangoFontDescription *body_font = pango_font_description_from_string("JetBrainsMono Nerd Font 7");

    double card_w = 230.0;
    double card_h = 105.0;
    double card_x = mx + 16.0;
    double card_y = my + 16.0;

    // Keep card inside viewport bounds
    if (card_x + card_w > w - 10.0) card_x = mx - card_w - 16.0;
    if (card_y + card_h > h - 10.0) card_y = my - card_h - 16.0;
    if (card_x < 10.0) card_x = 10.0;
    if (card_y < 10.0) card_y = 10.0;

    cairo_save(cr);
    draw_tech_frame(cr, card_x, card_y, card_w, card_h);

    if (sys->hovered_window_id != 0) {
        // Window Node Telemetry
        const window_info_t *win = NULL;
        for (int i = 0; i < sys->window_count; i++) {
            if (sys->windows[i].id == sys->hovered_window_id) {
                win = &sys->windows[i];
                break;
            }
        }
        if (win) {
            // Header
            pango_layout_set_font_description(layout, title_font);
            char h_buf[64];
            snprintf(h_buf, sizeof(h_buf), "CONSOLE // %s", win->display_name);
            pango_layout_set_text(layout, h_buf, -1);
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 1.0);
            cairo_move_to(cr, card_x + 10.0, card_y + 6.0);
            pango_cairo_show_layout(cr, layout);

            // Body
            pango_layout_set_font_description(layout, body_font);
            char body_buf[256];
            snprintf(body_buf, sizeof(body_buf),
                     "TITLE: %.26s\nPID: %d | WS: #%d\nTYPE: %s (%dx%d)\n[CLICK] Focus | [DBL] Inspect",
                     win->title, win->pid, win->workspace_idx,
                     win->is_floating ? "FLOATING" : "TILED",
                     win->tile_width, win->tile_height);
            pango_layout_set_text(layout, body_buf, -1);
            cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.9);
            cairo_move_to(cr, card_x + 10.0, card_y + 26.0);
            pango_cairo_show_layout(cr, layout);
        }
    } else if (sys->hovered_workspace_idx != -1) {
        // Workspace Telemetry
        const workspace_info_t *ws = NULL;
        for (int i = 0; i < sys->workspace_count; i++) {
            if (sys->workspaces[i].idx == sys->hovered_workspace_idx) {
                ws = &sys->workspaces[i];
                break;
            }
        }
        if (ws) {
            // Header
            pango_layout_set_font_description(layout, title_font);
            char h_buf[64];
            snprintf(h_buf, sizeof(h_buf), "WORKSPACE // %02d [%s]", ws->idx, ws->raw_name);
            pango_layout_set_text(layout, h_buf, -1);
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 1.0);
            cairo_move_to(cr, card_x + 10.0, card_y + 6.0);
            pango_cairo_show_layout(cr, layout);

            // Body
            pango_layout_set_font_description(layout, body_font);
            char body_buf[256];
            snprintf(body_buf, sizeof(body_buf),
                     "STATUS: %s\nCONSOLES: %d OPEN WINDOWS\nCALLSIGN: %s\n[CLICK] Switch Workspace",
                     ws->is_active ? "ACTIVE (ENGAGED)" : "STANDBY (ORBITAL)",
                     ws->window_count,
                     ws->station_callsign);
            pango_layout_set_text(layout, body_buf, -1);
            cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.9);
            cairo_move_to(cr, card_x + 10.0, card_y + 26.0);
            pango_cairo_show_layout(cr, layout);
        }
    } else if (sys->hovered_core) {
        // Session Core Telemetry
        pango_layout_set_font_description(layout, title_font);
        pango_layout_set_text(layout, "SOLARIS // SESSION CORE", -1);
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 1.0);
        cairo_move_to(cr, card_x + 10.0, card_y + 6.0);
        pango_cairo_show_layout(cr, layout);

        pango_layout_set_font_description(layout, body_font);
        char body_buf[256];
        snprintf(body_buf, sizeof(body_buf),
                 "COMPOSITOR: NIRI WAYLAND\nACTIVE STATIONS: %d\nTOTAL WINDOWS: %d\n[DRAG] Rotate Hologram",
                 sys->workspace_count, sys->window_count);
        pango_layout_set_text(layout, body_buf, -1);
        cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.9);
        cairo_move_to(cr, card_x + 10.0, card_y + 26.0);
        pango_cairo_show_layout(cr, layout);
    }

    cairo_restore(cr);

    pango_font_description_free(title_font);
    pango_font_description_free(body_font);
    g_object_unref(layout);
}

void render_hud_connection_status(cairo_t *cr, const orbital_system_t *sys, double w, double h) {
    if (sys->is_connected) return;

    double banner_w = 340.0;
    double banner_h = 44.0;
    double bx = (w - banner_w) * 0.5;
    double by = 16.0;

    cairo_save(cr);
    draw_tech_frame(cr, bx, by, banner_w, banner_h);

    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *font_desc = pango_font_description_from_string("JetBrainsMono Nerd Font Bold 8");
    pango_layout_set_font_description(layout, font_desc);
    pango_font_description_free(font_desc);

    pango_layout_set_text(layout, "▲ SYSTEM LINK // NIRI CONNECTION LOST\nAUTOMATIC RECONNECT SEQUENCE ACTIVE...", -1);
    cairo_set_source_rgba(cr, COLOR_ALERT_R, 0.3, 0.3, 1.0);
    cairo_move_to(cr, bx + 16.0, by + 8.0);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);
    cairo_restore(cr);
}
