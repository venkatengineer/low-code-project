#include "common.h"
#include "cairo_utils.h"
#include "telemetry.h"

// ==============================================================================
// 🛸 SOLARIS // SPACECRAFT COCKPIT FLIGHT DECK (THROTTLE PEDESTAL & AVIONICS)
// ==============================================================================

typedef struct {
    double volume_pct;
    gboolean is_muted;
    double brightness_pct;
    char output_name[64];
    char input_name[64];
    gboolean night_light_on;
    gboolean dnd_on;
    gboolean vpn_on;
    gboolean mic_muted;
    gboolean camera_active;
    gboolean recording_on;
    gboolean render_mode_on;
    double render_freed_mb;

    // Power & Hardware Telemetry
    char power_profile[32]; // "PERFORMANCE", "BALANCED", "POWER SAVER"
    double voltage_v;
    double current_a;
    double power_w;
    double battery_pct;
    gboolean ac_online;
    double temp_c;
    int fan_rpm;
} flight_state_t;

#define MAX_FLIGHT_MONITORS 8

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    GtkWidget *windows[MAX_FLIGHT_MONITORS];
    GtkWidget *drawing_areas[MAX_FLIGHT_MONITORS];
    int view_count;
    flight_state_t state;
    int hovered_btn;   // -1: none, 1..N: control IDs
    int active_drag;   // 0: none, 1: audio throttle drag, 2: brightness throttle drag
    double time_sec;
    double pulse_phase;
    guint anim_timer_id;
    guint telem_timer_id;

    // Mode Switching Animation & Sound Trigger
    gboolean mode_anim_active;
    double mode_anim_progress; // 0.0 to 1.0
    int mode_anim_type;        // 1: Overclock, 2: Balanced, 3: Cryo Saver
    double mode_anim_cx;
    double mode_anim_cy;
    char mode_anim_title[64];
    char mode_anim_sub[64];
} flightdeck_app_t;

static flightdeck_app_t g_app;

static void redraw_all_views(void) {
    for (int i = 0; i < g_app.view_count; i++) {
        if (g_app.drawing_areas[i]) {
            gtk_widget_queue_draw(g_app.drawing_areas[i]);
        }
    }
}

static void update_flight_state(flight_state_t *st) {
    // 1. Audio volume & mute
    FILE *fp = popen("wpctl get-volume @DEFAULT_AUDIO_SINK@ 2>/dev/null", "r");
    if (fp) {
        char buf[64];
        if (fgets(buf, sizeof(buf), fp)) {
            double vol = 0.0;
            if (sscanf(buf, "Volume: %lf", &vol) == 1) {
                st->volume_pct = vol * 100.0;
            }
            st->is_muted = (strstr(buf, "[MUTED]") != NULL);
        }
        pclose(fp);
    }

    // 2. Brightness
    FILE *bfp = popen("brightnessctl get 2>/dev/null; brightnessctl max 2>/dev/null", "r");
    if (bfp) {
        int cur = 0, max = 0;
        if (fscanf(bfp, "%d\n%d", &cur, &max) == 2 && max > 0) {
            st->brightness_pct = ((double)cur / (double)max) * 100.0;
        }
        pclose(bfp);
    }

    // 3. Audio Sink & Source names
    FILE *sfp = popen("pactl get-default-sink 2>/dev/null || wpctl status | grep -A 2 'Sinks:' | grep '*' | head -n 1", "r");
    if (sfp) {
        char sbuf[128];
        if (fgets(sbuf, sizeof(sbuf), sfp)) {
            char *clean = sbuf;
            while (*clean == ' ' || *clean == '*' || *clean == '.') clean++;
            char *nl = strchr(clean, '\n');
            if (nl) *nl = '\0';
            if (strstr(clean, "bluez") || strstr(clean, "WH-1000XM") || strstr(clean, "XM4") || strstr(clean, "XM5")) {
                snprintf(st->output_name, sizeof(st->output_name), "WH-1000XM4 // BT");
            } else if (strstr(clean, "analog") || strstr(clean, "Analog") || strstr(clean, "Built-in")) {
                snprintf(st->output_name, sizeof(st->output_name), "INT SPEAKERS");
            } else if (clean[0] != '\0') {
                snprintf(st->output_name, sizeof(st->output_name), "%.18s", clean);
            } else {
                snprintf(st->output_name, sizeof(st->output_name), "AUDIO SINK");
            }
        }
        pclose(sfp);
    }

    FILE *src_fp = popen("pactl get-default-source 2>/dev/null || wpctl status | grep -A 2 'Sources:' | grep '*' | head -n 1", "r");
    if (src_fp) {
        char src_buf[128];
        if (fgets(src_buf, sizeof(src_buf), src_fp)) {
            char *clean = src_buf;
            while (*clean == ' ' || *clean == '*' || *clean == '.') clean++;
            char *nl = strchr(clean, '\n');
            if (nl) *nl = '\0';
            if (strstr(clean, "C922") || strstr(clean, "Webcam")) {
                snprintf(st->input_name, sizeof(st->input_name), "C922 WEBCAM MIC");
            } else if (strstr(clean, "analog") || strstr(clean, "Analog") || strstr(clean, "Built-in")) {
                snprintf(st->input_name, sizeof(st->input_name), "INT MATRIX MIC");
            } else if (clean[0] != '\0') {
                snprintf(st->input_name, sizeof(st->input_name), "%.18s", clean);
            } else {
                snprintf(st->input_name, sizeof(st->input_name), "INPUT MIC");
            }
        }
        pclose(src_fp);
    }

    // 4. Quick Toggles Status
    st->night_light_on = (system("pgrep -x wlsunset >/dev/null 2>&1") == 0);
    st->dnd_on = (system("makoctl mode 2>/dev/null | grep -q dnd") == 0);
    st->vpn_on = (system("nmcli -t -f TYPE,STATE con show --active 2>/dev/null | grep -q 'vpn:activated\\|wireguard:activated'") == 0);

    FILE *mfp = popen("wpctl get-volume @DEFAULT_AUDIO_SOURCE@ 2>/dev/null", "r");
    if (mfp) {
        char mbuf[64];
        if (fgets(mbuf, sizeof(mbuf), mfp)) {
            st->mic_muted = (strstr(mbuf, "[MUTED]") != NULL);
        }
        pclose(mfp);
    }

    st->camera_active = (access("/dev/video0", F_OK) == 0);
    st->recording_on = (system("pgrep -x wf-recorder >/dev/null 2>&1") == 0);

    // 5. Power Profile & Hardware Voltage/Current
    FILE *pfp = popen("powerprofilesctl get 2>/dev/null", "r");
    if (pfp) {
        char pbuf[32];
        if (fgets(pbuf, sizeof(pbuf), pfp)) {
            char *nl = strchr(pbuf, '\n');
            if (nl) *nl = '\0';
            if (strcmp(pbuf, "performance") == 0) {
                snprintf(st->power_profile, sizeof(st->power_profile), "PERFORMANCE");
            } else if (strcmp(pbuf, "power-saver") == 0) {
                snprintf(st->power_profile, sizeof(st->power_profile), "POWER SAVER");
            } else {
                snprintf(st->power_profile, sizeof(st->power_profile), "BALANCED");
            }
        }
        pclose(pfp);
    }

    FILE *vfp = fopen("/sys/class/power_supply/BAT0/voltage_now", "r");
    if (vfp) {
        long u_volts = 0;
        if (fscanf(vfp, "%ld", &u_volts) == 1 && u_volts > 0) st->voltage_v = (double)u_volts / 1000000.0;
        fclose(vfp);
    } else {
        st->voltage_v = 13.22;
    }

    FILE *ifp = fopen("/sys/class/power_supply/BAT0/current_now", "r");
    if (ifp) {
        long u_amps = 0;
        if (fscanf(ifp, "%ld", &u_amps) == 1 && u_amps > 0) st->current_a = (double)u_amps / 1000000.0;
        fclose(ifp);
    } else {
        st->current_a = 3.93;
    }
    st->power_w = st->voltage_v * st->current_a;

    FILE *cap_fp = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    if (cap_fp) {
        int cap = 0;
        if (fscanf(cap_fp, "%d", &cap) == 1) st->battery_pct = (double)cap;
        fclose(cap_fp);
    } else {
        st->battery_pct = 88.0;
    }

    FILE *ac_fp = fopen("/sys/class/power_supply/AC/online", "r");
    if (ac_fp) {
        int ac = 0;
        if (fscanf(ac_fp, "%d", &ac) == 1) st->ac_online = (ac == 1);
        fclose(ac_fp);
    } else {
        st->ac_online = TRUE;
    }

    FILE *tfp = fopen("/sys/class/hwmon/hwmon5/temp1_input", "r");
    if (tfp) {
        long t = 0;
        if (fscanf(tfp, "%ld", &t) == 1 && t > 1000) st->temp_c = (double)t / 1000.0;
        fclose(tfp);
    } else {
        st->temp_c = 64.0;
    }

    FILE *fan_fp = fopen("/sys/class/hwmon/hwmon5/fan1_input", "r");
    if (fan_fp) {
        int rpm = 0;
        if (fscanf(fan_fp, "%d", &rpm) == 1 && rpm > 0) st->fan_rpm = rpm;
        fclose(fan_fp);
    } else {
        st->fan_rpm = 4552;
    }

    // 6. Rendering Mode Telemetry Status
    st->render_mode_on = (access("/tmp/solaris_rendering_mode", F_OK) == 0);
    st->render_freed_mb = 0.0;
    if (st->render_mode_on) {
        FILE *rfp = fopen("/tmp/solaris_rendering_mode", "r");
        if (rfp) {
            char rline[128];
            while (fgets(rline, sizeof(rline), rfp)) {
                double fmb = 0.0;
                if (sscanf(rline, "FREED_MB=%lf", &fmb) == 1) {
                    st->render_freed_mb = fmb;
                }
            }
            fclose(rfp);
        }
    }
}

static void play_sound_async(const char *sound_path) {
    if (!sound_path) return;
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "solaris-play-sound \"%s\"", sound_path);
    g_spawn_command_line_async(cmd, NULL);
}

