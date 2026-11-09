#ifndef SOLARIS_HOLOGRAM_COMMON_H
#define SOLARIS_HOLOGRAM_COMMON_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <cairo.h>
#include <pango/pangocairo.h>
#include <json-c/json.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// COLOR PALETTE & VISUAL CONSTANTS (SPACECRAFT HOLOGRAPHIC COMMAND INTERFACE)
// =============================================================================

#define COLOR_CYAN_HEX        "#00e5ff"
#define COLOR_CYAN_R          0.000
#define COLOR_CYAN_G          0.898
#define COLOR_CYAN_B          1.000

#define COLOR_TEAL_R          0.000
#define COLOR_TEAL_G          0.651
#define COLOR_TEAL_B          0.984

#define COLOR_DEEP_BLUE_R     0.024
#define COLOR_DEEP_BLUE_G     0.055
#define COLOR_DEEP_BLUE_B     0.110

#define COLOR_VIOLET_R        0.616
#define COLOR_VIOLET_G        0.306
#define COLOR_VIOLET_B        0.933

#define COLOR_GREEN_R         0.000
#define COLOR_GREEN_G         1.000
#define COLOR_GREEN_B         0.400

#define COLOR_AMBER_R         1.000
#define COLOR_AMBER_G         0.667
#define COLOR_AMBER_B         0.000

#define COLOR_ALERT_R         1.000
#define COLOR_ALERT_G         0.165
#define COLOR_ALERT_B         0.373

#define COLOR_TEXT_BRIGHT_R   0.900
#define COLOR_TEXT_BRIGHT_G   0.969
#define COLOR_TEXT_BRIGHT_B   1.000

#define COLOR_TEXT_MUTED_R    0.350
#define COLOR_TEXT_MUTED_G    0.500
#define COLOR_TEXT_MUTED_B    0.650

// Station names mapping for Solaris OS
#define STATION_1_NAME "COMMAND"
#define STATION_2_NAME "ENGINEERING"
#define STATION_3_NAME "NAVIGATION"
#define STATION_4_NAME "COMMS"
#define STATION_5_NAME "SCIENCE"
#define STATION_6_NAME "AUXILIARY"

static inline const char *get_station_short_name(int idx) {
    switch (idx) {
        case 1: return "CMD";
        case 2: return "ENG";
        case 3: return "NAV";
        case 4: return "COM";
        case 5: return "SCI";
        case 6: return "AUX";
        default: return "STA";
    }
}

typedef struct {
    double r, g, b;
} color_rgb_t;

static inline color_rgb_t get_station_color(int idx) {
    switch (idx) {
        case 1: return (color_rgb_t){1.000, 0.720, 0.050}; // Station 1: COMMAND (Gold / Amber)
        case 2: return (color_rgb_t){0.000, 1.000, 0.835}; // Station 2: ENGINEERING (Quantum Emerald / Teal)
        case 3: return (color_rgb_t){0.000, 0.658, 1.000}; // Station 3: NAVIGATION (Hyper Blue / Azure)
        case 4: return (color_rgb_t){0.000, 0.941, 1.000}; // Station 4: COMMS (Electric Cyan)
        case 5: return (color_rgb_t){0.780, 0.380, 0.980}; // Station 5: SCIENCE (Cosmic Violet / Purple)
        case 6: return (color_rgb_t){1.000, 0.250, 0.420}; // Station 6: AUXILIARY (Neon Rose / Coral)
        default: return (color_rgb_t){0.000, 0.898, 1.000};
    }
}

// Limits
#define MAX_WORKSPACES 16
#define MAX_WINDOWS 64
#define MAX_PARTICLES 70

// Forward structure declarations
typedef struct vec3_s vec3_t;
typedef struct mat4_s mat4_t;
typedef struct camera3d_s camera3d_t;
typedef struct window_info_s window_info_t;
typedef struct workspace_info_s workspace_info_t;
typedef struct app_node_s app_node_t;
typedef struct orbital_system_s orbital_system_t;
typedef struct hologram_app_s hologram_app_t;

#endif // SOLARIS_HOLOGRAM_COMMON_H
