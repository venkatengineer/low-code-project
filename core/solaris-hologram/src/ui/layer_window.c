#include "layer_window.h"
#include "render_core.h"
#include "render_orbits.h"
#include "render_nodes.h"
#include "render_hud.h"
#include "orbital_core.h"
#include <gdk/gdkkeysyms.h>
#include <malloc.h>

static void solaris_play_sound(const char *name) {
    if (!name) return;
    char cmd[320];
    snprintf(cmd, sizeof(cmd), "pw-play /mnt/d/linux/projects/solaris-os/assets/sounds/edex/%s.wav 2>/dev/null || paplay /mnt/d/linux/projects/solaris-os/assets/sounds/edex/%s.wav 2>/dev/null &", name, name);
    system(cmd);
}

static void niri_event_handler(const char *event_type, struct json_object *event_data, gpointer user_data) {
    hologram_app_t *app = (hologram_app_t *)user_data;
    orbital_system_handle_event(&app->sys, event_type, event_data);
    for (int i = 0; i < app->view_count; i++) {
        if (app->views[i].drawing_area) {
            gtk_widget_queue_draw(app->views[i].drawing_area);
        }
    }
}

static gboolean on_frame_tick(gpointer user_data) {
    hologram_app_t *app = (hologram_app_t *)user_data;

    static int frame_count = 0;
    if (++frame_count % 120 == 0) {
        malloc_trim(0);
    }

    struct timeval now;
    gettimeofday(&now, NULL);
    double dt = (now.tv_sec - app->last_time.tv_sec) +
                (now.tv_usec - app->last_time.tv_usec) * 1e-6;
    app->last_time = now;

    if (dt > 0.1) dt = 0.1;

    double w = app->is_command_center_mode ? 1706.0 : 740.0;
    double h = app->is_command_center_mode ? 1066.0 : 680.0;

    // Simulation tick
    orbital_system_tick(&app->sys, dt, w, h);
    particle_system_update(&app->ps, dt, (app->sys.active_workspace_idx > 0));

    // Update hover states
    if (app->mouse_in_window) {
        uint64_t win_id;
        int ws_idx;
        gboolean create_btn, core;
        if (orbital_system_hit_test(&app->sys, app->mouse_x, app->mouse_y,
                                     &win_id, &ws_idx, &create_btn, &core)) {
            app->sys.hovered_window_id = win_id;
            app->sys.hovered_workspace_idx = ws_idx;
            app->sys.hovered_create_btn = create_btn;
            app->sys.hovered_core = core;
        } else {
            app->sys.hovered_window_id = 0;
            app->sys.hovered_workspace_idx = -1;
            app->sys.hovered_create_btn = FALSE;
            app->sys.hovered_core = FALSE;
        }
    } else {
        app->sys.hovered_window_id = 0;
        app->sys.hovered_workspace_idx = -1;
        app->sys.hovered_create_btn = FALSE;
        app->sys.hovered_core = FALSE;
    }

    for (int i = 0; i < app->view_count; i++) {
        if (app->views[i].drawing_area) {
            gtk_widget_queue_draw(app->views[i].drawing_area);
        }
    }

    return G_SOURCE_CONTINUE;
}

static double get_hologram_awakening_elapsed(void) {
    static double s_cached_stamp = 0.0;
    static double s_last_check_time = 0.0;
    static gboolean s_is_locked = FALSE;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    double now = (double)tv.tv_sec + ((double)tv.tv_usec / 1000000.0);

    if (now - s_last_check_time > 0.030) {
        s_last_check_time = now;

        FILE *lfp = fopen("/tmp/solaris_lock_state", "r");
        if (lfp) {
            char lbuf[16];
            if (fgets(lbuf, sizeof(lbuf), lfp) && strncmp(lbuf, "LOCKED", 6) == 0) {
                s_is_locked = TRUE;
            } else {
                s_is_locked = FALSE;
            }
            fclose(lfp);
        } else {
            s_is_locked = FALSE;
        }

        FILE *fp = fopen("/tmp/solaris_awakening_time", "r");
        if (fp) {
            double stamp = 0.0;
            if (fscanf(fp, "%lf", &stamp) == 1) {
                s_cached_stamp = stamp;
            }
            fclose(fp);
        }
    }

    if (s_is_locked) return -1.0;
    if (s_cached_stamp <= 0.0) return 999.0;
    double elapsed = now - s_cached_stamp;
    if (elapsed < 0.0 || elapsed > 6.0) return 999.0;
    return elapsed;
}

