#include "common.h"
#include "telemetry.h"
#include "cairo_utils.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    power_telemetry_t pwr;
    workspace_telemetry_t ws;
    double anim_phase;
    guint timer_id;
} power_app_t;

static power_app_t g_app;

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    int width = gtk_widget_get_allocated_width(widget);
    (void)widget;
    (void)user_data;

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);

    double pad_x = 10.0;
    double cur_y = 10.0;
    double panel_w = (double)width - 20.0;

    // -------------------------------------------------------------
    // MODULE 1: AEROSPACE AVIONICS PLASMA FUEL CELL BATTERY CORE
    // -------------------------------------------------------------
    double m1_h = 240.0;
    cairo_draw_beveled_panel(cr, pad_x, cur_y, panel_w, m1_h, 8.0);
    cairo_draw_module_header(cr, pad_x, cur_y, panel_w, "AVIONICS FUEL CELL // BATTERY CORE", COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);

    cairo_draw_aerospace_power_core(cr, pad_x, cur_y + 24.0, panel_w, m1_h - 30.0,
                                    g_app.pwr.bat_pct, g_app.pwr.ac_online,
                                    g_app.pwr.bat_status, g_app.pwr.profile,
                                    g_app.anim_phase);

    cur_y += m1_h + 12.0;

    // -------------------------------------------------------------
    // MODULE 2: THERMAL TURBINE & ENVIRONMENTAL DIAGNOSTICS
    // -------------------------------------------------------------
    double m2_h = 135.0;
    cairo_draw_beveled_panel(cr, pad_x, cur_y, panel_w, m2_h, 8.0);
    cairo_draw_module_header(cr, pad_x, cur_y, panel_w, "COOLING TURBINE // THERMAL", COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // Rotating Turbine Fan
    double turb_cx = pad_x + 36.0;
    double turb_cy = cur_y + 68.0;
    cairo_draw_cooling_turbine(cr, turb_cx, turb_cy, 24.0, g_app.pwr.fan_rpm, g_app.pwr.temp_c, g_app.anim_phase);

    // Thermal metrics on right of turbine
    double th_x = pad_x + 72.0;
    cairo_set_font_size(cr, 9.0);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.9);
    cairo_move_to(cr, th_x, cur_y + 50.0);
    cairo_show_text(cr, "CHASSIS CORE TEMP:");

    char temp_str[32];
    snprintf(temp_str, sizeof(temp_str), "%.1f°C", g_app.pwr.temp_c);
    cairo_set_source_rgba(cr, (g_app.pwr.temp_c < 65) ? COLOR_GREEN_R : COLOR_ALERT_R,
                              (g_app.pwr.temp_c < 65) ? COLOR_GREEN_G : COLOR_ALERT_G,
                              (g_app.pwr.temp_c < 65) ? COLOR_GREEN_B : COLOR_ALERT_B, 1.0);
    cairo_move_to(cr, pad_x + panel_w - 75.0, cur_y + 50.0);
    cairo_show_text(cr, temp_str);

    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.9);
    cairo_move_to(cr, th_x, cur_y + 70.0);
    cairo_show_text(cr, "COOLING TURBINE:");

    char fan_str[32];
    snprintf(fan_str, sizeof(fan_str), "%d RPM", g_app.pwr.fan_rpm);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 1.0);
    cairo_move_to(cr, pad_x + panel_w - 75.0, cur_y + 70.0);
    cairo_show_text(cr, fan_str);

    cairo_draw_segmented_bar(cr, th_x, cur_y + 82.0, panel_w - th_x + pad_x - 14.0, 6.0,
                             100.0 - (g_app.pwr.temp_c * 0.8), 16,
                             COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    cur_y += m2_h + 12.0;

    // -------------------------------------------------------------
    // MODULE 3: NIRI BRIDGE STATIONS (WORKSPACES)
    // -------------------------------------------------------------
    double m3_h = 240.0;
    cairo_draw_beveled_panel(cr, pad_x, cur_y, panel_w, m3_h, 8.0);
    cairo_draw_module_header(cr, pad_x, cur_y, panel_w, "NIRI WORKSPACES // STATIONS", COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    const char *ws_names[] = {
        "01 COMMAND",
        "02 ENGINEERING",
        "03 NAVIGATION",
        "04 COMMS",
        "05 SCIENCE",
        "06 AUXILIARY"
    };

    cairo_set_font_size(cr, 9.5);
    for (int i = 0; i < 6; i++) {
        double row_y = cur_y + 44.0 + i * 28.0;
        gboolean is_active = (g_app.ws.active_ws == (i + 1));
        gboolean has_win = g_app.ws.ws_active[i];

        if (is_active) {
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.15);
            cairo_rectangle(cr, pad_x + 12.0, row_y - 12.0, panel_w - 24.0, 22.0);
            cairo_fill(cr);

            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.6);
            cairo_set_line_width(cr, 1.0);
            cairo_rectangle(cr, pad_x + 12.0, row_y - 12.0, panel_w - 24.0, 22.0);
            cairo_stroke(cr);
        }

        cairo_set_source_rgba(cr, is_active ? COLOR_CYAN_R : (has_win ? COLOR_TEXT_BRIGHT_R : COLOR_TEXT_MUTED_R),
                                  is_active ? COLOR_CYAN_G : (has_win ? COLOR_TEXT_BRIGHT_G : COLOR_TEXT_MUTED_G),
                                  is_active ? COLOR_CYAN_B : (has_win ? COLOR_TEXT_BRIGHT_B : COLOR_TEXT_MUTED_B), 0.95);
        cairo_move_to(cr, pad_x + 20.0, row_y + 3.0);
        cairo_show_text(cr, ws_names[i]);

        if (is_active) {
            cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 1.0);
            cairo_arc(cr, pad_x + panel_w - 24.0, row_y - 1.0, 4.0, 0, 2 * M_PI);
            cairo_fill(cr);
        } else if (has_win) {
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.8);
            cairo_arc(cr, pad_x + panel_w - 24.0, row_y - 1.0, 3.0, 0, 2 * M_PI);
            cairo_stroke(cr);
        } else {
            cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.3);
            cairo_arc(cr, pad_x + panel_w - 24.0, row_y - 1.0, 2.0, 0, 2 * M_PI);
            cairo_stroke(cr);
        }
    }

    return FALSE;
}

