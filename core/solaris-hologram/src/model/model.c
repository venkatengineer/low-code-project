#include "model.h"
#include <ctype.h>

static void format_display_name_and_glyph(const char *app_id, const char *title,
                                          char *out_name, size_t name_len,
                                          char *out_glyph, size_t glyph_len) {
    if (!app_id || app_id[0] == '\0') {
        snprintf(out_name, name_len, "CONSOLE");
        snprintf(out_glyph, glyph_len, "󰘳");
        return;
    }

    char lower[128];
    size_t i = 0;
    for (; app_id[i] && i < sizeof(lower) - 1; i++) {
        lower[i] = tolower(app_id[i]);
    }
    lower[i] = '\0';

    if (strstr(lower, "kitty") || strstr(lower, "alacritty") || strstr(lower, "foot") || strstr(lower, "terminal") || strstr(lower, "wezterm")) {
        snprintf(out_name, name_len, "KITTY");
        snprintf(out_glyph, glyph_len, "");
    } else if (strstr(lower, "vivaldi") || strstr(lower, "firefox") || strstr(lower, "chromium") || strstr(lower, "chrome") || strstr(lower, "zen")) {
        snprintf(out_name, name_len, "VIVALDI");
        snprintf(out_glyph, glyph_len, "󰖟");
    } else if (strstr(lower, "nautilus") || strstr(lower, "thunar") || strstr(lower, "dolphin") || strstr(lower, "yazi") || strstr(lower, "explorer")) {
        snprintf(out_name, name_len, "NAUTILUS");
        snprintf(out_glyph, glyph_len, "󰉋");
    } else if (strstr(lower, "code") || strstr(lower, "nvim") || strstr(lower, "emacs") || strstr(lower, "sublime") || strstr(lower, "zed")) {
        snprintf(out_name, name_len, "VS CODE");
        snprintf(out_glyph, glyph_len, "󰨞");
    } else if (strstr(lower, "spotify") || strstr(lower, "music") || strstr(lower, "audio") || strstr(lower, "player")) {
        snprintf(out_name, name_len, "SPOTIFY");
        snprintf(out_glyph, glyph_len, "󰎆");
    } else if (strstr(lower, "discord") || strstr(lower, "telegram") || strstr(lower, "slack") || strstr(lower, "element")) {
        snprintf(out_name, name_len, "DISCORD");
        snprintf(out_glyph, glyph_len, "󰭹");
    } else if (strstr(lower, "xenomon") || strstr(lower, "btop") || strstr(lower, "htop")) {
        snprintf(out_name, name_len, "XENOMON");
        snprintf(out_glyph, glyph_len, "󰍛");
    } else if (strstr(lower, "easyeffects") || strstr(lower, "pavucontrol")) {
        snprintf(out_name, name_len, "DSP CORE");
        snprintf(out_glyph, glyph_len, "󰓃");
    } else {
        // Use last component of domain/app_id
        const char *dot = strrchr(app_id, '.');
        const char *src = dot ? dot + 1 : app_id;
        size_t j = 0;
        for (; src[j] && j < name_len - 1; j++) {
            out_name[j] = toupper(src[j]);
        }
        out_name[j] = '\0';
        snprintf(out_glyph, glyph_len, "󰘳");
    }
}

static GdkPixbuf *load_app_icon(GtkIconTheme *theme, const char *app_id) {
    if (!theme || !app_id || app_id[0] == '\0') return NULL;
    GError *err = NULL;
    GdkPixbuf *pix = gtk_icon_theme_load_icon(theme, app_id, 24, GTK_ICON_LOOKUP_FORCE_SIZE, &err);
    if (err) {
        g_error_free(err);
        err = NULL;
    }
    if (!pix) {
        // Try lower-cased or common fallbacks
        char lower[128];
        size_t i = 0;
        for (; app_id[i] && i < sizeof(lower) - 1; i++) {
            lower[i] = tolower(app_id[i]);
        }
        lower[i] = '\0';
        pix = gtk_icon_theme_load_icon(theme, lower, 24, GTK_ICON_LOOKUP_FORCE_SIZE, &err);
        if (err) g_error_free(err);
    }
    return pix;
}