static void draw_hologram_awakening_overlay(cairo_t *cr, double w, double h, double start_delay, double duration) {
    double el = get_hologram_awakening_elapsed();
    if (el >= 999.0) return;

    cairo_save(cr);
    cairo_rectangle(cr, 0, 0, w, h);
    cairo_clip(cr);

    if (el < 0.0 || el < start_delay) {
        cairo_rectangle(cr, 0, 0, w, h);
        cairo_set_source_rgba(cr, 0.00, 0.01, 0.02, 0.96);
        cairo_fill(cr);
        cairo_restore(cr);
        return;
    }

    if (el >= (start_delay + duration + 0.25)) {
        cairo_restore(cr);
        return;
    }

    double p = (el - start_delay) / duration;
    p = CLAMP(p, 0.0, 1.0);
    double inv_p = 1.0 - p;

    if (p < 1.0) {
        double dim_alpha = (1.0 - p) * 0.88;
        cairo_rectangle(cr, 0, 0, w, h);
        cairo_set_source_rgba(cr, 0.00, 0.02, 0.04, dim_alpha);
        cairo_fill(cr);
    }

    // Laser Scanline
    double scan_y = p * h;
    cairo_move_to(cr, 0, scan_y);
    cairo_line_to(cr, w, scan_y);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, inv_p * 0.95);
    cairo_set_line_width(cr, 2.2);
    cairo_stroke(cr);

    // Border Flash
    cairo_rectangle(cr, 1.0, 1.0, w - 2.0, h - 2.0);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, inv_p * 0.75);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);

    // Diagnostic HUD Pill
    if (p > 0.12 && p < 0.88) {
        double bw = 180.0;
        double bh = 22.0;
        double bx = (w * 0.5) - (bw * 0.5);
        double by = (h * 0.5) - (bh * 0.5);

        cairo_rectangle(cr, bx, by, bw, bh);
        cairo_set_source_rgba(cr, 0.01, 0.03, 0.06, 0.94);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.90);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);

        cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 7.5);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.98);
        cairo_move_to(cr, bx + 10.0, by + 14.5);
        cairo_show_text(cr, "⚡ 3D QUANTUM CORE // ONLINE");
    }

    cairo_restore(cr);
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    hologram_app_t *app = (hologram_app_t *)user_data;
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    double w = (double)width;
    double h = (double)height;

    // Center coordinates
    double cx = app->is_command_center_mode ? (w * 0.55) : (w * 0.5);
    double cy = app->is_command_center_mode ? (h * 0.50) : (h * 0.48);

    // 1. Clear background to full alpha transparency
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    // 2. Command Center background overlay (when active)
    if (app->is_command_center_mode) {
        cairo_save(cr);
        cairo_set_source_rgba(cr, 0.015, 0.035, 0.065, 0.92);
        cairo_rectangle(cr, 0, 0, w, h);
        cairo_fill(cr);

        cairo_pattern_t *vignette = cairo_pattern_create_radial(cx, cy, 0, cx, cy, w * 0.7);
        cairo_pattern_add_color_stop_rgba(vignette, 0.0, 0.0, 0.0, 0.0, 0.0);
        cairo_pattern_add_color_stop_rgba(vignette, 1.0, 0.0, 0.0, 0.0, 0.60);
        cairo_set_source(cr, vignette);
        cairo_rectangle(cr, 0, 0, w, h);
        cairo_fill(cr);
        cairo_pattern_destroy(vignette);
        cairo_restore(cr);
    }

    // 3. Technical Grid & HUD Brackets
    render_hud_background_and_grid(cr, &app->sys, w, h);

    // 4. Stardust & Sparks Particles
    particle_system_render(&app->ps, cr, &app->sys.camera, cx, cy, app->sys.is_connected);

    // 5. Planetary Holographic Core & Orbital Workspace System (Pure 60FPS Planet Mode)
    cairo_save(cr);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    // Back-half of Orbital Rings (depth behind core)
    render_orbital_rings_back(cr, &app->sys, cx, cy);

    // Central Glowing Planetary Core (Reactor, Dual Geodesic Shields, Astrolabes, Corona)
    render_session_core(cr, &app->sys, cx, cy);

    // Front-half of Orbital Rings (depth in front of core)
    render_orbital_rings_front(cr, &app->sys, cx, cy);

    // Depth-sorted Window Nodes (with tethers, glowing borders, badges)
    render_application_nodes(cr, &app->sys, cx, cy);

    // Station Labels & Controls (+ Button, Station Badges)
    render_station_labels_and_controls(cr, &app->sys, cx, cy);

    cairo_restore(cr);

    // 6. Foreground HUD Overlays
    if (app->is_command_center_mode) {
        command_center_render(cr, &app->cc, &app->sys, w, h);
        if (!app->cc.is_active) {
            layer_window_set_mode(app, FALSE);
        }
    } else {
        // Compact Floating Hover Telemetry Card
        render_hud_hover_card(cr, &app->sys, app->mouse_x, app->mouse_y, w, h);
    }

    // 7. Connection Status Banner (if Niri offline)
    render_hud_connection_status(cr, &app->sys, w, h);

    // 8. Spacecraft Sequential Power-On Awakening Overlay (Starts at 2.0s)
    draw_hologram_awakening_overlay(cr, w, h, 2.0, 0.8);

    return FALSE;
}

