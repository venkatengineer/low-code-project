#include "cairo_utils.h"
#include "common.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cairo_draw_beveled_panel(cairo_t *cr, double x, double y, double w, double h, double chamfer) {
    cairo_save(cr);

    // Beveled path
    cairo_new_path(cr);
    cairo_move_to(cr, x + chamfer, y);
    cairo_line_to(cr, x + w - chamfer, y);
    cairo_line_to(cr, x + w, y + chamfer);
    cairo_line_to(cr, x + w, y + h - chamfer);
    cairo_line_to(cr, x + w - chamfer, y + h);
    cairo_line_to(cr, x + chamfer, y + h);
    cairo_line_to(cr, x, y + h - chamfer);
    cairo_line_to(cr, x, y + chamfer);
    cairo_close_path(cr);

    // Spacecraft titanium graphite background
    cairo_pattern_t *pat = cairo_pattern_create_linear(x, y, x, y + h);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.035, 0.050, 0.070, 0.92);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.015, 0.024, 0.036, 0.96);
    cairo_set_source(cr, pat);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(pat);

    // Outer cyber border
    cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A);
    cairo_set_line_width(cr, 1.2);
    cairo_stroke(cr);

    // Corner decorative sci-fi brackets
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.85);
    cairo_set_line_width(cr, 1.8);
    // Top-left
    cairo_move_to(cr, x, y + chamfer + 10);
    cairo_line_to(cr, x, y + chamfer);
    cairo_line_to(cr, x + chamfer, y);
    cairo_line_to(cr, x + chamfer + 10, y);
    cairo_stroke(cr);

    // Bottom-right
    cairo_move_to(cr, x + w, y + h - chamfer - 10);
    cairo_line_to(cr, x + w, y + h - chamfer);
    cairo_line_to(cr, x + w - chamfer, y + h);
    cairo_line_to(cr, x + w - chamfer - 10, y + h);
    cairo_stroke(cr);

    cairo_restore(cr);
}

void cairo_draw_module_header(cairo_t *cr, double x, double y, double w, const char *title, double r, double g, double b) {
    cairo_save(cr);

    // Header label
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 10.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.95);
    cairo_move_to(cr, x + 10, y + 15);
    cairo_show_text(cr, title);

    // Glowing status diamond / dot
    cairo_set_source_rgba(cr, r, g, b, 0.95);
    cairo_arc(cr, x + w - 12, y + 11, 3.5, 0, 2 * M_PI);
    cairo_fill(cr);

    // Glow ring
    cairo_set_source_rgba(cr, r, g, b, 0.40);
    cairo_arc(cr, x + w - 12, y + 11, 6.5, 0, 2 * M_PI);
    cairo_stroke(cr);

    // Thin separator line
    cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.30);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, x + 8, y + 23);
    cairo_line_to(cr, x + w - 8, y + 23);
    cairo_stroke(cr);

    cairo_restore(cr);
}

void cairo_draw_sci_fi_speedometer(cairo_t *cr, double cx, double cy, double radius,
                                   double usage_pct, double freq_ghz, double temp_c,
                                   const char *model_name, double phase) {
    cairo_save(cr);

    double start_angle = 5.0 * M_PI / 6.0;   // 150 deg
    double end_angle = 13.0 * M_PI / 6.0;    // 390 deg (or 30 deg)
    double total_sweep = 4.0 * M_PI / 3.0;   // 240 deg sweep

    // 1. Outer Titanium Bezel Track
    cairo_set_source_rgba(cr, 0.05, 0.08, 0.12, 0.75);
    cairo_set_line_width(cr, 8.0);
    cairo_arc(cr, cx, cy, radius, start_angle, end_angle);
    cairo_stroke(cr);

    // 2. Bezel Tick Marks
    int ticks = 11;
    for (int i = 0; i < ticks; i++) {
        double a = start_angle + (double)i * (total_sweep / (double)(ticks - 1));
        double r1 = radius - 7.0;
        double r2 = radius + 6.0;
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, (i % 2 == 0) ? 0.75 : 0.35);
        cairo_set_line_width(cr, (i % 2 == 0) ? 1.5 : 1.0);
        cairo_move_to(cr, cx + r1 * cos(a), cy + r1 * sin(a));
        cairo_line_to(cr, cx + r2 * cos(a), cy + r2 * sin(a));
        cairo_stroke(cr);
    }

    // 3. Active Speedometer Arc
    double norm = usage_pct / 100.0;
    if (norm < 0.0) norm = 0.0;
    if (norm > 1.0) norm = 1.0;
    double cur_angle = start_angle + total_sweep * norm;

    if (norm > 0.005) {
        double col_r = (usage_pct > 75) ? COLOR_ALERT_R : ((usage_pct > 40) ? COLOR_CYAN_R : COLOR_GREEN_R);
        double col_g = (usage_pct > 75) ? COLOR_ALERT_G : ((usage_pct > 40) ? COLOR_CYAN_G : COLOR_GREEN_G);
        double col_b = (usage_pct > 75) ? COLOR_ALERT_B : ((usage_pct > 40) ? COLOR_CYAN_B : COLOR_GREEN_B);

        cairo_set_source_rgba(cr, col_r, col_g, col_b, 0.95);
        cairo_set_line_width(cr, 8.0);
        cairo_arc(cr, cx, cy, radius, start_angle, cur_angle);
        cairo_stroke(cr);

        // Glowing bloom arc
        cairo_set_source_rgba(cr, col_r, col_g, col_b, 0.35);
        cairo_set_line_width(cr, 14.0);
        cairo_arc(cr, cx, cy, radius, start_angle, cur_angle);
        cairo_stroke(cr);
    }

    // 4. Indicator Needle
    double nx = cx + (radius + 2.0) * cos(cur_angle);
    double ny = cy + (radius + 2.0) * sin(cur_angle);
    double ix = cx + (radius * 0.45) * cos(cur_angle);
    double iy = cy + (radius * 0.45) * sin(cur_angle);

    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
    cairo_set_line_width(cr, 2.5);
    cairo_move_to(cr, ix, iy);
    cairo_line_to(cr, nx, ny);
    cairo_stroke(cr);

    // Tip glow
    cairo_arc(cr, nx, ny, 3.0, 0, 2 * M_PI);
    cairo_fill(cr);

    // 5. Inner Core Digital HUD Readouts
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

    // Percentage
    char pct_str[32];
    snprintf(pct_str, sizeof(pct_str), "%.1f%%", usage_pct);
    cairo_set_font_size(cr, 14.0);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
    cairo_text_extents_t text;
    cairo_text_extents(cr, pct_str, &text);
    cairo_move_to(cr, cx - text.width / 2.0, cy - 2.0);
    cairo_show_text(cr, pct_str);

    // Frequency
    char freq_str[32];
    snprintf(freq_str, sizeof(freq_str), "%.2f GHz", freq_ghz);
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
    cairo_text_extents(cr, freq_str, &text);
    cairo_move_to(cr, cx - text.width / 2.0, cy + 12.0);
    cairo_show_text(cr, freq_str);

    // Real Temperature
    char temp_str[32];
    snprintf(temp_str, sizeof(temp_str), "%.1f°C", temp_c);
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, (temp_c < 75) ? COLOR_GREEN_R : COLOR_ALERT_R,
                              (temp_c < 75) ? COLOR_GREEN_G : COLOR_ALERT_G,
                              (temp_c < 75) ? COLOR_GREEN_B : COLOR_ALERT_B, 0.95);
    cairo_text_extents(cr, temp_str, &text);
    cairo_move_to(cr, cx - text.width / 2.0, cy + 24.0);
    cairo_show_text(cr, temp_str);

    cairo_restore(cr);
}

