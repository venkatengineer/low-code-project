#include "niri_ipc.h"
#include <dirent.h>

static gboolean find_niri_socket(char *out_path, size_t max_len) {
    const char *env_sock = getenv("NIRI_SOCKET");
    if (env_sock && access(env_sock, F_OK) == 0) {
        snprintf(out_path, max_len, "%s", env_sock);
        return TRUE;
    }

    // Try finding in /run/user/<uid>/
    uid_t uid = getuid();
    char run_dir[256];
    snprintf(run_dir, sizeof(run_dir), "/run/user/%d", uid);
    DIR *dir = opendir(run_dir);
    if (!dir) return FALSE;

    struct dirent *entry;
    gboolean found = FALSE;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "niri.", 5) == 0 && strstr(entry->d_name, ".sock")) {
            snprintf(out_path, max_len, "%s/%s", run_dir, entry->d_name);
            found = TRUE;
            break;
        }
    }
    closedir(dir);
    return found;
}

static int open_niri_socket_fd(const char *path) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

typedef struct {
    char *event_type;
    struct json_object *event_data;
    niri_event_callback_fn callback;
    gpointer user_data;
} event_dispatch_t;

static gboolean dispatch_event_main_thread(gpointer user_data) {
    event_dispatch_t *disp = (event_dispatch_t *)user_data;
    if (disp->callback) {
        disp->callback(disp->event_type, disp->event_data, disp->user_data);
    }
    if (disp->event_data) {
        json_object_put(disp->event_data);
    }
    free(disp->event_type);
    free(disp);
    return G_SOURCE_REMOVE;
}

static void *event_listener_thread_func(void *arg) {
    niri_ipc_t *ipc = (niri_ipc_t *)arg;

    while (ipc->thread_running) {
        int fd = open_niri_socket_fd(ipc->socket_path);
        if (fd < 0) {
            pthread_mutex_lock(&ipc->lock);
            ipc->is_connected = FALSE;
            pthread_mutex_unlock(&ipc->lock);

            // Notify UI of disconnect
            event_dispatch_t *disp = calloc(1, sizeof(event_dispatch_t));
            disp->event_type = strdup("ConnectionLost");
            disp->event_data = NULL;
            disp->callback = ipc->callback;
            disp->user_data = ipc->user_data;
            g_idle_add(dispatch_event_main_thread, disp);

            sleep(1);
            // Retry finding socket if path changed
            find_niri_socket(ipc->socket_path, sizeof(ipc->socket_path));
            continue;
        }

        pthread_mutex_lock(&ipc->lock);
        ipc->is_connected = TRUE;
        pthread_mutex_unlock(&ipc->lock);

        // Notify UI of connection
        event_dispatch_t *disp_conn = calloc(1, sizeof(event_dispatch_t));
        disp_conn->event_type = strdup("ConnectionRestored");
        disp_conn->event_data = NULL;
        disp_conn->callback = ipc->callback;
        disp_conn->user_data = ipc->user_data;
        g_idle_add(dispatch_event_main_thread, disp_conn);

        // Request EventStream
        const char *req = "\"EventStream\"\n";
        if (write(fd, req, strlen(req)) < 0) {
            close(fd);
            continue;
        }

        FILE *fp = fdopen(fd, "r");
        if (!fp) {
            close(fd);
            continue;
        }

        char *line = NULL;
        size_t len = 0;
        ssize_t read_bytes;

        while (ipc->thread_running && (read_bytes = getline(&line, &len, fp)) != -1) {
            if (read_bytes <= 1) continue;

            // Trim newline
            if (line[read_bytes - 1] == '\n') line[read_bytes - 1] = '\0';

            struct json_object *parsed = json_tokener_parse(line);
            if (!parsed) continue;

            // Niri events are objects with a single key, e.g. {"WorkspacesChanged": ...}, {"WindowsChanged": ...}
            // or initial {"Ok": "Handled"}
            json_object_object_foreach(parsed, key, val) {
                if (strcmp(key, "Ok") == 0) {
                    continue;
                }

                event_dispatch_t *disp = calloc(1, sizeof(event_dispatch_t));
                disp->event_type = strdup(key);
                disp->event_data = json_object_get(val); // Increment ref count for transfer
                disp->callback = ipc->callback;
                disp->user_data = ipc->user_data;
                g_idle_add(dispatch_event_main_thread, disp);
            }
            json_object_put(parsed);
        }

        if (line) free(line);
        fclose(fp); // Also closes fd

        pthread_mutex_lock(&ipc->lock);
        ipc->is_connected = FALSE;
        pthread_mutex_unlock(&ipc->lock);

        if (ipc->thread_running) {
            sleep(1);
        }
    }

    return NULL;
}

gboolean niri_ipc_init(niri_ipc_t *ipc, niri_event_callback_fn cb, gpointer user_data) {
    memset(ipc, 0, sizeof(*ipc));
    pthread_mutex_init(&ipc->lock, NULL);
    ipc->callback = cb;
    ipc->user_data = user_data;

    if (!find_niri_socket(ipc->socket_path, sizeof(ipc->socket_path))) {
        fprintf(stderr, "[Solaris Hologram] Warning: Niri socket not found at startup, will keep trying.\n");
    } else {
        printf("[Solaris Hologram] Discovered Niri socket: %s\n", ipc->socket_path);
    }

    ipc->thread_running = TRUE;
    if (pthread_create(&ipc->listener_thread, NULL, event_listener_thread_func, ipc) != 0) {
        fprintf(stderr, "[Solaris Hologram] Error creating Niri event listener thread.\n");
        return FALSE;
    }

    return TRUE;
}

