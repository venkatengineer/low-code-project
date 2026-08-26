#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <cairo.h>

#define WIDTH 1920
#define HEIGHT 1080
#define TOTAL_TIME 7.5
#define PI 3.14159265358979323846

static double clamp(double v, double min_v, double max_v) {
    if (v < min_v) return min_v;
    if (v > max_v) return max_v;
    return v;
}

static double ease_in_out(double t) {
    return t < 0.5 ? 2.0 * t * t : -1.0 + (4.0 - 2.0 * t) * t;
}

static double ease_out_quad(double t) {
    return t * (2.0 - t);
}

static double ease_out_cubic(double t) {
    double f = t - 1.0;
    return f * f * f + 1.0;
}

static double ease_out_back(double t) {
    double c1 = 1.70158;
    double c3 = c1 + 1.0;
    double f = t - 1.0;
    return 1.0 + c3 * f * f * f + c1 * f * f;
}

static double hash(int n) {
    n = (n << 13) ^ n;
    return (1.0 - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0) * 0.5 + 0.5;
}

// -----------------------------------------------------------------------------
// 1. Background & Sci-Fi Ambient Environment
// -----------------------------------------------------------------------------
void draw_background(cairo_t *cr, double t) {
    // Deep dark indigo-graphite gradient
    cairo_pattern_t *bg_pat = cairo_pattern_create_radial(WIDTH * 0.5, HEIGHT * 0.45, 100, WIDTH * 0.5, HEIGHT * 0.5, 950);
    cairo_pattern_add_color_stop_rgb(bg_pat, 0.0, 0.04, 0.08, 0.14);
    cairo_pattern_add_color_stop_rgb(bg_pat, 0.5, 0.02, 0.04, 0.08);
    cairo_pattern_add_color_stop_rgb(bg_pat, 1.0, 0.01, 0.02, 0.04);
    cairo_set_source(cr, bg_pat);
    cairo_paint(cr);
    cairo_pattern_destroy(bg_pat);

    // Subtle sci-fi hexagonal floor / wall grid lines
    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgba(cr, 0.0, 0.90, 0.60, 0.04);
    for (int x = 0; x <= WIDTH; x += 60) {
        cairo_move_to(cr, x, 0);
        cairo_line_to(cr, x, HEIGHT);
    }
    for (int y = 0; y <= HEIGHT; y += 60) {
        cairo_move_to(cr, 0, y);
        cairo_line_to(cr, WIDTH, y);
    }
    cairo_stroke(cr);

    // Distant soft floating ambient dust motes / glowing particles
    for (int i = 0; i < 45; i++) {
        double px = fmod(hash(i * 7 + 1) * WIDTH + t * (10.0 + hash(i * 3) * 15.0), WIDTH);
        double py = fmod(hash(i * 13 + 5) * HEIGHT - t * (8.0 + hash(i * 5) * 12.0) + HEIGHT, HEIGHT);
        double size = 1.0 + hash(i * 19) * 2.5;
        double alpha = (0.2 + 0.5 * sin(t * 2.0 + hash(i * 23) * 6.28)) * 0.4;
        
        cairo_set_source_rgba(cr, 0.0, 0.95, 0.70, alpha);
        cairo_arc(cr, px, py, size, 0, 2 * PI);
        cairo_fill(cr);
    }

    // Soft neon rim lighting at the bottom
    cairo_pattern_t *floor_glow = cairo_pattern_create_linear(0, HEIGHT * 0.75, 0, HEIGHT);
    cairo_pattern_add_color_stop_rgba(floor_glow, 0.0, 0.0, 0.90, 0.50, 0.0);
    cairo_pattern_add_color_stop_rgba(floor_glow, 0.7, 0.0, 0.85, 0.75, 0.06);
    cairo_pattern_add_color_stop_rgba(floor_glow, 1.0, 0.0, 0.40, 0.60, 0.12);
    cairo_set_source(cr, floor_glow);
    cairo_rectangle(cr, 0, HEIGHT * 0.75, WIDTH, HEIGHT * 0.25);
    cairo_fill(cr);
    cairo_pattern_destroy(floor_glow);
}

