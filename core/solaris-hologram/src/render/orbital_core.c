#include "orbital_core.h"
#include <math.h>

static void draw_bracket_tab(cairo_t *cr, double x, double y, double w, double h, double c) {
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

void orbital_core_init(orbital_core_t *core) {
    memset(core, 0, sizeof(*core));
    core->current_state = CORE_STATE_ORBITAL;
    core->target_state = CORE_STATE_ORBITAL;
    core->orbital_to_server_t = 0.0;
    core->server_to_terminal_t = 0.0;
    core->hint_opacity = 1.0;
    core->hint_dismissed = FALSE;
    core->hovered_mode_tab = -1;
    core->hovered_hotspot = HOTSPOT_NONE;
    core->hovered_term_btn = -1;

    core->tab_width = 124.0;
    core->tab_height = 24.0;
    core->tabs_y = 12.0;

    telemetry_init(&core->telemetry);
    terminal_init(&core->terminal);
}

void orbital_core_set_state(orbital_core_t *core, core_state_t target) {
    core->target_state = target;
    core->hint_dismissed = TRUE;
}

void orbital_core_toggle_state(orbital_core_t *core) {
    if (core->target_state == CORE_STATE_ORBITAL) {
        orbital_core_set_state(core, CORE_STATE_SERVER_CORE);
    } else {
        orbital_core_set_state(core, CORE_STATE_ORBITAL);
    }
}

void orbital_core_update(orbital_core_t *core, double dt) {
    // 1. Smooth Transition Interpolation (spring damping ease)
    double target_orb_serv = (core->target_state >= CORE_STATE_SERVER_CORE) ? 1.0 : 0.0;
    double target_serv_term = (core->target_state == CORE_STATE_TERMINAL) ? 1.0 : 0.0;

    double speed = 6.0 * dt;
    if (speed > 1.0) speed = 1.0;

    core->orbital_to_server_t += (target_orb_serv - core->orbital_to_server_t) * speed;
    core->server_to_terminal_t += (target_serv_term - core->server_to_terminal_t) * speed;

    if (fabs(core->orbital_to_server_t - target_orb_serv) < 0.002) {
        core->orbital_to_server_t = target_orb_serv;
    }
    if (fabs(core->server_to_terminal_t - target_serv_term) < 0.002) {
        core->server_to_terminal_t = target_serv_term;
        core->current_state = core->target_state;
    }

    // Hint fade out
    if (core->hint_dismissed) {
        core->hint_opacity -= dt * 2.0;
        if (core->hint_opacity < 0.0) core->hint_opacity = 0.0;
    }

    // 2. Continuous 3D Computational Animations
    // Subdued camera sway rather than spinning turntable
    core->server_yaw = 0.08 * sin(core->server_pulse * 0.35);
    core->fan_angle += dt * (2.5 + (core->telemetry.cpu_usage * 0.12));
    core->circuit_pulse += dt * (1.8 + (core->telemetry.cpu_usage * 0.05));
    core->server_pulse += dt * (1.8 + (core->telemetry.cpu_usage * 0.04));
    core->terminal.cursor_blink_timer += dt;

    // 3. Periodic Telemetry Sampling (every 0.5s)
    core->telemetry_timer += dt;
    if (core->telemetry_timer >= 0.50) {
        core->telemetry_timer = 0.0;
        telemetry_update(&core->telemetry);
    }
}

void render_core_mode_selector(cairo_t *cr, const orbital_core_t *core, double cx, double w, double h) {
    cairo_save(cr);

    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *f_tab = pango_font_description_from_string("JetBrainsMono Nerd Font Bold 7.5");
    pango_layout_set_font_description(layout, f_tab);

    const char *labels[3] = {
        "󰨓 ORBITAL",
        "󰒋 VENKAT-SERVER",
        " TERMINAL"
    };

    double tw = core->tab_width;
    double th = core->tab_height;
    double total_w = (tw * 3.0) + (2.0 * 6.0);
    double start_x = cx - (total_w * 0.5);
    double ty = core->tabs_y;

    // Outer backing bar
    draw_bracket_tab(cr, start_x - 4.0, ty - 3.0, total_w + 8.0, th + 6.0, 4.0);
    cairo_set_source_rgba(cr, 0.01, 0.025, 0.05, 0.90);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_TEAL_R, COLOR_TEAL_G, COLOR_TEAL_B, 0.40);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);

    for (int i = 0; i < 3; i++) {
        double tx = start_x + (i * (tw + 6.0));
        gboolean is_active = (core->target_state == (core_state_t)i);
        gboolean is_hovered = (core->hovered_mode_tab == i);

        draw_bracket_tab(cr, tx, ty, tw, th, 3.0);

        if (is_active) {
            // Active Mode: Cyan highlight fill
            cairo_set_source_rgba(cr, COLOR_CYAN_R * 0.25, COLOR_CYAN_G * 0.35, COLOR_CYAN_B * 0.45, 0.95);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 1.0);
            cairo_set_line_width(cr, 1.4);
            cairo_stroke(cr);
        } else if (is_hovered) {
            // Hovered Mode: Subtle teal glow
            cairo_set_source_rgba(cr, COLOR_DEEP_BLUE_R, COLOR_DEEP_BLUE_G, COLOR_DEEP_BLUE_B, 0.85);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, COLOR_TEAL_R, COLOR_TEAL_G, COLOR_TEAL_B, 0.85);
            cairo_set_line_width(cr, 1.0);
            cairo_stroke(cr);
        } else {
            // Inactive Mode
            cairo_set_source_rgba(cr, 0.015, 0.035, 0.07, 0.75);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.45);
            cairo_set_line_width(cr, 0.7);
            cairo_stroke(cr);
        }

        // Label Text
        pango_layout_set_text(layout, labels[i], -1);
        int lw, lh;
        pango_layout_get_pixel_size(layout, &lw, &lh);
        if (is_active) {
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
        } else if (is_hovered) {
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
        } else {
            cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.3, COLOR_TEXT_MUTED_G + 0.3, COLOR_TEXT_MUTED_B + 0.3, 0.85);
        }
        cairo_move_to(cr, tx + (tw - lw) * 0.5, ty + (th - lh) * 0.5);
        pango_cairo_show_layout(cr, layout);
    }

    g_object_unref(layout);
    pango_font_description_free(f_tab);
    cairo_restore(cr);
}

