#ifndef SOLARIS_HOLOGRAM_MODEL_H
#define SOLARIS_HOLOGRAM_MODEL_H

#include "common.h"
#include "math3d.h"

struct window_info_s {
    uint64_t id;
    char title[256];
    char app_id[128];
    char display_name[64];
    char glyph[8];
    GdkPixbuf *icon_pixbuf;
    int pid;
    uint64_t workspace_id;
    int workspace_idx;
    gboolean is_focused;
    gboolean is_floating;
    gboolean is_urgent;
    int tile_width;
    int tile_height;
    
    // Animation & Interactive states
    double focus_anim;       // 0.0 to 1.0 (bright pulse on focus change)
    double hover_anim;       // 0.0 to 1.0 (smooth glow on hover)
    double orbit_angle;      // Angular position in radians along workspace orbit
    double orbit_angle_target;
    double drag_offset_x;    // Screen drag offsets
    double drag_offset_y;
    gboolean is_dragged;

    // 2D Screen projection cache
    vec2_t screen_pos;
    double screen_scale;
    double screen_depth;
};

struct workspace_info_s {
    uint64_t id;
    int idx;
    char raw_name[64];
    char station_title[64];
    char station_callsign[64];
    gboolean is_active;
    gboolean is_focused;
    gboolean is_urgent;
    int window_count;
    uint64_t active_window_id;

    // 3D Orbital Parameters
    double base_radius;
    double current_radius;
    double target_radius;
    double inclination;      // Orbital tilt angle (rad)
    double y_height;         // Staggered Y elevation in 3D
    double orbit_rot;        // Current rotation phase angle (rad)
    double rot_speed;        // Radians per second
    double pulse_pos;        // Photon wave packet position (0.0 to 2*PI)
    double active_anim;      // 0.0 to 1.0 smooth active brightness
    double hover_anim;       // 0.0 to 1.0 smooth hover glow

    // Screen bounds & label cache
    vec2_t label_screen_pos;
    double label_scale;
    double label_depth;
};

struct orbital_system_s {
    workspace_info_t workspaces[MAX_WORKSPACES];
    int workspace_count;

    window_info_t windows[MAX_WINDOWS];
    int window_count;

    int active_workspace_idx;
    uint64_t focused_window_id;

    // Hover & Selection
    int hovered_workspace_idx;
    uint64_t hovered_window_id;
    gboolean hovered_create_btn;
    gboolean hovered_core;

    uint64_t selected_window_id;
    int selected_workspace_idx;

    vec2_t create_btn_pos;
    double create_btn_scale;

    gboolean is_connected;
    double connection_retry_time;

    // Global Time & Core Animation
    double time_sec;
    double core_rot_yaw;
    double core_rot_pitch;
    double core_pulse;
    double scanline_phase;

    camera3d_t camera;
    GtkIconTheme *icon_theme;
};

// Lifecycle & Init
void orbital_system_init(orbital_system_t *sys);
void orbital_system_cleanup(orbital_system_t *sys);

// Model update from Niri JSON
void orbital_system_update_workspaces_json(orbital_system_t *sys, struct json_object *ws_array);
void orbital_system_update_windows_json(orbital_system_t *sys, struct json_object *win_array);
void orbital_system_handle_event(orbital_system_t *sys, const char *event_type, struct json_object *event_data);

// Frame simulation (dt in seconds)
void orbital_system_tick(orbital_system_t *sys, double dt, double screen_w, double screen_h);

// Queries & Lookups
workspace_info_t *orbital_system_get_workspace_by_idx(orbital_system_t *sys, int idx);
workspace_info_t *orbital_system_get_workspace_by_id(orbital_system_t *sys, uint64_t ws_id);
window_info_t *orbital_system_get_window_by_id(orbital_system_t *sys, uint64_t win_id);

// Hit testing (mouse screen coordinates -> hit target)
gboolean orbital_system_hit_test(orbital_system_t *sys, double mx, double my,
                                 uint64_t *out_win_id, int *out_ws_idx,
                                 gboolean *out_create_btn, gboolean *out_core);

#endif // SOLARIS_HOLOGRAM_MODEL_H