void layer_window_set_keyboard_active(hologram_app_t *app, gboolean active) {
    if (app->is_command_center_mode) return;
    for (int i = 0; i < app->view_count; i++) {
        GtkWidget *win = app->views[i].window;
        if (win) {
            gtk_layer_set_keyboard_interactivity(GTK_WINDOW(win), active);
        }
    }
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    hologram_app_t *app = (hologram_app_t *)user_data;
    double mx = event->x;
    double my = event->y;

    app->drag_start_x = mx;
    app->drag_start_y = my;
    app->drag_last_x = mx;
    app->drag_last_y = my;
    app->is_dragging_camera = TRUE;

    app->pending_tab = -1;
    app->pending_hotspot = HOTSPOT_NONE;
    app->pending_term_btn = -1;

    if (app->is_command_center_mode) {
        if (command_center_handle_click(&app->cc, &app->sys, &app->ipc, mx, my)) {
            return TRUE;
        }
    }

    if (event->type == GDK_2BUTTON_PRESS) {
        camera3d_reset(&app->sys.camera);
        solaris_play_sound("granted");
        return TRUE;
    }

    // Window Nodes & Workspace Orbit Hit Testing (Active Planetary State)
    uint64_t hit_win;
    int hit_ws;
    gboolean hit_create, hit_core;

    if (orbital_system_hit_test(&app->sys, mx, my, &hit_win, &hit_ws, &hit_create, &hit_core)) {
        if (hit_create) {
            int next_idx = app->sys.workspace_count + 1;
            niri_ipc_create_new_workspace(&app->ipc, next_idx);
            solaris_play_sound("expand");
            return TRUE;
        } else if (hit_win != 0) {
            niri_ipc_focus_window(&app->ipc, hit_win);
            solaris_play_sound("panels");
            return TRUE;
        } else if (hit_ws != -1) {
            niri_ipc_focus_workspace_idx(&app->ipc, hit_ws);
            solaris_play_sound("folder");
            return TRUE;
        } else if (hit_core) {
            solaris_play_sound("scan");
            return TRUE;
        }
    }

    return TRUE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    hologram_app_t *app = (hologram_app_t *)user_data;
    app->is_dragging_camera = FALSE;
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    hologram_app_t *app = (hologram_app_t *)user_data;
    app->mouse_x = event->x;
    app->mouse_y = event->y;
    app->mouse_in_window = TRUE;

    if (app->is_dragging_camera) {
        double dx = event->x - app->drag_last_x;
        double dy = event->y - app->drag_last_y;
        app->drag_last_x = event->x;
        app->drag_last_y = event->y;

        app->sys.camera.target_yaw += dx * 0.007;
        app->sys.camera.target_pitch = fmax(-1.1, fmin(1.1, app->sys.camera.target_pitch - dy * 0.007));
        gtk_widget_queue_draw(widget);
    }

    // Audio feedback on hovering new objects
    static uint64_t last_hovered_win = 0;
    static int last_hovered_ws = -1;
    if ((app->sys.hovered_window_id != 0 && app->sys.hovered_window_id != last_hovered_win) ||
        (app->sys.hovered_workspace_idx != -1 && app->sys.hovered_workspace_idx != last_hovered_ws)) {
        solaris_play_sound("scan");
        last_hovered_win = app->sys.hovered_window_id;
        last_hovered_ws = app->sys.hovered_workspace_idx;
    }

    return FALSE;
}