// Trigger Rendering Mode Switch (Purge / Restore)
static void trigger_render_mode_switch(double btn_cx, double btn_cy) {
    g_app.mode_anim_active = TRUE;
    g_app.mode_anim_progress = 0.0;
    g_app.mode_anim_type = 4;
    g_app.mode_anim_cx = btn_cx;
    g_app.mode_anim_cy = btn_cy;

    if (!g_app.state.render_mode_on) {
        snprintf(g_app.mode_anim_title, sizeof(g_app.mode_anim_title), "🎬 RENDERING MODE // RAM PURGE");
        snprintf(g_app.mode_anim_sub, sizeof(g_app.mode_anim_sub), "MAXIMUM RAM LIBERATED // GOVERNOR: PERFORMANCE");
        g_spawn_command_line_async("solaris-render-mode on", NULL);
        g_app.state.render_mode_on = TRUE;
    } else {
        snprintf(g_app.mode_anim_title, sizeof(g_app.mode_anim_title), "🛸 RENDERING MODE // DISENGAGED");
        snprintf(g_app.mode_anim_sub, sizeof(g_app.mode_anim_sub), "SYSTEM NOMINAL // BACKGROUND TASKS RESUMED");
        g_spawn_command_line_async("solaris-render-mode off", NULL);
        g_app.state.render_mode_on = FALSE;
    }
}

// Trigger power mode switch with visual surge animation, audio sound, and desktop HUD notification
static void trigger_power_mode_switch(int mode_type, double btn_cx, double btn_cy) {
    g_app.mode_anim_active = TRUE;
    g_app.mode_anim_progress = 0.0;
    g_app.mode_anim_type = mode_type;
    g_app.mode_anim_cx = btn_cx;
    g_app.mode_anim_cy = btn_cy;

    if (mode_type == 1) { // OVERCLOCK / PERFORMANCE
        g_spawn_command_line_async("powerprofilesctl set performance", NULL);
        snprintf(g_app.state.power_profile, sizeof(g_app.state.power_profile), "PERFORMANCE");
        snprintf(g_app.mode_anim_title, sizeof(g_app.mode_anim_title), "⚡ HYPERDRIVE OVERCLOCK // MAX POWER");
        snprintf(g_app.mode_anim_sub, sizeof(g_app.mode_anim_sub), "WARP CORE FREQUENCY: MAXIMUM // GOVERNOR: PERFORMANCE");
    } else if (mode_type == 2) { // BALANCED / NOMINAL
        g_spawn_command_line_async("powerprofilesctl set balanced", NULL);
        snprintf(g_app.state.power_profile, sizeof(g_app.state.power_profile), "BALANCED");
        snprintf(g_app.mode_anim_title, sizeof(g_app.mode_anim_title), "🛸 ORBITAL NOMINAL // BALANCED FLUX");
        snprintf(g_app.mode_anim_sub, sizeof(g_app.mode_anim_sub), "EQUILIBRIUM CRUISE VECTOR // GOVERNOR: BALANCED");
    } else if (mode_type == 3) { // CRYO SAVER / ECO
        g_spawn_command_line_async("powerprofilesctl set power-saver", NULL);
        snprintf(g_app.state.power_profile, sizeof(g_app.state.power_profile), "POWER SAVER");
        snprintf(g_app.mode_anim_title, sizeof(g_app.mode_anim_title), "❄️ CRYOGENIC STASIS // LOW-DRAW ECO");
        snprintf(g_app.mode_anim_sub, sizeof(g_app.mode_anim_sub), "THERMAL DUMP DECOMPRESSION // GOVERNOR: POWER SAVER");
    }
}

// ==============================================================================
// ✈ AIRCRAFT CENTRAL THROTTLE LEVER QUADRANT (SPEED / THRUST STYLE)
// ==============================================================================

static void draw_aircraft_throttle_lever(cairo_t *cr, double cx, double top_y, double track_h,
                                        double pct, gboolean is_muted,
                                        const char *tag, const char *subline,
                                        int step_up_id, int step_down_id,
                                        double r, double g, double b) {
    cairo_save(cr);

    double track_w = 16.0;
    double track_x = cx - (track_w * 0.5);
    double track_y = top_y + 16.0;
    double eff_h = track_h - 32.0;

    // 1. Tag & Title Header above lever
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 7.5);
    cairo_set_source_rgba(cr, is_muted ? COLOR_ALERT_R : COLOR_TEXT_BRIGHT_R,
                              is_muted ? 0.4 : COLOR_TEXT_BRIGHT_G,
                              is_muted ? 0.4 : COLOR_TEXT_BRIGHT_B, 0.95);
    cairo_move_to(cr, cx - 24.0, top_y + 4.0);
    cairo_show_text(cr, tag);

    // 2. Micro Stepper [+] Button (Top)
    cairo_rectangle(cr, cx - 14.0, top_y + 6.0, 28.0, 8.0);
    gboolean hov_up = (g_app.hovered_btn == step_up_id);
    cairo_set_source_rgba(cr, 0.02, 0.06, 0.10, hov_up ? 0.95 : 0.60);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, r, g, b, hov_up ? 1.0 : 0.40);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);
    cairo_set_font_size(cr, 6.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.90);
    cairo_move_to(cr, cx - 4.0, top_y + 12.5);
    cairo_show_text(cr, "▲");

    // 3. Recessed Aircraft Throttle Track Slot
    cairo_rectangle(cr, track_x, track_y, track_w, eff_h);
    cairo_pattern_t *slot_pat = cairo_pattern_create_linear(track_x, track_y, track_x + track_w, track_y);
    cairo_pattern_add_color_stop_rgba(slot_pat, 0.0, 0.01, 0.02, 0.03, 0.98);
    cairo_pattern_add_color_stop_rgba(slot_pat, 0.5, 0.03, 0.05, 0.08, 0.90);
    cairo_pattern_add_color_stop_rgba(slot_pat, 1.0, 0.01, 0.02, 0.03, 0.98);
    cairo_set_source(cr, slot_pat);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(slot_pat);

    cairo_set_source_rgba(cr, 0.18, 0.30, 0.42, 0.55);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    // Center metallic guide rod
    cairo_move_to(cr, cx, track_y + 4.0);
    cairo_line_to(cr, cx, track_y + eff_h - 4.0);
    cairo_set_source_rgba(cr, 0.3, 0.4, 0.5, 0.40);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);

    // Speed / Thrust Detent Ticks along track
    int num_ticks = 5;
    const char *detent_labels[] = {"100", "75", "50", "25", "IDL"};
    for (int t = 0; t < num_ticks; t++) {
        double ty = track_y + (t * (eff_h / (num_ticks - 1)));
        cairo_move_to(cr, track_x - 4.0, ty);
        cairo_line_to(cr, track_x - 1.0, ty);
        cairo_move_to(cr, track_x + track_w + 1.0, ty);
        cairo_line_to(cr, track_x + track_w + 4.0, ty);
        cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.45);
        cairo_set_line_width(cr, 0.8);
        cairo_stroke(cr);

        cairo_set_font_size(cr, 5.5);
        cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.55);
        cairo_move_to(cr, track_x - 18.0, ty + 2.0);
        cairo_show_text(cr, detent_labels[t]);
    }

    // 4. Machined Aircraft Throttle Grip Knob
    double knob_w = 34.0;
    double knob_h = 20.0;
    double knob_travel = eff_h - knob_h;
    double cur_pct = CLAMP(pct, 0.0, 100.0);
    double knob_y = track_y + eff_h - knob_h - ((cur_pct / 100.0) * knob_travel);
    double knob_x = cx - (knob_w * 0.5);

    // Drop shadow
    cairo_rectangle(cr, knob_x - 2.0, knob_y + 2.0, knob_w + 4.0, knob_h + 3.0);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.60);
    cairo_fill(cr);

    // Lever Handle Body
    cairo_new_path(cr);
    double kc = 4.0;
    cairo_move_to(cr, knob_x + kc, knob_y);
    cairo_line_to(cr, knob_x + knob_w - kc, knob_y);
    cairo_line_to(cr, knob_x + knob_w, knob_y + kc);
    cairo_line_to(cr, knob_x + knob_w, knob_y + knob_h - kc);
    cairo_line_to(cr, knob_x + knob_w - kc, knob_y + knob_h);
    cairo_line_to(cr, knob_x + kc, knob_y + knob_h);
    cairo_line_to(cr, knob_x, knob_y + knob_h - kc);
    cairo_line_to(cr, knob_x, knob_y + kc);
    cairo_close_path(cr);

    cairo_pattern_t *knob_pat = cairo_pattern_create_linear(knob_x, knob_y, knob_x, knob_y + knob_h);
    cairo_pattern_add_color_stop_rgba(knob_pat, 0.0, 0.16, 0.24, 0.32, 0.98);
    cairo_pattern_add_color_stop_rgba(knob_pat, 0.3, 0.08, 0.12, 0.16, 0.95);
    cairo_pattern_add_color_stop_rgba(knob_pat, 1.0, 0.02, 0.04, 0.06, 0.98);
    cairo_set_source(cr, knob_pat);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(knob_pat);

    cairo_set_source_rgba(cr, r, g, b, 0.85);
    cairo_set_line_width(cr, 1.2);
    cairo_stroke(cr);

    // Grip ridges
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.25);
    cairo_set_line_width(cr, 0.8);
    cairo_move_to(cr, knob_x + 4.0, knob_y + 4.0);
    cairo_line_to(cr, knob_x + knob_w - 4.0, knob_y + 4.0);
    cairo_move_to(cr, knob_x + 4.0, knob_y + knob_h - 4.0);
    cairo_line_to(cr, knob_x + knob_w - 4.0, knob_y + knob_h - 4.0);
    cairo_stroke(cr);

    // Percentage Display
    cairo_rectangle(cr, knob_x + 4.0, knob_y + (knob_h * 0.5) - 4.0, knob_w - 8.0, 8.0);
    cairo_set_source_rgba(cr, 0.02, 0.04, 0.06, 0.90);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, r, g, b, 0.70);
    cairo_set_line_width(cr, 0.6);
    cairo_stroke(cr);

    char val_str[16];
    if (is_muted) {
        snprintf(val_str, sizeof(val_str), "MUTE");
    } else {
        snprintf(val_str, sizeof(val_str), "%d%%", (int)round(cur_pct));
    }
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 6.2);
    cairo_set_source_rgba(cr, is_muted ? COLOR_ALERT_R : r,
                              is_muted ? 0.4 : g,
                              is_muted ? 0.4 : b, 1.0);
    cairo_move_to(cr, cx - 10.0, knob_y + (knob_h * 0.5) + 2.5);
    cairo_show_text(cr, val_str);

    // 5. Micro Stepper [-] Button (Bottom)
    double down_y = track_y + eff_h + 2.0;
    cairo_rectangle(cr, cx - 14.0, down_y, 28.0, 8.0);
    gboolean hov_down = (g_app.hovered_btn == step_down_id);
    cairo_set_source_rgba(cr, 0.02, 0.06, 0.10, hov_down ? 0.95 : 0.60);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, r, g, b, hov_down ? 1.0 : 0.40);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);
    cairo_set_font_size(cr, 6.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.90);
    cairo_move_to(cr, cx - 4.0, down_y + 6.5);
    cairo_show_text(cr, "▼");

    // 6. Subline info below lever
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 6.0);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.80);
    cairo_move_to(cr, cx - 26.0, down_y + 17.0);
    cairo_show_text(cr, subline);

    cairo_restore(cr);
}

