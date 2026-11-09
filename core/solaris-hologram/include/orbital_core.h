#ifndef SOLARIS_ORBITAL_CORE_H
#define SOLARIS_ORBITAL_CORE_H

#include "common.h"
#include "math3d.h"
#include "telemetry.h"

typedef enum {
    CORE_STATE_ORBITAL = 0,
    CORE_STATE_SERVER_CORE = 1,
    CORE_STATE_TERMINAL = 2
} core_state_t;

typedef enum {
    HOTSPOT_NONE = 0,
    HOTSPOT_CPU = 1,
    HOTSPOT_RAM = 2,
    HOTSPOT_STORAGE = 3,
    HOTSPOT_NETWORK = 4,
    HOTSPOT_CORE = 5,
    HOTSPOT_VESSEL = 6,
    HOTSPOT_CONNECT_BTN = 7,
    HOTSPOT_TERMINAL_BTN = 8
} server_hotspot_t;

#define TERMINAL_MAX_LINES 128
#define TERMINAL_LINE_LEN 256
#define TERMINAL_CMD_LEN 128

typedef struct {
    char lines[TERMINAL_MAX_LINES][TERMINAL_LINE_LEN];
    int line_count;
    int scroll_offset;
    char current_input[TERMINAL_CMD_LEN];
    int cursor_pos;
    gboolean has_focus;
    double cursor_blink_timer;
    char last_command[TERMINAL_CMD_LEN];
    double anim_progress; // 0.0 to 1.0 opening expansion
} holographic_terminal_t;

typedef struct {
    core_state_t current_state;
    core_state_t target_state;
    double transition_progress; // 0.0 to 1.0
    double orbital_to_server_t; // 0.0 = ORBITAL, 1.0 = SERVER_CORE
    double server_to_terminal_t; // 0.0 = SERVER_CORE, 1.0 = TERMINAL

    // Hints and discovery
    gboolean hint_dismissed;
    double hint_opacity;

    // Gesture tracking
    gboolean gesture_active;
    double gesture_start_x;
    double gesture_start_y;
    double gesture_accum_dx;
    double gesture_accum_dy;

    // Hover hotspots
    server_hotspot_t hovered_hotspot;
    int hovered_mode_tab;  // 0 = ORBITAL, 1 = SERVER, 2 = TERMINAL, -1 = none
    int hovered_term_btn;  // Quick action buttons in terminal

    // 3D Server geometry & animations
    double server_yaw;
    double server_pitch;
    double server_pulse;
    double fan_angle;
    double circuit_pulse;

    // Live Telemetry
    server_telemetry_t telemetry;
    double telemetry_timer;

    // Holographic Terminal
    holographic_terminal_t terminal;

    // UI Tab Layout cache
    double tabs_x;
    double tabs_y;
    double tab_width;
    double tab_height;
} orbital_core_t;

// Lifecycle
void orbital_core_init(orbital_core_t *core);
void orbital_core_update(orbital_core_t *core, double dt);
void orbital_core_set_state(orbital_core_t *core, core_state_t target);
void orbital_core_toggle_state(orbital_core_t *core);

// Terminal operations
void terminal_init(holographic_terminal_t *term);
void terminal_write_line(holographic_terminal_t *term, const char *line);
void terminal_execute_command(holographic_terminal_t *term, const char *cmd, server_telemetry_t *tel);
void terminal_handle_key(holographic_terminal_t *term, guint keyval, server_telemetry_t *tel, orbital_core_t *core);

// Rendering modules
void render_server_chassis(cairo_t *cr, const orbital_core_t *core, const camera3d_t *cam, double cx, double cy, double scale, double alpha);
void render_holographic_terminal(cairo_t *cr, orbital_core_t *core, const camera3d_t *cam, double cx, double cy, double w, double h);
void render_core_mode_selector(cairo_t *cr, const orbital_core_t *core, double cx, double w, double h);
void render_core_context_hint(cairo_t *cr, const orbital_core_t *core, double cx, double cy, double h);

// Hit testing
gboolean orbital_core_hit_test(orbital_core_t *core, double mx, double my, double cx, double cy,
                               int *out_tab, server_hotspot_t *out_hotspot, int *out_term_btn);

#endif // SOLARIS_ORBITAL_CORE_H