// -----------------------------------------------------------------------------
// 2. Futuristic Floating Desk & Chair
// -----------------------------------------------------------------------------
void draw_desk_and_chair(cairo_t *cr, double cx, double cy, double t) {
    // Desk Surface (Sleek aerodynamic bevel)
    double desk_w = 780.0;
    double desk_h = 42.0;
    double desk_x = cx - desk_w / 2.0;
    double desk_y = cy + 130.0;

    // Ambient glow under desk
    cairo_pattern_t *desk_glow = cairo_pattern_create_radial(cx, desk_y + desk_h, 20, cx, desk_y + desk_h, 380);
    cairo_pattern_add_color_stop_rgba(desk_glow, 0.0, 0.0, 0.95, 0.65, 0.25);
    cairo_pattern_add_color_stop_rgba(desk_glow, 1.0, 0.0, 0.95, 0.65, 0.0);
    cairo_set_source(cr, desk_glow);
    cairo_rectangle(cr, desk_x - 100, desk_y, desk_w + 200, 200);
    cairo_fill(cr);
    cairo_pattern_destroy(desk_glow);

    // Floating magnetic pedestal base
    cairo_set_source_rgba(cr, 0.06, 0.10, 0.16, 0.95);
    cairo_move_to(cr, cx - 140, desk_y + desk_h);
    cairo_line_to(cr, cx - 90, desk_y + 190);
    cairo_line_to(cr, cx + 90, desk_y + 190);
    cairo_line_to(cr, cx + 140, desk_y + desk_h);
    cairo_close_path(cr);
    cairo_fill(cr);

    // Base emitter light
    cairo_set_source_rgba(cr, 0.0, 0.90, 0.80, 0.85);
    cairo_rectangle(cr, cx - 80, desk_y + 185, 160, 4);
    cairo_fill(cr);

    // Desk Top Plate (Dark carbon metallic with cyan chamfer)
    cairo_pattern_t *plate_pat = cairo_pattern_create_linear(desk_x, desk_y, desk_x, desk_y + desk_h);
    cairo_pattern_add_color_stop_rgb(plate_pat, 0.0, 0.14, 0.20, 0.28);
    cairo_pattern_add_color_stop_rgb(plate_pat, 0.5, 0.08, 0.12, 0.18);
    cairo_pattern_add_color_stop_rgb(plate_pat, 1.0, 0.04, 0.07, 0.11);
    cairo_set_source(cr, plate_pat);

    cairo_move_to(cr, desk_x + 30, desk_y);
    cairo_line_to(cr, desk_x + desk_w - 30, desk_y);
    cairo_line_to(cr, desk_x + desk_w, desk_y + desk_h);
    cairo_line_to(cr, desk_x, desk_y + desk_h);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(plate_pat);

    // Desk Neon Trim
    cairo_set_source_rgba(cr, 0.0, 0.95, 0.70, 0.80);
    cairo_set_line_width(cr, 2.0);
    cairo_stroke(cr);

    // Glowing energy strip on desk edge
    cairo_set_source_rgba(cr, 0.0, 0.90, 1.0, 0.90);
    cairo_move_to(cr, desk_x + 50, desk_y + desk_h - 4);
    cairo_line_to(cr, desk_x + desk_w - 50, desk_y + desk_h - 4);
    cairo_set_line_width(cr, 2.5);
    cairo_stroke(cr);

    // High-back cockpit pilot chair behind alien
    double chair_cx = cx;
    double chair_cy = cy - 20.0;
    cairo_set_source_rgba(cr, 0.05, 0.08, 0.12, 0.95);
    cairo_arc(cr, chair_cx, chair_cy - 120, 140, PI, 2 * PI);
    cairo_line_to(cr, chair_cx + 150, chair_cy + 120);
    cairo_line_to(cr, chair_cx - 150, chair_cy + 120);
    cairo_close_path(cr);
    cairo_fill(cr);

    // Chair neon headrest contour
    cairo_set_source_rgba(cr, 0.50, 0.20, 0.95, 0.60);
    cairo_set_line_width(cr, 3.0);
    cairo_arc(cr, chair_cx, chair_cy - 120, 136, PI + 0.3, 2 * PI - 0.3);
    cairo_stroke(cr);
}