static const char *get_default_station_name(int idx) {
    switch (idx) {
        case 1: return STATION_1_NAME;
        case 2: return STATION_2_NAME;
        case 3: return STATION_3_NAME;
        case 4: return STATION_4_NAME;
        case 5: return STATION_5_NAME;
        case 6: return STATION_6_NAME;
        default: return "AUXILIARY MATRIX";
    }
}

void orbital_system_init(orbital_system_t *sys) {
    memset(sys, 0, sizeof(*sys));
    camera3d_init(&sys->camera);
    sys->icon_theme = gtk_icon_theme_get_default();
    sys->hovered_workspace_idx = -1;
    sys->hovered_window_id = 0;
    sys->active_workspace_idx = 1;
    sys->is_connected = TRUE;

    // Initialize default orbital stations 1..6
    sys->workspace_count = 6;
    for (int i = 0; i < 6; i++) {
        workspace_info_t *ws = &sys->workspaces[i];
        ws->id = i + 1;
        ws->idx = i + 1;
        snprintf(ws->raw_name, sizeof(ws->raw_name), "%s", get_default_station_name(i + 1));
        snprintf(ws->station_title, sizeof(ws->station_title), "STATION %02d // %s", i + 1, get_default_station_name(i + 1));
        snprintf(ws->station_callsign, sizeof(ws->station_callsign), "ORBIT-%02d", i + 1);
        
        ws->base_radius = 70.0 + (i * 40.0);
        ws->current_radius = ws->base_radius;
        ws->target_radius = ws->base_radius;
        ws->inclination = 0.0;
        ws->y_height = 0.0;
        ws->orbit_rot = (i * 0.6);
        ws->rot_speed = 0.04 - (i * 0.003); // Outer orbits rotate slightly slower
        ws->pulse_pos = 0.0;
        ws->active_anim = (i == 0) ? 1.0 : 0.0;
        ws->is_active = (i == 0);
        ws->is_focused = (i == 0);
    }
}

void orbital_system_cleanup(orbital_system_t *sys) {
    for (int i = 0; i < sys->window_count; i++) {
        if (sys->windows[i].icon_pixbuf) {
            g_object_unref(sys->windows[i].icon_pixbuf);
            sys->windows[i].icon_pixbuf = NULL;
        }
    }
}