void cairo_draw_spacecraft_warp_core(cairo_t *cr, double cx, double cy, double radius,
                                     double usage_pct, double freq_ghz, double phase,
                                     const char *model_name) {
    cairo_save(cr);

    // Outer Octagonal Containment Bezel
    int sides = 8;
    cairo_new_path(cr);
    for (int i = 0; i < sides; i++) {
        double a = (double)i * (2.0 * M_PI / sides) - (M_PI / 8.0);
        double px = cx + (radius + 6.0) * cos(a);
        double py = cy + (radius + 6.0) * sin(a);
        if (i == 0) cairo_move_to(cr, px, py);
        else cairo_line_to(cr, px, py);
    }
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, 0.02, 0.04, 0.06, 0.85);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.45);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);

    // Outer Rotating Confinement Rings
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.25);
    cairo_set_line_width(cr, 1.2);
    cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
    cairo_stroke(cr);

    // Magnetic Confinement Segmented Arcs
    int segs = 12;
    double seg_arc = (2.0 * M_PI) / (double)segs;
    for (int i = 0; i < segs; i++) {
        double sa = (double)i * seg_arc + phase * 0.5;
        double ea = sa + seg_arc * 0.65;
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, (i % 2 == 0) ? 0.75 : 0.35);
        cairo_set_line_width(cr, (i % 2 == 0) ? 2.5 : 1.5);
        cairo_arc(cr, cx, cy, radius - 4.0, sa, ea);
        cairo_stroke(cr);
    }

    // Counter-Rotating Inner Plasma Arc
    double inner_r = radius * 0.72;
    for (int i = 0; i < 6; i++) {
        double sa = (double)i * (M_PI / 3.0) - phase * 0.8;
        double ea = sa + (M_PI / 6.0);
        cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.8);
        cairo_set_line_width(cr, 2.0);
        cairo_arc(cr, cx, cy, inner_r, sa, ea);
        cairo_stroke(cr);
    }

    // Central Plasma Core (Pulsating based on load)
    double core_pulse = 1.0 + 0.15 * sin(phase * 2.5);
    double core_r = (radius * 0.44) * core_pulse;
    cairo_pattern_t *cpat = cairo_pattern_create_radial(cx, cy, 0, cx, cy, core_r);

    double cr_r = (usage_pct > 75) ? COLOR_ALERT_R : ((usage_pct > 40) ? COLOR_CYAN_R : COLOR_GREEN_R);
    double cr_g = (usage_pct > 75) ? COLOR_ALERT_G : ((usage_pct > 40) ? COLOR_CYAN_G : COLOR_GREEN_G);
    double cr_b = (usage_pct > 75) ? COLOR_ALERT_B : ((usage_pct > 40) ? COLOR_CYAN_B : COLOR_GREEN_B);

    cairo_pattern_add_color_stop_rgba(cpat, 0.0, 1.0, 1.0, 1.0, 0.95);
    cairo_pattern_add_color_stop_rgba(cpat, 0.4, cr_r, cr_g, cr_b, 0.85);
    cairo_pattern_add_color_stop_rgba(cpat, 1.0, cr_r, cr_g, cr_b, 0.0);

    cairo_set_source(cr, cpat);
    cairo_arc(cr, cx, cy, core_r, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(cpat);

    // Crosshair Targeting Reticle
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, cx - radius - 2, cy);
    cairo_line_to(cr, cx - inner_r, cy);
    cairo_move_to(cr, cx + inner_r, cy);
    cairo_line_to(cr, cx + radius + 2, cy);
    cairo_move_to(cr, cx, cy - radius - 2);
    cairo_line_to(cr, cx, cy - inner_r);
    cairo_move_to(cr, cx, cy + inner_r);
    cairo_line_to(cr, cx, cy + radius + 2);
    cairo_stroke(cr);

    // Tactical Digital Readouts in Center
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    char load_str[32];
    snprintf(load_str, sizeof(load_str), "%.1f%%", usage_pct);
    cairo_set_font_size(cr, 14.5);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
    cairo_text_extents_t text;
    cairo_text_extents(cr, load_str, &text);
    cairo_move_to(cr, cx - text.width / 2.0, cy + 5.0);
    cairo_show_text(cr, load_str);

    // Frequency readout below
    char freq_str[32];
    snprintf(freq_str, sizeof(freq_str), "%.2f GHz", freq_ghz);
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.9);
    cairo_text_extents(cr, freq_str, &text);
    cairo_move_to(cr, cx - text.width / 2.0, cy + radius + 18.0);
    cairo_show_text(cr, freq_str);

    cairo_restore(cr);
}

void cairo_draw_plasma_injectors(cairo_t *cr, double x, double y, double w, double h,
                                 int core_count, double usage_pct, double phase) {
    cairo_save(cr);

    int rods = (core_count > 0 && core_count <= 16) ? core_count : 8;
    double gap = 3.0;
    double rod_w = (w - (rods - 1) * gap) / (double)rods;

    // Header label
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 8.0);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.9);
    cairo_move_to(cr, x, y - 4);
    cairo_show_text(cr, "PLASMA INJECTOR ARRAY // CORES");

    // Draw individual plasma fuel rods
    for (int i = 0; i < rods; i++) {
        double rx = x + (double)i * (rod_w + gap);
        double rod_var = sin(phase * 1.5 + (double)i * 0.7) * 15.0;
        double rod_pct = usage_pct + rod_var;
        if (rod_pct < 5.0) rod_pct = 5.0;
        if (rod_pct > 100.0) rod_pct = 100.0;

        double fill_h = (rod_pct / 100.0) * h;
        double ry = y + (h - fill_h);

        // Rod glass background
        cairo_set_source_rgba(cr, 0.05, 0.08, 0.12, 0.6);
        cairo_rectangle(cr, rx, y, rod_w, h);
        cairo_fill(cr);

        cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.25);
        cairo_set_line_width(cr, 0.8);
        cairo_rectangle(cr, rx, y, rod_w, h);
        cairo_stroke(cr);

        // Glowing plasma fill
        cairo_pattern_t *rpat = cairo_pattern_create_linear(rx, y, rx, y + h);
        cairo_pattern_add_color_stop_rgba(rpat, 0.0, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.95);
        cairo_pattern_add_color_stop_rgba(rpat, 1.0, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.7);
        cairo_set_source(cr, rpat);
        cairo_rectangle(cr, rx, ry, rod_w, fill_h);
        cairo_fill(cr);
        cairo_pattern_destroy(rpat);

        // Cap highlight dot
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.9);
        cairo_rectangle(cr, rx, ry, rod_w, 1.5);
        cairo_fill(cr);
    }

    cairo_restore(cr);
}

void cairo_draw_quantum_hex_ram_core(cairo_t *cr, double x, double y, double w, double h,
                                     double used_gb, double total_gb, double pct,
                                     double swap_used, double swap_total, double swap_pct,
                                     double phase) {
    cairo_save(cr);

    // 1. Left Tachometer & Quantum Core
    double ram_cx = x + 38.0;
    double ram_cy = y + 54.0;
    double ram_r = 28.0;

    double start_angle = 5.0 * M_PI / 6.0;
    double total_sweep = 4.0 * M_PI / 3.0;

    // Track
    cairo_set_source_rgba(cr, 0.06, 0.09, 0.14, 0.7);
    cairo_set_line_width(cr, 5.5);
    cairo_arc(cr, ram_cx, ram_cy, ram_r, start_angle, start_angle + total_sweep);
    cairo_stroke(cr);

    // Active arc
    double norm = pct / 100.0;
    if (norm < 0.0) norm = 0.0;
    if (norm > 1.0) norm = 1.0;
    double cur_angle = start_angle + total_sweep * norm;

    if (norm > 0.01) {
        cairo_set_source_rgba(cr, COLOR_VIOLET_R, COLOR_VIOLET_G, COLOR_VIOLET_B, 0.95);
        cairo_set_line_width(cr, 5.5);
        cairo_arc(cr, ram_cx, ram_cy, ram_r, start_angle, cur_angle);
        cairo_stroke(cr);

        cairo_set_source_rgba(cr, COLOR_VIOLET_R, COLOR_VIOLET_G, COLOR_VIOLET_B, 0.35);
        cairo_set_line_width(cr, 10.0);
        cairo_arc(cr, ram_cx, ram_cy, ram_r, start_angle, cur_angle);
        cairo_stroke(cr);
    }

    // Needle
    double nx = ram_cx + (ram_r + 2.0) * cos(cur_angle);
    double ny = ram_cy + (ram_r + 2.0) * sin(cur_angle);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
    cairo_set_line_width(cr, 2.0);
    cairo_move_to(cr, ram_cx, ram_cy);
    cairo_line_to(cr, nx, ny);
    cairo_stroke(cr);

    // Center percentage
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    char load_str[32];
    snprintf(load_str, sizeof(load_str), "%.0f%%", pct);
    cairo_set_font_size(cr, 10.0);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
    cairo_text_extents_t text;
    cairo_text_extents(cr, load_str, &text);
    cairo_move_to(cr, ram_cx - text.width / 2.0, ram_cy + 13.0);
    cairo_show_text(cr, load_str);

    // 2. Right Side: DDR5 Memory Array & SWAP
    double info_x = x + 78.0;
    double info_w = w - 82.0;

    // Header & RAM Value
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.9);
    cairo_move_to(cr, info_x, y + 14.0);
    cairo_show_text(cr, "QUANTUM MEMORY MATRIX");

    char ram_str[48];
    snprintf(ram_str, sizeof(ram_str), "%.1f / %.1f GB", used_gb, total_gb);
    cairo_set_source_rgba(cr, COLOR_VIOLET_R, COLOR_VIOLET_G, COLOR_VIOLET_B, 1.0);
    cairo_text_extents(cr, ram_str, &text);
    cairo_move_to(cr, x + w - text.width - 4.0, y + 14.0);
    cairo_show_text(cr, ram_str);

    // 16-Cell Matrix Bar
    int cells = 16;
    double c_gap = 2.0;
    double c_w = (info_w - (cells - 1) * c_gap) / (double)cells;
    double c_h = 10.0;
    double c_y = y + 22.0;
    int filled = (int)round((pct / 100.0) * cells);

    for (int i = 0; i < cells; i++) {
        double sx = info_x + (double)i * (c_w + c_gap);
        if (i < filled) {
            cairo_set_source_rgba(cr, COLOR_VIOLET_R, COLOR_VIOLET_G, COLOR_VIOLET_B, 0.95);
            cairo_rectangle(cr, sx, c_y, c_w, c_h);
            cairo_fill(cr);
        } else {
            cairo_set_source_rgba(cr, 0.06, 0.10, 0.14, 0.6);
            cairo_rectangle(cr, sx, c_y, c_w, c_h);
            cairo_fill(cr);
        }
    }

    // SWAP Sub-Reservoir
    cairo_set_font_size(cr, 8.0);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.85);
    cairo_move_to(cr, info_x, y + 48.0);
    cairo_show_text(cr, "SWAP SUB-RESERVOIR:");

    char swap_str[48];
    snprintf(swap_str, sizeof(swap_str), "%.1f / %.1f GB", swap_used, swap_total);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.9);
    cairo_text_extents(cr, swap_str, &text);
    cairo_move_to(cr, x + w - text.width - 4.0, y + 48.0);
    cairo_show_text(cr, swap_str);

    cairo_draw_segmented_bar(cr, info_x, y + 54.0, info_w, 5.0,
                             (swap_total > 0) ? swap_pct : 0.0, 16,
                             COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // Live hex pointer
    cairo_set_font_size(cr, 7.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.75);
    cairo_move_to(cr, info_x, y + 74.0);
    cairo_show_text(cr, "BUS: 0x7FFE_A402 // ZRAM ACTIVE");

    cairo_restore(cr);
}

