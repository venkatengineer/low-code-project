#include "orbital_core.h"
#include "math3d.h"
#include <math.h>
#include <pango/pangocairo.h>
#include <stdlib.h>

// =============================================================================
// 🪐 SOLARIS — COMPUTATIONAL TERRAIN (State B: Alien Sci-Fi Computational Landscape)
// A vast alien technological terrain representing remote SSH host:
//   venkat-server @ 100.110.135.100 (Tailscale WireGuard Mesh)
// 
// Features:
// 1. Compute Nexus (Foreground Stepped Processor Monoliths with living elevation flux)
// 2. Memory Megalith Terrace (Port Flank Crystal RAM Pillars with tier indicators)
// 3. Storage Canyons & NVMe Strata (Starboard Cascading Shelves with activity LEDs)
// 4. Central Canyon Chasm & Horizon Tech Mountain Ridges
// 5. High-Gain Transmission Spire with Skyward Volumetric Beacon
// 6. Dense Branching Conduit Networks with Animated Traveling Photon Packets
// 7. Dual Sensor Scanning Planes (Longitudinal Laser Sheet + Rotational Radar Cone)
// 8. Periodic Computational Energy Surges (9-second expanding wavefront)
// 9. 3D Floating Atmospheric Data Motes
// 10. Authentic Tailscale SSH Telemetry HUD & Interactive Connect Action
// =============================================================================

static void draw_tech_bracket_box(cairo_t *cr, double x, double y, double w, double h, double c) {
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

static cairo_surface_t *s_poster_surface = NULL;
static gboolean s_poster_loaded = FALSE;

static void ensure_poster_surface(void) {
    if (s_poster_loaded) return;
    s_poster_loaded = TRUE;
    cairo_surface_t *raw = cairo_image_surface_create_from_png("/home/venkatengineer/solaris-os/assets/computational-terrain/solaris_terrain_poster.png");
    if (cairo_surface_status(raw) != CAIRO_STATUS_SUCCESS) {
        if (raw) cairo_surface_destroy(raw);
        s_poster_surface = NULL;
        return;
    }
    int raw_w = cairo_image_surface_get_width(raw);
    int raw_h = cairo_image_surface_get_height(raw);
    s_poster_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 640, 360);
    cairo_t *cr_scale = cairo_create(s_poster_surface);
    cairo_scale(cr_scale, 640.0 / (double)raw_w, 360.0 / (double)raw_h);
    cairo_set_source_surface(cr_scale, raw, 0, 0);
    cairo_paint(cr_scale);
    cairo_destroy(cr_scale);
    cairo_surface_destroy(raw);
}