static gboolean on_leave_notify(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
    hologram_app_t *app = (hologram_app_t *)user_data;
    app->mouse_in_window = FALSE;
    app->is_dragging_camera = FALSE;
    return FALSE;
}

static gboolean on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
    hologram_app_t *app = (hologram_app_t *)user_data;
    if (event->direction == GDK_SCROLL_UP) {
        app->sys.camera.target_distance = fmax(220.0, app->sys.camera.target_distance - 45.0);
    } else if (event->direction == GDK_SCROLL_DOWN) {
        app->sys.camera.target_distance = fmin(950.0, app->sys.camera.target_distance + 45.0);
    } else if (event->direction == GDK_SCROLL_SMOOTH) {
        app->sys.camera.target_distance = fmax(220.0, fmin(950.0, app->sys.camera.target_distance + event->delta_y * 35.0));
    }
    gtk_widget_queue_draw(widget);
    return TRUE;
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    hologram_app_t *app = (hologram_app_t *)user_data;

    if (app->is_command_center_mode) {
        return command_center_handle_key(&app->cc, &app->sys, &app->ipc, event->keyval);
    }

    if (event->keyval >= GDK_KEY_1 && event->keyval <= GDK_KEY_6) {
        niri_ipc_focus_workspace_idx(&app->ipc, event->keyval - GDK_KEY_0);
        return TRUE;
    }

    if (event->keyval == GDK_KEY_h || event->keyval == GDK_KEY_H) {
        layer_window_toggle_mode(app);
        return TRUE;
    }

    return FALSE;
}

