#ifndef SOLARIS_HOLOGRAM_LAYER_WINDOW_H
#define SOLARIS_HOLOGRAM_LAYER_WINDOW_H

#include "common.h"
#include "model.h"
#include "niri_ipc.h"
#include "particle_system.h"
#include "command_center.h"

struct hologram_app_s {
    GtkWidget *window;
    GtkWidget *drawing_area;

    orbital_system_t sys;
    niri_ipc_t ipc;
    particle_system_t ps;
    command_center_state_t cc;

    gboolean is_command_center_mode;
    gboolean is_dragging_camera;
    double drag_last_x;
    double drag_last_y;
    double mouse_x;
    double mouse_y;
    gboolean mouse_in_window;

    struct timeval last_time;
    guint timer_id;
};

// Lifecycle
gboolean layer_window_init(hologram_app_t *app, gboolean start_as_command_center);
void layer_window_set_mode(hologram_app_t *app, gboolean command_center_mode);
void layer_window_toggle_mode(hologram_app_t *app);

#endif // SOLARIS_HOLOGRAM_LAYER_WINDOW_H