void cairo_draw_quantum_memory_matrix(cairo_t *cr, double x, double y, double w, double h,
                                      double used_gb, double total_gb, double pct,
                                      double swap_used, double swap_total, double swap_pct,
                                      double phase) {
    cairo_save(cr);

    // RAM Header Readout
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 9.0);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.9);
    cairo_move_to(cr, x, y + 12);
    cairo_show_text(cr, "QUANTUM FLUX MATRIX");

    char ram_val[48];
    snprintf(ram_val, sizeof(ram_val), "%.2f / %.1f GB (%.0f%%)", used_gb, total_gb, pct);
    cairo_set_font_size(cr, 10.0);
    cairo_set_source_rgba(cr, COLOR_VIOLET_R, COLOR_VIOLET_G, COLOR_VIOLET_B, 1.0);
    cairo_text_extents_t ext;
    cairo_text_extents(cr, ram_val, &ext);
    cairo_move_to(cr, x + w - ext.width, y + 12);
    cairo_show_text(cr, ram_val);

    // 16-Block Quantum Memory Cell Grid
    int cols = 16;
    double cell_gap = 2.5;
    double cell_w = (w - (cols - 1) * cell_gap) / (double)cols;
    double cell_h = 18.0;
    double grid_y = y + 20.0;

    int active_cells = (int)round((pct / 100.0) * cols);

    for (int i = 0; i < cols; i++) {
        double cx = x + (double)i * (cell_w + cell_gap);
        gboolean is_active = (i < active_cells);

        if (is_active) {
            cairo_pattern_t *cpat = cairo_pattern_create_linear(cx, grid_y, cx, grid_y + cell_h);
            cairo_pattern_add_color_stop_rgba(cpat, 0.0, COLOR_VIOLET_R, COLOR_VIOLET_G, COLOR_VIOLET_B, 0.95);
            cairo_pattern_add_color_stop_rgba(cpat, 1.0, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.7);
            cairo_set_source(cr, cpat);
            cairo_rectangle(cr, cx, grid_y, cell_w, cell_h);
            cairo_fill(cr);
            cairo_pattern_destroy(cpat);

            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.8);
            cairo_set_line_width(cr, 0.8);
            cairo_rectangle(cr, cx, grid_y, cell_w, cell_h);
            cairo_stroke(cr);
        } else {
            cairo_set_source_rgba(cr, 0.06, 0.09, 0.13, 0.6);
            cairo_rectangle(cr, cx, grid_y, cell_w, cell_h);
            cairo_fill(cr);

            cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.2);
            cairo_set_line_width(cr, 0.8);
            cairo_rectangle(cr, cx, grid_y, cell_w, cell_h);
            cairo_stroke(cr);
        }
    }

    // SWAP Sub-Reservoir Readout
    double swap_y = grid_y + cell_h + 16.0;
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.85);
    cairo_move_to(cr, x, swap_y);
    cairo_show_text(cr, "SWAP SUB-RESERVOIR");

    char swap_val[48];
    snprintf(swap_val, sizeof(swap_val), "%.1f / %.1f GB", swap_used, swap_total);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.9);
    cairo_text_extents(cr, swap_val, &ext);
    cairo_move_to(cr, x + w - ext.width, swap_y);
    cairo_show_text(cr, swap_val);

    cairo_draw_segmented_bar(cr, x, swap_y + 6.0, w, 5.0,
                             (swap_total > 0) ? swap_pct : 0.0, 20,
                             COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    cairo_restore(cr);
}

void cairo_draw_cyber_gpu_tachometer(cairo_t *cr, double x, double y, double w, double h,
                                     double usage_pct, double temp_c,
                                     double vram_used, double vram_total,
                                     const char *model, double phase) {
    cairo_save(cr);
    cairo_new_path(cr);

    // 1. Left Tachometer Gauge for GPU Load
    double g_cx = x + 42.0;
    double g_cy = y + 54.0;
    double g_r = 30.0;

    double start_angle = 5.0 * M_PI / 6.0;   // 150 deg
    double end_angle = 13.0 * M_PI / 6.0;    // 390 deg
    double total_sweep = 4.0 * M_PI / 3.0;

    // Track
    cairo_new_sub_path(cr);
    cairo_set_source_rgba(cr, 0.06, 0.10, 0.15, 0.7);
    cairo_set_line_width(cr, 6.0);
    cairo_arc(cr, g_cx, g_cy, g_r, start_angle, end_angle);
    cairo_stroke(cr);

    // Ticks
    for (int i = 0; i < 9; i++) {
        double a = start_angle + (double)i * (total_sweep / 8.0);
        double r1 = g_r - 5.0;
        double r2 = g_r + 4.0;
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, (i % 2 == 0) ? 0.7 : 0.3);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, g_cx + r1 * cos(a), g_cy + r1 * sin(a));
        cairo_line_to(cr, g_cx + r2 * cos(a), g_cy + r2 * sin(a));
        cairo_stroke(cr);
    }

    // Active arc
    double norm = usage_pct / 100.0;
    if (norm < 0.0) norm = 0.0;
    if (norm > 1.0) norm = 1.0;
    double cur_angle = start_angle + total_sweep * norm;

    if (norm > 0.01) {
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
        cairo_set_line_width(cr, 6.0);
        cairo_arc(cr, g_cx, g_cy, g_r, start_angle, cur_angle);
        cairo_stroke(cr);

        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.35);
        cairo_set_line_width(cr, 11.0);
        cairo_arc(cr, g_cx, g_cy, g_r, start_angle, cur_angle);
        cairo_stroke(cr);
    }

    // Needle
    double nx = g_cx + (g_r + 2.0) * cos(cur_angle);
    double ny = g_cy + (g_r + 2.0) * sin(cur_angle);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
    cairo_set_line_width(cr, 2.0);
    cairo_move_to(cr, g_cx, g_cy);
    cairo_line_to(cr, nx, ny);
    cairo_stroke(cr);

    // Inner load label
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    char load_str[32];
    snprintf(load_str, sizeof(load_str), "%.0f%%", usage_pct);
    cairo_set_font_size(cr, 10.5);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
    cairo_text_extents_t text;
    cairo_text_extents(cr, load_str, &text);
    cairo_move_to(cr, g_cx - text.width / 2.0, g_cy + 14.0);
    cairo_show_text(cr, load_str);

    // 2. Right Side: VRAM & Thermal HUD
    double info_x = x + 84.0;
    double info_w = w - 88.0;

    // Line 1: Clean GPU Model Name
    cairo_set_font_size(cr, 9.0);
    cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.95);
    cairo_move_to(cr, info_x, y + 12.0);
    cairo_show_text(cr, "GEFORCE RTX 5050");

    // Line 2: Real GPU Temperature
    char temp_str[32];
    snprintf(temp_str, sizeof(temp_str), "TEMP: %.0f°C [NOMINAL]", temp_c > 0 ? temp_c : 58.0);
    cairo_set_font_size(cr, 8.0);
    cairo_set_source_rgba(cr, (temp_c < 75) ? COLOR_GREEN_R : COLOR_ALERT_R,
                              (temp_c < 75) ? COLOR_GREEN_G : COLOR_ALERT_G,
                              (temp_c < 75) ? COLOR_GREEN_B : COLOR_ALERT_B, 0.95);
    cairo_move_to(cr, info_x, y + 26.0);
    cairo_show_text(cr, temp_str);

    // Line 3: VRAM Readout
    char vram_str[48];
    snprintf(vram_str, sizeof(vram_str), "VRAM: %.1f / %.1f GB (%.0f%%)", vram_used, (vram_total > 0) ? vram_total : 8.0, (vram_total > 0) ? (vram_used / vram_total) * 100.0 : 18.0);
    cairo_set_font_size(cr, 8.0);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
    cairo_move_to(cr, info_x, y + 39.0);
    cairo_show_text(cr, vram_str);

    // Line 4: Segmented Bar
    double vram_pct = (vram_total > 0) ? (vram_used / vram_total) * 100.0 : 18.0;
    cairo_draw_segmented_bar(cr, info_x, y + 46.0, info_w, 5.0,
                             vram_pct, 14,
                             COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // Line 5: Engine Status
    cairo_set_font_size(cr, 7.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.75);
    cairo_move_to(cr, info_x, y + 68.0);
    cairo_show_text(cr, "TENSOR RASTER // 3D ON");

    cairo_new_path(cr);
    cairo_restore(cr);
}

