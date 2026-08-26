#ifndef SOLARIS_HOLOGRAM_COMMAND_CENTER_H
#define SOLARIS_HOLOGRAM_COMMAND_CENTER_H

#include "common.h"
#include "model.h"
#include "niri_ipc.h"

typedef struct {
    gboolean is_active;
    char search_query[128];
    int search_selected_idx;
    uint64_t inspected_window_id;
    double search_anim;
} command_center_state_t;

void command_center_init(command_center_state_t *cc);
void command_center_render(cairo_t *cr, command_center_state_t *cc,
                           const orbital_system_t *sys, double w, double h);
gboolean command_center_handle_key(command_center_state_t *cc, orbital_system_t *sys,
                                   niri_ipc_t *ipc, guint keyval);
gboolean command_center_handle_click(command_center_state_t *cc, orbital_system_t *sys,
                                     niri_ipc_t *ipc, double mx, double my);

#endif // SOLARIS_HOLOGRAM_COMMAND_CENTER_H
