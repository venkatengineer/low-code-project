#include "common.h"
#include "telemetry.h"
#include "cairo_utils.h"
#include <math.h>
#include <signal.h>
#include <time.h>

#define ALIEN_W 360
#define ALIEN_H 300

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    cpu_telemetry_t cpu;
    mem_telemetry_t mem;
    power_telemetry_t pwr;
    guint anim_timer;
    guint telemetry_timer;
    
    // Animation state
    double anim_time;
    double mouse_x;
    double mouse_y;
    gboolean mouse_inside;
    
    // Interaction state
    gboolean is_waving;
    double wave_timer;
    gboolean is_turbo;
    double turbo_timer;
    
    // Speech bubble state
    gboolean show_dialog;
    double dialog_timer;
    char dialog_text[128];
    int quote_idx;
} alien_app_t;

static alien_app_t g_alien;

static const char *g_quotes[] = {
    "🛸 SOLARIS OS // ALL SYSTEMS NOMINAL",
    "⚡ 16 PLASMA CORES FIRING AT MAX SPEED!",
    "🔋 POWER CELL AT MAXIMUM CHARGE!",
    "🚀 WARP DRIVE READY FOR INTERSTELLAR JUMP",
    "👽 ARCH LINUX KERNEL RUNNING SILKY SMOOTH",
    "✨ GLAD TO FLY WITH YOU, CAPTAIN!",
    "👾 COMPUTING QUANTUM HYPERSPACE VECTORS..."
};

static void trigger_dialog(const char *custom_msg) {
    g_alien.show_dialog = TRUE;
    g_alien.dialog_timer = 4.5; // Show for 4.5s
    g_alien.is_waving = TRUE;
    g_alien.wave_timer = 2.0;

    if (custom_msg) {
        snprintf(g_alien.dialog_text, sizeof(g_alien.dialog_text), "%s", custom_msg);
    } else {
        int n_quotes = sizeof(g_quotes) / sizeof(g_quotes[0]);
        snprintf(g_alien.dialog_text, sizeof(g_alien.dialog_text), "%s", g_quotes[g_alien.quote_idx % n_quotes]);
        g_alien.quote_idx++;
    }
}

static void on_sigusr1(int signum) {
    (void)signum;
    trigger_dialog("🛸 IGNITION VERIFIED // WELCOME ABOARD!");
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    (void)user_data;

    if (event->button == GDK_BUTTON_PRIMARY) {
        // Left click: Talk, wave & give telemetry update
        char live_status[128];
        snprintf(live_status, sizeof(live_status), "CPU: %.1f GHz (%.0f%%) | RAM: %.1fG | BAT: %.0f%%",
                 g_alien.cpu.freq_ghz, g_alien.cpu.usage_pct, g_alien.mem.used_gb, g_alien.pwr.bat_pct);
        trigger_dialog(live_status);
        
        // Play subtle click chime
        g_spawn_command_line_async("canberra-gtk-play -i bell 2>/dev/null || true", NULL);
        return TRUE;
    } else if (event->button == GDK_BUTTON_SECONDARY) {
        // Right click: Overclock turbo typing mode
        g_alien.is_turbo = TRUE;
        g_alien.turbo_timer = 3.0;
        trigger_dialog("⚡ TURBO OVERCLOCK ENGAGED! COMPILING...");
        return TRUE;
    } else if (event->button == GDK_BUTTON_MIDDLE) {
        // Middle click: Launch Kitty terminal
        g_spawn_command_line_async("kitty", NULL);
        return TRUE;
    }
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    (void)widget;
    (void)user_data;
    g_alien.mouse_x = event->x;
    g_alien.mouse_y = event->y;
    g_alien.mouse_inside = TRUE;
    return FALSE;
}

static gboolean on_leave_notify(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
    (void)widget;
    (void)event;
    (void)user_data;
    g_alien.mouse_inside = FALSE;
    return FALSE;
}