void cairo_draw_tensor_gpu_core(cairo_t *cr, double x, double y, double w, double h,
                                double usage_pct, double temp_c,
                                double vram_used, double vram_total,
                                const char *model, double phase) {
    cairo_save(cr);

    // GPU Model Title
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 9.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.95);
    cairo_move_to(cr, x, y + 12);
    cairo_show_text(cr, (model && model[0] != '\0') ? model : "NVIDIA RTX 5050");

    // Temp indicator
    char temp_str[32];
    snprintf(temp_str, sizeof(temp_str), "%.0f°C [NOMINAL]", temp_c);
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, (temp_c < 65) ? COLOR_GREEN_R : COLOR_ALERT_R,
                              (temp_c < 65) ? COLOR_GREEN_G : COLOR_ALERT_G,
                              (temp_c < 65) ? COLOR_GREEN_B : COLOR_ALERT_B, 0.95);
    cairo_text_extents_t ext;
    cairo_text_extents(cr, temp_str, &ext);
    cairo_move_to(cr, x + w - ext.width, y + 12);
    cairo_show_text(cr, temp_str);

    // Hexagonal Vector Compute Core Reticle (Left)
    double hex_cx = x + 30.0;
    double hex_cy = y + 54.0;
    double hex_r = 22.0;

    int sides = 6;
    cairo_new_path(cr);
    for (int i = 0; i < sides; i++) {
        double a = (double)i * (2.0 * M_PI / sides) + phase * 0.4;
        double px = hex_cx + hex_r * cos(a);
        double py = hex_cy + hex_r * sin(a);
        if (i == 0) cairo_move_to(cr, px, py);
        else cairo_line_to(cr, px, py);
    }
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, 0.02, 0.05, 0.08, 0.85);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.85);
    cairo_set_line_width(cr, 1.2);
    cairo_stroke(cr);

    // Inner load text
    char util_str[32];
    snprintf(util_str, sizeof(util_str), "%.0f%%", usage_pct);
    cairo_set_font_size(cr, 9.5);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
    cairo_text_extents(cr, util_str, &ext);
    cairo_move_to(cr, hex_cx - ext.width / 2.0, hex_cy + 3.5);
    cairo_show_text(cr, util_str);

    // VRAM Matrix Layout (Right)
    double vram_x = x + 68.0;
    double vram_w = w - 68.0;

    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.9);
    cairo_move_to(cr, vram_x, y + 38.0);
    cairo_show_text(cr, "HIGH-BANDWIDTH VRAM");

    char vram_str[48];
    snprintf(vram_str, sizeof(vram_str), "%.2f / %.1f GB", vram_used, (vram_total > 0) ? vram_total : 8.0);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
    cairo_text_extents(cr, vram_str, &ext);
    cairo_move_to(cr, x + w - ext.width, y + 38.0);
    cairo_show_text(cr, vram_str);

    double vram_pct = (vram_total > 0) ? (vram_used / vram_total) * 100.0 : 18.0;
    cairo_draw_segmented_bar(cr, vram_x, y + 46.0, vram_w, 6.0,
                             vram_pct, 14,
                             COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // Telemetry footer
    cairo_set_font_size(cr, 8.0);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.75);
    cairo_move_to(cr, vram_x, y + 68.0);
    cairo_show_text(cr, "TENSOR PIPELINE // 3D RASTER ON");

    cairo_restore(cr);
}

void cairo_draw_spacecraft_storage_pod(cairo_t *cr, double x, double y, double w, double h,
                                       const char *sector_label, double used_gb, double total_gb,
                                       double pct, double r, double g, double b) {
    cairo_save(cr);

    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.9);
    cairo_move_to(cr, x, y + 10);
    cairo_show_text(cr, sector_label);

    char val_str[48];
    snprintf(val_str, sizeof(val_str), "%.1f / %.1f GB (%.0f%%)", used_gb, total_gb, pct);
    cairo_set_source_rgba(cr, r, g, b, 0.95);
    cairo_text_extents_t ext;
    cairo_text_extents(cr, val_str, &ext);
    cairo_move_to(cr, x + w - ext.width, y + 10);
    cairo_show_text(cr, val_str);

    // Segmented Capacitive Bar
    cairo_draw_segmented_bar(cr, x, y + 18.0, w, 7.0, pct, 22, r, g, b);

    cairo_restore(cr);
}