void render_core_context_hint(cairo_t *cr, const orbital_core_t *core, double cx, double cy, double h) {
    if (core->hint_opacity <= 0.001) return;
    if (core->target_state != CORE_STATE_ORBITAL) return;

    cairo_save(cr);
    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *f_hint = pango_font_description_from_string("JetBrainsMono Nerd Font Bold 7");
    pango_layout_set_font_description(layout, f_hint);

    pango_layout_set_text(layout, "◆ 2-FINGER SWIPE RIGHT // ACCESS REMOTE VESSEL (SSH) ◆", -1);
    int tw, th;
    pango_layout_get_pixel_size(layout, &tw, &th);

    double hint_x = cx - (tw * 0.5);
    double hint_y = cy + 180.0;

    // Small backing pill
    draw_bracket_tab(cr, hint_x - 10.0, hint_y - 4.0, tw + 20.0, th + 8.0, 3.0);
    cairo_set_source_rgba(cr, 0.01, 0.02, 0.04, core->hint_opacity * 0.80);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, core->hint_opacity * 0.45);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);

    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, core->hint_opacity * 0.88);
    cairo_move_to(cr, hint_x, hint_y);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);
    pango_font_description_free(f_hint);
    cairo_restore(cr);
}

gboolean orbital_core_hit_test(orbital_core_t *core, double mx, double my, double cx, double cy,
                               int *out_tab, server_hotspot_t *out_hotspot, int *out_term_btn) {
    if (out_tab) *out_tab = -1;
    if (out_hotspot) *out_hotspot = HOTSPOT_NONE;
    if (out_term_btn) *out_term_btn = -1;

    // 1. Check Mode Switcher Tabs
    double tw = core->tab_width;
    double th = core->tab_height;
    double total_w = (tw * 3.0) + (2.0 * 6.0);
    double start_x = cx - (total_w * 0.5);
    double ty = core->tabs_y;

    if (my >= ty && my <= ty + th) {
        for (int i = 0; i < 3; i++) {
            double tx = start_x + (i * (tw + 6.0));
            if (mx >= tx && mx <= tx + tw) {
                if (out_tab) *out_tab = i;
                return TRUE;
            }
        }
    }

    // 2. Check Terminal Buttons (if Terminal open)
    if (core->target_state == CORE_STATE_TERMINAL && core->server_to_terminal_t > 0.5) {
        double term_w = 620.0;
        double term_h = 440.0;
        double term_x = cx - (term_w * 0.5);
        double term_y = cy - (term_h * 0.5) + 12.0;

        // Header Buttons: [  KITTY ] [ - MIN ] [ ✕ CLOSE ]
        double h_btn_w = 66.0;
        for (int b = 0; b < 3; b++) {
            double bx = term_x + term_w - 12.0 - ((3 - b) * (h_btn_w + 4.0));
            double by = term_y + 6.0;
            if (mx >= bx && mx <= bx + h_btn_w && my >= by && my <= by + 20.0) {
                if (out_term_btn) *out_term_btn = b; // 0=kitty, 1=min, 2=close
                return TRUE;
            }
        }

        // Quick Command Bar Buttons: [ FASTFETCH ] [ TOP ] [ DISK ] [ NET ] [ UPTIME ] [ CLEAR ]
        double bar_y = term_y + 32.0 + 4.0;
        double q_w = (term_w - 24.0) / 6.0;
        for (int q = 0; q < 6; q++) {
            double qx = term_x + 12.0 + (q * q_w);
            if (mx >= qx && mx <= qx + q_w && my >= bar_y && my <= bar_y + 20.0) {
                if (out_term_btn) *out_term_btn = 10 + q; // 10=fastfetch, 11=top, etc.
                return TRUE;
            }
        }

        // If clicked anywhere inside terminal window, it handled the click
        if (mx >= term_x && mx <= term_x + term_w && my >= term_y && my <= term_y + term_h) {
            return TRUE;
        }
    }

    // 3. Check Spacecraft View Buttons (in SERVER_CORE mode)
    if (core->target_state == CORE_STATE_SERVER_CORE && core->orbital_to_server_t > 0.5) {
        // Connect Button: [ 󰒋 CONNECT // SSH (venkat-server) ]
        double btn_y = cy + 130.0;
        if (fabs(mx - cx) <= 135.0 && fabs(my - (btn_y + 15.0)) <= 18.0) {
            if (out_hotspot) *out_hotspot = HOTSPOT_CONNECT_BTN;
            return TRUE;
        }

        // Spacecraft Vessel Center Body
        double dx = mx - cx;
        double dy = my - cy;
        if (fabs(dx) <= 75.0 && fabs(dy) <= 50.0) {
            if (out_hotspot) *out_hotspot = HOTSPOT_VESSEL;
            return TRUE;
        }
    }

    return FALSE;
}