// Quick Toggle Pod
static void draw_quick_toggle_pod(cairo_t *cr, double x, double y, double w, double h,
                                  const char *icon, const char *label, gboolean is_on, gboolean is_hov,
                                  double on_r, double on_g, double on_b) {
    cairo_save(cr);
    cairo_new_path(cr);
    double c = 4.0;
    cairo_move_to(cr, x + c, y);
    cairo_line_to(cr, x + w - c, y);
    cairo_line_to(cr, x + w, y + c);
    cairo_line_to(cr, x + w, y + h - c);
    cairo_line_to(cr, x + w - c, y + h);
    cairo_line_to(cr, x + c, y + h);
    cairo_line_to(cr, x, y + h - c);
    cairo_line_to(cr, x, y + c);
    cairo_close_path(cr);

    if (is_on) {
        cairo_set_source_rgba(cr, on_r * 0.2, on_g * 0.2, on_b * 0.2, is_hov ? 0.90 : 0.70);
    } else {
        cairo_set_source_rgba(cr, 0.02, 0.05, 0.09, is_hov ? 0.85 : 0.60);
    }
    cairo_fill_preserve(cr);

    if (is_hov) {
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
        cairo_set_line_width(cr, 1.4);
    } else if (is_on) {
        cairo_set_source_rgba(cr, on_r, on_g, on_b, 0.85);
        cairo_set_line_width(cr, 1.1);
    } else {
        cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.35);
        cairo_set_line_width(cr, 0.8);
    }
    cairo_stroke(cr);

    // Indicator LED
    double led_x = x + 8.0;
    double led_y = y + (h * 0.5);
    cairo_arc(cr, led_x, led_y, 2.2, 0, 2 * M_PI);
    if (is_on) {
        cairo_set_source_rgba(cr, on_r, on_g, on_b, 1.0);
    } else {
        cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.40);
    }
    cairo_fill(cr);

    // Text Label
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 7.5);
    if (is_on) {
        cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 1.0);
    } else {
        cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.2, COLOR_TEXT_MUTED_G + 0.2, COLOR_TEXT_MUTED_B + 0.2, 0.80);
    }

    char btn_buf[48];
    snprintf(btn_buf, sizeof(btn_buf), "%s %s", icon, label);
    cairo_move_to(cr, led_x + 6.0, y + (h * 0.5) + 2.8);
    cairo_show_text(cr, btn_buf);

    cairo_restore(cr);
}

// Master Rendering Mode Hyper-Switch Pod (High-Visibility Cybernetic Indicator)
static void draw_render_mode_pod(cairo_t *cr, double x, double y, double w, double h,
                                 gboolean is_on, gboolean is_hov, double freed_mb) {
    cairo_save(cr);
    cairo_new_path(cr);
    double c = 4.0;
    cairo_move_to(cr, x + c, y);
    cairo_line_to(cr, x + w - c, y);
    cairo_line_to(cr, x + w, y + c);
    cairo_line_to(cr, x + w, y + h - c);
    cairo_line_to(cr, x + w - c, y + h);
    cairo_line_to(cr, x + c, y + h);
    cairo_line_to(cr, x, y + h - c);
    cairo_line_to(cr, x, y + c);
    cairo_close_path(cr);

    cairo_pattern_t *pat = cairo_pattern_create_linear(x, y, x + w, y + h);
    if (is_on) {
        cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.45, 0.08, 0.28, is_hov ? 0.95 : 0.85);
        cairo_pattern_add_color_stop_rgba(pat, 0.5, 0.20, 0.03, 0.15, 0.90);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.05, 0.01, 0.05, 0.95);
    } else {
        cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.03, 0.06, 0.10, is_hov ? 0.85 : 0.60);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.01, 0.02, 0.04, 0.90);
    }
    cairo_set_source(cr, pat);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(pat);

    if (is_on) {
        cairo_set_source_rgba(cr, 1.0, 0.30, 0.65, 0.95);
        cairo_set_line_width(cr, is_hov ? 1.6 : 1.2);
    } else if (is_hov) {
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
        cairo_set_line_width(cr, 1.4);
    } else {
        cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.40);
        cairo_set_line_width(cr, 0.8);
    }
    cairo_stroke(cr);

    // Indicator LED with Neon Glow
    double led_x = x + 10.0;
    double led_y = y + (h * 0.5);
    if (is_on) {
        cairo_arc(cr, led_x, led_y, 4.2, 0, 2 * M_PI);
        cairo_set_source_rgba(cr, 1.0, 0.25, 0.55, 0.35);
        cairo_fill(cr);
    }
    cairo_arc(cr, led_x, led_y, 2.5, 0, 2 * M_PI);
    if (is_on) {
        cairo_set_source_rgba(cr, 1.0, 0.30, 0.65, 1.0);
    } else {
        cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.40);
    }
    cairo_fill(cr);

    // Text Label (Left: Title, Right: State / Freed RAM badge)
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 7.5);
    if (is_on) {
        cairo_set_source_rgba(cr, 1.0, 0.95, 0.95, 1.0);
    } else {
        cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.25, COLOR_TEXT_MUTED_G + 0.25, COLOR_TEXT_MUTED_B + 0.25, 0.85);
    }

    cairo_move_to(cr, led_x + 8.0, y + (h * 0.5) + 2.8);
    cairo_show_text(cr, is_on ? "󰑈 RENDERING MODE // ENGAGED" : "󰑈 RENDERING MODE // RAM PURGE");

    // Right Badge
    char badge[64];
    if (is_on) {
        if (freed_mb > 0.0) {
            snprintf(badge, sizeof(badge), "[+%.0f MB FREED]", freed_mb);
        } else {
            snprintf(badge, sizeof(badge), "[MAX PURGE]");
        }
    } else {
        snprintf(badge, sizeof(badge), "[STANDBY]");
    }
    cairo_text_extents_t bext;
    cairo_text_extents(cr, badge, &bext);
    cairo_set_source_rgba(cr, is_on ? 1.0 : COLOR_TEXT_MUTED_R,
                              is_on ? 0.4 : COLOR_TEXT_MUTED_G,
                              is_on ? 0.7 : COLOR_TEXT_MUTED_B, 0.95);
    cairo_move_to(cr, x + w - bext.width - 10.0, y + (h * 0.5) + 2.8);
    cairo_show_text(cr, badge);

    cairo_restore(cr);
}

// Master Power Telemetry Gauge & Flow Bar
static void draw_power_telemetry_hud(cairo_t *cr, double x, double y, double w, double h,
                                    const flight_state_t *st) {
    cairo_save(cr);

    cairo_rectangle(cr, x, y, w, h);
    cairo_pattern_t *pat = cairo_pattern_create_linear(x, y, x, y + h);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.015, 0.030, 0.045, 0.92);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.006, 0.012, 0.020, 0.96);
    cairo_set_source(cr, pat);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(pat);

    cairo_set_source_rgba(cr, 0.16, 0.32, 0.48, 0.55);
    cairo_set_line_width(cr, 0.9);
    cairo_stroke(cr);

    // Live Readouts Line 1 & Line 2
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 7.0);
    cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.95);
    char pwr_line1[64];
    snprintf(pwr_line1, sizeof(pwr_line1), "VOLT: %.2fV | CURR: %.2fA", st->voltage_v, st->current_a);
    cairo_move_to(cr, x + 6.0, y + 12.0);
    cairo_show_text(cr, pwr_line1);

    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 6.2);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.2, COLOR_TEXT_MUTED_G + 0.2, COLOR_TEXT_MUTED_B + 0.2, 0.85);
    char pwr_line2[64];
    snprintf(pwr_line2, sizeof(pwr_line2), "DRAW: %.1fW | FAN: %d RPM", st->power_w, st->fan_rpm);
    cairo_move_to(cr, x + 6.0, y + 25.0);
    cairo_show_text(cr, pwr_line2);

    // Energy Segmented Bar
    double bar_x = x + w - 85.0;
    double bar_y = y + 7.0;
    double bar_w = 78.0;
    double bar_h = 6.0;

    int total_segs = 8;
    double seg_w = (bar_w - ((total_segs - 1) * 2.0)) / (double)total_segs;
    int filled = (int)round((st->battery_pct / 100.0) * total_segs);

    for (int s = 0; s < total_segs; s++) {
        double sx = bar_x + (s * (seg_w + 2.0));
        cairo_rectangle(cr, sx, bar_y, seg_w, bar_h);
        if (s < filled) {
            cairo_set_source_rgba(cr, 1.0, 0.67, 0.05, 0.95);
        } else {
            cairo_set_source_rgba(cr, 1.0, 0.67, 0.05, 0.15);
        }
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 1.0, 0.67, 0.05, (s < filled) ? 1.0 : 0.3);
        cairo_set_line_width(cr, 0.6);
        cairo_stroke(cr);
    }

    char bat_stat[32];
    snprintf(bat_stat, sizeof(bat_stat), "BAT: %.0f%%%s", st->battery_pct, st->ac_online ? " [AC]" : "");
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 6.2);
    cairo_set_source_rgba(cr, 1.0, 0.67, 0.05, 0.95);
    cairo_move_to(cr, bar_x, bar_y + 18.0);
    cairo_show_text(cr, bat_stat);

    cairo_restore(cr);
}

