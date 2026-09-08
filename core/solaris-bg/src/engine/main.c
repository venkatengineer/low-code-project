#include "common.h"
#include "telemetry.h"
#include "cairo_utils.h"
#include <signal.h>
#include <math.h>

#define MAX_ENGINE_MONITORS 8
typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    GtkWidget *windows[MAX_ENGINE_MONITORS];
    GtkWidget *drawing_areas[MAX_ENGINE_MONITORS];
    int view_count;
    cpu_telemetry_t cpu;
    mem_telemetry_t mem;
    gpu_telemetry_t gpu;
    disk_telemetry_t disk;
    power_telemetry_t pwr;
    workspace_telemetry_t ws;
    guint timer_id;

    // Interactivity
    double mouse_x;
    double mouse_y;
    int hovered_bay; // 1 to 4
    double turbo_rev; // manual rev boost decay
    double fan_boost;
} engine_app_t;

static engine_app_t g_app;

static void play_sound_async(const char *sound_path) {
    if (!sound_path) return;
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "solaris-play-sound \"%s\"", sound_path);
    g_spawn_command_line_async(cmd, NULL);
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    (void)user_data;

    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    (void)height;

    double pad_x = 6.0;
    double total_w = (double)width - 12.0;
    double bay_gap = 10.0;
    double bay_w = (total_w - (3.0 * bay_gap)) / 4.0;

    double mx = event->x;
    double my = event->y;

    // Check Bay 1: CPU Speedometer
    if (mx >= pad_x && mx < pad_x + bay_w) {
        g_app.turbo_rev = 1.0;
        g_spawn_command_line_async("sh -c 'canberra-gtk-play -i bell 2>/dev/null || true'", NULL);
        return TRUE;
    }

    // Check Bay 2: Memory & Storage
    double b2_x = pad_x + bay_w + bay_gap;
    if (mx >= b2_x && mx < b2_x + bay_w) {
        if (my <= 126.0) {
            g_spawn_command_line_async("solaris-render-mode toggle", NULL);
        } else {
            g_spawn_command_line_async("solaris-explorer", NULL);
        }
        return TRUE;
    }

    // Check Bay 3: GPU & Cooling Fan
    double b3_x = b2_x + bay_w + bay_gap;
    if (mx >= b3_x && mx < b3_x + bay_w) {
        g_app.fan_boost = 1.0;
        return TRUE;
    }

    // Check Bay 4: Battery & Workspaces
    double b4_x = b3_x + bay_w + bay_gap;
    if (mx >= b4_x && mx < b4_x + bay_w) {
        // Check if clicked inside Workspaces list (right side of Bay 4)
        double ws_x = b4_x + (bay_w * 0.54);
        if (mx >= ws_x) {
            for (int i = 0; i < 6; i++) {
                double wy = 6.0 + 48.0 + (double)i * 24.0;
                if (my >= wy && my <= wy + 20.0) {
                    char cmd[64];
                    snprintf(cmd, sizeof(cmd), "niri msg action focus-workspace %d", i + 1);
                    g_spawn_command_line_async(cmd, NULL);
                    return TRUE;
                }
            }
        } else {
            // Clicked on battery core: cycle power profile
            if (g_strcmp0(g_app.pwr.profile, "performance") == 0) {
                g_spawn_command_line_async("powerprofilesctl set power-saver", NULL);
                play_sound_async("cryo_saver");
                g_spawn_command_line_async("notify-send -a 'Solaris Reactor' -i preferences-system-power -h string:x-canonical-private-synchronous:solaris-pwr '❄️ CRYO SAVER' 'Low-Draw Thermal Conservation Active'", NULL);
            } else if (g_strcmp0(g_app.pwr.profile, "power-saver") == 0 || g_strcmp0(g_app.pwr.profile, "powersave") == 0) {
                g_spawn_command_line_async("powerprofilesctl set balanced", NULL);
                play_sound_async("nominal_balanced");
                g_spawn_command_line_async("notify-send -a 'Solaris Reactor' -i preferences-system-power -h string:x-canonical-private-synchronous:solaris-pwr '🛸 ORBITAL BALANCED' 'Nominal Flight Vector & Equilibrium Active'", NULL);
            } else {
                g_spawn_command_line_async("powerprofilesctl set performance", NULL);
                play_sound_async("warp_overclock");
                g_spawn_command_line_async("notify-send -a 'Solaris Reactor' -i preferences-system-power -h string:x-canonical-private-synchronous:solaris-pwr '⚡ WARP OVERCLOCK' 'Maximum Frequency & Performance Governor Active'", NULL);
            }
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    (void)widget;
    (void)user_data;

    // Clear background to transparent
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);

    double pad_x = 6.0;
    double cur_y = 6.0;
    double panel_h = (double)height - 12.0;

    // 4 Symmetrical Instrument Bays across the Dashboard width
    double total_w = (double)width - 12.0;
    double bay_gap = 10.0;
    double bay_w = (total_w - (3.0 * bay_gap)) / 4.0;

    // Compute effective telemetry values with manual interactivity revs
    double effective_cpu = g_app.cpu.usage_pct;
    double effective_gpu = g_app.gpu.usage_pct;
    double effective_fan = g_app.pwr.fan_rpm > 0 ? g_app.pwr.fan_rpm : 3800;

    if (g_app.turbo_rev > 0.001) {
        effective_cpu += g_app.turbo_rev * (100.0 - effective_cpu);
    }
    if (g_app.fan_boost > 0.001) {
        effective_fan = 5800.0;
    }

    // =========================================================================
    // BAY 1: PROPULSION TACHOMETER // CPU & INJECTORS
    // =========================================================================
    double b1_x = pad_x;
    cairo_draw_beveled_panel(cr, b1_x, cur_y, bay_w, panel_h, 8.0);
    cairo_draw_module_header(cr, b1_x, cur_y, bay_w, "PROPULSION // CPU TACHOMETER",
                             COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);

    // Left Speedometer (Sweeps to 100% on awakening!)
    double cpu_cx = b1_x + 76.0;
    double cpu_cy = cur_y + 118.0;
    cairo_draw_sci_fi_speedometer(cr, cpu_cx, cpu_cy, 50.0,
                                  effective_cpu, g_app.cpu.freq_ghz, g_app.pwr.temp_c,
                                  g_app.cpu.model_name, g_app.cpu.plasma_phase);

    // Right Plasma Injectors & Sparkline
    double inj_x = b1_x + 154.0;
    double inj_w = bay_w - 164.0;

    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 8.0);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.85);
    cairo_move_to(cr, inj_x, cur_y + 44.0);
    cairo_show_text(cr, "16-CORE PLASMA INJECTORS:");

    cairo_draw_plasma_injectors(cr, inj_x, cur_y + 54.0, inj_w, 42.0,
                                g_app.cpu.core_count, effective_cpu, g_app.cpu.plasma_phase);

    cairo_set_font_size(cr, 8.0);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.85);
    cairo_move_to(cr, inj_x, cur_y + 120.0);
    cairo_show_text(cr, "PROPULSION HISTORY:");

    cairo_draw_sparkline(cr, inj_x, cur_y + 130.0, inj_w, 60.0,
                         g_app.cpu.history, g_app.cpu.history_count,
                         0.0, 100.0,
                         COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);

    // =========================================================================
    // BAY 2: QUANTUM MEMORY MATRIX & STORAGE ARRAYS
    // =========================================================================
    double b2_x = b1_x + bay_w + bay_gap;
    cairo_draw_beveled_panel(cr, b2_x, cur_y, bay_w, panel_h, 8.0);
    gboolean is_render_on = (access("/tmp/solaris_rendering_mode", F_OK) == 0);
    cairo_draw_module_header(cr, b2_x, cur_y, bay_w,
                             is_render_on ? "QUANTUM BUFFER // RENDER PURGE ACTIVE" : "QUANTUM BUFFER // MEMORY & NVME",
                             is_render_on ? 1.0 : COLOR_VIOLET_R,
                             is_render_on ? 0.25 : COLOR_VIOLET_G,
                             is_render_on ? 0.65 : COLOR_VIOLET_B);

    // Upper: Quantum RAM Core
    cairo_draw_quantum_hex_ram_core(cr, b2_x + 10.0, cur_y + 26.0, bay_w - 20.0, 95.0,
                                    g_app.mem.used_gb, g_app.mem.total_gb, g_app.mem.usage_pct,
                                    g_app.mem.swap_used_gb, g_app.mem.swap_total_gb, g_app.mem.swap_pct,
                                    g_app.mem.flux_phase);

    // Lower: Storage Pods
    cairo_draw_spacecraft_storage_pod(cr, b2_x + 12.0, cur_y + 125.0, bay_w - 24.0, 34.0,
                                      "PRIMARY ROOT (/) // NVME-0", g_app.disk.root_used_gb, g_app.disk.root_total_gb,
                                      g_app.disk.root_pct, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);

    cairo_draw_spacecraft_storage_pod(cr, b2_x + 12.0, cur_y + 168.0, bay_w - 24.0, 34.0,
                                      "SECTOR HOME (/home) // NVME-1", g_app.disk.home_used_gb, g_app.disk.home_total_gb,
                                      g_app.disk.home_pct, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // =========================================================================
    // BAY 3: TENSOR RASTER // GPU HUD & COOLING TURBINE
    // =========================================================================
    double b3_x = b2_x + bay_w + bay_gap;
    cairo_draw_beveled_panel(cr, b3_x, cur_y, bay_w, panel_h, 8.0);
    cairo_draw_module_header(cr, b3_x, cur_y, bay_w, "TENSOR RASTER // GPU & COOLING",
                             COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // Upper: GPU HUD (Sweeps on awakening!)
    cairo_draw_cyber_gpu_tachometer(cr, b3_x + 10.0, cur_y + 26.0, bay_w - 20.0, 85.0,
                                    effective_gpu, g_app.gpu.temp_c,
                                    g_app.gpu.vram_used_gb, g_app.gpu.vram_total_gb,
                                    g_app.gpu.model_name, g_app.gpu.tensor_phase);

    // Lower: Cooling Turbine Fan
    double turb_cx = b3_x + 44.0;
    double turb_cy = cur_y + 164.0;
    cairo_new_path(cr);
    cairo_draw_cooling_turbine(cr, turb_cx, turb_cy, 28.0, (int)effective_fan, g_app.pwr.temp_c, g_app.gpu.tensor_phase);

    double turb_info_x = b3_x + 86.0;
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.9);
    cairo_move_to(cr, turb_info_x, cur_y + 144.0);
    cairo_show_text(cr, "CHASSIS COOLING FAN:");

    char fan_val[32];
    snprintf(fan_val, sizeof(fan_val), "%d RPM", (int)effective_fan);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
    cairo_move_to(cr, b3_x + bay_w - 75.0, cur_y + 144.0);
    cairo_show_text(cr, fan_val);

    cairo_draw_segmented_bar(cr, turb_info_x, cur_y + 152.0, bay_w - 96.0, 5.0,
                             (effective_fan / 5000.0) * 100.0, 14,
                             COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    cairo_set_font_size(cr, 7.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.8);
    cairo_move_to(cr, turb_info_x, cur_y + 178.0);
    cairo_show_text(cr, "EXHAUST // ACTIVE CRYOGENIC VAPOR");

    // =========================================================================
    // BAY 4: AVIONICS FUEL CELL BATTERY & BRIDGE STATIONS
    // =========================================================================
    double b4_x = b3_x + bay_w + bay_gap;
    cairo_draw_beveled_panel(cr, b4_x, cur_y, bay_w, panel_h, 8.0);
    cairo_draw_module_header(cr, b4_x, cur_y, bay_w, "AVIONICS POWER // BATTERY & BRIDGE",
                             COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);

    // Left: Boeing 10-Cell Battery Core
    cairo_draw_aerospace_power_core(cr, b4_x + 8.0, cur_y + 24.0, (bay_w * 0.52), panel_h - 30.0,
                                    g_app.pwr.bat_pct, g_app.pwr.ac_online,
                                    g_app.pwr.bat_status, g_app.pwr.profile,
                                    g_app.gpu.tensor_phase);

    // Right: Niri Bridge Stations (Workspaces 1-6)
    double ws_x = b4_x + (bay_w * 0.54);
    double ws_w = bay_w - (bay_w * 0.54) - 10.0;

    cairo_set_font_size(cr, 8.0);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.85);
    cairo_move_to(cr, ws_x, cur_y + 36.0);
    cairo_show_text(cr, "BRIDGE STATIONS:");

    const char *ws_titles[] = {
        "01 CMD", "02 ENG", "03 NAV",
        "04 COM", "05 SCI", "06 AUX"
    };

    for (int i = 0; i < 6; i++) {
        double wy = cur_y + 48.0 + (double)i * 24.0;
        gboolean is_active = (g_app.ws.active_ws == (i + 1));
        gboolean has_win = g_app.ws.ws_active[i];

        if (is_active) {
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.18);
            cairo_rectangle(cr, ws_x, wy, ws_w, 20.0);
            cairo_fill(cr);

            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.75);
            cairo_set_line_width(cr, 1.0);
            cairo_rectangle(cr, ws_x, wy, ws_w, 20.0);
            cairo_stroke(cr);
        }

        cairo_set_font_size(cr, 8.0);
        cairo_set_source_rgba(cr, is_active ? COLOR_CYAN_R : (has_win ? COLOR_TEXT_BRIGHT_R : COLOR_TEXT_MUTED_R),
                                  is_active ? COLOR_CYAN_G : (has_win ? COLOR_TEXT_BRIGHT_G : COLOR_TEXT_MUTED_G),
                                  is_active ? COLOR_CYAN_B : (has_win ? COLOR_TEXT_BRIGHT_B : COLOR_TEXT_MUTED_B),
                                  0.95);
        cairo_move_to(cr, ws_x + 6.0, wy + 13.0);
        cairo_show_text(cr, ws_titles[i]);

        if (is_active) {
            cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 1.0);
            cairo_arc(cr, ws_x + ws_w - 8.0, wy + 10.0, 3.5, 0, 2 * M_PI);
            cairo_fill(cr);
        } else if (has_win) {
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.8);
            cairo_arc(cr, ws_x + ws_w - 8.0, wy + 10.0, 2.5, 0, 2 * M_PI);
            cairo_stroke(cr);
        }
    }

    // Spacecraft Sequential Power-On Awakening Overlay (Starts at 0.0s)
    solaris_draw_awakening_overlay(cr, pad_x, cur_y, total_w, panel_h, 0.0, 0.8, "PRIMARY WARP CORE");

    return FALSE;
}