static void draw_dialog_bubble(cairo_t *cr, double cx, double cy, const char *text, double alpha) {
    if (alpha <= 0.001) return;
    cairo_save(cr);

    double bw = 320.0;
    double bh = 48.0;
    double bx = cx - bw / 2.0;
    double by = cy - 130.0;
    double chamfer = 6.0;

    // Outer glow
    cairo_pattern_t *glow = cairo_pattern_create_radial(cx, by + bh / 2.0, 10, cx, by + bh / 2.0, bw / 2.0 + 20);
    cairo_pattern_add_color_stop_rgba(glow, 0.0, 0.0, 0.95, 0.85, 0.25 * alpha);
    cairo_pattern_add_color_stop_rgba(glow, 1.0, 0.0, 0.95, 0.85, 0.0);
    cairo_set_source(cr, glow);
    cairo_rectangle(cr, bx - 20, by - 20, bw + 40, bh + 50);
    cairo_fill(cr);
    cairo_pattern_destroy(glow);

    // Beveled bubble body
    cairo_move_to(cr, bx + chamfer, by);
    cairo_line_to(cr, bx + bw - chamfer, by);
    cairo_line_to(cr, bx + bw, by + chamfer);
    cairo_line_to(cr, bx + bw, by + bh - chamfer);
    cairo_line_to(cr, bx + bw - chamfer, by + bh);
    // Speech indicator pointer
    cairo_line_to(cr, cx + 12, by + bh);
    cairo_line_to(cr, cx, by + bh + 14);
    cairo_line_to(cr, cx - 12, by + bh);
    cairo_line_to(cr, bx + chamfer, by + bh);
    cairo_line_to(cr, bx, by + bh - chamfer);
    cairo_line_to(cr, bx, by + chamfer);
    cairo_close_path(cr);

    // Dark graphite background
    cairo_pattern_t *bpat = cairo_pattern_create_linear(bx, by, bx, by + bh);
    cairo_pattern_add_color_stop_rgba(bpat, 0.0, 0.05, 0.09, 0.14, 0.96 * alpha);
    cairo_pattern_add_color_stop_rgba(bpat, 1.0, 0.02, 0.04, 0.07, 0.98 * alpha);
    cairo_set_source(cr, bpat);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(bpat);

    // Neon border
    cairo_set_source_rgba(cr, 0.0, 0.95, 0.70, 0.90 * alpha);
    cairo_set_line_width(cr, 1.4);
    cairo_stroke(cr);

    // Text readout
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 10.0);
    cairo_set_source_rgba(cr, 0.90, 0.98, 1.0, 0.95 * alpha);

    cairo_text_extents_t ext;
    cairo_text_extents(cr, text, &ext);
    cairo_move_to(cr, cx - ext.width / 2.0, by + bh / 2.0 + 4.0);
    cairo_show_text(cr, text);

    cairo_restore(cr);
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    (void)widget;
    (void)user_data;

    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    // Clear background to fully transparent
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);

    double t = g_alien.anim_time;
    double cx = width / 2.0;
    double cy = height / 2.0 + 20.0;

    // Mouse tracking eye look offset
    double look_dx = 0.0;
    double look_dy = 0.0;
    if (g_alien.mouse_inside) {
        look_dx = (g_alien.mouse_x - cx) / (width / 2.0) * 4.0;
        look_dy = (g_alien.mouse_y - cy) / (height / 2.0) * 3.0;
    }

    // Breathing & typing physics
    double speed_mult = g_alien.is_turbo ? 2.5 : 1.0;
    double breath = sin(t * 3.0) * 2.0;
    double type_bob = sin(t * 22.0 * speed_mult) * 1.5;
    double head_y = cy - 50.0 + breath + type_bob;
    double head_x = cx;

    gboolean is_waving = g_alien.is_waving;
    double wave_angle = is_waving ? sin(t * 14.0) * 0.35 : 0.0;

    // 1. Desk Base
    double desk_w = 260.0;
    double desk_h = 24.0;
    double desk_x = cx - desk_w / 2.0;
    double desk_y = cy + 70.0;

    // Glowing Desk Plate
    cairo_pattern_t *desk_pat = cairo_pattern_create_linear(desk_x, desk_y, desk_x, desk_y + desk_h);
    cairo_pattern_add_color_stop_rgba(desk_pat, 0.0, 0.12, 0.18, 0.26, 0.90);
    cairo_pattern_add_color_stop_rgba(desk_pat, 1.0, 0.04, 0.07, 0.11, 0.95);
    cairo_set_source(cr, desk_pat);
    cairo_rectangle(cr, desk_x, desk_y, desk_w, desk_h);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(desk_pat);

    cairo_set_source_rgba(cr, 0.0, 0.95, 0.70, 0.75);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);

    // 2. Alien Body (Cozy Hoodie)
    double body_y = head_y + 65.0;
    cairo_pattern_t *hoodie_pat = cairo_pattern_create_linear(cx, body_y, cx, body_y + 80);
    cairo_pattern_add_color_stop_rgba(hoodie_pat, 0.0, 0.10, 0.16, 0.24, 0.98);
    cairo_pattern_add_color_stop_rgba(hoodie_pat, 1.0, 0.04, 0.08, 0.13, 0.98);
    cairo_set_source(cr, hoodie_pat);

    cairo_move_to(cr, cx - 45, body_y);
    cairo_curve_to(cr, cx - 75, body_y + 25, cx - 80, body_y + 70, cx - 65, body_y + 75);
    cairo_line_to(cr, cx + 65, body_y + 75);
    cairo_curve_to(cr, cx + 80, body_y + 70, cx + 75, body_y + 25, cx + 45, body_y);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_pattern_destroy(hoodie_pat);

    // Glowing Collar & Badge
    cairo_set_source_rgba(cr, 0.0, 0.90, 0.65, 0.85);
    cairo_set_line_width(cr, 2.0);
    cairo_move_to(cr, cx - 30, body_y + 6);
    cairo_curve_to(cr, cx, body_y + 30, cx, body_y + 30, cx + 30, body_y + 6);
    cairo_stroke(cr);

    // 3. Alien Arms & Hands
    double left_hand_x = cx - 50.0 + sin(t * 26.0 * speed_mult) * 8.0;
    double left_hand_y = cy + 60.0 + cos(t * 28.0 * speed_mult) * 3.0;

    cairo_set_source_rgba(cr, 0.10, 0.16, 0.24, 1.0);
    cairo_set_line_width(cr, 16.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr, cx - 60, body_y + 25);
    cairo_curve_to(cr, cx - 75, body_y + 50, cx - 65, left_hand_y, left_hand_x, left_hand_y);
    cairo_stroke(cr);

    // Left Paw
    cairo_set_source_rgba(cr, 0.22, 0.85, 0.38, 1.0);
    cairo_arc(cr, left_hand_x, left_hand_y, 8.0, 0, 2 * M_PI);
    cairo_fill(cr);

    // Right Arm
    double right_hand_x, right_hand_y;
    if (is_waving) {
        right_hand_x = cx + 85.0 + sin(t * 14.0) * 12.0;
        right_hand_y = head_y - 20.0 + cos(t * 14.0) * 6.0;

        cairo_set_source_rgba(cr, 0.10, 0.16, 0.24, 1.0);
        cairo_set_line_width(cr, 16.0);
        cairo_move_to(cr, cx + 60, body_y + 25);
        cairo_curve_to(cr, cx + 85, body_y - 5, cx + 95, right_hand_y + 20, right_hand_x, right_hand_y);
        cairo_stroke(cr);

        cairo_save(cr);
        cairo_translate(cr, right_hand_x, right_hand_y);
        cairo_rotate(cr, wave_angle);
        cairo_set_source_rgba(cr, 0.22, 0.85, 0.38, 1.0);
        cairo_arc(cr, 0, 0, 9.0, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_arc(cr, -5, -6, 4.5, 0, 2 * M_PI);
        cairo_arc(cr, 0, -8, 4.5, 0, 2 * M_PI);
        cairo_arc(cr, 5, -6, 4.5, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_restore(cr);
    } else {
        right_hand_x = cx + 50.0 + cos(t * 24.0 * speed_mult) * 8.0;
        right_hand_y = cy + 60.0 + sin(t * 30.0 * speed_mult) * 3.0;

        cairo_set_source_rgba(cr, 0.10, 0.16, 0.24, 1.0);
        cairo_set_line_width(cr, 16.0);
        cairo_move_to(cr, cx + 60, body_y + 25);
        cairo_curve_to(cr, cx + 75, body_y + 50, cx + 65, right_hand_y, right_hand_x, right_hand_y);
        cairo_stroke(cr);

        cairo_set_source_rgba(cr, 0.22, 0.85, 0.38, 1.0);
        cairo_arc(cr, right_hand_x, right_hand_y, 8.0, 0, 2 * M_PI);
        cairo_fill(cr);
    }

    // 4. Alien Head
    cairo_save(cr);
    cairo_pattern_t *alien_skin = cairo_pattern_create_radial(head_x - 10, head_y - 18, 8, head_x, head_y, 60);
    cairo_pattern_add_color_stop_rgb(alien_skin, 0.0, 0.32, 0.92, 0.44); // Bright Alien Lime Green
    cairo_pattern_add_color_stop_rgb(alien_skin, 0.6, 0.20, 0.80, 0.34);
    cairo_pattern_add_color_stop_rgb(alien_skin, 1.0, 0.12, 0.58, 0.24);
    cairo_set_source(cr, alien_skin);

    cairo_move_to(cr, head_x, head_y - 55);
    cairo_curve_to(cr, head_x + 48, head_y - 55, head_x + 62, head_y - 20, head_x + 62, head_y + 5);
    cairo_curve_to(cr, head_x + 62, head_y + 35, head_x + 32, head_y + 58, head_x, head_y + 58);
    cairo_curve_to(cr, head_x - 32, head_y + 58, head_x - 62, head_y + 35, head_x - 62, head_y + 5);
    cairo_curve_to(cr, head_x - 62, head_y - 20, head_x - 48, head_y - 55, head_x, head_y - 55);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(alien_skin);

    cairo_set_source_rgba(cr, 0.08, 0.45, 0.18, 0.60);
    cairo_set_line_width(cr, 2.0);
    cairo_stroke(cr);
    cairo_restore(cr);

    // 5. Antennae
    double ant_sway = sin(t * 5.0) * 6.0;
    // Left
    cairo_set_source_rgba(cr, 0.18, 0.75, 0.30, 1.0);
    cairo_set_line_width(cr, 3.2);
    cairo_move_to(cr, head_x - 24, head_y - 48);
    cairo_curve_to(cr, head_x - 42, head_y - 80, head_x - 52 + ant_sway, head_y - 90, head_x - 56 + ant_sway, head_y - 108);
    cairo_stroke(cr);

    // Left Tip Glow
    cairo_set_source_rgba(cr, 0.0, 0.95, 0.85, 0.95);
    cairo_arc(cr, head_x - 56 + ant_sway, head_y - 108, 5.0, 0, 2 * M_PI);
    cairo_fill(cr);

    // Right
    cairo_set_source_rgba(cr, 0.18, 0.75, 0.30, 1.0);
    cairo_move_to(cr, head_x + 24, head_y - 48);
    cairo_curve_to(cr, head_x + 42, head_y - 80, head_x + 52 - ant_sway, head_y - 90, head_x + 56 - ant_sway, head_y - 108);
    cairo_stroke(cr);

    // Right Tip Glow
    cairo_set_source_rgba(cr, 0.0, 0.95, 0.85, 0.95);
    cairo_arc(cr, head_x + 56 - ant_sway, head_y - 108, 5.0, 0, 2 * M_PI);
    cairo_fill(cr);

    // 6. Eyes (Big Glossy Eyes + Blinking)
    double blink_phase = fmod(t, 2.8);
    double eye_scale_y = 1.0;
    if (blink_phase > 2.62) {
        eye_scale_y = (blink_phase < 2.71) ? (1.0 - (blink_phase - 2.62) / 0.09) : ((blink_phase - 2.71) / 0.09);
        eye_scale_y = eye_scale_y < 0.05 ? 0.05 : eye_scale_y;
    }

    double eye_spacing = 28.0;
    double eye_y = head_y - 4.0;
    double eye_rx = 16.0;
    double eye_ry = 24.0 * eye_scale_y;

    for (int side = -1; side <= 1; side += 2) {
        double ex = head_x + side * eye_spacing;

        cairo_save(cr);
        cairo_translate(cr, ex, eye_y);
        cairo_rotate(cr, side * 0.12);

        // Black eyeball
        cairo_pattern_t *eye_pat = cairo_pattern_create_linear(0, -eye_ry, 0, eye_ry);
        cairo_pattern_add_color_stop_rgb(eye_pat, 0.0, 0.03, 0.05, 0.08);
        cairo_pattern_add_color_stop_rgb(eye_pat, 1.0, 0.0, 0.35, 0.35);
        cairo_set_source(cr, eye_pat);
        cairo_scale(cr, 1.0, eye_ry / 24.0);
        cairo_arc(cr, 0, 0, eye_rx, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_pattern_destroy(eye_pat);

        // White Sparkles + Look Direction
        if (eye_scale_y > 0.3) {
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
            cairo_arc(cr, -4 + look_dx, -8 + look_dy, 4.8, 0, 2 * M_PI);
            cairo_fill(cr);
            cairo_arc(cr, 5 + look_dx, 4 + look_dy, 2.5, 0, 2 * M_PI);
            cairo_fill(cr);
        }
        cairo_restore(cr);
    }

    // Mouth
    cairo_set_source_rgba(cr, 0.08, 0.40, 0.18, 0.90);
    cairo_set_line_width(cr, 2.0);
    if (is_waving) {
        cairo_new_sub_path(cr);
        cairo_arc(cr, head_x, eye_y + 30.0, 8.0, 0.1 * M_PI, 0.9 * M_PI);
        cairo_set_source_rgba(cr, 0.90, 0.30, 0.45, 0.95);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 0.08, 0.40, 0.18, 0.95);
        cairo_stroke(cr);
    } else {
        cairo_move_to(cr, head_x - 8, eye_y + 34.0);
        cairo_curve_to(cr, head_x, eye_y + 38.0, head_x + 6, eye_y + 37.0, head_x + 9, eye_y + 33.0);
        cairo_stroke(cr);
    }

    // 7. Glowing Laptop
    double lap_cx = cx;
    double lap_cy = cy + 70.0;
    double lap_w = 180.0;
    double lap_h = 12.0;

    cairo_set_source_rgba(cr, 0.08, 0.12, 0.18, 0.98);
    cairo_rectangle(cr, lap_cx - lap_w / 2.0, lap_cy, lap_w, lap_h);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.0, 0.90, 0.80, 0.85);
    cairo_set_line_width(cr, 1.2);
    cairo_stroke(cr);

    // Screen
    double scr_w = 160.0;
    double scr_h = 100.0;
    double scr_top_y = lap_cy - scr_h;

    cairo_set_source_rgba(cr, 0.05, 0.08, 0.12, 0.98);
    cairo_rectangle(cr, lap_cx - scr_w / 2.0, scr_top_y, scr_w, scr_h);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.0, 0.95, 0.70, 0.90);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);

    // Screen Code Glow
    cairo_pattern_t *screen_pat = cairo_pattern_create_linear(lap_cx, scr_top_y, lap_cx, lap_cy);
    cairo_pattern_add_color_stop_rgba(screen_pat, 0.0, 0.02, 0.18, 0.28, 0.95);
    cairo_pattern_add_color_stop_rgba(screen_pat, 1.0, 0.0, 0.45, 0.55, 0.90);
    cairo_set_source(cr, screen_pat);
    cairo_rectangle(cr, lap_cx - scr_w / 2.0 + 5, scr_top_y + 5, scr_w - 10, scr_h - 10);
    cairo_fill(cr);
    cairo_pattern_destroy(screen_pat);

    // Animated Code Lines
    cairo_set_line_width(cr, 1.5);
    for (int l = 0; l < 7; l++) {
        double ly = scr_top_y + 14.0 + l * 11.0;
        double lw = 30.0 + fmod(l * 37.0 + t * 40.0, 80.0);
        cairo_set_source_rgba(cr, (l % 2 == 0) ? 0.0 : 0.4, 0.95, (l % 2 == 0) ? 0.65 : 1.0, 0.85);
        cairo_move_to(cr, lap_cx - scr_w / 2.0 + 12, ly);
        cairo_line_to(cr, lap_cx - scr_w / 2.0 + 12 + lw, ly);
        cairo_stroke(cr);
    }

    // Dynamic Screen Light on Face
    cairo_pattern_t *fglow = cairo_pattern_create_radial(lap_cx, lap_cy - 20, 20, lap_cx, head_y, 140);
    cairo_pattern_add_color_stop_rgba(fglow, 0.0, 0.0, 0.90, 0.80, 0.25);
    cairo_pattern_add_color_stop_rgba(fglow, 1.0, 0.0, 0.0, 0.0, 0.0);
    cairo_set_source(cr, fglow);
    cairo_arc(cr, lap_cx, head_y, 130, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(fglow);

    // 8. Dialog speech bubble
    if (g_alien.show_dialog && g_alien.dialog_timer > 0.0) {
        double alpha = g_alien.dialog_timer > 0.5 ? 1.0 : (g_alien.dialog_timer / 0.5);
        draw_dialog_bubble(cr, cx, cy, g_alien.dialog_text, alpha);
    }

    return FALSE;
}