void orbital_system_update_workspaces_json(orbital_system_t *sys, struct json_object *ws_array) {
    if (!ws_array || !json_object_is_type(ws_array, json_type_array)) return;

    size_t n = json_object_array_length(ws_array);
    if (n == 0) return;

    // Reset active flags
    for (int i = 0; i < sys->workspace_count; i++) {
        sys->workspaces[i].is_active = FALSE;
        sys->workspaces[i].is_focused = FALSE;
    }

    // Track existing workspaces and update them
    for (size_t i = 0; i < n && i < MAX_WORKSPACES; i++) {
        struct json_object *j_ws = json_object_array_get_idx(ws_array, i);
        if (!j_ws) continue;

        struct json_object *j_id = NULL, *j_idx = NULL, *j_name = NULL, *j_active = NULL, *j_focused = NULL, *j_urgent = NULL, *j_active_win = NULL;
        json_object_object_get_ex(j_ws, "id", &j_id);
        json_object_object_get_ex(j_ws, "idx", &j_idx);
        json_object_object_get_ex(j_ws, "name", &j_name);
        json_object_object_get_ex(j_ws, "is_active", &j_active);
        json_object_object_get_ex(j_ws, "is_focused", &j_focused);
        json_object_object_get_ex(j_ws, "is_urgent", &j_urgent);
        json_object_object_get_ex(j_ws, "active_window_id", &j_active_win);

        int idx = j_idx ? json_object_get_int(j_idx) : (int)(i + 1);
        uint64_t id = j_id ? (uint64_t)json_object_get_int64(j_id) : (uint64_t)idx;
        gboolean is_active = j_active ? json_object_get_boolean(j_active) : FALSE;
        gboolean is_focused = j_focused ? json_object_get_boolean(j_focused) : FALSE;
        gboolean is_urgent = j_urgent ? json_object_get_boolean(j_urgent) : FALSE;
        const char *name_str = j_name ? json_object_get_string(j_name) : NULL;
        uint64_t active_win_id = (j_active_win && !json_object_is_type(j_active_win, json_type_null)) ?
                                 (uint64_t)json_object_get_int64(j_active_win) : 0;

        workspace_info_t *ws = orbital_system_get_workspace_by_idx(sys, idx);
        if (!ws) {
            // Find slot
            for (int s = 0; s < MAX_WORKSPACES; s++) {
                if (sys->workspaces[s].idx == 0 || sys->workspaces[s].idx == idx) {
                    ws = &sys->workspaces[s];
                    ws->idx = idx;
                    if (s >= sys->workspace_count) sys->workspace_count = s + 1;
                    break;
                }
            }
        }

        if (ws) {
            ws->id = id;
            ws->idx = idx;
            ws->is_active = is_active;
            ws->is_focused = is_focused;
            ws->is_urgent = is_urgent;
            ws->active_window_id = active_win_id;

            ws->base_radius = 70.0 + ((idx - 1) * 40.0);
            if (ws->current_radius < 40.0) {
                ws->current_radius = ws->base_radius;
                ws->target_radius = ws->base_radius;
            }
            ws->inclination = 0.0;
            ws->y_height = 0.0;
            ws->rot_speed = 0.04 - (idx * 0.003);

            if (name_str && name_str[0] != '\0') {
                snprintf(ws->raw_name, sizeof(ws->raw_name), "%.32s", name_str);
                char upper[32];
                size_t k = 0;
                for (; name_str[k] && k < sizeof(upper) - 1; k++) {
                    upper[k] = toupper(name_str[k]);
                }
                upper[k] = '\0';
                snprintf(ws->station_title, sizeof(ws->station_title), "STATION %02d // %.32s", idx, upper);
            } else {
                snprintf(ws->raw_name, sizeof(ws->raw_name), "%s", get_default_station_name(idx));
                snprintf(ws->station_title, sizeof(ws->station_title), "STATION %02d // %s", idx, get_default_station_name(idx));
            }
            snprintf(ws->station_callsign, sizeof(ws->station_callsign), "ORBIT-%02d", idx);

            if (is_active) {
                sys->active_workspace_idx = idx;
            }
        }
    }

    if (sys->workspace_count < 6) {
        sys->workspace_count = 6;
    }
}