static gboolean on_anim_tick(gpointer user_data) {
    (void)user_data;
    g_app.cpu.plasma_phase += 0.08 + (g_app.cpu.usage_pct * 0.002);
    if (g_app.cpu.plasma_phase > 2.0 * M_PI) g_app.cpu.plasma_phase -= 2.0 * M_PI;
    g_app.mem.flux_phase += 0.05;
    if (g_app.mem.flux_phase > 2.0 * M_PI) g_app.mem.flux_phase -= 2.0 * M_PI;
    g_app.gpu.tensor_phase += 0.06;
    if (g_app.gpu.tensor_phase > 2.0 * M_PI) g_app.gpu.tensor_phase -= 2.0 * M_PI;

    // Decay rev & fan boost
    if (g_app.turbo_rev > 0.0) {
        g_app.turbo_rev -= 0.03;
        if (g_app.turbo_rev < 0.0) g_app.turbo_rev = 0.0;
    }
    if (g_app.fan_boost > 0.0) {
        g_app.fan_boost -= 0.02;
        if (g_app.fan_boost < 0.0) g_app.fan_boost = 0.0;
    }

    for (int i = 0; i < g_app.view_count; i++) {
        if (g_app.drawing_areas[i]) {
            gtk_widget_queue_draw(g_app.drawing_areas[i]);
        }
    }
    return G_SOURCE_CONTINUE;
}