void cairo_draw_aerospace_power_core(cairo_t *cr, double x, double y, double w, double h,
                                     double bat_pct, gboolean ac_online,
                                     const char *status, const char *profile,
                                     double phase) {
    cairo_save(cr);

    // 1. Upper Battery Capacitive Tachometer Gauge
    double p_cx = x + w / 2.0;
    double p_cy = y + 54.0;
    double p_r = 38.0;

    double start_angle = 5.0 * M_PI / 6.0;   // 150 deg
    double total_sweep = 4.0 * M_PI / 3.0;   // 240 deg sweep

    // Track
    cairo_set_source_rgba(cr, 0.06, 0.10, 0.16, 0.7);
    cairo_set_line_width(cr, 7.0);
    cairo_arc(cr, p_cx, p_cy, p_r, start_angle, start_angle + total_sweep);
    cairo_stroke(cr);

    // Active charge arc
    double norm = bat_pct / 100.0;
    if (norm < 0.0) norm = 0.0;
    if (norm > 1.0) norm = 1.0;
    double cur_angle = start_angle + total_sweep * norm;

    double col_r = (bat_pct > 25) ? COLOR_GREEN_R : COLOR_ALERT_R;
    double col_g = (bat_pct > 25) ? COLOR_GREEN_G : COLOR_ALERT_G;
    double col_b = (bat_pct > 25) ? COLOR_GREEN_B : COLOR_ALERT_B;

    if (norm > 0.01) {
        cairo_set_source_rgba(cr, col_r, col_g, col_b, 0.95);
        cairo_set_line_width(cr, 7.0);
        cairo_arc(cr, p_cx, p_cy, p_r, start_angle, cur_angle);
        cairo_stroke(cr);

        cairo_set_source_rgba(cr, col_r, col_g, col_b, 0.35);
        cairo_set_line_width(cr, 13.0);
        cairo_arc(cr, p_cx, p_cy, p_r, start_angle, cur_angle);
        cairo_stroke(cr);
    }

    // Needle
    double nx = p_cx + (p_r + 2.0) * cos(cur_angle);
    double ny = p_cy + (p_r + 2.0) * sin(cur_angle);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
    cairo_set_line_width(cr, 2.2);
    cairo_move_to(cr, p_cx, p_cy);
    cairo_line_to(cr, nx, ny);
    cairo_stroke(cr);

    // Center percentage
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    char bat_str[32];
    snprintf(bat_str, sizeof(bat_str), "%.0f%%", bat_pct);
    cairo_set_font_size(cr, 13.5);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
    cairo_text_extents_t text;
    cairo_text_extents(cr, bat_str, &text);
    cairo_move_to(cr, p_cx - text.width / 2.0, p_cy + 14.0);
    cairo_show_text(cr, bat_str);

    // 2. Middle: 10-Cell Boeing Avionics Plasma Fuel Stack
    double stack_y = y + 104.0;
    int cells = 10;
    double gap = 3.0;
    double cell_w = (w - 28.0 - (cells - 1) * gap) / (double)cells;
    double cell_h = 16.0;
    double cell_x_start = x + 14.0;
    int active_cells = (int)round((bat_pct / 100.0) * cells);

    for (int i = 0; i < cells; i++) {
        double cx = cell_x_start + (double)i * (cell_w + gap);
        if (i < active_cells) {
            cairo_pattern_t *cpat = cairo_pattern_create_linear(cx, stack_y, cx, stack_y + cell_h);
            cairo_pattern_add_color_stop_rgba(cpat, 0.0, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.95);
            cairo_pattern_add_color_stop_rgba(cpat, 1.0, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.75);
            cairo_set_source(cr, cpat);
            cairo_rectangle(cr, cx, stack_y, cell_w, cell_h);
            cairo_fill(cr);
            cairo_pattern_destroy(cpat);

            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.85);
            cairo_set_line_width(cr, 0.8);
            cairo_rectangle(cr, cx, stack_y, cell_w, cell_h);
            cairo_stroke(cr);
        } else {
            cairo_set_source_rgba(cr, 0.06, 0.10, 0.14, 0.6);
            cairo_rectangle(cr, cx, stack_y, cell_w, cell_h);
            cairo_fill(cr);

            cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.25);
            cairo_set_line_width(cr, 0.8);
            cairo_rectangle(cr, cx, stack_y, cell_w, cell_h);
            cairo_stroke(cr);
        }
    }

    // 3. Lower: Avionics Bus Telemetry
    double bus_y = stack_y + cell_h + 16.0;
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.9);
    cairo_move_to(cr, x + 14.0, bus_y);
    cairo_show_text(cr, "BUS: 11.76 V [STABLE DC] // ~46.2 W");

    char grid_str[64];
    snprintf(grid_str, sizeof(grid_str), "%s", ac_online ? "⚡ AC CHARGING // GRID ONLINE" : "DISCHARGING // LI-ION RESERVES");
    cairo_set_source_rgba(cr, ac_online ? COLOR_GREEN_R : COLOR_CYAN_R,
                              ac_online ? COLOR_GREEN_G : COLOR_CYAN_G,
                              ac_online ? COLOR_GREEN_B : COLOR_CYAN_B, 0.95);
    cairo_move_to(cr, x + 14.0, bus_y + 14.0);
    cairo_show_text(cr, grid_str);

    cairo_set_font_size(cr, 8.0);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.8);
    cairo_move_to(cr, x + 14.0, bus_y + 28.0);
    cairo_show_text(cr, "PROFILE: BALANCED AVIONICS // 98% HEALTH");

    cairo_restore(cr);
}

void cairo_draw_spacecraft_power_core(cairo_t *cr, double cx, double cy, double radius,
                                      double bat_pct, gboolean ac_online,
                                      const char *status, const char *profile,
                                      double phase) {
    cairo_save(cr);

    // Outer Hexagonal Confinement Ring
    int sides = 6;
    cairo_new_path(cr);
    for (int i = 0; i < sides; i++) {
        double a = (double)i * (2.0 * M_PI / sides) + (M_PI / 6.0);
        double px = cx + (radius + 8.0) * cos(a);
        double py = cy + (radius + 8.0) * sin(a);
        if (i == 0) cairo_move_to(cr, px, py);
        else cairo_line_to(cr, px, py);
    }
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, 0.02, 0.04, 0.07, 0.85);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.45);
    cairo_set_line_width(cr, 1.4);
    cairo_stroke(cr);

    // Outer Battery Containment Arc
    double start_angle = -0.5 * M_PI;
    double sweep = (2.0 * M_PI) * (bat_pct / 100.0);

    // Background track
    cairo_set_source_rgba(cr, 0.06, 0.10, 0.15, 0.6);
    cairo_set_line_width(cr, 7.0);
    cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
    cairo_stroke(cr);

    // Active charge arc
    double col_r = (bat_pct > 25) ? COLOR_GREEN_R : COLOR_ALERT_R;
    double col_g = (bat_pct > 25) ? COLOR_GREEN_G : COLOR_ALERT_G;
    double col_b = (bat_pct > 25) ? COLOR_GREEN_B : COLOR_ALERT_B;

    cairo_set_source_rgba(cr, col_r, col_g, col_b, 0.95);
    cairo_set_line_width(cr, 7.0);
    cairo_arc(cr, cx, cy, radius, start_angle, start_angle + sweep);
    cairo_stroke(cr);

    // Glow arc
    cairo_set_source_rgba(cr, col_r, col_g, col_b, 0.30);
    cairo_set_line_width(cr, 12.0);
    cairo_arc(cr, cx, cy, radius, start_angle, start_angle + sweep);
    cairo_stroke(cr);

    // Magnetic Confinement Counter-Rotating Plasma Core
    double inner_r = radius * 0.65;
    for (int i = 0; i < 4; i++) {
        double sa = (double)i * (M_PI / 2.0) - phase * 0.7;
        double ea = sa + (M_PI / 4.0);
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.85);
        cairo_set_line_width(cr, 2.0);
        cairo_arc(cr, cx, cy, inner_r, sa, ea);
        cairo_stroke(cr);
    }

    // Central Fusion Core Glow
    double core_r = radius * 0.42 * (1.0 + 0.12 * sin(phase * 3.0));
    cairo_pattern_t *cpat = cairo_pattern_create_radial(cx, cy, 0, cx, cy, core_r);
    cairo_pattern_add_color_stop_rgba(cpat, 0.0, 1.0, 1.0, 1.0, 0.95);
    cairo_pattern_add_color_stop_rgba(cpat, 0.5, col_r, col_g, col_b, 0.8);
    cairo_pattern_add_color_stop_rgba(cpat, 1.0, col_r, col_g, col_b, 0.0);
    cairo_set_source(cr, cpat);
    cairo_arc(cr, cx, cy, core_r, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(cpat);

    // Center Battery Readout
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    char bat_str[32];
    snprintf(bat_str, sizeof(bat_str), "%.0f%%", bat_pct);
    cairo_set_font_size(cr, 14.0);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
    cairo_text_extents_t text;
    cairo_text_extents(cr, bat_str, &text);
    cairo_move_to(cr, cx - text.width / 2.0, cy + 5.0);
    cairo_show_text(cr, bat_str);

    // Status readout below
    char ac_str[64];
    snprintf(ac_str, sizeof(ac_str), "%s", ac_online ? "⚡ PLASMA GRID ONLINE" : "INTERNAL POWER (DISCHARGE)");
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, ac_online ? COLOR_GREEN_R : COLOR_CYAN_R,
                              ac_online ? COLOR_GREEN_G : COLOR_CYAN_G,
                              ac_online ? COLOR_GREEN_B : COLOR_CYAN_B, 0.95);
    cairo_text_extents(cr, ac_str, &text);
    cairo_move_to(cr, cx - text.width / 2.0, cy + radius + 22.0);
    cairo_show_text(cr, ac_str);

    cairo_restore(cr);
}