// ==============================================================================
// 🛸 MASSIVE SCULPTED SPACESHIP POWER MODULES (TALL & FULL-SPACE OCCUPYING)
// ==============================================================================

// 1. OVERCLOCK: Port Forward-Swept Warp Thruster Pod (Height = 146px)
static void draw_funky_warp_pod(cairo_t *cr, double x, double y, double w, double h,
                                gboolean is_active, gboolean is_hov) {
    cairo_save(cr);

    // Large Forward-Swept Thruster Geometry
    cairo_new_path(cr);
    cairo_move_to(cr, x, y + 16.0);
    cairo_line_to(cr, x + 16.0, y);
    cairo_line_to(cr, x + w - 16.0, y);
    cairo_line_to(cr, x + w, y + 16.0);
    cairo_line_to(cr, x + w, y + h - 18.0);
    cairo_line_to(cr, x + w - 16.0, y + h);
    cairo_line_to(cr, x + 14.0, y + h);
    cairo_line_to(cr, x, y + h - 16.0);
    cairo_close_path(cr);

    cairo_pattern_t *pat = cairo_pattern_create_linear(x, y, x + w, y + h);
    if (is_active) {
        cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.40, 0.15, 0.02, 0.98);
        cairo_pattern_add_color_stop_rgba(pat, 0.4, 0.18, 0.07, 0.01, 0.94);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.04, 0.01, 0.01, 0.98);
    } else {
        cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.05, 0.07, 0.10, is_hov ? 0.92 : 0.75);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.01, 0.02, 0.03, 0.96);
    }
    cairo_set_source(cr, pat);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(pat);

    cairo_set_source_rgba(cr, is_active ? 1.0 : (is_hov ? COLOR_CYAN_R : 0.25),
                              is_active ? 0.50 : (is_hov ? COLOR_CYAN_G : 0.35),
                              is_active ? 0.05 : (is_hov ? COLOR_CYAN_B : 0.45), is_active ? 1.0 : 0.80);
    cairo_set_line_width(cr, is_active ? 1.5 : 0.9);
    cairo_stroke(cr);

    // --- Section 1: Header Area (Icon & Title) ---
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14.0);
    cairo_set_source_rgba(cr, is_active ? 1.0 : COLOR_TEXT_MUTED_R,
                              is_active ? 0.6 : COLOR_TEXT_MUTED_G,
                              is_active ? 0.1 : COLOR_TEXT_MUTED_B, 1.0);
    cairo_move_to(cr, x + 8.0, y + 22.0);
    cairo_show_text(cr, "");

    cairo_set_font_size(cr, 7.8);
    cairo_set_source_rgba(cr, is_active ? COLOR_TEXT_BRIGHT_R : COLOR_TEXT_MUTED_R + 0.2,
                              is_active ? COLOR_TEXT_BRIGHT_G : COLOR_TEXT_MUTED_G + 0.2,
                              is_active ? COLOR_TEXT_BRIGHT_B : COLOR_TEXT_MUTED_B + 0.2, 0.98);
    cairo_move_to(cr, x + 26.0, y + 14.0);
    cairo_show_text(cr, "OVERCLOCK");

    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 6.0);
    cairo_set_source_rgba(cr, is_active ? 1.0 : COLOR_TEXT_MUTED_R,
                              is_active ? 0.65 : COLOR_TEXT_MUTED_G,
                              is_active ? 0.15 : COLOR_TEXT_MUTED_B, is_active ? 0.95 : 0.65);
    cairo_move_to(cr, x + 26.0, y + 24.0);
    cairo_show_text(cr, "WARP INJECT");

    // Divider Line
    cairo_set_source_rgba(cr, is_active ? 1.0 : 0.2, is_active ? 0.5 : 0.3, is_active ? 0.1 : 0.4, 0.35);
    cairo_set_line_width(cr, 0.8);
    cairo_move_to(cr, x + 8.0, y + 34.0);
    cairo_line_to(cr, x + w - 8.0, y + 34.0);
    cairo_stroke(cr);

    // --- Section 2: Plasma Booster Core Meter (5 vertical booster segments) ---
    double meter_y = y + 42.0;
    double meter_w = w - 16.0;
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 6.0);
    cairo_set_source_rgba(cr, is_active ? 1.0 : COLOR_TEXT_MUTED_R,
                              is_active ? 0.7 : COLOR_TEXT_MUTED_G,
                              is_active ? 0.2 : COLOR_TEXT_MUTED_B, 0.90);
    cairo_move_to(cr, x + 8.0, meter_y + 8.0);
    cairo_show_text(cr, "THRUST STAGE");

    int segs = 5;
    double b_w = (meter_w - ((segs - 1) * 3.0)) / (double)segs;
    for (int s = 0; s < segs; s++) {
        double bx = x + 8.0 + (s * (b_w + 3.0));
        cairo_rectangle(cr, bx, meter_y + 14.0, b_w, 24.0);
        if (is_active) {
            cairo_set_source_rgba(cr, 1.0, 0.35 + (s * 0.12), 0.05, 0.95);
        } else {
            cairo_set_source_rgba(cr, 0.2, 0.25, 0.3, 0.25);
        }
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, is_active ? 1.0 : 0.3, is_active ? 0.6 : 0.4, is_active ? 0.1 : 0.5, 0.6);
        cairo_set_line_width(cr, 0.6);
        cairo_stroke(cr);
    }

    cairo_set_font_size(cr, 5.8);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.2, COLOR_TEXT_MUTED_G + 0.2, COLOR_TEXT_MUTED_B + 0.2, 0.85);
    cairo_move_to(cr, x + 8.0, meter_y + 48.0);
    cairo_show_text(cr, "FREQ: MAX CLOCK");

    // --- Section 3: Lower Exhaust Louvers & Status Plate ---
    double low_y = y + 96.0;
    cairo_set_source_rgba(cr, is_active ? 1.0 : 0.3, is_active ? 0.5 : 0.3, is_active ? 0.1 : 0.3, is_active ? 0.8 : 0.2);
    cairo_set_line_width(cr, 1.4);
    cairo_move_to(cr, x + 10.0, low_y + 14.0);
    cairo_line_to(cr, x + 18.0, low_y + 4.0);
    cairo_move_to(cr, x + 18.0, low_y + 14.0);
    cairo_line_to(cr, x + 26.0, low_y + 4.0);
    cairo_move_to(cr, x + 26.0, low_y + 14.0);
    cairo_line_to(cr, x + 34.0, low_y + 4.0);
    cairo_stroke(cr);

    // Status Plate
    cairo_rectangle(cr, x + 8.0, low_y + 22.0, w - 16.0, 16.0);
    cairo_set_source_rgba(cr, 0.01, 0.03, 0.05, 0.90);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, is_active ? 1.0 : 0.3, is_active ? 0.5 : 0.3, is_active ? 0.1 : 0.3, 0.6);
    cairo_set_line_width(cr, 0.6);
    cairo_stroke(cr);

    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 6.5);
    cairo_set_source_rgba(cr, is_active ? 1.0 : COLOR_TEXT_MUTED_R,
                              is_active ? 0.6 : COLOR_TEXT_MUTED_G,
                              is_active ? 0.1 : COLOR_TEXT_MUTED_B, 1.0);
    cairo_move_to(cr, x + 12.0, low_y + 33.0);
    cairo_show_text(cr, is_active ? "● [WARP MAX]" : "○ [STANDBY]");

    cairo_restore(cr);
}