static gboolean on_anim_tick(gpointer user_data) {
    (void)user_data;
    g_alien.anim_time += 0.033; // ~30 FPS

    if (g_alien.wave_timer > 0.0) {
        g_alien.wave_timer -= 0.033;
        if (g_alien.wave_timer <= 0.0) g_alien.is_waving = FALSE;
    }

    if (g_alien.turbo_timer > 0.0) {
        g_alien.turbo_timer -= 0.033;
        if (g_alien.turbo_timer <= 0.0) g_alien.is_turbo = FALSE;
    }

    if (g_alien.dialog_timer > 0.0) {
        g_alien.dialog_timer -= 0.033;
        if (g_alien.dialog_timer <= 0.0) g_alien.show_dialog = FALSE;
    }

    if (g_alien.drawing_area) {
        gtk_widget_queue_draw(g_alien.drawing_area);
    }
    return G_SOURCE_CONTINUE;
}

static gboolean update_telemetry_tick(gpointer user_data) {
    (void)user_data;
    update_cpu_telemetry(&g_alien.cpu);
    update_mem_telemetry(&g_alien.mem);
    update_power_telemetry(&g_alien.pwr);
    return G_SOURCE_CONTINUE;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    solaris_load_css();

    memset(&g_alien, 0, sizeof(g_alien));
    init_telemetry();

    update_cpu_telemetry(&g_alien.cpu);
    update_mem_telemetry(&g_alien.mem);
    update_power_telemetry(&g_alien.pwr);

    // Setup signal handler for USR1 (solaris-bg wake)
    struct sigaction sa;
    sa.sa_handler = on_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    g_alien.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    // Anchor to Center-Top of desktop (above wallpaper, beneath application windows)
    solaris_setup_layer_window(g_alien.window, "solaris-bg-alien",
                               ALIEN_W, ALIEN_H,
                               TRUE, FALSE, FALSE, FALSE, // Anchor Top
                               48, 0, 0, 0);

    g_alien.drawing_area = gtk_drawing_area_new();
    gtk_widget_add_events(g_alien.drawing_area,
                          GDK_BUTTON_PRESS_MASK |
                          GDK_POINTER_MOTION_MASK |
                          GDK_ENTER_NOTIFY_MASK |
                          GDK_LEAVE_NOTIFY_MASK);

    g_signal_connect(g_alien.drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(g_alien.drawing_area, "button-press-event", G_CALLBACK(on_button_press), NULL);
    g_signal_connect(g_alien.drawing_area, "motion-notify-event", G_CALLBACK(on_motion_notify), NULL);
    g_signal_connect(g_alien.drawing_area, "leave-notify-event", G_CALLBACK(on_leave_notify), NULL);

    gtk_container_add(GTK_CONTAINER(g_alien.window), g_alien.drawing_area);

    g_alien.anim_timer = g_timeout_add(33, on_anim_tick, NULL);       // 30 FPS
    g_alien.telemetry_timer = g_timeout_add(1000, update_telemetry_tick, NULL);

    gtk_widget_show_all(g_alien.window);
    gtk_main();
    return 0;
}