void niri_ipc_cleanup(niri_ipc_t *ipc) {
    ipc->thread_running = FALSE;
    pthread_join(ipc->listener_thread, NULL);
    pthread_mutex_destroy(&ipc->lock);
}

char *niri_ipc_query_json(niri_ipc_t *ipc, const char *request_json) {
    if (ipc->socket_path[0] == '\0') {
        if (!find_niri_socket(ipc->socket_path, sizeof(ipc->socket_path))) {
            return NULL;
        }
    }

    int fd = open_niri_socket_fd(ipc->socket_path);
    if (fd < 0) return NULL;

    size_t req_len = strlen(request_json);
    if (write(fd, request_json, req_len) < 0 || write(fd, "\n", 1) < 0) {
        close(fd);
        return NULL;
    }

    FILE *fp = fdopen(fd, "r");
    if (!fp) {
        close(fd);
        return NULL;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read_bytes = getline(&line, &len, fp);
    fclose(fp); // Also closes fd

    if (read_bytes <= 0 || !line) {
        if (line) free(line);
        return NULL;
    }

    if (line[read_bytes - 1] == '\n') {
        line[read_bytes - 1] = '\0';
    }

    return line;
}

static gboolean send_action_json(niri_ipc_t *ipc, const char *action_json) {
    char *resp = niri_ipc_query_json(ipc, action_json);
    if (resp) {
        free(resp);
        return TRUE;
    }
    return FALSE;
}

gboolean niri_ipc_get_workspaces_and_windows(niri_ipc_t *ipc, struct json_object **out_ws, struct json_object **out_wins) {
    char *ws_resp = niri_ipc_query_json(ipc, "\"Workspaces\"");
    if (!ws_resp) return FALSE;

    struct json_object *parsed_ws = json_tokener_parse(ws_resp);
    free(ws_resp);

    if (parsed_ws) {
        struct json_object *ok_obj = NULL;
        if (json_object_object_get_ex(parsed_ws, "Ok", &ok_obj)) {
            struct json_object *ws_array = NULL;
            if (json_object_object_get_ex(ok_obj, "Workspaces", &ws_array)) {
                *out_ws = json_object_get(ws_array);
            }
        }
        json_object_put(parsed_ws);
    }

    char *win_resp = niri_ipc_query_json(ipc, "\"Windows\"");
    if (win_resp) {
        struct json_object *parsed_win = json_tokener_parse(win_resp);
        free(win_resp);
        if (parsed_win) {
            struct json_object *ok_obj = NULL;
            if (json_object_object_get_ex(parsed_win, "Ok", &ok_obj)) {
                struct json_object *win_array = NULL;
                if (json_object_object_get_ex(ok_obj, "Windows", &win_array)) {
                    *out_wins = json_object_get(win_array);
                }
            }
            json_object_put(parsed_win);
        }
    }

    return (*out_ws != NULL);
}

gboolean niri_ipc_focus_workspace_idx(niri_ipc_t *ipc, int idx) {
    char buf[256];
    snprintf(buf, sizeof(buf), "{\"Action\":{\"FocusWorkspace\":{\"reference\":{\"Index\":%d}}}}", idx);
    return send_action_json(ipc, buf);
}

gboolean niri_ipc_focus_workspace_name(niri_ipc_t *ipc, const char *name) {
    char buf[256];
    snprintf(buf, sizeof(buf), "{\"Action\":{\"FocusWorkspace\":{\"reference\":{\"Name\":\"%s\"}}}}", name);
    return send_action_json(ipc, buf);
}

gboolean niri_ipc_focus_window(niri_ipc_t *ipc, uint64_t window_id) {
    char buf[256];
    snprintf(buf, sizeof(buf), "{\"Action\":{\"FocusWindow\":{\"id\":%lu}}}", (unsigned long)window_id);
    return send_action_json(ipc, buf);
}

gboolean niri_ipc_move_window_to_workspace(niri_ipc_t *ipc, uint64_t window_id, int workspace_idx) {
    char buf[256];
    snprintf(buf, sizeof(buf), "{\"Action\":{\"MoveWindowToWorkspace\":{\"window_id\":%lu,\"reference\":{\"Index\":%d},\"focus\":true}}}",
             (unsigned long)window_id, workspace_idx);
    return send_action_json(ipc, buf);
}

gboolean niri_ipc_create_new_workspace(niri_ipc_t *ipc, int next_idx) {
    return niri_ipc_focus_workspace_idx(ipc, next_idx);
}

gboolean niri_ipc_close_window(niri_ipc_t *ipc, uint64_t window_id) {
    // Focus window first, then close
    niri_ipc_focus_window(ipc, window_id);
    return send_action_json(ipc, "{\"Action\":{\"CloseWindow\":null}}");
}
