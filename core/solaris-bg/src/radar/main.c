#include "common.h"
#include "telemetry.h"
#include "cairo_utils.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    net_telemetry_t net;
    wifi_scan_telemetry_t wifi_scan;
    double radar_angle;
    double time_sec;
    guint anim_timer_id;
    guint telem_timer_id;
    int hovered_idx;
} radar_app_t;

static radar_app_t g_app;

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    (void)user_data;

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);

    double pad_x = 10.0;
    double cur_y = 10.0;
    double total_w = (double)width - 20.0;
    double total_h = (double)height - 20.0;

    // Outer overarching holographic deck panel
    cairo_draw_beveled_panel(cr, pad_x, cur_y, total_w, total_h, 10.0);
    cairo_draw_module_header(cr, pad_x, cur_y, total_w, "TACTICAL RF RADAR // DISCOVERED NETWORK BEACONS", COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // Layout: Tri-panel arena
    // Left: Wi-Fi RF Matrix (w: 195)
    // Center: Holographic Sweep Radar (w: 320)
    // Right: Discovered Target Matrix with Connect Actions (w: 245)

    double left_w = 200.0;
    double right_w = 265.0;
    double left_x = pad_x + 12.0;
    double right_x = pad_x + total_w - right_w - 8.0;

    // -------------------------------------------------------------
    // LEFT SUB-PANEL: ACTIVE WI-FI LINK TELEMETRY
    // -------------------------------------------------------------
    cairo_draw_beveled_panel(cr, left_x, cur_y + 30.0, left_w, total_h - 40.0, 6.0);
    cairo_draw_module_header(cr, left_x, cur_y + 30.0, left_w, "ACTIVE CARRIER LINK", COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);

    // Mini radar
    cairo_draw_mini_radar(cr, left_x + 100.0, cur_y + 86.0, 34.0, g_app.radar_angle * 1.5,
                          COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);

    // Active network details
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.9);
    cairo_move_to(cr, left_x + 10.0, cur_y + 136.0);
    cairo_show_text(cr, "ACTIVE BEACON (SSID):");

    cairo_set_font_size(cr, 11.0);
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 1.0);
    cairo_move_to(cr, left_x + 10.0, cur_y + 154.0);
    cairo_show_text(cr, g_app.net.ssid);

    char signal_str[48];
    snprintf(signal_str, sizeof(signal_str), "SIGNAL: %d dBm (%d%%)", g_app.net.signal_dbm, g_app.net.signal_pct);
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.9);
    cairo_move_to(cr, left_x + 10.0, cur_y + 172.0);
    cairo_show_text(cr, signal_str);

    char iface_str[64];
    snprintf(iface_str, sizeof(iface_str), "IFACE: %s [5 GHz INFRA]", g_app.net.iface);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.85);
    cairo_move_to(cr, left_x + 10.0, cur_y + 188.0);
    cairo_show_text(cr, iface_str);

    char net_rate_str[48];
    snprintf(net_rate_str, sizeof(net_rate_str), "THROUGHPUT: %.1f KB/s", g_app.net.rx_kbs);
    cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.9);
    cairo_move_to(cr, left_x + 10.0, cur_y + 204.0);
    cairo_show_text(cr, net_rate_str);

    // RX Sparkline
    cairo_draw_sparkline(cr, left_x + 10.0, cur_y + 214.0, left_w - 20.0, 26.0,
                         g_app.net.history_rx, g_app.net.history_count,
                         0.0, 500.0,
                         COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);

    // -------------------------------------------------------------
    // CENTER SUB-PANEL: TACTICAL DEEP-SPACE SWEEP RADAR
    // -------------------------------------------------------------
    double radar_cx = pad_x + 12.0 + left_w + (right_x - (left_x + left_w)) / 2.0;
    double radar_cy = cur_y + 138.0;
    double radar_radius = 96.0;

    cairo_draw_tactical_radar_deepspace(cr, radar_cx, radar_cy, radar_radius, g_app.radar_angle, g_app.time_sec, &g_app.wifi_scan);

    // Coordinate telemetry under radar
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.9);

    double deg = fmod(g_app.radar_angle * (180.0 / M_PI), 360.0);
    char coord_str[64];
    snprintf(coord_str, sizeof(coord_str), "AZIMUTH: %05.1f° // TARGETS: %02d // RF LOCK ACTIVE", deg, g_app.wifi_scan.target_count);
    cairo_text_extents_t cext;
    cairo_text_extents(cr, coord_str, &cext);
    cairo_move_to(cr, radar_cx - cext.width / 2.0, cur_y + 252.0);
    cairo_show_text(cr, coord_str);

    // -------------------------------------------------------------
    // RIGHT SUB-PANEL: DISCOVERED TARGETS & INTERACTIVE CONNECT
    // -------------------------------------------------------------
    cairo_draw_beveled_panel(cr, right_x, cur_y + 30.0, right_w, total_h - 40.0, 6.0);
    cairo_draw_module_header(cr, right_x, cur_y + 30.0, right_w, "DISCOVERED TARGETS // CONNECT", COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    cairo_set_font_size(cr, 8.5);

    int max_rows = 5;
    for (int i = 0; i < g_app.wifi_scan.target_count && i < max_rows; i++) {
        const wifi_target_t *tgt = &g_app.wifi_scan.targets[i];
        double ry = cur_y + 54.0 + (double)i * 38.0;
        gboolean is_hover = (g_app.hovered_idx == i);

        // Row button highlight
        if (tgt->is_active) {
            cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.15);
            cairo_rectangle(cr, right_x + 8.0, ry - 10.0, right_w - 16.0, 32.0);
            cairo_fill(cr);

            cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.7);
            cairo_set_line_width(cr, 1.0);
            cairo_rectangle(cr, right_x + 8.0, ry - 10.0, right_w - 16.0, 32.0);
            cairo_stroke(cr);
        } else if (is_hover) {
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.20);
            cairo_rectangle(cr, right_x + 8.0, ry - 10.0, right_w - 16.0, 32.0);
            cairo_fill(cr);

            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.8);
            cairo_set_line_width(cr, 1.0);
            cairo_rectangle(cr, right_x + 8.0, ry - 10.0, right_w - 16.0, 32.0);
            cairo_stroke(cr);
        } else {
            cairo_set_source_rgba(cr, 0.04, 0.07, 0.10, 0.5);
            cairo_rectangle(cr, right_x + 8.0, ry - 10.0, right_w - 16.0, 32.0);
            cairo_fill(cr);

            cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.2);
            cairo_set_line_width(cr, 0.8);
            cairo_rectangle(cr, right_x + 8.0, ry - 10.0, right_w - 16.0, 32.0);
            cairo_stroke(cr);
        }

        // Target Name & Security
        cairo_set_source_rgba(cr, tgt->is_active ? COLOR_GREEN_R : COLOR_TEXT_BRIGHT_R,
                                  tgt->is_active ? COLOR_GREEN_G : COLOR_TEXT_BRIGHT_G,
                                  tgt->is_active ? COLOR_GREEN_B : COLOR_TEXT_BRIGHT_B, 0.95);
        cairo_move_to(cr, right_x + 14.0, ry + 2.0);
        cairo_show_text(cr, tgt->ssid);

        char sub_info[48];
        snprintf(sub_info, sizeof(sub_info), "%s // %d%% // %s", tgt->band, tgt->signal_pct, tgt->security);
        cairo_set_font_size(cr, 7.5);
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.85);
        cairo_move_to(cr, right_x + 14.0, ry + 16.0);
        cairo_show_text(cr, sub_info);
        cairo_set_font_size(cr, 8.5);

        // Action Tag [CONNECTED] or [CONNECT]
        if (tgt->is_active) {
            cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.95);
            cairo_move_to(cr, right_x + right_w - 75.0, ry + 8.0);
            cairo_show_text(cr, "● ACTIVE");
        } else {
            cairo_set_source_rgba(cr, is_hover ? COLOR_CYAN_R : COLOR_TEXT_MUTED_R,
                                      is_hover ? COLOR_CYAN_G : COLOR_TEXT_MUTED_G,
                                      is_hover ? COLOR_CYAN_B : COLOR_TEXT_MUTED_B, 0.9);
            cairo_move_to(cr, right_x + right_w - 80.0, ry + 8.0);
            cairo_show_text(cr, "[CONNECT]");
        }
    }

    return FALSE;
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    (void)user_data;

    int width = gtk_widget_get_allocated_width(widget);
    double pad_x = 10.0;
    double cur_y = 10.0;
    double total_w = (double)width - 20.0;
    double right_w = 265.0;
    double right_x = pad_x + total_w - right_w - 8.0;

    if (event->x >= right_x && event->x <= right_x + right_w) {
        for (int i = 0; i < g_app.wifi_scan.target_count && i < 5; i++) {
            double ry = cur_y + 54.0 + (double)i * 38.0;
            if (event->y >= ry - 10.0 && event->y <= ry + 22.0) {
                // Clicked target row!
                const wifi_target_t *tgt = &g_app.wifi_scan.targets[i];
                trigger_wifi_connect(tgt->ssid);
                return TRUE;
            }
        }
    }
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    (void)user_data;
    int width = gtk_widget_get_allocated_width(widget);
    double pad_x = 10.0;
    double cur_y = 10.0;
    double total_w = (double)width - 20.0;
    double right_w = 265.0;
    double right_x = pad_x + total_w - right_w - 8.0;

    int old_hover = g_app.hovered_idx;
    g_app.hovered_idx = -1;

    if (event->x >= right_x && event->x <= right_x + right_w) {
        for (int i = 0; i < g_app.wifi_scan.target_count && i < 5; i++) {
            double ry = cur_y + 54.0 + (double)i * 38.0;
            if (event->y >= ry - 10.0 && event->y <= ry + 22.0) {
                g_app.hovered_idx = i;
                break;
            }
        }
    }

    if (old_hover != g_app.hovered_idx) {
        gtk_widget_queue_draw(widget);
    }
    return FALSE;
}