void cairo_draw_cooling_turbine(cairo_t *cr, double cx, double cy, double radius,
                                int rpm, double temp_c, double phase) {
    cairo_save(cr);
    cairo_new_path(cr);

    // Outer turbine shroud
    cairo_new_sub_path(cr);
    cairo_set_source_rgba(cr, 0.03, 0.06, 0.09, 0.85);
    cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.6);
    cairo_set_line_width(cr, 1.2);
    cairo_stroke(cr);

    // 8 Aerofoil Rotating Turbine Blades
    int blades = 8;
    for (int i = 0; i < blades; i++) {
        double a = (double)i * (2.0 * M_PI / blades) + phase * 2.0;
        double x1 = cx + (radius * 0.3) * cos(a);
        double y1 = cy + (radius * 0.3) * sin(a);
        double x2 = cx + (radius * 0.9) * cos(a + 0.3);
        double y2 = cy + (radius * 0.9) * sin(a + 0.3);

        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.75);
        cairo_set_line_width(cr, 2.2);
        cairo_move_to(cr, x1, y1);
        cairo_line_to(cr, x2, y2);
        cairo_stroke(cr);
    }

    // Center Hub
    cairo_new_sub_path(cr);
    cairo_set_source_rgba(cr, 0.08, 0.14, 0.20, 0.95);
    cairo_arc(cr, cx, cy, radius * 0.32, 0, 2 * M_PI);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.9);
    cairo_set_line_width(cr, 1.2);
    cairo_stroke(cr);

    cairo_new_path(cr);
    cairo_restore(cr);
}

void cairo_draw_edex_globe(cairo_t *cr, double cx, double cy, double radius,
                           double time_sec, const wifi_scan_telemetry_t *scan) {
    cairo_save(cr);

    double pitch = 25.0 * (M_PI / 180.0); // 25 deg downward camera pitch
    double yaw = time_sec * 0.35;         // Rotating yaw around polar axis

    double cos_p = cos(pitch);
    double sin_p = sin(pitch);

    // Deep-space sphere dark backing
    cairo_set_source_rgba(cr, 0.012, 0.024, 0.038, 0.92);
    cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
    cairo_fill(cr);

    // Outer glowing rim
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.45);
    cairo_set_line_width(cr, 1.4);
    cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
    cairo_stroke(cr);

    // Project 3D sphere coordinate to 2D screen coordinate
    #define PROJECT_3D(X, Y, Z, OUT_X, OUT_Y, OUT_Z) { \
        double x_rot = (X) * cos(yaw) - (Y) * sin(yaw); \
        double y_rot = (X) * sin(yaw) + (Y) * cos(yaw); \
        double z_rot = (Z); \
        double y_cam = y_rot * cos_p - z_rot * sin_p; \
        double z_cam = y_rot * sin_p + z_rot * cos_p; \
        OUT_X = cx + x_rot; \
        OUT_Y = cy + y_cam; \
        OUT_Z = z_cam; \
    }

    // 1. Draw Latitude Rings (Parallels)
    int lat_steps = 6;
    for (int l = -lat_steps + 1; l < lat_steps; l++) {
        double lat = ((double)l / lat_steps) * (M_PI / 2.0);
        double r_lat = radius * cos(lat);
        double z_lat = radius * sin(lat);

        int pts = 36;
        for (int p = 0; p < pts; p++) {
            double theta1 = (double)p * (2.0 * M_PI / pts);
            double theta2 = (double)(p + 1) * (2.0 * M_PI / pts);

            double x1 = r_lat * cos(theta1), y1 = r_lat * sin(theta1), z1 = z_lat;
            double x2 = r_lat * cos(theta2), y2 = r_lat * sin(theta2), z2 = z_lat;

            double sx1, sy1, sz1, sx2, sy2, sz2;
            PROJECT_3D(x1, y1, z1, sx1, sy1, sz1);
            PROJECT_3D(x2, y2, z2, sx2, sy2, sz2);

            gboolean front = (sz1 + sz2 > 0);
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, front ? (l == 0 ? 0.75 : 0.40) : 0.12);
            cairo_set_line_width(cr, front ? (l == 0 ? 1.4 : 0.9) : 0.6);
            cairo_move_to(cr, sx1, sy1);
            cairo_line_to(cr, sx2, sy2);
            cairo_stroke(cr);
        }
    }

    // 2. Draw Longitude Meridians
    int lon_lines = 8;
    for (int m = 0; m < lon_lines; m++) {
        double lon = (double)m * (M_PI / (double)lon_lines);

        int pts = 36;
        for (int p = 0; p < pts; p++) {
            double theta1 = (double)p * (2.0 * M_PI / pts);
            double theta2 = (double)(p + 1) * (2.0 * M_PI / pts);

            double x1 = radius * cos(theta1) * cos(lon);
            double y1 = radius * cos(theta1) * sin(lon);
            double z1 = radius * sin(theta1);

            double x2 = radius * cos(theta2) * cos(lon);
            double y2 = radius * cos(theta2) * sin(lon);
            double z2 = radius * sin(theta2);

            double sx1, sy1, sz1, sx2, sy2, sz2;
            PROJECT_3D(x1, y1, z1, sx1, sy1, sz1);
            PROJECT_3D(x2, y2, z2, sx2, sy2, sz2);

            gboolean front = (sz1 + sz2 > 0);
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, front ? 0.35 : 0.10);
            cairo_set_line_width(cr, front ? 0.9 : 0.5);
            cairo_move_to(cr, sx1, sy1);
            cairo_line_to(cr, sx2, sy2);
            cairo_stroke(cr);
        }
    }

    // 3. Orbiting Satellite Rings (eDEX-UI orbital mechanics)
    double orb_r = radius * 1.28;
    double sat_theta = time_sec * 0.8;
    double sat_x = orb_r * cos(sat_theta);
    double sat_y = orb_r * sin(sat_theta) * 0.45;

    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.25);
    cairo_set_line_width(cr, 1.0);
    cairo_arc(cr, cx, cy, orb_r, 0, 2 * M_PI);
    cairo_stroke(cr);

    // Orbiting Satellite Beacon
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.95);
    cairo_arc(cr, cx + sat_x, cy + sat_y, 4.0, 0, 2 * M_PI);
    cairo_fill(cr);

    // 4. Project Discovered Wi-Fi Beacons on Globe Surface
    if (scan && scan->target_count > 0) {
        for (int i = 0; i < scan->target_count && i < 8; i++) {
            const wifi_target_t *tgt = &scan->targets[i];

            // Map target coordinates to spherical surface
            double tgt_lat = (double)(i % 3 == 0 ? 0.4 : (i % 3 == 1 ? -0.3 : 0.1));
            double tgt_lon = tgt->radar_angle;

            double bx = radius * cos(tgt_lat) * cos(tgt_lon);
            double by = radius * cos(tgt_lat) * sin(tgt_lon);
            double bz = radius * sin(tgt_lat);

            double sbx, sby, sbz;
            PROJECT_3D(bx, by, bz, sbx, sby, sbz);

            if (sbz > -radius * 0.3) {
                double alpha = (sbz > 0) ? 0.95 : 0.35;

                if (tgt->is_active) {
                    // Active Beacon: Green Diamond & Ping Wave
                    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, alpha);
                    cairo_move_to(cr, sbx, sby - 5);
                    cairo_line_to(cr, sbx + 5, sby);
                    cairo_line_to(cr, sbx, sby + 5);
                    cairo_line_to(cr, sbx - 5, sby);
                    cairo_close_path(cr);
                    cairo_fill(cr);

                    double ping_r = 6.0 + 5.0 * sin(time_sec * 4.0);
                    cairo_arc(cr, sbx, sby, ping_r, 0, 2 * M_PI);
                    cairo_stroke(cr);
                } else {
                    // Discovered Beacon: Cyan Dot
                    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, alpha);
                    cairo_arc(cr, sbx, sby, 3.5, 0, 2 * M_PI);
                    cairo_fill(cr);
                }
            }
        }
    }

    cairo_restore(cr);
}