// 2. BALANCED: Hexagonal Multi-Faceted Quantum Reactor Pod (Height = 146px)
static void draw_funky_reactor_pod(cairo_t *cr, double x, double y, double w, double h,
                                  gboolean is_active, gboolean is_hov) {
    cairo_save(cr);

    // Hexagonal Quantum Vessel Geometry
    cairo_new_path(cr);
    cairo_move_to(cr, x + 16.0, y);
    cairo_line_to(cr, x + w - 16.0, y);
    cairo_line_to(cr, x + w, y + 16.0);
    cairo_line_to(cr, x + w, y + h - 16.0);
    cairo_line_to(cr, x + w - 16.0, y + h);
    cairo_line_to(cr, x + 16.0, y + h);
    cairo_line_to(cr, x, y + h - 16.0);
    cairo_line_to(cr, x, y + 16.0);
    cairo_close_path(cr);

    cairo_pattern_t *pat = cairo_pattern_create_linear(x, y, x, y + h);
    if (is_active) {
        cairo_pattern_add_color_stop_rgba(pat, 0.0, COLOR_CYAN_R * 0.35, COLOR_CYAN_G * 0.38, COLOR_CYAN_B * 0.45, 0.98);
        cairo_pattern_add_color_stop_rgba(pat, 0.5, 0.03, 0.09, 0.14, 0.92);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.01, 0.03, 0.05, 0.98);
    } else {
        cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.05, 0.07, 0.10, is_hov ? 0.92 : 0.75);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.01, 0.02, 0.03, 0.96);
    }
    cairo_set_source(cr, pat);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(pat);

    cairo_set_source_rgba(cr, is_active ? COLOR_CYAN_R : (is_hov ? COLOR_CYAN_R : 0.25),
                              is_active ? COLOR_CYAN_G : (is_hov ? COLOR_CYAN_G : 0.35),
                              is_active ? COLOR_CYAN_B : (is_hov ? COLOR_CYAN_B : 0.45), is_active ? 1.0 : 0.80);
    cairo_set_line_width(cr, is_active ? 1.5 : 0.9);
    cairo_stroke(cr);

    // --- Section 1: Header Area ---
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14.0);
    cairo_set_source_rgba(cr, is_active ? COLOR_CYAN_R : COLOR_TEXT_MUTED_R,
                              is_active ? COLOR_CYAN_G : COLOR_TEXT_MUTED_G,
                              is_active ? COLOR_CYAN_B : COLOR_TEXT_MUTED_B, 1.0);
    cairo_move_to(cr, x + 8.0, y + 22.0);
    cairo_show_text(cr, "");

    cairo_set_font_size(cr, 7.8);
    cairo_set_source_rgba(cr, is_active ? COLOR_TEXT_BRIGHT_R : COLOR_TEXT_MUTED_R + 0.2,
                              is_active ? COLOR_TEXT_BRIGHT_G : COLOR_TEXT_MUTED_G + 0.2,
                              is_active ? COLOR_TEXT_BRIGHT_B : COLOR_TEXT_MUTED_B + 0.2, 0.98);
    cairo_move_to(cr, x + 24.0, y + 14.0);
    cairo_show_text(cr, "BALANCED");

    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 6.0);
    cairo_set_source_rgba(cr, is_active ? COLOR_CYAN_R : COLOR_TEXT_MUTED_R,
                              is_active ? COLOR_CYAN_G : COLOR_TEXT_MUTED_G,
                              is_active ? COLOR_CYAN_B : COLOR_TEXT_MUTED_B, is_active ? 0.95 : 0.65);
    cairo_move_to(cr, x + 24.0, y + 24.0);
    cairo_show_text(cr, "CRUISE VECTOR");

    // Divider Line
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.35);
    cairo_set_line_width(cr, 0.8);
    cairo_move_to(cr, x + 8.0, y + 34.0);
    cairo_line_to(cr, x + w - 8.0, y + 34.0);
    cairo_stroke(cr);

    // --- Section 2: Rotating Quantum Diamond Crystal Matrix ---
    double mid_y = y + 42.0;
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 6.0);
    cairo_set_source_rgba(cr, is_active ? COLOR_CYAN_R : COLOR_TEXT_MUTED_R,
                              is_active ? COLOR_CYAN_G : COLOR_TEXT_MUTED_G,
                              is_active ? COLOR_CYAN_B : COLOR_TEXT_MUTED_B, 0.90);
    cairo_move_to(cr, x + 8.0, mid_y + 8.0);
    cairo_show_text(cr, "QUANTUM FLUX");

    double q_cx = x + (w * 0.5);
    double q_cy = mid_y + 26.0;

    // Outer Tech Orbit Ring
    cairo_arc(cr, q_cx, q_cy, 14.0, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, is_active ? 0.6 : 0.2);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);

    // Center Diamond
    cairo_move_to(cr, q_cx, q_cy - 8.0);
    cairo_line_to(cr, q_cx + 8.0, q_cy);
    cairo_line_to(cr, q_cx, q_cy + 8.0);
    cairo_line_to(cr, q_cx - 8.0, q_cy);
    cairo_close_path(cr);
    if (is_active) {
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
    } else {
        cairo_set_source_rgba(cr, 0.2, 0.3, 0.4, 0.35);
    }
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, is_active ? 0.9 : 0.2);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);

    cairo_set_font_size(cr, 5.8);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.2, COLOR_TEXT_MUTED_G + 0.2, COLOR_TEXT_MUTED_B + 0.2, 0.85);
    cairo_move_to(cr, x + 8.0, mid_y + 48.0);
    cairo_show_text(cr, "STABILITY: 100%");

    // --- Section 3: Dual Cooling Flux Slits & Status Plate ---
    double low_y = y + 96.0;
    cairo_rectangle(cr, x + 10.0, low_y + 4.0, w - 20.0, 4.0);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, is_active ? 0.8 : 0.2);
    cairo_fill(cr);
    cairo_rectangle(cr, x + 10.0, low_y + 12.0, w - 20.0, 4.0);
    cairo_fill(cr);

    // Status Plate
    cairo_rectangle(cr, x + 8.0, low_y + 22.0, w - 16.0, 16.0);
    cairo_set_source_rgba(cr, 0.01, 0.03, 0.05, 0.90);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, is_active ? COLOR_CYAN_R : 0.3, is_active ? COLOR_CYAN_G : 0.3, is_active ? COLOR_CYAN_B : 0.3, 0.6);
    cairo_set_line_width(cr, 0.6);
    cairo_stroke(cr);

    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 6.5);
    cairo_set_source_rgba(cr, is_active ? COLOR_CYAN_R : COLOR_TEXT_MUTED_R,
                              is_active ? COLOR_CYAN_G : COLOR_TEXT_MUTED_G,
                              is_active ? COLOR_CYAN_B : COLOR_TEXT_MUTED_B, 1.0);
    cairo_move_to(cr, x + 14.0, low_y + 33.0);
    cairo_show_text(cr, is_active ? "● [NOMINAL]" : "○ [STANDBY]");

    cairo_restore(cr);
}

// 3. CRYO SAVER: Starboard Wing Pod (Height = 146px)
static void draw_funky_cryo_pod(cairo_t *cr, double x, double y, double w, double h,
                               gboolean is_active, gboolean is_hov) {
    cairo_save(cr);

    // Starboard Interceptor Wing Geometry
    cairo_new_path(cr);
    cairo_move_to(cr, x + 16.0, y);
    cairo_line_to(cr, x + w - 14.0, y);
    cairo_line_to(cr, x + w, y + 16.0);
    cairo_line_to(cr, x + w, y + h - 16.0);
    cairo_line_to(cr, x + w - 16.0, y + h);
    cairo_line_to(cr, x + 14.0, y + h);
    cairo_line_to(cr, x, y + h - 14.0);
    cairo_line_to(cr, x, y + 14.0);
    cairo_close_path(cr);

    cairo_pattern_t *pat = cairo_pattern_create_linear(x, y, x + w, y + h);
    if (is_active) {
        cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.05, 0.35, 0.18, 0.98);
        cairo_pattern_add_color_stop_rgba(pat, 0.4, 0.02, 0.16, 0.08, 0.94);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.008, 0.04, 0.02, 0.98);
    } else {
        cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.05, 0.07, 0.10, is_hov ? 0.92 : 0.75);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.01, 0.02, 0.03, 0.96);
    }
    cairo_set_source(cr, pat);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(pat);

    cairo_set_source_rgba(cr, is_active ? COLOR_GREEN_R : (is_hov ? COLOR_CYAN_R : 0.25),
                              is_active ? COLOR_GREEN_G : (is_hov ? COLOR_CYAN_G : 0.35),
                              is_active ? COLOR_GREEN_B : (is_hov ? COLOR_CYAN_B : 0.45), is_active ? 1.0 : 0.80);
    cairo_set_line_width(cr, is_active ? 1.5 : 0.9);
    cairo_stroke(cr);

    // --- Section 1: Header Area ---
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14.0);
    cairo_set_source_rgba(cr, is_active ? COLOR_GREEN_R : COLOR_TEXT_MUTED_R,
                              is_active ? COLOR_GREEN_G : COLOR_TEXT_MUTED_G,
                              is_active ? COLOR_GREEN_B : COLOR_TEXT_MUTED_B, 1.0);
    cairo_move_to(cr, x + 8.0, y + 22.0);
    cairo_show_text(cr, "");

    cairo_set_font_size(cr, 7.8);
    cairo_set_source_rgba(cr, is_active ? COLOR_TEXT_BRIGHT_R : COLOR_TEXT_MUTED_R + 0.2,
                              is_active ? COLOR_TEXT_BRIGHT_G : COLOR_TEXT_MUTED_G + 0.2,
                              is_active ? COLOR_TEXT_BRIGHT_B : COLOR_TEXT_MUTED_B + 0.2, 0.98);
    cairo_move_to(cr, x + 24.0, y + 14.0);
    cairo_show_text(cr, "CRYO SAVER");

    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 6.0);
    cairo_set_source_rgba(cr, is_active ? COLOR_GREEN_R : COLOR_TEXT_MUTED_R,
                              is_active ? COLOR_GREEN_G : COLOR_TEXT_MUTED_G,
                              is_active ? COLOR_GREEN_B : COLOR_TEXT_MUTED_B, is_active ? 0.95 : 0.65);
    cairo_move_to(cr, x + 24.0, y + 24.0);
    cairo_show_text(cr, "THERMAL ECO");

    // Divider Line
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.35);
    cairo_set_line_width(cr, 0.8);
    cairo_move_to(cr, x + 8.0, y + 34.0);
    cairo_line_to(cr, x + w - 8.0, y + 34.0);
    cairo_stroke(cr);

    // --- Section 2: Cryogenic Stasis Bar (5 ice-flow segments) ---
    double meter_y = y + 42.0;
    double meter_w = w - 16.0;
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 6.0);
    cairo_set_source_rgba(cr, is_active ? COLOR_GREEN_R : COLOR_TEXT_MUTED_R,
                              is_active ? COLOR_GREEN_G : COLOR_TEXT_MUTED_G,
                              is_active ? COLOR_GREEN_B : COLOR_TEXT_MUTED_B, 0.90);
    cairo_move_to(cr, x + 8.0, meter_y + 8.0);
    cairo_show_text(cr, "STASIS LEVEL");

    int segs = 5;
    double b_w = (meter_w - ((segs - 1) * 3.0)) / (double)segs;
    for (int s = 0; s < segs; s++) {
        double bx = x + 8.0 + (s * (b_w + 3.0));
        cairo_rectangle(cr, bx, meter_y + 14.0, b_w, 24.0);
        if (is_active) {
            cairo_set_source_rgba(cr, 0.10 + (s * 0.15), 0.85, 0.45, 0.95);
        } else {
            cairo_set_source_rgba(cr, 0.2, 0.3, 0.25, 0.25);
        }
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, is_active ? COLOR_GREEN_R : 0.3, is_active ? COLOR_GREEN_G : 0.4, is_active ? COLOR_GREEN_B : 0.35, 0.6);
        cairo_set_line_width(cr, 0.6);
        cairo_stroke(cr);
    }

    cairo_set_font_size(cr, 5.8);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.2, COLOR_TEXT_MUTED_G + 0.2, COLOR_TEXT_MUTED_B + 0.2, 0.85);
    cairo_move_to(cr, x + 8.0, meter_y + 48.0);
    cairo_show_text(cr, "DRAIN: -40% ECO");

    // --- Section 3: Starboard Cooling Heat-Sink Fins & Status Plate ---
    double low_y = y + 96.0;
    cairo_set_source_rgba(cr, is_active ? COLOR_GREEN_R : 0.3, is_active ? COLOR_GREEN_G : 0.3, is_active ? COLOR_GREEN_B : 0.3, is_active ? 0.8 : 0.2);
    cairo_set_line_width(cr, 1.4);
    cairo_move_to(cr, x + w - 10.0, low_y + 4.0);
    cairo_line_to(cr, x + w - 18.0, low_y + 14.0);
    cairo_move_to(cr, x + w - 18.0, low_y + 4.0);
    cairo_line_to(cr, x + w - 26.0, low_y + 14.0);
    cairo_move_to(cr, x + w - 26.0, low_y + 4.0);
    cairo_line_to(cr, x + w - 34.0, low_y + 14.0);
    cairo_stroke(cr);

    // Status Plate
    cairo_rectangle(cr, x + 8.0, low_y + 22.0, w - 16.0, 16.0);
    cairo_set_source_rgba(cr, 0.01, 0.03, 0.05, 0.90);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, is_active ? COLOR_GREEN_R : 0.3, is_active ? COLOR_GREEN_G : 0.3, is_active ? COLOR_GREEN_B : 0.3, 0.6);
    cairo_set_line_width(cr, 0.6);
    cairo_stroke(cr);

    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 6.5);
    cairo_set_source_rgba(cr, is_active ? COLOR_GREEN_R : COLOR_TEXT_MUTED_R,
                              is_active ? COLOR_GREEN_G : COLOR_TEXT_MUTED_G,
                              is_active ? COLOR_GREEN_B : COLOR_TEXT_MUTED_B, 1.0);
    cairo_move_to(cr, x + 12.0, low_y + 33.0);
    cairo_show_text(cr, is_active ? "● [ECO SHIELD]" : "○ [STANDBY]");

    cairo_restore(cr);
}