static gboolean on_anim_tick(gpointer user_data) {
    (void)user_data;
    g_app.anim_phase += 0.08;
    if (g_app.anim_phase > 2.0 * M_PI) g_app.anim_phase -= 2.0 * M_PI;

    if (g_app.drawing_area) {
        gtk_widget_queue_draw(g_app.drawing_area);
    }
    return G_SOURCE_CONTINUE;
}

static gboolean update_telemetry_tick(gpointer user_data) {
    (void)user_data;
    update_power_telemetry(&g_app.pwr);
    update_workspace_telemetry(&g_app.ws);
    return G_SOURCE_CONTINUE;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    solaris_load_css();

    memset(&g_app, 0, sizeof(g_app));
    init_telemetry();

    update_power_telemetry(&g_app.pwr);
    update_workspace_telemetry(&g_app.ws);

    g_app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    solaris_setup_layer_window(g_app.window, "solaris-bg-power",
                               330, 680,
                               TRUE, FALSE, FALSE, TRUE, // Top-Right anchor
                               50, 0, 0, 16);

    g_app.drawing_area = gtk_drawing_area_new();
    g_signal_connect(g_app.drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    gtk_container_add(GTK_CONTAINER(g_app.window), g_app.drawing_area);

    // Fast 20 FPS turbine/plasma animation
    g_timeout_add(50, on_anim_tick, NULL);
    // 1.0s telemetry update
    g_app.timer_id = g_timeout_add(1000, update_telemetry_tick, NULL);

    gtk_widget_show_all(g_app.window);
    gtk_main();
    return 0;
}