void orbital_system_update_windows_json(orbital_system_t *sys, struct json_object *win_array) {
    if (!win_array || !json_object_is_type(win_array, json_type_array)) return;

    size_t n = json_object_array_length(win_array);

    // Track old icons and angles to reuse smoothly
    window_info_t old_wins[MAX_WINDOWS];
    int old_count = sys->window_count;
    memcpy(old_wins, sys->windows, sizeof(old_wins));

    // Clear window counts per workspace
    for (int i = 0; i < sys->workspace_count; i++) {
        sys->workspaces[i].window_count = 0;
    }

    sys->window_count = 0;
    sys->focused_window_id = 0;

    for (size_t i = 0; i < n && sys->window_count < MAX_WINDOWS; i++) {
        struct json_object *j_win = json_object_array_get_idx(win_array, i);
        if (!j_win) continue;

        struct json_object *j_id = NULL, *j_title = NULL, *j_app_id = NULL, *j_pid = NULL,
                           *j_ws_id = NULL, *j_focused = NULL, *j_floating = NULL, *j_urgent = NULL,
                           *j_layout = NULL;

        json_object_object_get_ex(j_win, "id", &j_id);
        json_object_object_get_ex(j_win, "title", &j_title);
        json_object_object_get_ex(j_win, "app_id", &j_app_id);
        json_object_object_get_ex(j_win, "pid", &j_pid);
        json_object_object_get_ex(j_win, "workspace_id", &j_ws_id);
        json_object_object_get_ex(j_win, "is_focused", &j_focused);
        json_object_object_get_ex(j_win, "is_floating", &j_floating);
        json_object_object_get_ex(j_win, "is_urgent", &j_urgent);
        json_object_object_get_ex(j_win, "layout", &j_layout);

        if (!j_id) continue;

        window_info_t *win = &sys->windows[sys->window_count++];
        memset(win, 0, sizeof(*win));

        win->id = (uint64_t)json_object_get_int64(j_id);
        win->workspace_id = j_ws_id ? (uint64_t)json_object_get_int64(j_ws_id) : 1;
        win->pid = j_pid ? json_object_get_int(j_pid) : 0;
        win->is_focused = j_focused ? json_object_get_boolean(j_focused) : FALSE;
        win->is_floating = j_floating ? json_object_get_boolean(j_floating) : FALSE;
        win->is_urgent = j_urgent ? json_object_get_boolean(j_urgent) : FALSE;

        const char *t_str = j_title ? json_object_get_string(j_title) : "Terminal";
        const char *a_str = j_app_id ? json_object_get_string(j_app_id) : "terminal";
        snprintf(win->title, sizeof(win->title), "%s", t_str);
        snprintf(win->app_id, sizeof(win->app_id), "%s", a_str);

        // Map workspace_id to workspace index
        workspace_info_t *ws = orbital_system_get_workspace_by_id(sys, win->workspace_id);
        if (ws) {
            win->workspace_idx = ws->idx;
            ws->window_count++;
        } else {
            win->workspace_idx = (int)win->workspace_id;
        }

        if (win->is_focused) {
            sys->focused_window_id = win->id;
            win->focus_anim = 1.0;
        }

        if (j_layout) {
            struct json_object *j_wsize = NULL;
            if (json_object_object_get_ex(j_layout, "window_size", &j_wsize) && json_object_is_type(j_wsize, json_type_array)) {
                if (json_object_array_length(j_wsize) >= 2) {
                    win->tile_width = json_object_get_int(json_object_array_get_idx(j_wsize, 0));
                    win->tile_height = json_object_get_int(json_object_array_get_idx(j_wsize, 1));
                }
            }
        }

        format_display_name_and_glyph(win->app_id, win->title, win->display_name, sizeof(win->display_name), win->glyph, sizeof(win->glyph));

        // Reuse cached icon or load
        for (int k = 0; k < old_count; k++) {
            if (old_wins[k].id == win->id && old_wins[k].icon_pixbuf) {
                win->icon_pixbuf = old_wins[k].icon_pixbuf;
                old_wins[k].icon_pixbuf = NULL; // Transferred ownership
                win->orbit_angle = old_wins[k].orbit_angle;
                win->focus_anim = old_wins[k].focus_anim;
                break;
            }
        }
        if (!win->icon_pixbuf) {
            win->icon_pixbuf = load_app_icon(sys->icon_theme, win->app_id);
        }
    }

    // Free any old pixbufs that were closed
    for (int k = 0; k < old_count; k++) {
        if (old_wins[k].icon_pixbuf) {
            g_object_unref(old_wins[k].icon_pixbuf);
            old_wins[k].icon_pixbuf = NULL;
        }
    }
}