void layer_window_set_mode(hologram_app_t *app, gboolean command_center_mode) {
    app->is_command_center_mode = command_center_mode;
    app->cc.is_active = command_center_mode;

    for (int i = 0; i < app->view_count; i++) {
        GtkWidget *win = app->views[i].window;
        GtkWidget *da = app->views[i].drawing_area;
        GdkMonitor *mon = app->views[i].monitor;
        if (!win || !da) continue;

        if (command_center_mode) {
            gtk_layer_set_layer(GTK_WINDOW(win), GTK_LAYER_SHELL_LAYER_OVERLAY);
            gtk_layer_set_keyboard_interactivity(GTK_WINDOW(win), TRUE);

            gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
            gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
            gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
            gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);

            gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_TOP, 0);
            gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_BOTTOM, 0);
            gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_LEFT, 0);
            gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_RIGHT, 0);

            GdkRectangle geom = {0, 0, 1920, 1080};
            if (mon) gdk_monitor_get_geometry(mon, &geom);
            gtk_widget_set_size_request(win, geom.width, geom.height);
            gtk_widget_set_size_request(da, geom.width, geom.height);
        } else {
            gtk_layer_set_layer(GTK_WINDOW(win), GTK_LAYER_SHELL_LAYER_BOTTOM);
            gtk_layer_set_keyboard_interactivity(GTK_WINDOW(win), FALSE);

            gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
            gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE);
            gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_LEFT, FALSE);
            gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_RIGHT, FALSE);

            gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_TOP, 48);
            gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_BOTTOM, 0);
            gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_LEFT, 0);
            gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_RIGHT, 0);

            gtk_widget_set_size_request(win, 740, 640);
            gtk_widget_set_size_request(da, 740, 640);
        }

        gtk_widget_queue_resize(win);
        gtk_widget_queue_draw(da);
    }
}

void layer_window_toggle_mode(hologram_app_t *app) {
    layer_window_set_mode(app, !app->is_command_center_mode);
}

static void load_hologram_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css =
        "window { background-color: transparent; }"
        "* { font-family: 'JetBrainsMono Nerd Font', monospace; }";
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    GdkScreen *screen = gdk_screen_get_default();
    if (screen) {
        gtk_style_context_add_provider_for_screen(
            screen,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
    g_object_unref(provider);
}

static void create_view_for_monitor(hologram_app_t *app, GdkMonitor *monitor, gboolean start_as_command_center) {
    if (!monitor) return;

    for (int i = 0; i < app->view_count; i++) {
        if (app->views[i].monitor == monitor) return; // already exists
    }

    if (app->view_count >= MAX_HOLOGRAM_MONITORS) return;

    int idx = app->view_count;
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "solaris-bg-hologram");

    // True background transparency
    GdkScreen *screen = gtk_widget_get_screen(win);
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual && gdk_screen_is_composited(screen)) {
        gtk_widget_set_visual(win, visual);
    }
    gtk_widget_set_app_paintable(win, TRUE);

    // Initialize layer shell
    gtk_layer_init_for_window(GTK_WINDOW(win));
    gtk_layer_set_monitor(GTK_WINDOW(win), monitor);
    gtk_layer_set_layer(GTK_WINDOW(win), start_as_command_center ? GTK_LAYER_SHELL_LAYER_OVERLAY : GTK_LAYER_SHELL_LAYER_BOTTOM);
    gtk_layer_set_keyboard_interactivity(GTK_WINDOW(win), start_as_command_center);
    gtk_layer_set_namespace(GTK_WINDOW(win), "solaris-bg");

    GtkWidget *da = gtk_drawing_area_new();

    if (start_as_command_center) {
        gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_TOP, 0);
        gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_BOTTOM, 0);
        gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_LEFT, 0);
        gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_RIGHT, 0);

        GdkRectangle geom = {0, 0, 1920, 1080};
        gdk_monitor_get_geometry(monitor, &geom);
        gtk_widget_set_size_request(win, geom.width, geom.height);
        gtk_widget_set_size_request(da, geom.width, geom.height);
    } else {
        gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE);
        gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_LEFT, FALSE);
        gtk_layer_set_anchor(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_RIGHT, FALSE);
        gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_TOP, 48);
        gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_BOTTOM, 0);
        gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_LEFT, 0);
        gtk_layer_set_margin(GTK_WINDOW(win), GTK_LAYER_SHELL_EDGE_RIGHT, 0);
        gtk_widget_set_size_request(win, 740, 640);
        gtk_widget_set_size_request(da, 740, 640);
    }

    gtk_widget_add_events(da,
                          GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK |
                          GDK_SCROLL_MASK |
                          GDK_ENTER_NOTIFY_MASK |
                          GDK_LEAVE_NOTIFY_MASK);

    g_signal_connect(da, "draw", G_CALLBACK(on_draw), app);
    g_signal_connect(da, "button-press-event", G_CALLBACK(on_button_press), app);
    g_signal_connect(da, "button-release-event", G_CALLBACK(on_button_release), app);
    g_signal_connect(da, "motion-notify-event", G_CALLBACK(on_motion_notify), app);
    g_signal_connect(da, "leave-notify-event", G_CALLBACK(on_leave_notify), app);
    g_signal_connect(da, "scroll-event", G_CALLBACK(on_scroll), app);
    g_signal_connect(win, "key-press-event", G_CALLBACK(on_key_press), app);

    gtk_container_add(GTK_CONTAINER(win), da);

    app->views[idx].window = win;
    app->views[idx].drawing_area = da;
    app->views[idx].monitor = monitor;
    app->view_count++;

    if (idx == 0) {
        app->window = win;
        app->drawing_area = da;
    }

    gtk_widget_show_all(win);
}

