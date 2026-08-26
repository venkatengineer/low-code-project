#include "common.h"
#include "telemetry.h"
#include "cairo_utils.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    net_telemetry_t net;
    wifi_scan_telemetry_t wifi_scan;
    double time_sec;
    guint anim_timer_id;
    guint telem_timer_id;
    int hovered_idx; // -1: none, -2: rescan button, 0..N: target index
    int scroll_offset;
    gboolean is_rescanning;
    double rescan_anim_timer;
} wifi_app_t;

static wifi_app_t g_app;

static void trigger_wifi_rescan(void) {
    g_app.is_rescanning = TRUE;
    g_app.rescan_anim_timer = 2.0;
    g_spawn_command_line_async("nmcli dev wifi rescan", NULL);
    update_wifi_scan_telemetry(&g_app.wifi_scan);
    if (g_app.drawing_area) {
        gtk_widget_queue_draw(g_app.drawing_area);
    }
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    (void)user_data;

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);

    double pad_x = 8.0;
    double cur_y = 8.0;
    double panel_w = (double)width - 16.0;
    double panel_h = (double)height - 16.0;

    // Outer Cyber Deck Panel
    cairo_draw_beveled_panel(cr, pad_x, cur_y, panel_w, panel_h, 8.0);
    cairo_draw_module_header(cr, pad_x, cur_y, panel_w, "WI-FI RF MATRIX // 3D GLOBE RADAR", COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);

    // -------------------------------------------------------------
    // LEFT: eDEX-UI 3D HOLOGRAPHIC GLOBE & RF TELEMETRY
    // -------------------------------------------------------------
    double globe_w = 190.0;
    double globe_cx = pad_x + 95.0;
    double globe_cy = cur_y + 80.0;
    double globe_r = 48.0;

    cairo_draw_edex_globe(cr, globe_cx, globe_cy, globe_r, g_app.time_sec, &g_app.wifi_scan);

    // Active Link Details (Clean non-overlapping vertical stack)
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

    char link_str[128];
    snprintf(link_str, sizeof(link_str), "ACTIVE: %s (%d%%)", g_app.net.ssid, g_app.net.signal_pct);
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.95);
    cairo_text_extents_t text;
    cairo_text_extents(cr, link_str, &text);
    cairo_move_to(cr, globe_cx - text.width / 2.0, cur_y + 140.0);
    cairo_show_text(cr, link_str);

    char tp_str[64];
    snprintf(tp_str, sizeof(tp_str), "THROUGHPUT: %.1f KB/s // 5 GHz", g_app.net.rx_kbs);
    cairo_set_font_size(cr, 7.5);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.85);
    cairo_text_extents(cr, tp_str, &text);
    cairo_move_to(cr, globe_cx - text.width / 2.0, cur_y + 154.0);
    cairo_show_text(cr, tp_str);

    // Dedicated RX Sparkline
    cairo_draw_sparkline(cr, pad_x + 10.0, cur_y + 166.0, globe_w - 12.0, 48.0,
                         g_app.net.history_rx, g_app.net.history_count,
                         0.0, 500.0,
                         COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);

    // -------------------------------------------------------------
    // RIGHT: DISCOVERED BEACONS MATRIX & CONNECT ACTIONS
    // -------------------------------------------------------------
    double right_x = pad_x + globe_w + 10.0;
    double right_w = panel_w - globe_w - 18.0;

    cairo_draw_beveled_panel(cr, right_x, cur_y + 26.0, right_w, panel_h - 34.0, 6.0);
    cairo_draw_module_header(cr, right_x, cur_y + 26.0, right_w, "DISCOVERED BEACONS", COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // Sleek Small Integrated [ ⟳ RESCAN ] Button in header
    double btn_w = 66.0;
    double btn_h = 16.0;
    double btn_x = right_x + right_w - btn_w - 6.0;
    double btn_y = cur_y + 30.0;
    gboolean is_btn_hover = (g_app.hovered_idx == -2);

    if (is_btn_hover || g_app.is_rescanning) {
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.25);
        cairo_rectangle(cr, btn_x, btn_y, btn_w, btn_h);
        cairo_fill(cr);

        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.9);
        cairo_set_line_width(cr, 1.0);
        cairo_rectangle(cr, btn_x, btn_y, btn_w, btn_h);
        cairo_stroke(cr);
    } else {
        cairo_set_source_rgba(cr, 0.05, 0.10, 0.16, 0.7);
        cairo_rectangle(cr, btn_x, btn_y, btn_w, btn_h);
        cairo_fill(cr);

        cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.4);
        cairo_set_line_width(cr, 0.8);
        cairo_rectangle(cr, btn_x, btn_y, btn_w, btn_h);
        cairo_stroke(cr);
    }

    cairo_set_font_size(cr, 7.5);
    cairo_set_source_rgba(cr, (is_btn_hover || g_app.is_rescanning) ? 1.0 : COLOR_CYAN_R,
                              (is_btn_hover || g_app.is_rescanning) ? 1.0 : COLOR_CYAN_G,
                              (is_btn_hover || g_app.is_rescanning) ? 1.0 : COLOR_CYAN_B, 0.95);
    cairo_move_to(cr, btn_x + 8.0, btn_y + 11.5);
    cairo_show_text(cr, g_app.is_rescanning ? "SCANNING..." : "⟳ RESCAN");

    int visible_rows = 5;
    double row_start_y = cur_y + 54.0;
    double row_h = 36.0;
    double row_gap = 4.0;
    double row_w = (g_app.wifi_scan.target_count > visible_rows) ? right_w - 18.0 : right_w - 12.0;

    // Clamp scroll offset
    int max_scroll = g_app.wifi_scan.target_count - visible_rows;
    if (max_scroll < 0) max_scroll = 0;
    if (g_app.scroll_offset > max_scroll) g_app.scroll_offset = max_scroll;
    if (g_app.scroll_offset < 0) g_app.scroll_offset = 0;

    for (int slot = 0; slot < visible_rows; slot++) {
        int i = g_app.scroll_offset + slot;
        if (i >= g_app.wifi_scan.target_count) break;

        const wifi_target_t *tgt = &g_app.wifi_scan.targets[i];
        double ry = row_start_y + (double)slot * (row_h + row_gap);
        gboolean is_hover = (g_app.hovered_idx == i);

        // Row background box
        if (tgt->is_active) {
            cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.15);
            cairo_rectangle(cr, right_x + 6.0, ry, row_w, row_h);
            cairo_fill(cr);

            cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.75);
            cairo_set_line_width(cr, 1.0);
            cairo_rectangle(cr, right_x + 6.0, ry, row_w, row_h);
            cairo_stroke(cr);
        } else if (is_hover) {
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.20);
            cairo_rectangle(cr, right_x + 6.0, ry, row_w, row_h);
            cairo_fill(cr);

            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.85);
            cairo_set_line_width(cr, 1.0);
            cairo_rectangle(cr, right_x + 6.0, ry, row_w, row_h);
            cairo_stroke(cr);
        } else {
            cairo_set_source_rgba(cr, 0.04, 0.07, 0.10, 0.5);
            cairo_rectangle(cr, right_x + 6.0, ry, row_w, row_h);
            cairo_fill(cr);

            cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.25);
            cairo_set_line_width(cr, 0.8);
            cairo_rectangle(cr, right_x + 6.0, ry, row_w, row_h);
            cairo_stroke(cr);
        }

        // SSID
        char disp_ssid[32];
        snprintf(disp_ssid, sizeof(disp_ssid), "%s", tgt->ssid);
        if (strlen(disp_ssid) > 15) {
            disp_ssid[12] = '.';
            disp_ssid[13] = '.';
            disp_ssid[14] = '.';
            disp_ssid[15] = '\0';
        }

        cairo_set_font_size(cr, 8.5);
        cairo_set_source_rgba(cr, tgt->is_active ? COLOR_GREEN_R : COLOR_TEXT_BRIGHT_R,
                                  tgt->is_active ? COLOR_GREEN_G : COLOR_TEXT_BRIGHT_G,
                                  tgt->is_active ? COLOR_GREEN_B : COLOR_TEXT_BRIGHT_B, 0.95);
        cairo_move_to(cr, right_x + 12.0, ry + 13.0);
        cairo_show_text(cr, disp_ssid);

        // Security / Band / Signal subline
        char details[48];
        snprintf(details, sizeof(details), "%s // %d%% // %s", tgt->band, tgt->signal_pct, tgt->security);
        cairo_set_font_size(cr, 7.5);
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.85);
        cairo_move_to(cr, right_x + 12.0, ry + 27.0);
        cairo_show_text(cr, details);

        // Action Badge on Right Side
        cairo_set_font_size(cr, 8.0);
        if (tgt->is_active) {
            cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.95);
            cairo_move_to(cr, right_x + row_w - 68.0, ry + 20.0);
            cairo_show_text(cr, "● ACTIVE");
        } else {
            cairo_set_source_rgba(cr, is_hover ? COLOR_CYAN_R : COLOR_TEXT_MUTED_R,
                                      is_hover ? COLOR_CYAN_G : COLOR_TEXT_MUTED_G,
                                      is_hover ? COLOR_CYAN_B : COLOR_TEXT_MUTED_B, 0.9);
            cairo_move_to(cr, right_x + row_w - 70.0, ry + 20.0);
            cairo_show_text(cr, "[CONNECT]");
        }
    }

    // Sleek Cyberpunk Scrollbar
    if (g_app.wifi_scan.target_count > visible_rows) {
        double track_x = right_x + right_w - 8.0;
        double track_y = row_start_y;
        double track_h = (double)visible_rows * (row_h + row_gap) - row_gap;

        // Track line
        cairo_set_source_rgba(cr, 0.05, 0.10, 0.15, 0.6);
        cairo_rectangle(cr, track_x, track_y, 4.0, track_h);
        cairo_fill(cr);

        // Thumb
        double thumb_h = track_h * ((double)visible_rows / (double)g_app.wifi_scan.target_count);
        if (thumb_h < 18.0) thumb_h = 18.0;
        double thumb_y = track_y + (track_h - thumb_h) * ((double)g_app.scroll_offset / (double)max_scroll);

        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.85);
        cairo_rectangle(cr, track_x, thumb_y, 4.0, thumb_h);
        cairo_fill(cr);
    }

    // Spacecraft Sequential Power-On Awakening Overlay (Starts at 1.4s)
    solaris_draw_awakening_overlay(cr, pad_x, cur_y, panel_w, panel_h, 1.4, 0.7, "WI-FI RF MATRIX");

    return FALSE;
}

static gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
    (void)user_data;
    int visible_rows = 5;
    int max_scroll = g_app.wifi_scan.target_count - visible_rows;
    if (max_scroll < 0) max_scroll = 0;

    double dx, dy;
    if (gdk_event_get_scroll_deltas((GdkEvent *)event, &dx, &dy)) {
        if (dy < 0 && g_app.scroll_offset > 0) {
            g_app.scroll_offset--;
        } else if (dy > 0 && g_app.scroll_offset < max_scroll) {
            g_app.scroll_offset++;
        }
    } else if (event->direction == GDK_SCROLL_UP) {
        if (g_app.scroll_offset > 0) g_app.scroll_offset--;
    } else if (event->direction == GDK_SCROLL_DOWN) {
        if (g_app.scroll_offset < max_scroll) g_app.scroll_offset++;
    }

    gtk_widget_queue_draw(widget);
    return TRUE;
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    (void)user_data;

    int width = gtk_widget_get_allocated_width(widget);
    double pad_x = 8.0;
    double cur_y = 8.0;
    double panel_w = (double)width - 16.0;
    double globe_w = 190.0;
    double right_x = pad_x + globe_w + 10.0;
    double right_w = panel_w - globe_w - 18.0;

    // Rescan button hit test
    double btn_w = 66.0;
    double btn_h = 16.0;
    double btn_x = right_x + right_w - btn_w - 6.0;
    double btn_y = cur_y + 30.0;
    if (event->x >= btn_x && event->x <= btn_x + btn_w && event->y >= btn_y && event->y <= btn_y + btn_h) {
        trigger_wifi_rescan();
        return TRUE;
    }

    int visible_rows = 5;
    double row_start_y = cur_y + 54.0;
    double row_h = 36.0;
    double row_gap = 4.0;

    if (event->x >= right_x && event->x <= right_x + right_w) {
        for (int slot = 0; slot < visible_rows; slot++) {
            int i = g_app.scroll_offset + slot;
            if (i >= g_app.wifi_scan.target_count) break;

            double ry = row_start_y + (double)slot * (row_h + row_gap);
            if (event->y >= ry && event->y <= ry + row_h) {
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
    double pad_x = 8.0;
    double cur_y = 8.0;
    double panel_w = (double)width - 16.0;
    double globe_w = 190.0;
    double right_x = pad_x + globe_w + 10.0;
    double right_w = panel_w - globe_w - 18.0;

    int old_hover = g_app.hovered_idx;
    g_app.hovered_idx = -1;

    // Rescan button hover test
    double btn_w = 66.0;
    double btn_h = 16.0;
    double btn_x = right_x + right_w - btn_w - 6.0;
    double btn_y = cur_y + 30.0;
    if (event->x >= btn_x && event->x <= btn_x + btn_w && event->y >= btn_y && event->y <= btn_y + btn_h) {
        g_app.hovered_idx = -2;
    } else if (event->x >= right_x && event->x <= right_x + right_w) {
        int visible_rows = 5;
        double row_start_y = cur_y + 54.0;
        double row_h = 36.0;
        double row_gap = 4.0;

        for (int slot = 0; slot < visible_rows; slot++) {
            int i = g_app.scroll_offset + slot;
            if (i >= g_app.wifi_scan.target_count) break;

            double ry = row_start_y + (double)slot * (row_h + row_gap);
            if (event->y >= ry && event->y <= ry + row_h) {
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
    g_app.time_sec += 0.040;
    if (g_app.is_rescanning) {
        g_app.rescan_anim_timer -= 0.040;
        if (g_app.rescan_anim_timer <= 0.0) {
            g_app.is_rescanning = FALSE;
        }
    }
    if (g_app.drawing_area) {
        gtk_widget_queue_draw(g_app.drawing_area);
    }
    return G_SOURCE_CONTINUE;
}

static gboolean on_telemetry_tick(gpointer user_data) {
    (void)user_data;
    update_net_telemetry(&g_app.net);
    update_wifi_scan_telemetry(&g_app.wifi_scan);
    solaris_trim_memory();
    return G_SOURCE_CONTINUE;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    solaris_load_css();

    memset(&g_app, 0, sizeof(g_app));
    g_app.hovered_idx = -1;
    g_app.scroll_offset = 0;
    init_telemetry();
    update_net_telemetry(&g_app.net);
    update_wifi_scan_telemetry(&g_app.wifi_scan);

    g_app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    solaris_setup_layer_window(g_app.window, "solaris-bg-wifi",
                               490, 280,
                               TRUE, FALSE, TRUE, FALSE, // Top-Left anchor
                               44, 0, 16, 0);

    g_app.drawing_area = gtk_drawing_area_new();
    gtk_widget_add_events(g_app.drawing_area, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK);
    g_signal_connect(g_app.drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(g_app.drawing_area, "button-press-event", G_CALLBACK(on_button_press), NULL);
    g_signal_connect(g_app.drawing_area, "motion-notify-event", G_CALLBACK(on_motion_notify), NULL);
    g_signal_connect(g_app.drawing_area, "scroll-event", G_CALLBACK(on_scroll_event), NULL);
    gtk_container_add(GTK_CONTAINER(g_app.window), g_app.drawing_area);

    // 25 FPS globe rotation
    g_app.anim_timer_id = g_timeout_add(40, on_animation_tick, NULL);
    // 1.5s telemetry update
    g_app.telem_timer_id = g_timeout_add(1500, on_telemetry_tick, NULL);

    gtk_widget_show_all(g_app.window);
    gtk_main();
    return 0;
}