// -----------------------------------------------------------------------------
// 3. Cute 2D Cartoon Alien ("Pip") with Laptop
// -----------------------------------------------------------------------------
void draw_alien_and_laptop(cairo_t *cr, double cx, double cy, double t) {
    // Breathing & typing head bob physics
    double breath = sin(t * 3.2) * 2.5;
    double type_bob = sin(t * 22.0) * 1.5;
    double head_y = cy - 65.0 + breath + type_bob;
    double head_x = cx;

    // Timeline phases:
    // 0.0 - 5.2s: Typing away passionately
    // 5.2 - 6.5s: Look up, smile, wave hand happily
    // 6.5 - 7.5s: System Ready thumbs up / celebration
    int is_waving = (t >= 5.3 && t < 7.5);
    double wave_angle = is_waving ? sin((t - 5.3) * 12.0) * 0.35 : 0.0;

    // --- A. Alien Body & Cozy Hoodie ---
    double body_y = head_y + 85.0;
    cairo_pattern_t *hoodie_pat = cairo_pattern_create_linear(cx, body_y, cx, body_y + 120);
    cairo_pattern_add_color_stop_rgb(hoodie_pat, 0.0, 0.10, 0.16, 0.24);
    cairo_pattern_add_color_stop_rgb(hoodie_pat, 1.0, 0.05, 0.08, 0.13);
    cairo_set_source(cr, hoodie_pat);

    // Torso (cute rounded cartoon shape)
    cairo_move_to(cr, cx - 65, body_y);
    cairo_curve_to(cr, cx - 110, body_y + 40, cx - 120, body_y + 110, cx - 100, body_y + 120);
    cairo_line_to(cr, cx + 100, body_y + 120);
    cairo_curve_to(cr, cx + 120, body_y + 110, cx + 110, body_y + 40, cx + 65, body_y);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_pattern_destroy(hoodie_pat);

    // Hoodie glowing collar & chest emblem
    cairo_set_source_rgba(cr, 0.0, 0.90, 0.65, 0.85);
    cairo_set_line_width(cr, 2.5);
    cairo_move_to(cr, cx - 45, body_y + 10);
    cairo_curve_to(cr, cx, body_y + 45, cx, body_y + 45, cx + 45, body_y + 10);
    cairo_stroke(cr);

    // Glowing Alien Star Emblem on chest
    cairo_set_source_rgba(cr, 0.0, 0.95, 0.85, 0.95);
    cairo_arc(cr, cx, body_y + 55, 7.0, 0, 2 * PI);
    cairo_fill(cr);

    // --- B. Alien Arms & Hands ---
    // Left Arm (typing on keyboard)
    double left_hand_x = cx - 75.0 + sin(t * 26.0) * 12.0;
    double left_hand_y = cy + 105.0 + cos(t * 28.0) * 4.0;

    cairo_set_source_rgba(cr, 0.10, 0.16, 0.24, 1.0);
    cairo_set_line_width(cr, 22.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr, cx - 90, body_y + 35);
    cairo_curve_to(cr, cx - 110, body_y + 80, cx - 95, left_hand_y, left_hand_x, left_hand_y);
    cairo_stroke(cr);

    // Left Alien Hand (Cute 3-fingered green paw)
    cairo_set_source_rgba(cr, 0.22, 0.85, 0.38, 1.0);
    cairo_arc(cr, left_hand_x, left_hand_y, 11.0, 0, 2 * PI);
    cairo_fill(cr);
    cairo_arc(cr, left_hand_x - 7, left_hand_y + 3, 5.0, 0, 2 * PI);
    cairo_arc(cr, left_hand_x, left_hand_y + 6, 5.0, 0, 2 * PI);
    cairo_arc(cr, left_hand_x + 7, left_hand_y + 3, 5.0, 0, 2 * PI);
    cairo_fill(cr);

    // Right Arm (typing or waving)
    double right_hand_x, right_hand_y;
    if (is_waving) {
        // Waving high up with a cute wave!
        right_hand_x = cx + 130.0 + sin(t * 14.0) * 18.0;
        right_hand_y = head_y - 30.0 + cos(t * 14.0) * 8.0;

        cairo_set_source_rgba(cr, 0.10, 0.16, 0.24, 1.0);
        cairo_set_line_width(cr, 22.0);
        cairo_move_to(cr, cx + 90, body_y + 35);
        cairo_curve_to(cr, cx + 130, body_y - 10, cx + 140, right_hand_y + 30, right_hand_x, right_hand_y);
        cairo_stroke(cr);

        // Right waving paw
        cairo_save(cr);
        cairo_translate(cr, right_hand_x, right_hand_y);
        cairo_rotate(cr, wave_angle);
        cairo_set_source_rgba(cr, 0.22, 0.85, 0.38, 1.0);
        cairo_arc(cr, 0, 0, 12.0, 0, 2 * PI);
        cairo_fill(cr);
        cairo_arc(cr, -8, -8, 6.0, 0, 2 * PI);
        cairo_arc(cr, 0, -11, 6.0, 0, 2 * PI);
        cairo_arc(cr, 8, -8, 6.0, 0, 2 * PI);
        cairo_fill(cr);
        cairo_restore(cr);
    } else {
        // Right hand typing fast
        right_hand_x = cx + 75.0 + cos(t * 24.0) * 12.0;
        right_hand_y = cy + 105.0 + sin(t * 30.0) * 4.0;

        cairo_set_source_rgba(cr, 0.10, 0.16, 0.24, 1.0);
        cairo_set_line_width(cr, 22.0);
        cairo_move_to(cr, cx + 90, body_y + 35);
        cairo_curve_to(cr, cx + 110, body_y + 80, cx + 95, right_hand_y, right_hand_x, right_hand_y);
        cairo_stroke(cr);

        // Right typing paw
        cairo_set_source_rgba(cr, 0.22, 0.85, 0.38, 1.0);
        cairo_arc(cr, right_hand_x, right_hand_y, 11.0, 0, 2 * PI);
        cairo_fill(cr);
        cairo_arc(cr, right_hand_x - 7, right_hand_y + 3, 5.0, 0, 2 * PI);
        cairo_arc(cr, right_hand_x, right_hand_y + 6, 5.0, 0, 2 * PI);
        cairo_arc(cr, right_hand_x + 7, right_hand_y + 3, 5.0, 0, 2 * PI);
        cairo_fill(cr);
    }

    // Keyboard typing neon tap sparks!
    if (!is_waving) {
        cairo_set_source_rgba(cr, 0.0, 0.95, 0.80, 0.75);
        cairo_arc(cr, left_hand_x, left_hand_y + 8, 3.5, 0, 2 * PI);
        cairo_arc(cr, right_hand_x, right_hand_y + 8, 3.5, 0, 2 * PI);
        cairo_fill(cr);
    }

    // --- C. Alien Head (Smooth Pear Shape with Bright Vibrant Green) ---
    cairo_save(cr);
    cairo_pattern_t *alien_skin = cairo_pattern_create_radial(head_x - 15, head_y - 25, 10, head_x, head_y, 85);
    cairo_pattern_add_color_stop_rgb(alien_skin, 0.0, 0.32, 0.92, 0.44); // Bright Alien Lime Green
    cairo_pattern_add_color_stop_rgb(alien_skin, 0.6, 0.20, 0.80, 0.34); // Rich Leaf Green
    cairo_pattern_add_color_stop_rgb(alien_skin, 1.0, 0.12, 0.58, 0.24); // Shaded Deep Green
    cairo_set_source(cr, alien_skin);

    // Pear-shaped cartoon alien cranium
    cairo_move_to(cr, head_x, head_y - 75);
    // Top skull curve
    cairo_curve_to(cr, head_x + 65, head_y - 75, head_x + 85, head_y - 30, head_x + 85, head_y + 5);
    // Right cheek & cute chin
    cairo_curve_to(cr, head_x + 85, head_y + 45, head_x + 45, head_y + 75, head_x, head_y + 75);
    // Left cheek & chin
    cairo_curve_to(cr, head_x - 45, head_y + 75, head_x - 85, head_y + 45, head_x - 85, head_y + 5);
    // Top left skull curve
    cairo_curve_to(cr, head_x - 85, head_y - 30, head_x - 65, head_y - 75, head_x, head_y - 75);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(alien_skin);

    // Cute rim outline
    cairo_set_source_rgba(cr, 0.08, 0.45, 0.18, 0.65);
    cairo_set_line_width(cr, 2.5);
    cairo_stroke(cr);
    cairo_restore(cr);

    // --- D. Antennae with Glowing Energy Orbs ---
    double ant_sway = sin(t * 6.0) * 8.0;
    // Left Antenna
    cairo_set_source_rgba(cr, 0.18, 0.75, 0.30, 1.0);
    cairo_set_line_width(cr, 4.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr, head_x - 32, head_y - 65);
    cairo_curve_to(cr, head_x - 55, head_y - 110, head_x - 70 + ant_sway, head_y - 120, head_x - 75 + ant_sway, head_y - 145);
    cairo_stroke(cr);

    // Left Antenna Glowing Tip
    cairo_pattern_t *ant_glow1 = cairo_pattern_create_radial(head_x - 75 + ant_sway, head_y - 145, 2, head_x - 75 + ant_sway, head_y - 145, 18);
    cairo_pattern_add_color_stop_rgba(ant_glow1, 0.0, 0.0, 0.98, 0.85, 0.95);
    cairo_pattern_add_color_stop_rgba(ant_glow1, 0.5, 0.0, 0.85, 0.60, 0.40);
    cairo_pattern_add_color_stop_rgba(ant_glow1, 1.0, 0.0, 0.85, 0.60, 0.0);
    cairo_set_source(cr, ant_glow1);
    cairo_arc(cr, head_x - 75 + ant_sway, head_y - 145, 16.0, 0, 2 * PI);
    cairo_fill(cr);
    cairo_pattern_destroy(ant_glow1);

    cairo_set_source_rgba(cr, 0.85, 1.0, 0.95, 1.0);
    cairo_arc(cr, head_x - 75 + ant_sway, head_y - 145, 6.0, 0, 2 * PI);
    cairo_fill(cr);

    // Right Antenna
    cairo_set_source_rgba(cr, 0.18, 0.75, 0.30, 1.0);
    cairo_set_line_width(cr, 4.0);
    cairo_move_to(cr, head_x + 32, head_y - 65);
    cairo_curve_to(cr, head_x + 55, head_y - 110, head_x + 70 - ant_sway, head_y - 120, head_x + 75 - ant_sway, head_y - 145);
    cairo_stroke(cr);

    // Right Antenna Glowing Tip
    cairo_pattern_t *ant_glow2 = cairo_pattern_create_radial(head_x + 75 - ant_sway, head_y - 145, 2, head_x + 75 - ant_sway, head_y - 145, 18);
    cairo_pattern_add_color_stop_rgba(ant_glow2, 0.0, 0.0, 0.98, 0.85, 0.95);
    cairo_pattern_add_color_stop_rgba(ant_glow2, 0.5, 0.0, 0.85, 0.60, 0.40);
    cairo_pattern_add_color_stop_rgba(ant_glow2, 1.0, 0.0, 0.85, 0.60, 0.0);
    cairo_set_source(cr, ant_glow2);
    cairo_arc(cr, head_x + 75 - ant_sway, head_y - 145, 16.0, 0, 2 * PI);
    cairo_fill(cr);
    cairo_pattern_destroy(ant_glow2);

    cairo_set_source_rgba(cr, 0.85, 1.0, 0.95, 1.0);
    cairo_arc(cr, head_x + 75 - ant_sway, head_y - 145, 6.0, 0, 2 * PI);
    cairo_fill(cr);

    // --- E. Big Expressive Glossy Cartoon Eyes ---
    // Eye Blinking Animation (blinks every ~2.5s)
    double blink_phase = fmod(t, 2.4);
    double eye_scale_y = 1.0;
    if (blink_phase > 2.22) {
        eye_scale_y = (blink_phase < 2.31) ? (1.0 - (blink_phase - 2.22) / 0.09) : ((blink_phase - 2.31) / 0.09);
        eye_scale_y = clamp(eye_scale_y, 0.05, 1.0);
    }

    double eye_spacing = 38.0;
    double eye_y = head_y - 5.0;
    double eye_rx = 22.0;
    double eye_ry = 32.0 * eye_scale_y;

    // Draw Left Eye & Right Eye
    for (int side = -1; side <= 1; side += 2) {
        double ex = head_x + side * eye_spacing;

        cairo_save(cr);
        cairo_translate(cr, ex, eye_y);
        cairo_rotate(cr, side * 0.12); // Cute subtle anime/cartoon outward tilt

        // Eye socket / shadow
        cairo_set_source_rgba(cr, 0.08, 0.45, 0.20, 0.45);
        cairo_scale(cr, 1.0, eye_ry / 32.0);
        cairo_arc(cr, 0, 0, eye_rx + 2.0, 0, 2 * PI);
        cairo_fill(cr);

        // Deep glossy black eyeball
        cairo_pattern_t *eye_pat = cairo_pattern_create_linear(0, -eye_ry, 0, eye_ry);
        cairo_pattern_add_color_stop_rgb(eye_pat, 0.0, 0.03, 0.05, 0.08);
        cairo_pattern_add_color_stop_rgb(eye_pat, 0.7, 0.06, 0.12, 0.18);
        cairo_pattern_add_color_stop_rgb(eye_pat, 1.0, 0.0, 0.35, 0.35); // Cyan screen reflection at bottom!
        cairo_set_source(cr, eye_pat);
        cairo_arc(cr, 0, 0, eye_rx, 0, 2 * PI);
        cairo_fill(cr);
        cairo_pattern_destroy(eye_pat);

        // Big Shiny White Catchlights / Highlights
        if (eye_scale_y > 0.3) {
            // Main upper sparkle
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
            cairo_arc(cr, -5, -10, 6.5, 0, 2 * PI);
            cairo_fill(cr);
            // Secondary cute baby sparkle
            cairo_arc(cr, 7, 6, 3.5, 0, 2 * PI);
            cairo_fill(cr);
        }
        cairo_restore(cr);
    }

    // --- F. Cute Small Mouth / Smile ---
    cairo_set_source_rgba(cr, 0.08, 0.40, 0.18, 0.90);
    cairo_set_line_width(cr, 2.5);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    if (is_waving) {
        // Happy open smiling mouth (happy kitty mouth '3' or cheerful grin)
        cairo_new_sub_path(cr);
        cairo_arc(cr, head_x, eye_y + 40.0, 12.0, 0.1 * PI, 0.9 * PI);
        cairo_set_source_rgba(cr, 0.90, 0.30, 0.45, 0.95); // cute pink tongue inside
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 0.08, 0.40, 0.18, 0.95);
        cairo_stroke(cr);
    } else {
        // Cute focused determined smirk / small smile
        cairo_move_to(cr, head_x - 10, eye_y + 44.0);
        cairo_curve_to(cr, head_x, eye_y + 49.0, head_x + 8, eye_y + 48.0, head_x + 12, eye_y + 43.0);
        cairo_stroke(cr);
    }

    // Cute Pink Cheek Blush
    cairo_pattern_t *blush_pat = cairo_pattern_create_radial(head_x - 48, eye_y + 24, 2, head_x - 48, eye_y + 24, 18);
    cairo_pattern_add_color_stop_rgba(blush_pat, 0.0, 0.95, 0.35, 0.45, 0.40);
    cairo_pattern_add_color_stop_rgba(blush_pat, 1.0, 0.95, 0.35, 0.45, 0.0);
    cairo_set_source(cr, blush_pat);
    cairo_arc(cr, head_x - 48, eye_y + 24, 16, 0, 2 * PI);
    cairo_fill(cr);
    cairo_pattern_destroy(blush_pat);

    cairo_pattern_t *blush_pat2 = cairo_pattern_create_radial(head_x + 48, eye_y + 24, 2, head_x + 48, eye_y + 24, 18);
    cairo_pattern_add_color_stop_rgba(blush_pat2, 0.0, 0.95, 0.35, 0.45, 0.40);
    cairo_pattern_add_color_stop_rgba(blush_pat2, 1.0, 0.95, 0.35, 0.45, 0.0);
    cairo_set_source(cr, blush_pat2);
    cairo_arc(cr, head_x + 48, eye_y + 24, 16, 0, 2 * PI);
    cairo_fill(cr);
    cairo_pattern_destroy(blush_pat2);

    // --- G. Futuristic Sci-Fi Laptop ---
    double lap_cx = cx;
    double lap_cy = cy + 120.0;
    double lap_w = 260.0;
    double lap_h = 16.0;

    // Laptop Base / Keyboard Deck
    cairo_set_source_rgba(cr, 0.08, 0.12, 0.18, 0.98);
    cairo_move_to(cr, lap_cx - lap_w * 0.44, lap_cy);
    cairo_line_to(cr, lap_cx + lap_w * 0.44, lap_cy);
    cairo_line_to(cr, lap_cx + lap_w * 0.50, lap_cy + lap_h);
    cairo_line_to(cr, lap_cx - lap_w * 0.50, lap_cy + lap_h);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);

    cairo_set_source_rgba(cr, 0.0, 0.90, 0.80, 0.85);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);

    // Glowing Keyboard Backlight Grid
    cairo_set_source_rgba(cr, 0.0, 0.95, 0.65, 0.65);
    for (int kx = -90; kx <= 90; kx += 18) {
        cairo_rectangle(cr, lap_cx + kx - 7, lap_cy + 3, 14, 8);
        cairo_fill(cr);
    }

    // Laptop Open Screen (Tilted upwards toward alien)
    double scr_w = 230.0;
    double scr_h = 145.0;
    double scr_top_y = lap_cy - scr_h;

    // Screen Outer Bezel
    cairo_set_source_rgba(cr, 0.05, 0.08, 0.12, 0.98);
    cairo_move_to(cr, lap_cx - scr_w * 0.46, scr_top_y);
    cairo_line_to(cr, lap_cx + scr_w * 0.46, scr_top_y);
    cairo_line_to(cr, lap_cx + scr_w * 0.44, lap_cy);
    cairo_line_to(cr, lap_cx - scr_w * 0.44, lap_cy);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);

    cairo_set_source_rgba(cr, 0.0, 0.95, 0.70, 0.90);
    cairo_set_line_width(cr, 2.0);
    cairo_stroke(cr);

    // Screen Display Active Glow
    cairo_pattern_t *screen_pat = cairo_pattern_create_linear(lap_cx, scr_top_y, lap_cx, lap_cy);
    cairo_pattern_add_color_stop_rgba(screen_pat, 0.0, 0.02, 0.18, 0.28, 0.95);
    cairo_pattern_add_color_stop_rgba(screen_pat, 0.8, 0.01, 0.08, 0.16, 0.95);
    cairo_pattern_add_color_stop_rgba(screen_pat, 1.0, 0.0, 0.45, 0.55, 0.90);
    cairo_set_source(cr, screen_pat);
    cairo_rectangle(cr, lap_cx - scr_w * 0.42, scr_top_y + 8, scr_w * 0.84, scr_h - 14);
    cairo_fill(cr);
    cairo_pattern_destroy(screen_pat);

    // Glowing Code Lines on Laptop Screen
    cairo_set_line_width(cr, 2.0);
    for (int l = 0; l < 9; l++) {
        double ly = scr_top_y + 20.0 + l * 12.0;
        double line_w = 40.0 + hash(l * 11 + (int)(t * 8.0)) * 90.0;
        double color_pick = hash(l * 5);
        if (color_pick > 0.6) {
            cairo_set_source_rgba(cr, 0.0, 0.95, 0.65, 0.85); // Emerald Green code
        } else if (color_pick > 0.3) {
            cairo_set_source_rgba(cr, 0.0, 0.85, 1.0, 0.85);  // Electric Cyan code
        } else {
            cairo_set_source_rgba(cr, 0.75, 0.40, 1.0, 0.85); // Purple accent code
        }
        cairo_move_to(cr, lap_cx - scr_w * 0.38, ly);
        cairo_line_to(cr, lap_cx - scr_w * 0.38 + line_w, ly);
        cairo_stroke(cr);
    }

    // Dynamic Screen Light Cast onto Alien's Face & Room
    cairo_pattern_t *face_glow = cairo_pattern_create_radial(lap_cx, lap_cy - 40, 30, lap_cx, head_y, 220);
    double screen_flicker = 0.22 + 0.06 * sin(t * 18.0);
    cairo_pattern_add_color_stop_rgba(face_glow, 0.0, 0.0, 0.90, 0.80, screen_flicker * 1.3);
    cairo_pattern_add_color_stop_rgba(face_glow, 0.5, 0.0, 0.80, 0.50, screen_flicker * 0.6);
    cairo_pattern_add_color_stop_rgba(face_glow, 1.0, 0.0, 0.0, 0.0, 0.0);
    cairo_set_source(cr, face_glow);
    cairo_arc(cr, lap_cx, head_y + 10, 180, 0, 2 * PI);
    cairo_fill(cr);
    cairo_pattern_destroy(face_glow);

    // --- H. Floating Holographic Matrix Code & Sparkles ---
    for (int p = 0; p < 18; p++) {
        double p_phase = fmod(t * 1.8 + hash(p * 7) * 4.0, 4.0) / 4.0;
        double px = lap_cx - 100.0 + hash(p * 13) * 200.0 + sin(t * 3.0 + p) * 15.0;
        double py = scr_top_y - p_phase * 120.0;
        double p_alpha = sin(p_phase * PI) * 0.85;

        cairo_set_source_rgba(cr, 0.0, 0.95, 0.70, p_alpha);
        if (p % 3 == 0) {
            // Little floating binary / hex glyph
            cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 11.0);
            cairo_move_to(cr, px, py);
            const char *glyphs[] = {"⬡", "01", "10", "⚡", "Ψ", "Δ", "⌘", "λ"};
            cairo_show_text(cr, glyphs[p % 8]);
        } else {
            // Little diamond sparkle
            cairo_move_to(cr, px, py - 4);
            cairo_line_to(cr, px + 3, py);
            cairo_line_to(cr, px, py + 4);
            cairo_line_to(cr, px - 3, py);
            cairo_close_path(cr);
            cairo_fill(cr);
        }
    }
}