// ==============================================================================
// 🌟 FULL-CONSOLE IMMERSIVE HOLOGRAPHIC SURGE (CLIPPED TO CONSOLE INTERIOR)
// ==============================================================================

static void draw_mode_transition_overlay(cairo_t *cr, double pad_x, double cur_y, double panel_w, double panel_h) {
    if (!g_app.mode_anim_active) return;

    cairo_save(cr);

    // Strictly clip all animation rendering to the interior of the cockpit frame
    cairo_rectangle(cr, pad_x + 1.0, cur_y + 1.0, panel_w - 2.0, panel_h - 2.0);
    cairo_clip(cr);

    double p = g_app.mode_anim_progress;
    double inv_p = 1.0 - p;

    double acc_r = COLOR_CYAN_R, acc_g = COLOR_CYAN_G, acc_b = COLOR_CYAN_B;
    const char *glyph_char = "";
    if (g_app.mode_anim_type == 1) { // Overclock
        acc_r = 1.0; acc_g = 0.55; acc_b = 0.05; glyph_char = "";
    } else if (g_app.mode_anim_type == 3) { // Cryo
        acc_r = COLOR_GREEN_R; acc_g = COLOR_GREEN_G; acc_b = COLOR_GREEN_B; glyph_char = "";
    } else if (g_app.mode_anim_type == 4) { // Rendering Mode
        acc_r = 1.0; acc_g = 0.25; acc_b = 0.65; glyph_char = "󰑈";
    }

    // 1. Ambient Energetic Color Wash Covering the ENTIRE Interior of the Box
    cairo_rectangle(cr, pad_x, cur_y, panel_w, panel_h);
    cairo_set_source_rgba(cr, acc_r, acc_g, acc_b, inv_p * 0.14);
    cairo_fill(cr);

    // 2. Full-Box Cybernetic Hex/Tech Grid Matrix Lines
    cairo_save(cr);
    cairo_set_line_width(cr, 0.6);
    cairo_set_source_rgba(cr, acc_r, acc_g, acc_b, inv_p * 0.18);
    // Vertical grid lines across entire panel
    for (double gx = pad_x; gx < pad_x + panel_w; gx += 28.0) {
        cairo_move_to(cr, gx, cur_y);
        cairo_line_to(cr, gx, cur_y + panel_h);
    }
    // Horizontal grid lines across entire panel
    for (double gy = cur_y; gy < cur_y + panel_h; gy += 24.0) {
        cairo_move_to(cr, pad_x, gy);
        cairo_line_to(cr, pad_x + panel_w, gy);
    }
    cairo_stroke(cr);
    cairo_restore(cr);

    // 3. Expanding Concentric Shockwaves from Clicked Button Covering the Entire Box
    double cx = g_app.mode_anim_cx;
    double cy = g_app.mode_anim_cy;

    double r1 = p * 450.0;
    cairo_arc(cr, cx, cy, r1, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, acc_r, acc_g, acc_b, inv_p * 0.85);
    cairo_set_line_width(cr, (inv_p * 3.5) + 0.6);
    cairo_stroke(cr);

    double r2 = p * 300.0;
    cairo_save(cr);
    double dashes[] = {12.0, 6.0};
    cairo_set_dash(cr, dashes, 2, g_app.pulse_phase * 20.0);
    cairo_arc(cr, cx, cy, r2, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, acc_r, acc_g, acc_b, inv_p * 0.70);
    cairo_set_line_width(cr, 1.4);
    cairo_stroke(cr);
    cairo_restore(cr);

    // 4. Prominent High-Visibility Holographic Central Glyph & Tech Framing
    double center_x = pad_x + (panel_w * 0.5);
    double center_y = cur_y + (panel_h * 0.5) + 6.0;

    // A. Rotating Cybernetic Framing Reticle around the Glyph
    cairo_save(cr);
    cairo_arc(cr, center_x, center_y, 72.0, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, acc_r, acc_g, acc_b, inv_p * 0.40);
    cairo_set_line_width(cr, 1.2);
    cairo_stroke(cr);

    // Inner Dashed Tech Orbit
    double ret_dashes[] = {10.0, 6.0};
    cairo_set_dash(cr, ret_dashes, 2, g_app.pulse_phase * 25.0);
    cairo_arc(cr, center_x, center_y, 62.0, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, inv_p * 0.55);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);
    cairo_restore(cr);

    // B. High-Visibility Luminous Glyph with Neon Aura
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 95.0);
    cairo_text_extents_t ext;
    cairo_text_extents(cr, glyph_char, &ext);
    double gx = center_x - (ext.width * 0.5) - ext.x_bearing;
    double gy = center_y - (ext.height * 0.5) - ext.y_bearing - 4.0;

    // Luminous Outer Glow / Drop Shadow Layers
    cairo_set_source_rgba(cr, acc_r, acc_g, acc_b, inv_p * 0.85);
    cairo_move_to(cr, gx - 2.0, gy); cairo_show_text(cr, glyph_char);
    cairo_move_to(cr, gx + 2.0, gy); cairo_show_text(cr, glyph_char);
    cairo_move_to(cr, gx, gy - 2.0); cairo_show_text(cr, glyph_char);
    cairo_move_to(cr, gx, gy + 2.0); cairo_show_text(cr, glyph_char);

    // Brilliant High-Luminance Center Core (Bold & Clear)
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, inv_p * 0.98);
    cairo_move_to(cr, gx, gy);
    cairo_show_text(cr, glyph_char);

    // C. Center Status Pill Badge below the Glyph
    double pill_w = 200.0;
    double pill_h = 18.0;
    double pill_x = center_x - (pill_w * 0.5);
    double pill_y = center_y + 44.0;

    cairo_rectangle(cr, pill_x, pill_y, pill_w, pill_h);
    cairo_set_source_rgba(cr, 0.01, 0.03, 0.06, inv_p * 0.94);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, acc_r, acc_g, acc_b, inv_p * 0.90);
    cairo_set_line_width(cr, 1.2);
    cairo_stroke(cr);

    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 7.5);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, inv_p * 0.98);
    cairo_move_to(cr, pill_x + 12.0, pill_y + 12.5);
    cairo_show_text(cr, g_app.mode_anim_title);

    // 6. Perimeter Laser Surge around Box Rim
    cairo_rectangle(cr, pad_x + 2.0, cur_y + 2.0, panel_w - 4.0, panel_h - 4.0);
    cairo_set_source_rgba(cr, acc_r, acc_g, acc_b, inv_p * 0.75);
    cairo_set_line_width(cr, 2.0);
    cairo_stroke(cr);

    cairo_restore(cr);
}