void orbital_system_handle_event(orbital_system_t *sys, const char *event_type, struct json_object *event_data) {
    if (!event_type) return;

    if (strcmp(event_type, "ConnectionLost") == 0) {
        sys->is_connected = FALSE;
        return;
    } else if (strcmp(event_type, "ConnectionRestored") == 0) {
        sys->is_connected = TRUE;
        return;
    }

    if (strcmp(event_type, "WorkspacesChanged") == 0) {
        struct json_object *ws_arr = NULL;
        if (event_data && json_object_object_get_ex(event_data, "workspaces", &ws_arr)) {
            orbital_system_update_workspaces_json(sys, ws_arr);
        }
    } else if (strcmp(event_type, "WorkspaceActivated") == 0) {
        struct json_object *j_id = NULL, *j_focused = NULL;
        if (event_data) {
            json_object_object_get_ex(event_data, "id", &j_id);
            json_object_object_get_ex(event_data, "focused", &j_focused);
            if (j_id) {
                uint64_t id = (uint64_t)json_object_get_int64(j_id);
                workspace_info_t *ws = orbital_system_get_workspace_by_id(sys, id);
                if (ws) {
                    sys->active_workspace_idx = ws->idx;
                    for (int i = 0; i < sys->workspace_count; i++) {
                        sys->workspaces[i].is_active = (sys->workspaces[i].idx == ws->idx);
                    }
                }
            }
        }
    } else if (strcmp(event_type, "WindowsChanged") == 0) {
        struct json_object *win_arr = NULL;
        if (event_data && json_object_object_get_ex(event_data, "windows", &win_arr)) {
            orbital_system_update_windows_json(sys, win_arr);
        }
    } else if (strcmp(event_type, "WindowFocusChanged") == 0) {
        struct json_object *j_id = NULL;
        if (event_data && json_object_object_get_ex(event_data, "id", &j_id)) {
            if (!json_object_is_type(j_id, json_type_null)) {
                uint64_t win_id = (uint64_t)json_object_get_int64(j_id);
                sys->focused_window_id = win_id;
                for (int i = 0; i < sys->window_count; i++) {
                    sys->windows[i].is_focused = (sys->windows[i].id == win_id);
                    if (sys->windows[i].is_focused) {
                        sys->windows[i].focus_anim = 1.0;
                    }
                }
            }
        }
    }
}