// -----------------------------------------------------------------------------
// 4. Progress HUD, System Telemetry & "SOLARIS OS // SYSTEM ONLINE"
// -----------------------------------------------------------------------------
void draw_hud_and_telemetry(cairo_t *cr, double t) {
    double progress = clamp(t / 5.2, 0.0, 1.0); // Reaches 100% at 5.2s
    double hud_cy = HEIGHT * 0.88;
    double hud_cx = WIDTH * 0.5;

    // Outer Progress Bar Track
    double bar_w = 640.0;
    double bar_h = 14.0;
    double bar_x = hud_cx - bar_w / 2.0;
    double bar_y = hud_cy;

    // Glow backing
    cairo_set_source_rgba(cr, 0.05, 0.09, 0.14, 0.90);
    cairo_rectangle(cr, bar_x - 4, bar_y - 4, bar_w + 8, bar_h + 8);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, 0.0, 0.90, 0.65, 0.35);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, bar_x - 4, bar_y - 4, bar_w + 8, bar_h + 8);
    cairo_stroke(cr);

    // Segmented Progress Bar Fill (28 segments)
    int total_segs = 28;
    int filled_segs = (int)(progress * total_segs);
    double seg_w = (bar_w - (total_segs - 1) * 3.0) / total_segs;

    for (int s = 0; s < total_segs; s++) {
        double sx = bar_x + s * (seg_w + 3.0);
        if (s < filled_segs) {
            cairo_pattern_t *seg_pat = cairo_pattern_create_linear(sx, bar_y, sx, bar_y + bar_h);
            cairo_pattern_add_color_stop_rgb(seg_pat, 0.0, 0.0, 0.98, 0.85);
            cairo_pattern_add_color_stop_rgb(seg_pat, 1.0, 0.0, 0.75, 0.50);
            cairo_set_source(cr, seg_pat);
            cairo_rectangle(cr, sx, bar_y, seg_w, bar_h);
            cairo_fill(cr);
            cairo_pattern_destroy(seg_pat);
        } else {
            cairo_set_source_rgba(cr, 0.08, 0.15, 0.22, 0.60);
            cairo_rectangle(cr, sx, bar_y, seg_w, bar_h);
            cairo_fill(cr);
        }
    }

    // Telemetry Status Text
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14.0);

    char status_str[128];
    if (t < 1.5) {
        snprintf(status_str, sizeof(status_str), "🛸 SOLARIS OS // INITIALIZING NEURAL ENGINE... [%d%%]", (int)(progress * 100));
        cairo_set_source_rgba(cr, 0.0, 0.90, 0.80, 0.90);
    } else if (t < 3.8) {
        snprintf(status_str, sizeof(status_str), "⚙ COMPILING ARCH LINUX KERNEL // SYNCING AI TELEMETRY [%d%%]", (int)(progress * 100));
        cairo_set_source_rgba(cr, 0.0, 0.95, 0.60, 0.95);
    } else if (t < 5.2) {
        snprintf(status_str, sizeof(status_str), "⚡ ALL INSTRUMENT BAYS CALIBRATED // READY FOR DESKTOP [%d%%]", (int)(progress * 100));
        cairo_set_source_rgba(cr, 0.40, 0.85, 1.0, 0.95);
    } else {
        snprintf(status_str, sizeof(status_str), "✔ SYSTEM COMPILED // WELCOME TO SOLARIS OS [100%%]");
        cairo_set_source_rgba(cr, 0.0, 1.0, 0.70, 1.0);
    }

    cairo_text_extents_t ext;
    cairo_text_extents(cr, status_str, &ext);
    cairo_move_to(cr, hud_cx - ext.width / 2.0, bar_y - 12.0);
    cairo_show_text(cr, status_str);

    // Final Solaris OS Emblem & Celebration Flash (t >= 5.5s)
    if (t >= 5.3) {
        double celeb_p = clamp((t - 5.3) / 1.0, 0.0, 1.0);
        double title_alpha = ease_out_cubic(celeb_p);

        // Top Big Glowing Title
        cairo_save(cr);
        cairo_set_font_size(cr, 32.0);
        const char *title = "SOLARIS OS // SYSTEM ONLINE";
        cairo_text_extents(cr, title, &ext);

        // Glowing title shadow
        cairo_set_source_rgba(cr, 0.0, 0.95, 0.65, 0.35 * title_alpha);
        cairo_move_to(cr, hud_cx - ext.width / 2.0, HEIGHT * 0.16);
        cairo_show_text(cr, title);

        cairo_pattern_t *title_pat = cairo_pattern_create_linear(hud_cx - ext.width / 2.0, 0, hud_cx + ext.width / 2.0, 0);
        cairo_pattern_add_color_stop_rgba(title_pat, 0.0, 1.0, 1.0, 1.0, title_alpha);
        cairo_pattern_add_color_stop_rgba(title_pat, 0.5, 0.0, 0.95, 0.85, title_alpha);
        cairo_pattern_add_color_stop_rgba(title_pat, 1.0, 0.0, 0.95, 0.50, title_alpha);
        cairo_set_source(cr, title_pat);
        cairo_move_to(cr, hud_cx - ext.width / 2.0, HEIGHT * 0.16);
        cairo_show_text(cr, title);
        cairo_pattern_destroy(title_pat);

        // Subtitle
        cairo_set_font_size(cr, 13.0);
        const char *sub = "EXTRATERRESTRIAL COCKPIT INTERFACE // ARCH LINUX & NIRI";
        cairo_text_extents(cr, sub, &ext);
        cairo_set_source_rgba(cr, 0.60, 0.80, 0.95, 0.80 * title_alpha);
        cairo_move_to(cr, hud_cx - ext.width / 2.0, HEIGHT * 0.16 + 24.0);
        cairo_show_text(cr, sub);
        cairo_restore(cr);
    }
}