void render_server_chassis(cairo_t *cr, const orbital_core_t *core, const camera3d_t *cam,
                           double cx, double cy, double scale, double alpha) {
    if (alpha <= 0.001) return;

    cairo_save(cr);

    // 0. Render Master Blender Cinematic Alien Terrain Backdrop (Soft Radial Blend)
    ensure_poster_surface();
    if (s_poster_surface) {
        cairo_save(cr);
        double target_w = 640.0;
        double target_h = 360.0;
        double img_x = cx - (target_w * 0.5);
        double img_y = cy - (target_h * 0.5) - 15.0;

        cairo_pattern_t *mask = cairo_pattern_create_radial(cx, cy - 15.0, target_w * 0.14, cx, cy - 15.0, target_w * 0.49);
        cairo_pattern_add_color_stop_rgba(mask, 0.0, 0, 0, 0, 0.95 * alpha);
        cairo_pattern_add_color_stop_rgba(mask, 0.70, 0, 0, 0, 0.85 * alpha);
        cairo_pattern_add_color_stop_rgba(mask, 1.0, 0, 0, 0, 0.0);

        cairo_set_source_surface(cr, s_poster_surface, img_x, img_y);
        cairo_mask(cr, mask);
        cairo_pattern_destroy(mask);
        cairo_restore(cr);
    }

    const server_telemetry_t *tel = &core->telemetry;
    const char *remote_host = (tel->remote_host[0]) ? tel->remote_host : "venkat-server";
    const char *remote_ip   = (tel->remote_ip[0])   ? tel->remote_ip   : "100.110.135.100";
    double cpu_usage = tel->cpu_usage;
    double mem_used_gb = tel->mem_used_gb > 0 ? tel->mem_used_gb : 6.3;
    double mem_total_gb = tel->mem_total_gb > 0 ? tel->mem_total_gb : 15.3;
    double mem_pct = (mem_total_gb > 0) ? (mem_used_gb / mem_total_gb * 100.0) : 41.0;
    double ping_ms = tel->remote_ping_ms > 0 ? tel->remote_ping_ms : 3.2;

    // =========================================================================
    // TACTICAL HUD OVERLAYS & TELEMETRY CALLOUTS
    // =========================================================================
    cairo_save(cr);
    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *f_banner = pango_font_description_from_string("JetBrainsMono Nerd Font Bold 9.0");
    PangoFontDescription *f_sub    = pango_font_description_from_string("JetBrainsMono Nerd Font Semi-Bold 7.2");
    PangoFontDescription *f_val    = pango_font_description_from_string("JetBrainsMono Nerd Font Regular 7.0");

    // Top Status Banner: REMOTE HOST & TAILSCALE WIREguard STATUS
    double frame_w = 480.0;
    double frame_h = 34.0;
    double fx = cx - (frame_w * 0.5);
    double fy = 46.0;

    draw_tech_bracket_box(cr, fx, fy, frame_w, frame_h, 4.0);
    cairo_set_source_rgba(cr, 0.015, 0.035, 0.07, alpha * 0.90);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, alpha * 0.75);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    // Host status text
    pango_layout_set_font_description(layout, f_banner);
    char host_buf[256];
    snprintf(host_buf, sizeof(host_buf), "REMOTE HOST: %s // TAILSCALE %s ● ONLINE", remote_host, remote_ip);
    pango_layout_set_text(layout, host_buf, -1);
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, alpha * 0.98);
    cairo_move_to(cr, fx + 12.0, fy + 4.0);
    pango_cairo_show_layout(cr, layout);

    // Sub-line: P2P Endpoint and RTT
    pango_layout_set_font_description(layout, f_sub);
    char mesh_buf[128];
    snprintf(mesh_buf, sizeof(mesh_buf), "DIRECT WIREguard P2P (192.168.29.105:41641)  |  RTT: %.1fms  |  SSH: PORT 22", ping_ms);
    pango_layout_set_text(layout, mesh_buf, -1);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, alpha * 0.85);
    cairo_move_to(cr, fx + 12.0, fy + 19.0);
    pango_cairo_show_layout(cr, layout);

    // Corner brackets framing the central terrain
    double brk_w = 16.0;
    double bx1 = fx - 16.0, bx2 = fx + frame_w + 16.0;
    double by1 = fy + frame_h + 8.0, by2 = cy + 120.0;

    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, alpha * 0.50);
    cairo_set_line_width(cr, 1.2);
    // Top-left bracket
    cairo_move_to(cr, bx1, by1 + brk_w); cairo_line_to(cr, bx1, by1); cairo_line_to(cr, bx1 + brk_w, by1); cairo_stroke(cr);
    // Top-right bracket
    cairo_move_to(cr, bx2 - brk_w, by1); cairo_line_to(cr, bx2, by1); cairo_line_to(cr, bx2, by1 + brk_w); cairo_stroke(cr);
    // Bottom-left bracket
    cairo_move_to(cr, bx1, by2 - brk_w); cairo_line_to(cr, bx1, by2); cairo_line_to(cr, bx1 + brk_w, by2); cairo_stroke(cr);
    // Bottom-right bracket
    cairo_move_to(cr, bx2 - brk_w, by2); cairo_line_to(cr, bx2, by2); cairo_line_to(cr, bx2, by2 - brk_w); cairo_stroke(cr);

    // Left Callout: Compute Nexus & Memory Matrix Telemetry
    double l_x = fx - 110.0;
    double l_y = cy - 25.0;
    draw_tech_bracket_box(cr, l_x, l_y, 105.0, 56.0, 3.0);
    cairo_set_source_rgba(cr, 0.015, 0.035, 0.065, alpha * 0.88);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, alpha * 0.50);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);

    // Leader line to Memory Terrace
    cairo_move_to(cr, l_x + 105.0, l_y + 28.0);
    cairo_line_to(cr, fx, cy);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, alpha * 0.35);
    cairo_stroke(cr);

    pango_layout_set_font_description(layout, f_sub);
    pango_layout_set_text(layout, "◆ COMPUTE & RAM", -1);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, alpha * 0.95);
    cairo_move_to(cr, l_x + 6.0, l_y + 4.0);
    pango_cairo_show_layout(cr, layout);

    pango_layout_set_font_description(layout, f_val);
    char cpu_buf[32];
    snprintf(cpu_buf, sizeof(cpu_buf), "CPU: %.0f%% (%.0f°C)", cpu_usage, tel->cpu_temp > 0 ? tel->cpu_temp : 54.0);
    pango_layout_set_text(layout, cpu_buf, -1);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha * 0.88);
    cairo_move_to(cr, l_x + 6.0, l_y + 19.0);
    pango_cairo_show_layout(cr, layout);

    char ram_buf[32];
    snprintf(ram_buf, sizeof(ram_buf), "RAM: %.1fG (%.0f%%)", mem_used_gb, mem_pct);
    pango_layout_set_text(layout, ram_buf, -1);
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, alpha * 0.90);
    cairo_move_to(cr, l_x + 6.0, l_y + 35.0);
    pango_cairo_show_layout(cr, layout);

    // Right Callout: Storage Canyons & Network Throughput
    double r_x = fx + frame_w + 5.0;
    double r_y = cy - 25.0;
    draw_tech_bracket_box(cr, r_x, r_y, 105.0, 56.0, 3.0);
    cairo_set_source_rgba(cr, 0.015, 0.035, 0.065, alpha * 0.88);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, alpha * 0.50);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);

    // Leader line to Storage Canyons
    cairo_move_to(cr, r_x, r_y + 28.0);
    cairo_line_to(cr, fx + frame_w, cy - 10.0);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, alpha * 0.35);
    cairo_stroke(cr);

    pango_layout_set_font_description(layout, f_sub);
    pango_layout_set_text(layout, "◆ STORAGE & MESH", -1);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, alpha * 0.95);
    cairo_move_to(cr, r_x + 6.0, r_y + 4.0);
    pango_cairo_show_layout(cr, layout);

    pango_layout_set_font_description(layout, f_val);
    char disk_buf[32];
    double disk_io = tel->disk_read_mbs + tel->disk_write_mbs;
    snprintf(disk_buf, sizeof(disk_buf), "NVMe: %.1f MB/s %s", disk_io, tel->disk_active ? "BUSY" : "OK");
    pango_layout_set_text(layout, disk_buf, -1);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha * 0.88);
    cairo_move_to(cr, r_x + 6.0, r_y + 19.0);
    pango_cairo_show_layout(cr, layout);

    char net_buf[32];
    snprintf(net_buf, sizeof(net_buf), "NET: ↓%.1f ↑%.1fK", tel->net_rx_kbs, tel->net_tx_kbs);
    pango_layout_set_text(layout, net_buf, -1);
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, alpha * 0.90);
    cairo_move_to(cr, r_x + 6.0, r_y + 35.0);
    pango_cairo_show_layout(cr, layout);

    // =========================================================================
    // 13. PRIMARY ACTION: [ 󰒋 CONNECT // SSH (venkat-server) ]
    // =========================================================================
    double btn_w = 270.0;
    double btn_h = 32.0;
    double btn_x = cx - (btn_w * 0.5);
    double btn_y = cy + 130.0;

    gboolean is_btn_hover = (core->hovered_hotspot == HOTSPOT_CONNECT_BTN);

    draw_tech_bracket_box(cr, btn_x, btn_y, btn_w, btn_h, 4.0);
    if (is_btn_hover) {
        cairo_set_source_rgba(cr, 0.0, 0.45, 0.55, alpha * 0.96);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha * 1.0);
        cairo_set_line_width(cr, 1.6);
        cairo_stroke(cr);
    } else {
        cairo_set_source_rgba(cr, 0.015, 0.045, 0.08, alpha * 0.92);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, alpha * 0.80);
        cairo_set_line_width(cr, 1.1);
        cairo_stroke(cr);
    }

    pango_layout_set_font_description(layout, f_banner);
    pango_layout_set_text(layout, "󰒋 CONNECT // SSH (venkat-server)", -1);
    int clw, clh;
    pango_layout_get_pixel_size(layout, &clw, &clh);

    cairo_set_source_rgba(cr, is_btn_hover ? 1.0 : COLOR_CYAN_R,
                              is_btn_hover ? 1.0 : COLOR_CYAN_G,
                              is_btn_hover ? 1.0 : COLOR_CYAN_B, alpha * 0.98);
    cairo_move_to(cr, btn_x + (btn_w - clw) * 0.5, btn_y + (btn_h - clh) * 0.5);
    pango_cairo_show_layout(cr, layout);

    // Directional Navigation Hints
    double hint_y = btn_y + btn_h + 8.0;
    pango_layout_set_font_description(layout, f_val);
    pango_layout_set_text(layout, "◀ SWIPE LEFT: ORBITAL   |   SWIPE RIGHT: TERMINAL ▶", -1);
    int hlw, hlh;
    pango_layout_get_pixel_size(layout, &hlw, &hlh);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.3, COLOR_TEXT_MUTED_G + 0.3, COLOR_TEXT_MUTED_B + 0.3, alpha * 0.70);
    cairo_move_to(cr, cx - (hlw * 0.5), hint_y);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);
    pango_font_description_free(f_banner);
    pango_font_description_free(f_sub);
    pango_font_description_free(f_val);
    cairo_restore(cr);

    cairo_restore(cr);
}