void orbital_system_tick(orbital_system_t *sys, double dt, double screen_w, double screen_h) {
    sys->time_sec += dt;

    // Smooth cinematic auto-orbit drift and breathing pitch
    sys->camera.target_yaw += dt * 0.06;
    sys->camera.target_pitch = 0.88 + 0.05 * sin(sys->time_sec * 0.4);

    // Update 3D Camera interpolation
    camera3d_update(&sys->camera, dt);

    // Rotate central reactor core
    sys->core_rot_yaw += dt * 0.45;
    sys->core_rot_pitch += dt * 0.22;
    sys->core_pulse = 0.5 + 0.5 * sin(sys->time_sec * 2.8);
    sys->scanline_phase = fmod(sys->time_sec * 0.6, 1.0);

    double cx = screen_w * 0.5;
    double cy = screen_h * 0.48;

    // Update workspaces
    for (int i = 0; i < sys->workspace_count; i++) {
        workspace_info_t *ws = &sys->workspaces[i];

        // Orbit rotation
        ws->orbit_rot += ws->rot_speed * dt;
        if (ws->orbit_rot > 2 * M_PI) ws->orbit_rot -= 2 * M_PI;

        // Pulse wave traveling around orbit
        ws->pulse_pos += (ws->is_active ? 2.4 : 1.0) * dt;
        if (ws->pulse_pos > 2 * M_PI) ws->pulse_pos -= 2 * M_PI;

        ws->y_height = 0.0;
        ws->inclination = 0.0;

        // Smooth active animation
        double target_act = ws->is_active ? 1.0 : 0.0;
        ws->active_anim += (target_act - ws->active_anim) * (1.0 - exp(-10.0 * dt));

        // Smooth hover animation
        double target_hov = (sys->hovered_workspace_idx == ws->idx) ? 1.0 : 0.0;
        ws->hover_anim += (target_hov - ws->hover_anim) * (1.0 - exp(-14.0 * dt));

        // Clean concentric radius (zero overlap)
        ws->target_radius = ws->base_radius;
        ws->current_radius = ws->base_radius;

        // Calculate Station Label 3D Position along front-facing arc of the orbit
        double label_angle = ws->is_active ? (M_PI * 0.5) :
                             ((M_PI * 0.5) + ((i % 2 == 0 ? 0.38 : -0.38) * (0.80 + (i * 0.05))));
        vec3_t label_3d = vec3_create(
            ws->current_radius * cos(label_angle),
            0.0,
            ws->current_radius * sin(label_angle)
        );

        project_to_screen(label_3d, &sys->camera, cx, cy,
                          &ws->label_screen_pos, &ws->label_scale, &ws->label_depth);
    }

    // Recount window count per workspace to guarantee synchronization
    for (int w = 0; w < sys->workspace_count; w++) {
        sys->workspaces[w].window_count = 0;
    }
    for (int i = 0; i < sys->window_count; i++) {
        for (int w = 0; w < sys->workspace_count; w++) {
            if (sys->workspaces[w].idx == sys->windows[i].workspace_idx) {
                sys->workspaces[w].window_count++;
                break;
            }
        }
    }

    // Update windows deterministic orbital positions
    for (int w = 0; w < sys->workspace_count; w++) {
        workspace_info_t *ws = &sys->workspaces[w];
        int count = ws->window_count;
        if (count <= 0) continue;

        int win_idx_in_ws = 0;
        for (int i = 0; i < sys->window_count; i++) {
            window_info_t *win = &sys->windows[i];
            if (win->workspace_idx != ws->idx) continue;

            // Deterministic angular spacing along orbit
            double target_angle = ws->orbit_rot + ((2.0 * M_PI * win_idx_in_ws) / (double)count);
            win->orbit_angle_target = target_angle;

            // Instant init on first frame
            if (fabs(win->orbit_angle) < 1e-5) {
                win->orbit_angle = target_angle;
            } else {
                double diff = target_angle - win->orbit_angle;
                while (diff > M_PI) diff -= 2 * M_PI;
                while (diff < -M_PI) diff += 2 * M_PI;
                win->orbit_angle += diff * (1.0 - exp(-12.0 * dt));
            }

            // Smooth focus and hover animations
            win->focus_anim = fmax(0.0, win->focus_anim - (dt * 1.5));
            double target_win_hov = (sys->hovered_window_id == win->id) ? 1.0 : 0.0;
            win->hover_anim += (target_win_hov - win->hover_anim) * (1.0 - exp(-15.0 * dt));

            // Position satellite directly on orbit trajectory
            double r = ws->current_radius;
            double ang = win->orbit_angle;
            vec3_t win_3d = vec3_create(
                r * cos(ang),
                ws->y_height + (sin(ws->inclination) * r * sin(ang)),
                r * sin(ang)
            );

            project_to_screen(win_3d, &sys->camera, cx, cy,
                              &win->screen_pos, &win->screen_scale, &win->screen_depth);

            win_idx_in_ws++;
        }
    }

    // Position of "+ New Station" button on outer horizon
    double outer_r = (sys->workspace_count > 0) ?
                     sys->workspaces[sys->workspace_count - 1].current_radius + 40.0 : 380.0;
    vec3_t btn_3d = vec3_create(outer_r * cos(-0.8), -15.0, outer_r * sin(-0.8));
    project_to_screen(btn_3d, &sys->camera, cx, cy,
                      &sys->create_btn_pos, &sys->create_btn_scale, NULL);
}

workspace_info_t *orbital_system_get_workspace_by_idx(orbital_system_t *sys, int idx) {
    for (int i = 0; i < sys->workspace_count; i++) {
        if (sys->workspaces[i].idx == idx) return &sys->workspaces[i];
    }
    return NULL;
}

workspace_info_t *orbital_system_get_workspace_by_id(orbital_system_t *sys, uint64_t ws_id) {
    for (int i = 0; i < sys->workspace_count; i++) {
        if (sys->workspaces[i].id == ws_id) return &sys->workspaces[i];
    }
    // Fallback search by idx
    return orbital_system_get_workspace_by_idx(sys, (int)ws_id);
}

window_info_t *orbital_system_get_window_by_id(orbital_system_t *sys, uint64_t win_id) {
    for (int i = 0; i < sys->window_count; i++) {
        if (sys->windows[i].id == win_id) return &sys->windows[i];
    }
    return NULL;
}