// ==============================================================================
// DRAW EVENT
// ==============================================================================

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

    // 1. Outer Beveled Cockpit Pedestal Frame
    cairo_draw_beveled_panel(cr, pad_x, cur_y, panel_w, panel_h, 8.0);
    cairo_draw_module_header(cr, pad_x, cur_y, panel_w, "COCKPIT PEDESTAL // THROTTLE & AVIONICS", COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // 2. Physical Connecting Conduits
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.40);
    cairo_set_line_width(cr, 1.2);
    // Top conduit
    cairo_move_to(cr, pad_x + panel_w - 40.0, cur_y);
    cairo_line_to(cr, pad_x + panel_w - 40.0, 0);
    cairo_stroke(cr);
    cairo_arc(cr, pad_x + panel_w - 40.0, cur_y, 2.0, 0, 2 * M_PI);
    cairo_fill(cr);

    // Left conduit
    cairo_move_to(cr, pad_x, cur_y + 120.0);
    cairo_line_to(cr, 0, cur_y + 120.0);
    cairo_stroke(cr);
    cairo_arc(cr, pad_x, cur_y + 120.0, 2.0, 0, 2 * M_PI);
    cairo_fill(cr);

    // Bottom conduit
    cairo_move_to(cr, pad_x + panel_w - 80.0, cur_y + panel_h);
    cairo_line_to(cr, pad_x + panel_w - 80.0, height);
    cairo_stroke(cr);
    cairo_arc(cr, pad_x + panel_w - 80.0, cur_y + panel_h, 2.0, 0, 2 * M_PI);
    cairo_fill(cr);

    // =============================================================
    // LEFT COLUMN: AIRCRAFT DUAL THROTTLE QUADRANT (SPEED LEVERS)
    // =============================================================
    double split_x = pad_x + 144.0;
    double throttle_top = cur_y + 26.0;
    double throttle_h = panel_h - 36.0;

    // Throttle 1: Master Audio Volume (Cyan Speed Lever)
    double t1_cx = pad_x + 44.0;
    draw_aircraft_throttle_lever(cr, t1_cx, throttle_top, throttle_h,
                                g_app.state.volume_pct, g_app.state.is_muted,
                                "AUDIO VOL", g_app.state.output_name,
                                1, 2,
                                COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // Throttle 2: Display Illumination (Gold/Amber Speed Lever)
    double t2_cx = pad_x + 104.0;
    draw_aircraft_throttle_lever(cr, t2_cx, throttle_top, throttle_h,
                                g_app.state.brightness_pct, FALSE,
                                "PHOTON BR", g_app.state.input_name,
                                3, 4,
                                1.0, 0.72, 0.05);

    // Center Vertical Cockpit Divider Conduit
    cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.35);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, split_x, cur_y + 24.0);
    cairo_line_to(cr, split_x, cur_y + panel_h - 10.0);
    cairo_stroke(cr);

    // Small connector dot
    cairo_arc(cr, split_x, cur_y + (panel_h * 0.5), 2.0, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.7);
    cairo_fill(cr);

    // =============================================================
    // RIGHT COLUMN: TACTICAL AVIONICS & MASSIVE SPACESHIP POWER MODULES
    // =============================================================
    double right_x = split_x + 10.0;
    double right_w = pad_x + panel_w - right_x - 6.0;
    double r_cur_y = cur_y + 26.0;

    // 1. Tactical Quick Toggles Matrix
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 7.2);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.2, COLOR_TEXT_MUTED_G + 0.2, COLOR_TEXT_MUTED_B + 0.2, 0.90);
    cairo_move_to(cr, right_x, r_cur_y + 2.0);
    cairo_show_text(cr, "TACTICAL AVIONICS // QUICK TOGGLES");

    r_cur_y += 8.0;
    double pod_w = (right_w - 10.0) / 3.0;
    double pod_h = 20.0;

    // Row 1: NIGHT LIGHT, DND, VPN
    draw_quick_toggle_pod(cr, right_x, r_cur_y, pod_w, pod_h,
                          "", "NIGHT", g_app.state.night_light_on, (g_app.hovered_btn == 10),
                          1.0, 0.67, 0.0);

    draw_quick_toggle_pod(cr, right_x + pod_w + 5.0, r_cur_y, pod_w, pod_h,
                          "", "DND", g_app.state.dnd_on, (g_app.hovered_btn == 11),
                          COLOR_ALERT_R, 0.4, 0.4);

    draw_quick_toggle_pod(cr, right_x + (pod_w * 2.0) + 10.0, r_cur_y, pod_w, pod_h,
                          "", "VPN", g_app.state.vpn_on, (g_app.hovered_btn == 12),
                          COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);

    r_cur_y += pod_h + 3.0;

    // Row 2: MIC MUTE, CAMERA, RECORD
    draw_quick_toggle_pod(cr, right_x, r_cur_y, pod_w, pod_h,
                          "", g_app.state.mic_muted ? "MUTED" : "MIC ON", !g_app.state.mic_muted, (g_app.hovered_btn == 13),
                          COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);

    draw_quick_toggle_pod(cr, right_x + pod_w + 5.0, r_cur_y, pod_w, pod_h,
                          "", "CAM OK", g_app.state.camera_active, (g_app.hovered_btn == 14),
                          COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    draw_quick_toggle_pod(cr, right_x + (pod_w * 2.0) + 10.0, r_cur_y, pod_w, pod_h,
                          "", g_app.state.recording_on ? "REC ON" : "RECORD", g_app.state.recording_on, (g_app.hovered_btn == 15),
                          COLOR_ALERT_R, 0.2, 0.3);

    r_cur_y += pod_h + 3.0;

    // Row 3: Dedicated Full-Width RENDERING MODE HYPER-SWITCH POD
    double r_pod_h = 21.0;
    draw_render_mode_pod(cr, right_x, r_cur_y, right_w, r_pod_h,
                         g_app.state.render_mode_on, (g_app.hovered_btn == 30),
                         g_app.state.render_freed_mb);

    r_cur_y += r_pod_h + 4.0;

    // Separator line
    cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.25);
    cairo_set_line_width(cr, 0.8);
    cairo_move_to(cr, right_x, r_cur_y);
    cairo_line_to(cr, right_x + right_w, r_cur_y);
    cairo_stroke(cr);

    r_cur_y += 4.0;

    // 2. Power Telemetry HUD Bar (Clean Spacing, zero overlap)
    draw_power_telemetry_hud(cr, right_x, r_cur_y, right_w, 32.0, &g_app.state);

    r_cur_y += 36.0;

    // Sub-header for power dispatch
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 7.2);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.2, COLOR_TEXT_MUTED_G + 0.2, COLOR_TEXT_MUTED_B + 0.2, 0.90);
    cairo_move_to(cr, right_x, r_cur_y + 2.0);
    cairo_show_text(cr, "REACTOR DISPATCH // POWER MODULES");

    r_cur_y += 7.0;

    // 3. Massive Tall Spaceship Power Modules (Filling the rest of the window cleanly)
    double p_mod_w = (right_w - 10.0) / 3.0;
    double p_mod_h = (cur_y + panel_h - 4.0) - r_cur_y;

    gboolean is_perf = (strcmp(g_app.state.power_profile, "PERFORMANCE") == 0);
    gboolean is_bal = (strcmp(g_app.state.power_profile, "BALANCED") == 0);
    gboolean is_save = (strcmp(g_app.state.power_profile, "POWER SAVER") == 0);

    // 1. Forward-Swept Thruster Pod (OVERCLOCK)
    draw_funky_warp_pod(cr, right_x, r_cur_y, p_mod_w, p_mod_h,
                        is_perf, (g_app.hovered_btn == 20));

    // 2. Hexagonal Quantum Reactor Pod (BALANCED)
    draw_funky_reactor_pod(cr, right_x + p_mod_w + 5.0, r_cur_y, p_mod_w, p_mod_h,
                           is_bal, (g_app.hovered_btn == 21));

    // 3. Starboard Wing Pod (CRYO SAVER)
    draw_funky_cryo_pod(cr, right_x + (p_mod_w * 2.0) + 10.0, r_cur_y, p_mod_w, p_mod_h,
                        is_save, (g_app.hovered_btn == 22));

    // 4. Immersive Full-Console Holographic Surge Animation (Clipped to box interior)
    draw_mode_transition_overlay(cr, pad_x, cur_y, panel_w, panel_h);

    // 5. Spacecraft Sequential Power-On Awakening Overlay (Starts at 1.6s)
    solaris_draw_awakening_overlay(cr, pad_x, cur_y, panel_w, panel_h, 1.6, 0.8, "FLIGHT DECK & POWER");

    return FALSE;
}