static void on_monitor_added(GdkDisplay *display, GdkMonitor *monitor, gpointer user_data) {
    hologram_app_t *app = (hologram_app_t *)user_data;
    create_view_for_monitor(app, monitor, app->is_command_center_mode);
}

static void on_monitor_removed(GdkDisplay *display, GdkMonitor *monitor, gpointer user_data) {
    hologram_app_t *app = (hologram_app_t *)user_data;
    for (int i = 0; i < app->view_count; i++) {
        if (app->views[i].monitor == monitor) {
            gtk_widget_destroy(app->views[i].window);
            for (int j = i; j < app->view_count - 1; j++) {
                app->views[j] = app->views[j + 1];
            }
            app->view_count--;
            break;
        }
    }
    if (app->view_count > 0) {
        app->window = app->views[0].window;
        app->drawing_area = app->views[0].drawing_area;
    } else {
        app->window = NULL;
        app->drawing_area = NULL;
    }
}

gboolean layer_window_init(hologram_app_t *app, gboolean start_as_command_center) {
    load_hologram_css();

    app->view_count = 0;
    app->window = NULL;
    app->drawing_area = NULL;

    GdkDisplay *display = gdk_display_get_default();
    if (!display) {
        fprintf(stderr, "[Solaris Error] Failed to get GdkDisplay.\n");
        return FALSE;
    }

    int n_monitors = gdk_display_get_n_monitors(display);
    for (int i = 0; i < n_monitors; i++) {
        GdkMonitor *mon = gdk_display_get_monitor(display, i);
        create_view_for_monitor(app, mon, start_as_command_center);
    }

    g_signal_connect(display, "monitor-added", G_CALLBACK(on_monitor_added), app);
    g_signal_connect(display, "monitor-removed", G_CALLBACK(on_monitor_removed), app);

    // Subsystems
    orbital_system_init(&app->sys);
    particle_system_init(&app->ps);
    command_center_init(&app->cc);

    app->is_command_center_mode = start_as_command_center;
    app->cc.is_active = start_as_command_center;

    // Niri IPC
    niri_ipc_init(&app->ipc, niri_event_handler, app);

    struct json_object *j_ws = NULL, *j_wins = NULL;
    if (niri_ipc_get_workspaces_and_windows(&app->ipc, &j_ws, &j_wins)) {
        if (j_ws) {
            orbital_system_update_workspaces_json(&app->sys, j_ws);
            json_object_put(j_ws);
        }
        if (j_wins) {
            orbital_system_update_windows_json(&app->sys, j_wins);
            json_object_put(j_wins);
        }
    }

    gettimeofday(&app->last_time, NULL);
    app->timer_id = g_timeout_add(16, on_frame_tick, app);

    return (app->view_count > 0);
}