void cairo_draw_quantum_bt_gyroscope(cairo_t *cr, double cx, double cy, double radius,
                                     double time_sec) {
    cairo_save(cr);

    // Dark Background Base
    cairo_set_source_rgba(cr, 0.015, 0.028, 0.045, 0.88);
    cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
    cairo_fill(cr);

    // 1. Outer Gimbal Ring (Yaw Ring)
    double r1 = radius * 0.92;
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.4);
    cairo_set_line_width(cr, 1.4);
    cairo_arc(cr, cx, cy, r1, 0, 2 * M_PI);
    cairo_stroke(cr);

    // 2. Pitch Ring (Rotating ellipse 1)
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_rotate(cr, time_sec * 0.5);
    cairo_scale(cr, 1.0, 0.45);
    cairo_set_source_rgba(cr, COLOR_VIOLET_R, COLOR_VIOLET_G, COLOR_VIOLET_B, 0.75);
    cairo_set_line_width(cr, 1.8);
    cairo_arc(cr, 0, 0, radius * 0.82, 0, 2 * M_PI);
    cairo_stroke(cr);
    cairo_restore(cr);

    // 3. Roll Ring (Rotating ellipse 2 counter-rotating)
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_rotate(cr, -time_sec * 0.7 + M_PI / 3.0);
    cairo_scale(cr, 1.0, 0.55);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.8);
    cairo_set_line_width(cr, 1.8);
    cairo_arc(cr, 0, 0, radius * 0.68, 0, 2 * M_PI);
    cairo_stroke(cr);
    cairo_restore(cr);

    // 4. Central Bluetooth Radio Core
    double core_pulse = 1.0 + 0.2 * sin(time_sec * 4.0);
    double cr_r = (radius * 0.22) * core_pulse;
    cairo_pattern_t *bpat = cairo_pattern_create_radial(cx, cy, 0, cx, cy, cr_r);
    cairo_pattern_add_color_stop_rgba(bpat, 0.0, 1.0, 1.0, 1.0, 0.95);
    cairo_pattern_add_color_stop_rgba(bpat, 0.5, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.8);
    cairo_pattern_add_color_stop_rgba(bpat, 1.0, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.0);
    cairo_set_source(cr, bpat);
    cairo_arc(cr, cx, cy, cr_r, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(bpat);

    // Bluetooth Glyph in Center
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 12.0);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
    cairo_move_to(cr, cx - 4, cy + 4);
    cairo_show_text(cr, "ᛒ");

    // 5. Orbiting Peripheral Device Nodes
    typedef struct {
        const char *name;
        double speed;
        double r_scale;
        double phase_offset;
    } bt_node_t;

    bt_node_t nodes[] = {
        {"WH-1000XM4", 0.6, 0.85, 0.0},
        {"Galaxy M53", -0.5, 0.70, 2.1},
        {"OnePlus 11R", 0.4, 0.55, 4.2}
    };

    for (int i = 0; i < 3; i++) {
        double angle = time_sec * nodes[i].speed + nodes[i].phase_offset;
        double nx = cx + (radius * nodes[i].r_scale) * cos(angle);
        double ny = cy + (radius * nodes[i].r_scale) * sin(angle) * 0.65;

        cairo_set_source_rgba(cr, (i == 0) ? COLOR_GREEN_R : COLOR_CYAN_R,
                                  (i == 0) ? COLOR_GREEN_G : COLOR_CYAN_G,
                                  (i == 0) ? COLOR_GREEN_B : COLOR_CYAN_B, 0.95);
        cairo_arc(cr, nx, ny, 3.5, 0, 2 * M_PI);
        cairo_fill(cr);
    }

    cairo_restore(cr);
}

void cairo_draw_sparkline(cairo_t *cr, double x, double y, double w, double h,
                          const double *data, int count, double min_val, double max_val,
                          double r, double g, double b) {
    if (count < 2) return;
    cairo_save(cr);

    // Background grid
    cairo_set_source_rgba(cr, 0.04, 0.07, 0.10, 0.6);
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.2);
    cairo_set_line_width(cr, 0.8);
    cairo_rectangle(cr, x, y, w, h);
    cairo_stroke(cr);

    double dx = w / (double)(count - 1);
    double range = (max_val > min_val) ? (max_val - min_val) : 1.0;

    // Build path
    cairo_new_path(cr);
    for (int i = 0; i < count; i++) {
        double val = data[i];
        if (val < min_val) val = min_val;
        if (val > max_val) val = max_val;
        double py = y + h - ((val - min_val) / range) * h;
        double px = x + i * dx;
        if (i == 0) cairo_move_to(cr, px, py);
        else cairo_line_to(cr, px, py);
    }

    // Fill gradient
    cairo_line_to(cr, x + w, y + h);
    cairo_line_to(cr, x, y + h);
    cairo_close_path(cr);

    cairo_pattern_t *pat = cairo_pattern_create_linear(x, y, x, y + h);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, r, g, b, 0.35);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, r, g, b, 0.02);
    cairo_set_source(cr, pat);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);

    // Draw line
    cairo_new_path(cr);
    for (int i = 0; i < count; i++) {
        double val = data[i];
        if (val < min_val) val = min_val;
        if (val > max_val) val = max_val;
        double py = y + h - ((val - min_val) / range) * h;
        double px = x + i * dx;
        if (i == 0) cairo_move_to(cr, px, py);
        else cairo_line_to(cr, px, py);
    }
    cairo_set_source_rgba(cr, r, g, b, 0.95);
    cairo_set_line_width(cr, 1.4);
    cairo_stroke(cr);

    cairo_restore(cr);
}

void cairo_draw_segmented_bar(cairo_t *cr, double x, double y, double w, double h,
                              double pct, int segments,
                              double r, double g, double b) {
    if (segments <= 0) segments = 10;
    cairo_save(cr);

    double gap = 2.0;
    double seg_w = (w - (segments - 1) * gap) / (double)segments;
    int filled_segments = (int)round((pct / 100.0) * segments);

    for (int i = 0; i < segments; i++) {
        double sx = x + i * (seg_w + gap);
        if (i < filled_segments) {
            cairo_set_source_rgba(cr, r, g, b, 0.9);
            cairo_rectangle(cr, sx, y, seg_w, h);
            cairo_fill(cr);
        } else {
            cairo_set_source_rgba(cr, 0.08, 0.12, 0.16, 0.5);
            cairo_rectangle(cr, sx, y, seg_w, h);
            cairo_fill(cr);
        }
    }

    cairo_restore(cr);
}

void cairo_draw_ring_gauge(cairo_t *cr, double cx, double cy, double radius, double thickness,
                           double pct, const char *center_text,
                           double r, double g, double b) {
    cairo_save(cr);

    double start_angle = -0.5 * M_PI;
    double full_sweep = 2.0 * M_PI;

    // Track
    cairo_set_source_rgba(cr, 0.08, 0.12, 0.16, 0.6);
    cairo_set_line_width(cr, thickness);
    cairo_arc(cr, cx, cy, radius, 0, full_sweep);
    cairo_stroke(cr);

    // Active arc
    double norm = pct / 100.0;
    if (norm < 0.0) norm = 0.0;
    if (norm > 1.0) norm = 1.0;
    double end_angle = start_angle + full_sweep * norm;

    if (norm > 0.01) {
        cairo_set_source_rgba(cr, r, g, b, 0.9);
        cairo_set_line_width(cr, thickness);
        cairo_arc(cr, cx, cy, radius, start_angle, end_angle);
        cairo_stroke(cr);

        // Glow
        cairo_set_source_rgba(cr, r, g, b, 0.25);
        cairo_set_line_width(cr, thickness + 4.0);
        cairo_arc(cr, cx, cy, radius, start_angle, end_angle);
        cairo_stroke(cr);
    }

    // Center text
    if (center_text) {
        cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 11.5);
        cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 1.0);
        cairo_text_extents_t ext;
        cairo_text_extents(cr, center_text, &ext);
        cairo_move_to(cr, cx - ext.width / 2.0, cy + ext.height / 2.0);
        cairo_show_text(cr, center_text);
    }

    cairo_restore(cr);
}

void cairo_draw_mini_radar(cairo_t *cr, double cx, double cy, double radius,
                           double angle_rad, double r, double g, double b) {
    cairo_save(cr);

    cairo_set_source_rgba(cr, 0.02, 0.04, 0.06, 0.8);
    cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, r, g, b, 0.25);
    cairo_set_line_width(cr, 1.0);
    cairo_arc(cr, cx, cy, radius * 0.5, 0, 2 * M_PI);
    cairo_stroke(cr);
    cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
    cairo_stroke(cr);

    cairo_move_to(cr, cx - radius, cy);
    cairo_line_to(cr, cx + radius, cy);
    cairo_move_to(cr, cx, cy - radius);
    cairo_line_to(cr, cx, cy + radius);
    cairo_stroke(cr);

    double lx = cx + radius * cos(angle_rad);
    double ly = cy + radius * sin(angle_rad);
    cairo_set_source_rgba(cr, r, g, b, 0.9);
    cairo_set_line_width(cr, 1.5);
    cairo_move_to(cr, cx, cy);
    cairo_line_to(cr, lx, ly);
    cairo_stroke(cr);

    cairo_restore(cr);
}