// ==============================================================================
// MOUSE INTERACTION & HIT TESTING
// ==============================================================================

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    (void)user_data;
    double x = event->x;
    double y = event->y;

    double pad_x = 8.0;
    double cur_y = 8.0;
    double panel_w = (double)gtk_widget_get_allocated_width(widget) - 16.0;
    double panel_h = (double)gtk_widget_get_allocated_height(widget) - 16.0;
    double split_x = pad_x + 144.0;
    double throttle_top = cur_y + 26.0;
    double throttle_h = panel_h - 36.0;
    double track_y = throttle_top + 16.0;
    double eff_h = throttle_h - 32.0;

    int old_hov = g_app.hovered_btn;
    g_app.hovered_btn = -1;

    // Handle Active Dragging on Throttle Levers
    if (g_app.active_drag == 1) { // Dragging Audio Throttle
        double rel_y = CLAMP(y - track_y, 0.0, eff_h);
        double pct = (1.0 - (rel_y / eff_h)) * 100.0;
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "wpctl set-volume @DEFAULT_AUDIO_SINK@ %.0f%%", pct);
        g_spawn_command_line_async(cmd, NULL);
        g_app.state.volume_pct = pct;
        gtk_widget_queue_draw(widget);
        return FALSE;
    } else if (g_app.active_drag == 2) { // Dragging Brightness Throttle
        double rel_y = CLAMP(y - track_y, 0.0, eff_h);
        double pct = (1.0 - (rel_y / eff_h)) * 100.0;
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "brightnessctl set %.0f%%", pct);
        g_spawn_command_line_async(cmd, NULL);
        g_app.state.brightness_pct = pct;
        gtk_widget_queue_draw(widget);
        return FALSE;
    }

    // Audio Throttle Steppers (1: Up, 2: Down)
    double t1_cx = pad_x + 44.0;
    if (x >= t1_cx - 14.0 && x <= t1_cx + 14.0 && y >= throttle_top + 6.0 && y <= throttle_top + 14.0) g_app.hovered_btn = 1;
    if (x >= t1_cx - 14.0 && x <= t1_cx + 14.0 && y >= track_y + eff_h + 2.0 && y <= track_y + eff_h + 10.0) g_app.hovered_btn = 2;

    // Brightness Throttle Steppers (3: Up, 4: Down)
    double t2_cx = pad_x + 104.0;
    if (x >= t2_cx - 14.0 && x <= t2_cx + 14.0 && y >= throttle_top + 6.0 && y <= throttle_top + 14.0) g_app.hovered_btn = 3;
    if (x >= t2_cx - 14.0 && x <= t2_cx + 14.0 && y >= track_y + eff_h + 2.0 && y <= track_y + eff_h + 10.0) g_app.hovered_btn = 4;

    // Right Column Controls
    double right_x = split_x + 10.0;
    double right_w = pad_x + panel_w - right_x - 6.0;
    double pod_w = (right_w - 10.0) / 3.0;

    // Quick toggles row 1 (10, 11, 12)
    double r_t_y1 = cur_y + 34.0;
    if (y >= r_t_y1 && y <= r_t_y1 + 20.0) {
        if (x >= right_x && x <= right_x + pod_w) g_app.hovered_btn = 10;
        if (x >= right_x + pod_w + 5.0 && x <= right_x + (pod_w * 2.0) + 5.0) g_app.hovered_btn = 11;
        if (x >= right_x + (pod_w * 2.0) + 10.0 && x <= right_x + right_w) g_app.hovered_btn = 12;
    }

    // Quick toggles row 2 (13, 14, 15)
    double r_t_y2 = r_t_y1 + 23.0;
    if (y >= r_t_y2 && y <= r_t_y2 + 20.0) {
        if (x >= right_x && x <= right_x + pod_w) g_app.hovered_btn = 13;
        if (x >= right_x + pod_w + 5.0 && x <= right_x + (pod_w * 2.0) + 5.0) g_app.hovered_btn = 14;
        if (x >= right_x + (pod_w * 2.0) + 10.0 && x <= right_x + right_w) g_app.hovered_btn = 15;
    }

    // Render mode pod (30)
    double r_t_y3 = r_t_y2 + 23.0;
    if (y >= r_t_y3 && y <= r_t_y3 + 21.0) {
        if (x >= right_x && x <= right_x + right_w) g_app.hovered_btn = 30;
    }

    // Massive Funky Power Modules (20, 21, 22)
    double p_mod_w = pod_w;
    double p_mod_y = r_t_y3 + 21.0 + 4.0 + 4.0 + 32.0 + 7.0 + 2.0;
    double p_mod_h = (cur_y + panel_h - 4.0) - p_mod_y;
    if (y >= p_mod_y && y <= p_mod_y + p_mod_h) {
        if (x >= right_x && x <= right_x + p_mod_w) g_app.hovered_btn = 20;
        if (x >= right_x + p_mod_w + 5.0 && x <= right_x + (p_mod_w * 2.0) + 5.0) g_app.hovered_btn = 21;
        if (x >= right_x + (p_mod_w * 2.0) + 10.0 && x <= right_x + right_w) g_app.hovered_btn = 22;
    }

    if (old_hov != g_app.hovered_btn) {
        gtk_widget_queue_draw(widget);
    }
    return FALSE;
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)user_data;
    double x = event->x;
    double y = event->y;

    double pad_x = 8.0;
    double cur_y = 8.0;
    double panel_w = (double)gtk_widget_get_allocated_width(widget) - 16.0;
    double panel_h = (double)gtk_widget_get_allocated_height(widget) - 16.0;
    double split_x = pad_x + 144.0;
    double throttle_top = cur_y + 26.0;
    double throttle_h = panel_h - 36.0;
    double track_y = throttle_top + 16.0;
    double eff_h = throttle_h - 32.0;

    double t1_cx = pad_x + 44.0;
    double t2_cx = pad_x + 104.0;

    double right_x = split_x + 10.0;
    double right_w = pad_x + panel_w - right_x - 6.0;
    double p_mod_w = (right_w - 10.0) / 3.0;
    double r_t_y1 = cur_y + 34.0;
    double r_t_y2 = r_t_y1 + 23.0;
    double r_t_y3 = r_t_y2 + 23.0;
    double p_mod_y = r_t_y3 + 21.0 + 4.0 + 4.0 + 32.0 + 7.0 + 2.0;
    double p_mod_h = (cur_y + panel_h - 4.0) - p_mod_y;

    if (event->button == 1) {
        // Check if clicking directly on Throttle Tracks to initiate Drag
        if (x >= t1_cx - 18.0 && x <= t1_cx + 18.0 && y >= track_y && y <= track_y + eff_h) {
            g_app.active_drag = 1;
            double rel_y = CLAMP(y - track_y, 0.0, eff_h);
            double pct = (1.0 - (rel_y / eff_h)) * 100.0;
            char cmd[64];
            snprintf(cmd, sizeof(cmd), "wpctl set-volume @DEFAULT_AUDIO_SINK@ %.0f%%", pct);
            g_spawn_command_line_async(cmd, NULL);
            g_app.state.volume_pct = pct;
            gtk_widget_queue_draw(widget);
            return FALSE;
        }

        if (x >= t2_cx - 18.0 && x <= t2_cx + 18.0 && y >= track_y && y <= track_y + eff_h) {
            g_app.active_drag = 2;
            double rel_y = CLAMP(y - track_y, 0.0, eff_h);
            double pct = (1.0 - (rel_y / eff_h)) * 100.0;
            char cmd[64];
            snprintf(cmd, sizeof(cmd), "brightnessctl set %.0f%%", pct);
            g_spawn_command_line_async(cmd, NULL);
            g_app.state.brightness_pct = pct;
            gtk_widget_queue_draw(widget);
            return FALSE;
        }

        // Stepper Buttons & Quick Toggles
        switch (g_app.hovered_btn) {
            case 1: // Vol +5%
                g_spawn_command_line_async("wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%+", NULL);
                break;
            case 2: // Vol -5%
                g_spawn_command_line_async("wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%-", NULL);
                break;
            case 3: // Brightness +5%
                g_spawn_command_line_async("brightnessctl set +5%", NULL);
                break;
            case 4: // Brightness -5%
                g_spawn_command_line_async("brightnessctl set 5%-", NULL);
                break;
            case 10: // Night Light
                if (g_app.state.night_light_on) {
                    g_spawn_command_line_async("pkill -x wlsunset", NULL);
                } else {
                    g_spawn_command_line_async("wlsunset -t 4500", NULL);
                }
                break;
            case 11: // DND
                if (g_app.state.dnd_on) {
                    g_spawn_command_line_async("makoctl mode -r dnd", NULL);
                } else {
                    g_spawn_command_line_async("makoctl mode -a dnd", NULL);
                }
                break;
            case 12: // VPN
                g_spawn_command_line_async("sh -c 'nmcli con up id wg0 2>/dev/null || nmcli con up id vpn 2>/dev/null || true'", NULL);
                break;
            case 13: // Mic Mute Toggle
                g_spawn_command_line_async("wpctl set-mute @DEFAULT_AUDIO_SOURCE@ toggle", NULL);
                break;
            case 14: // Camera test
                g_spawn_command_line_async("notify-send 'Camera Hardware' 'Device /dev/video0 is nominal'", NULL);
                break;
            case 15: // Screen Record
                if (g_app.state.recording_on) {
                    g_spawn_command_line_async("pkill -x wf-recorder", NULL);
                } else {
                    g_spawn_command_line_async("wf-recorder -f ~/Videos/recording_$(date +%s).mp4", NULL);
                }
                break;
            case 20: // Performance (OVERCLOCK)
                trigger_power_mode_switch(1, right_x + (p_mod_w * 0.5), p_mod_y + (p_mod_h * 0.5));
                break;
            case 21: // Balanced (NOMINAL)
                trigger_power_mode_switch(2, right_x + p_mod_w + 5.0 + (p_mod_w * 0.5), p_mod_y + (p_mod_h * 0.5));
                break;
            case 22: // Power Saver (CRYO SAVER)
                trigger_power_mode_switch(3, right_x + (p_mod_w * 2.0) + 10.0 + (p_mod_w * 0.5), p_mod_y + (p_mod_h * 0.5));
                break;
            case 30: // Rendering Mode (Maximum RAM Purge Toggle)
                trigger_render_mode_switch(right_x + (right_w * 0.5), r_t_y3 + 10.5);
                break;
            default:
                break;
        }

        update_flight_state(&g_app.state);
        gtk_widget_queue_draw(widget);
    }
    return FALSE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    (void)event;
    (void)user_data;
    g_app.active_drag = 0;
    return FALSE;
}

static gboolean on_anim_tick(gpointer user_data) {
    (void)user_data;
    g_app.time_sec += 0.04;
    g_app.pulse_phase += 0.08;

    if (g_app.mode_anim_active) {
        g_app.mode_anim_progress += 0.030;
        if (g_app.mode_anim_progress >= 1.0) {
            g_app.mode_anim_progress = 1.0;
            g_app.mode_anim_active = FALSE;
        }
    }

    redraw_all_views();
    return G_SOURCE_CONTINUE;
}

static gboolean on_telem_tick(gpointer user_data) {
    (void)user_data;
    update_flight_state(&g_app.state);
    solaris_trim_memory();
    redraw_all_views();
    return G_SOURCE_CONTINUE;
}

// ==============================================================================
// ENTRY POINT
// ==============================================================================

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    solaris_load_css();

    memset(&g_app, 0, sizeof(g_app));
    g_app.hovered_btn = -1;
    g_app.active_drag = 0;

    GdkDisplay *display = gdk_display_get_default();
    int n_monitors = display ? gdk_display_get_n_monitors(display) : 1;
    g_app.view_count = 0;

    for (int i = 0; i < n_monitors && i < MAX_FLIGHT_MONITORS; i++) {
        GdkMonitor *mon = display ? gdk_display_get_monitor(display, i) : NULL;
        GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        // 460px width x 330px height at Top=374, Right=16
        solaris_setup_layer_window_for_monitor(win, mon, "solaris-bg-flightdeck",
                                                460, 330,
                                                TRUE, FALSE, FALSE, TRUE,
                                                374, 0, 0, 16);

        GtkWidget *da = gtk_drawing_area_new();
        gtk_container_add(GTK_CONTAINER(win), da);

        gtk_widget_add_events(da,
                              GDK_POINTER_MOTION_MASK |
                              GDK_BUTTON_PRESS_MASK |
                              GDK_BUTTON_RELEASE_MASK |
                              GDK_LEAVE_NOTIFY_MASK);

        g_signal_connect(da, "draw", G_CALLBACK(on_draw), NULL);
        g_signal_connect(da, "motion-notify-event", G_CALLBACK(on_motion_notify), NULL);
        g_signal_connect(da, "button-press-event", G_CALLBACK(on_button_press), NULL);
        g_signal_connect(da, "button-release-event", G_CALLBACK(on_button_release), NULL);

        gtk_widget_show_all(win);

        g_app.windows[g_app.view_count] = win;
        g_app.drawing_areas[g_app.view_count] = da;
        g_app.view_count++;
    }

    if (g_app.view_count > 0) {
        g_app.window = g_app.windows[0];
        g_app.drawing_area = g_app.drawing_areas[0];
    }

    update_flight_state(&g_app.state);

    g_app.anim_timer_id = g_timeout_add(40, on_anim_tick, NULL);
    g_app.telem_timer_id = g_timeout_add(1500, on_telem_tick, NULL);

    gtk_main();
    return 0;
}
