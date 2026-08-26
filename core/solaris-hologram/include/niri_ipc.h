#ifndef SOLARIS_HOLOGRAM_NIRI_IPC_H
#define SOLARIS_HOLOGRAM_NIRI_IPC_H

#include "common.h"

typedef void (*niri_event_callback_fn)(const char *event_type, struct json_object *event_data, gpointer user_data);

typedef struct {
    char socket_path[512];
    gboolean is_connected;
    pthread_t listener_thread;
    gboolean thread_running;
    niri_event_callback_fn callback;
    gpointer user_data;
    pthread_mutex_t lock;
} niri_ipc_t;

// Lifecycle
gboolean niri_ipc_init(niri_ipc_t *ipc, niri_event_callback_fn cb, gpointer user_data);
void niri_ipc_cleanup(niri_ipc_t *ipc);

// State query
char *niri_ipc_query_json(niri_ipc_t *ipc, const char *request_json);
gboolean niri_ipc_get_workspaces_and_windows(niri_ipc_t *ipc, struct json_object **out_ws, struct json_object **out_wins);

// Actions
gboolean niri_ipc_focus_workspace_idx(niri_ipc_t *ipc, int idx);
gboolean niri_ipc_focus_workspace_name(niri_ipc_t *ipc, const char *name);
gboolean niri_ipc_focus_window(niri_ipc_t *ipc, uint64_t window_id);
gboolean niri_ipc_move_window_to_workspace(niri_ipc_t *ipc, uint64_t window_id, int workspace_idx);
gboolean niri_ipc_create_new_workspace(niri_ipc_t *ipc, int next_idx);
gboolean niri_ipc_close_window(niri_ipc_t *ipc, uint64_t window_id);

#endif // SOLARIS_HOLOGRAM_NIRI_IPC_H