static gboolean update_telemetry_tick(gpointer user_data) {
    (void)user_data;
    update_cpu_telemetry(&g_app.cpu);
    update_mem_telemetry(&g_app.mem);
    update_gpu_telemetry(&g_app.gpu);
    update_disk_telemetry(&g_app.disk);
    update_power_telemetry(&g_app.pwr);
    update_workspace_telemetry(&g_app.ws);
    solaris_trim_memory();
    return G_SOURCE_CONTINUE;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    solaris_load_css();

    memset(&g_app, 0, sizeof(g_app));
    init_telemetry();

    // Initial query
    update_cpu_telemetry(&g_app.cpu);
    update_mem_telemetry(&g_app.mem);
    update_gpu_telemetry(&g_app.gpu);
    update_disk_telemetry(&g_app.disk);
    update_power_telemetry(&g_app.pwr);
    update_workspace_telemetry(&g_app.ws);

    GdkDisplay *display = gdk_display_get_default();
    int n_monitors = display ? gdk_display_get_n_monitors(display) : 1;
    g_app.view_count = 0;

    for (int i = 0; i < n_monitors && i < MAX_ENGINE_MONITORS; i++) {
        GdkMonitor *mon = display ? gdk_display_get_monitor(display, i) : NULL;
        GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        solaris_setup_layer_window_for_monitor(win, mon, "solaris-bg-engine",
                                               1674, 220,
                                               FALSE, TRUE, TRUE, TRUE, // Bottom Console Anchor
                                               0, 56, 16, 16);

        GtkWidget *da = gtk_drawing_area_new();
        gtk_widget_add_events(da,
                              GDK_BUTTON_PRESS_MASK |
                              GDK_POINTER_MOTION_MASK |
                              GDK_LEAVE_NOTIFY_MASK);

        g_signal_connect(da, "draw", G_CALLBACK(on_draw), NULL);
        g_signal_connect(da, "button-press-event", G_CALLBACK(on_button_press), NULL);

        gtk_container_add(GTK_CONTAINER(win), da);
        gtk_widget_show_all(win);

        g_app.windows[g_app.view_count] = win;
        g_app.drawing_areas[g_app.view_count] = da;
        g_app.view_count++;
    }

    if (g_app.view_count > 0) {
        g_app.window = g_app.windows[0];
        g_app.drawing_area = g_app.drawing_areas[0];
    }

    // Fast 20 FPS animation tick
    g_timeout_add(50, on_anim_tick, NULL);
    // 1.0s telemetry poll tick
    g_app.timer_id = g_timeout_add(1000, update_telemetry_tick, NULL);

    gtk_main();
    return 0;
}