static gboolean on_animation_tick(gpointer user_data) {
    (void)user_data;
    g_app.radar_angle += 0.05;
    if (g_app.radar_angle > 2.0 * M_PI) g_app.radar_angle -= 2.0 * M_PI;
    g_app.time_sec += 0.040;

    if (g_app.drawing_area) {
        gtk_widget_queue_draw(g_app.drawing_area);
    }
    return G_SOURCE_CONTINUE;
}

static gboolean on_telemetry_tick(gpointer user_data) {
    (void)user_data;
    update_net_telemetry(&g_app.net);
    update_wifi_scan_telemetry(&g_app.wifi_scan);
    return G_SOURCE_CONTINUE;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    solaris_load_css();

    memset(&g_app, 0, sizeof(g_app));
    g_app.hovered_idx = -1;
    init_telemetry();
    update_net_telemetry(&g_app.net);
    update_wifi_scan_telemetry(&g_app.wifi_scan);

    g_app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    solaris_setup_layer_window(g_app.window, "solaris-bg-radar",
                               800, 310,
                               FALSE, TRUE, TRUE, FALSE, // Bottom-Left / Center
                               0, 75, 330, 0);

    g_app.drawing_area = gtk_drawing_area_new();
    gtk_widget_add_events(g_app.drawing_area, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(g_app.drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(g_app.drawing_area, "button-press-event", G_CALLBACK(on_button_press), NULL);
    g_signal_connect(g_app.drawing_area, "motion-notify-event", G_CALLBACK(on_motion_notify), NULL);
    gtk_container_add(GTK_CONTAINER(g_app.window), g_app.drawing_area);

    // 25 FPS animation timer
    g_app.anim_timer_id = g_timeout_add(40, on_animation_tick, NULL);
    // Telemetry tick every 1.5s
    g_app.telem_timer_id = g_timeout_add(1500, on_telemetry_tick, NULL);

    gtk_widget_show_all(g_app.window);
    gtk_main();
    return 0;
}