gboolean orbital_system_hit_test(orbital_system_t *sys, double mx, double my,
                                 uint64_t *out_win_id, int *out_ws_idx,
                                 gboolean *out_create_btn, gboolean *out_core) {
    if (out_win_id) *out_win_id = 0;
    if (out_ws_idx) *out_ws_idx = -1;
    if (out_create_btn) *out_create_btn = FALSE;
    if (out_core) *out_core = FALSE;

    // 1. Check "+ New Station" button
    double dx_btn = mx - sys->create_btn_pos.x;
    double dy_btn = my - sys->create_btn_pos.y;
    if (sqrt(dx_btn * dx_btn + dy_btn * dy_btn) < 22.0) {
        if (out_create_btn) *out_create_btn = TRUE;
        return TRUE;
    }

    // 2. Check Window Nodes (priority: closest to mouse & frontmost z-depth)
    double best_win_dist = 24.0;
    uint64_t hit_win = 0;
    for (int i = 0; i < sys->window_count; i++) {
        window_info_t *win = &sys->windows[i];
        double node_w = 48.0 * win->screen_scale;
        double node_h = 24.0 * win->screen_scale;
        double dx = mx - (win->screen_pos.x + win->drag_offset_x);
        double dy = my - (win->screen_pos.y + win->drag_offset_y);

        if (fabs(dx) <= node_w && fabs(dy) <= node_h) {
            double d = sqrt(dx * dx + dy * dy);
            if (d < best_win_dist) {
                best_win_dist = d;
                hit_win = win->id;
            }
        }
    }
    if (hit_win != 0) {
        if (out_win_id) *out_win_id = hit_win;
        // Also associate with its workspace
        window_info_t *w = orbital_system_get_window_by_id(sys, hit_win);
        if (w && out_ws_idx) *out_ws_idx = w->workspace_idx;
        return TRUE;
    }

    // 3. Check Workspace Ring Labels & Orbits
    // Check Station Labels first
    for (int i = 0; i < sys->workspace_count; i++) {
        workspace_info_t *ws = &sys->workspaces[i];
        double label_w = 70.0 * ws->label_scale;
        double label_h = 16.0 * ws->label_scale;
        double dx = mx - ws->label_screen_pos.x;
        double dy = my - ws->label_screen_pos.y;
        if (fabs(dx) <= label_w && fabs(dy) <= label_h) {
            if (out_ws_idx) *out_ws_idx = ws->idx;
            return TRUE;
        }
    }

    // Check distance to projected 3D orbit segments
    double best_orbit_dist = 14.0;
    int hit_orbit_idx = -1;
    for (int i = 0; i < sys->workspace_count; i++) {
        workspace_info_t *ws = &sys->workspaces[i];
        double r = ws->current_radius;
        const int SEGMENTS = 36;
        for (int s = 0; s < SEGMENTS; s++) {
            double a1 = (2.0 * M_PI * s) / SEGMENTS;
            double a2 = (2.0 * M_PI * (s + 1)) / SEGMENTS;
            vec3_t p1_3d = vec3_create(r * cos(a1), ws->y_height + sin(ws->inclination)*r*sin(a1), r * sin(a1));
            vec3_t p2_3d = vec3_create(r * cos(a2), ws->y_height + sin(ws->inclination)*r*sin(a2), r * sin(a2));
            vec2_t s1, s2;
            if (project_to_screen(p1_3d, &sys->camera, sys->camera.pan_x, sys->camera.pan_y, &s1, NULL, NULL) &&
                project_to_screen(p2_3d, &sys->camera, sys->camera.pan_x, sys->camera.pan_y, &s2, NULL, NULL)) {
                double dist = distance_point_to_segment(mx, my, s1.x, s1.y, s2.x, s2.y);
                if (dist < best_orbit_dist) {
                    best_orbit_dist = dist;
                    hit_orbit_idx = ws->idx;
                }
            }
        }
    }
    if (hit_orbit_idx != -1) {
        if (out_ws_idx) *out_ws_idx = hit_orbit_idx;
        return TRUE;
    }

    // 4. Check Center Core
    vec2_t core_screen;
    if (project_to_screen(vec3_create(0, 0, 0), &sys->camera, 0, 0, &core_screen, NULL, NULL)) {
        double dx = mx - core_screen.x;
        double dy = my - core_screen.y;
        if (sqrt(dx * dx + dy * dy) < 45.0) {
            if (out_core) *out_core = TRUE;
            return TRUE;
        }
    }

    return FALSE;
}