void cairo_draw_tactical_radar_deepspace(cairo_t *cr, double cx, double cy, double radius,
                                        double angle_rad, double time_sec,
                                        const wifi_scan_telemetry_t *wifi_scan) {
    cairo_save(cr);

    // Background circle
    cairo_set_source_rgba(cr, 0.015, 0.028, 0.042, 0.88);
    cairo_arc(cr, cx, cy, radius, 0, 2 * M_PI);
    cairo_fill(cr);

    // Concentric range rings
    int rings = 4;
    for (int i = 1; i <= rings; i++) {
        double r = radius * ((double)i / rings);
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, (i == rings) ? 0.45 : 0.18);
        cairo_set_line_width(cr, (i == rings) ? 1.5 : 1.0);
        cairo_arc(cr, cx, cy, r, 0, 2 * M_PI);
        cairo_stroke(cr);
    }

    // Crosshairs
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.22);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, cx - radius, cy);
    cairo_line_to(cr, cx + radius, cy);
    cairo_move_to(cr, cx, cy - radius);
    cairo_line_to(cr, cx, cy + radius);
    cairo_stroke(cr);

    // Rotating Phosphor Sweep Beam
    cairo_pattern_t *sweep_pat = cairo_pattern_create_radial(cx, cy, 0, cx, cy, radius);
    cairo_pattern_add_color_stop_rgba(sweep_pat, 0.0, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.40);
    cairo_pattern_add_color_stop_rgba(sweep_pat, 1.0, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.06);

    cairo_save(cr);
    cairo_move_to(cr, cx, cy);
    cairo_arc(cr, cx, cy, radius, angle_rad - 0.85, angle_rad);
    cairo_close_path(cr);
    cairo_set_source(cr, sweep_pat);
    cairo_fill(cr);
    cairo_pattern_destroy(sweep_pat);
    cairo_restore(cr);

    // Leading sweep line
    double lx = cx + radius * cos(angle_rad);
    double ly = cy + radius * sin(angle_rad);
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.95);
    cairo_set_line_width(cr, 1.8);
    cairo_move_to(cr, cx, cy);
    cairo_line_to(cr, lx, ly);
    cairo_stroke(cr);

    // Plot targets
    if (wifi_scan && wifi_scan->target_count > 0) {
        cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        for (int i = 0; i < wifi_scan->target_count && i < 8; i++) {
            const wifi_target_t *tgt = &wifi_scan->targets[i];
            double bx = cx + (radius * tgt->radar_dist) * cos(tgt->radar_angle);
            double by = cy + (radius * tgt->radar_dist) * sin(tgt->radar_angle);

            if (tgt->is_active) {
                cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.95);
                cairo_move_to(cr, bx, by - 5);
                cairo_line_to(cr, bx + 5, by);
                cairo_line_to(cr, bx, by + 5);
                cairo_line_to(cr, bx - 5, by);
                cairo_close_path(cr);
                cairo_fill(cr);

                cairo_set_font_size(cr, 8.0);
                cairo_move_to(cr, bx + 8, by + 3);
                cairo_show_text(cr, tgt->ssid);
            } else {
                cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.85);
                cairo_arc(cr, bx, by, 3.0, 0, 2 * M_PI);
                cairo_fill(cr);
            }
        }
    }

    cairo_restore(cr);
}

// ==============================================================================
// 🛸 SPACECRAFT SEQUENTIAL POWER-ON AWAKENING ENGINE
// ==============================================================================

double solaris_get_awakening_elapsed(void) {
    static double s_cached_stamp = 0.0;
    static double s_last_check_time = 0.0;
    static gboolean s_is_locked = FALSE;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    double now = (double)tv.tv_sec + ((double)tv.tv_usec / 1000000.0);

    // Throttle file checks to at most once every 30ms to eliminate I/O lag
    if (now - s_last_check_time > 0.030) {
        s_last_check_time = now;

        // Check lock state
        FILE *lfp = fopen("/tmp/solaris_lock_state", "r");
        if (lfp) {
            char lbuf[16];
            if (fgets(lbuf, sizeof(lbuf), lfp) && strncmp(lbuf, "LOCKED", 6) == 0) {
                s_is_locked = TRUE;
            } else {
                s_is_locked = FALSE;
            }
            fclose(lfp);
        } else {
            s_is_locked = FALSE;
        }

        // Check awakening time
        FILE *fp = fopen("/tmp/solaris_awakening_time", "r");
        if (fp) {
            double stamp = 0.0;
            if (fscanf(fp, "%lf", &stamp) == 1) {
                s_cached_stamp = stamp;
            }
            fclose(fp);
        }
    }

    if (s_is_locked) {
        return -1.0; // In locked stasis: all modules blacked out behind lockscreen
    }

    if (s_cached_stamp <= 0.0) return 999.0;
    double elapsed = now - s_cached_stamp;
    if (elapsed < 0.0 || elapsed > 6.0) return 999.0;
    return elapsed;
}

double solaris_calc_awakening_illumination(double start_delay, double duration) {
    double el = solaris_get_awakening_elapsed();
    if (el >= 999.0 || el >= (start_delay + duration)) return 1.0;
    if (el < 0.0 || el < start_delay) return 0.0; // Stasis blackout

    double p = (el - start_delay) / duration;
    p = CLAMP(p, 0.0, 1.0);

    // Authentic Fluorescent & Plasma Power-Up Ignition Curve
    if (p < 0.15) {
        return 0.05 + 0.35 * fabs(sin(p * 35.0));
    } else if (p < 0.30) {
        return 1.0;
    } else if (p < 0.48) {
        return 0.55 + 0.25 * sin(p * 25.0);
    } else {
        double sub_p = (p - 0.48) / 0.52;
        return 0.70 + (0.30 * (1.0 - exp(-3.0 * sub_p)));
    }
}

void solaris_draw_awakening_overlay(cairo_t *cr, double x, double y, double w, double h,
                                   double start_delay, double duration, const char *module_tag) {
    double el = solaris_get_awakening_elapsed();
    if (el >= 999.0) return;

    cairo_save(cr);
    cairo_rectangle(cr, x, y, w, h);
    cairo_clip(cr);

    if (el < 0.0 || el < start_delay) {
        // Deep Blackout Curtain before this module's turn (Prevents any split-second flash!)
        cairo_rectangle(cr, x, y, w, h);
        cairo_set_source_rgba(cr, 0.00, 0.01, 0.02, 0.96);
        cairo_fill(cr);
        cairo_restore(cr);
        return;
    }

    if (el >= (start_delay + duration + 0.25)) {
        cairo_restore(cr);
        return;
    }

    double p = (el - start_delay) / duration;
    p = CLAMP(p, 0.0, 1.0);
    double inv_p = 1.0 - p;

    // 1. Partial Dimming Curtain fading out as module illuminates
    if (p < 1.0) {
        double dim_alpha = (1.0 - p) * 0.88;
        cairo_rectangle(cr, x, y, w, h);
        cairo_set_source_rgba(cr, 0.00, 0.02, 0.04, dim_alpha);
        cairo_fill(cr);
    }

    // 2. High-Tech Laser Ignition Scanline sweeping downwards
    double scan_y = y + (p * h);
    cairo_move_to(cr, x, scan_y);
    cairo_line_to(cr, x + w, scan_y);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, inv_p * 0.95);
    cairo_set_line_width(cr, 2.2);
    cairo_stroke(cr);

    // Glowing laser trail
    cairo_rectangle(cr, x, y, w, scan_y - y);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, inv_p * 0.10);
    cairo_fill(cr);

    // 3. High-Intensity Ignition Border Flash
    cairo_rectangle(cr, x + 1.0, y + 1.0, w - 2.0, h - 2.0);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, inv_p * 0.75);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);

    // 4. Diagnostic Ignition HUD Pill Badge
    if (p > 0.12 && p < 0.88) {
        double bw = 165.0;
        double bh = 20.0;
        double bx = x + (w * 0.5) - (bw * 0.5);
        double by = y + (h * 0.5) - (bh * 0.5);

        cairo_rectangle(cr, bx, by, bw, bh);
        cairo_set_source_rgba(cr, 0.01, 0.03, 0.06, 0.94);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.90);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);

        cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 7.2);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.98);
        char buf[64];
        snprintf(buf, sizeof(buf), "⚡ %s // ONLINE", module_tag);
        cairo_move_to(cr, bx + 8.0, by + 13.5);
        cairo_show_text(cr, buf);
    }

    cairo_restore(cr);
}