// -----------------------------------------------------------------------------
// Main Render Frame Driver
// -----------------------------------------------------------------------------
void render_frame(cairo_t *cr, double t) {
    draw_background(cr, t);
    draw_desk_and_chair(cr, WIDTH * 0.5, HEIGHT * 0.52, t);
    draw_alien_and_laptop(cr, WIDTH * 0.5, HEIGHT * 0.52, t);
    draw_hud_and_telemetry(cr, t);
}

int main(int argc, char **argv) {
    if (argc < 5) {
        printf("Usage: %s <output_dir> <fps> <width> <height>\n", argv[0]);
        return 1;
    }

    const char *out_dir = argv[1];
    int fps = atoi(argv[2]);
    int w = atoi(argv[3]);
    int h = atoi(argv[4]);
    int total_frames = (int)(TOTAL_TIME * fps);

    printf("=== Solaris Boot Splash: 2D Cartoony Alien on Laptop ===\n");
    printf("Output: %s | FPS: %d | Total Frames: %d | Res: %dx%d\n", out_dir, fps, total_frames, w, h);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = cairo_create(surface);

    // Apply resolution scale if different from 1920x1080
    cairo_scale(cr, (double)w / WIDTH, (double)h / HEIGHT);

    for (int f = 0; f < total_frames; f++) {
        double t = (double)f / (double)fps;
        render_frame(cr, t);

        char filename[512];
        snprintf(filename, sizeof(filename), "%s/progress-%d.png", out_dir, f);
        cairo_surface_write_to_png(surface, filename);

        if (f % (fps == 60 ? 30 : 15) == 0 || f == total_frames - 1) {
            printf("  [Frame %3d / %3d] (t=%.2fs) rendered -> %s\n", f, total_frames, t, filename);
            fflush(stdout);
        }
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    printf("=== Render Complete! Successfully wrote %d frames to %s ===\n", total_frames, out_dir);
    return 0;
}
