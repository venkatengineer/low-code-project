#include "common.h"
#include "cairo_utils.h"
#include "telemetry.h"
#include <json-c/json.h>

typedef struct {
    char title[128];
    char app_id[64];
    uint64_t win_id;
    int workspace_idx;
    gboolean is_focused;
} focus_context_t;

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    focus_context_t focus;
    gboolean is_search_hovered;
    double time_sec;
    guint anim_timer_id;
    guint telem_timer_id;
} conduit_app_t;

static conduit_app_t g_app;

static void update_focus_context(conduit_app_t *app) {
    memset(&app->focus, 0, sizeof(app->focus));

    FILE *fp = popen("niri msg -j windows 2>/dev/null", "r");
    if (fp) {
        GString *str = g_string_new("");
        char buf[512];
        while (fgets(buf, sizeof(buf), fp)) {
            g_string_append(str, buf);
        }
        pclose(fp);

        if (str->len > 0) {
            struct json_object *parsed = json_tokener_parse(str->str);
            if (parsed && json_object_is_type(parsed, json_type_array)) {
                size_t len = json_object_array_length(parsed);
                for (size_t i = 0; i < len; i++) {
                    struct json_object *j_win = json_object_array_get_idx(parsed, i);
                    struct json_object *j_foc = NULL, *j_title = NULL, *j_app_id = NULL, *j_id = NULL, *j_ws = NULL;
                    json_object_object_get_ex(j_win, "is_focused", &j_foc);
                    if (j_foc && json_object_get_boolean(j_foc)) {
                        json_object_object_get_ex(j_win, "title", &j_title);
                        json_object_object_get_ex(j_win, "app_id", &j_app_id);
                        json_object_object_get_ex(j_win, "id", &j_id);
                        json_object_object_get_ex(j_win, "workspace_id", &j_ws);

                        app->focus.is_focused = TRUE;
                        if (j_id) app->focus.win_id = (uint64_t)json_object_get_int64(j_id);
                        if (j_ws) app->focus.workspace_idx = json_object_get_int(j_ws);
                        if (j_title) snprintf(app->focus.title, sizeof(app->focus.title), "%s", json_object_get_string(j_title));
                        if (j_app_id) snprintf(app->focus.app_id, sizeof(app->focus.app_id), "%s", json_object_get_string(j_app_id));
                        break;
                    }
                }
                json_object_put(parsed);
            }
        }
        g_string_free(str, TRUE);
    }
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    (void)user_data;

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);

    double pad_x = 8.0;
    double cur_y = 6.0;
    double panel_w = (double)width - 16.0;
    double panel_h = (double)height - 12.0;

    // 1. Long Horizontal Command Conduit Beveled Rail
    cairo_draw_beveled_panel(cr, pad_x, cur_y, panel_w, panel_h, 6.0);

    // 2. Physical Structural Conduits (Connecting Left Mission Queue, Center Singularity, Right Flight Deck)
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.45);
    cairo_set_line_width(cr, 1.2);
    // Left junction node (up to Mission Queue)
    cairo_move_to(cr, pad_x + 80.0, cur_y);
    cairo_line_to(cr, pad_x + 80.0, 0);
    cairo_stroke(cr);
    cairo_arc(cr, pad_x + 80.0, cur_y, 2.5, 0, 2 * M_PI);
    cairo_fill(cr);

    // Right junction node (up to Flight Deck)
    cairo_move_to(cr, pad_x + panel_w - 80.0, cur_y);
    cairo_line_to(cr, pad_x + panel_w - 80.0, 0);
    cairo_stroke(cr);
    cairo_arc(cr, pad_x + panel_w - 80.0, cur_y, 2.5, 0, 2 * M_PI);
    cairo_fill(cr);

    // Center vertical laser conduit (connects up to Central Holographic Singularity)
    double center_x = pad_x + (panel_w * 0.5);
    cairo_move_to(cr, center_x, cur_y);
    cairo_line_to(cr, center_x, 0);
    cairo_stroke(cr);
    cairo_arc(cr, center_x, cur_y, 2.5, 0, 2 * M_PI);
    cairo_fill(cr);

    // 3. Left Section: CONTEXT FOCUS & TELEMETRY
    double lx = pad_x + 18.0;
    double ly = cur_y + 12.0;

    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 8.5);

    char focus_header[128];
    if (g_app.focus.is_focused) {
        snprintf(focus_header, sizeof(focus_header), "◆ FOCUS // %s", g_app.focus.app_id[0] ? g_app.focus.app_id : "APPLICATION");
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.98);
    } else {
        snprintf(focus_header, sizeof(focus_header), "◆ COMMAND CONDUIT // IDLE LINK");
        cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.90);
    }
    cairo_move_to(cr, lx, ly + 10.0);
    cairo_show_text(cr, focus_header);

    // Secondary line (window title or system state)
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 7.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.2, COLOR_TEXT_MUTED_G + 0.2, COLOR_TEXT_MUTED_B + 0.2, 0.85);

    char sub_focus[128];
    if (g_app.focus.is_focused) {
        snprintf(sub_focus, sizeof(sub_focus), "TITLE: %.38s", g_app.focus.title);
    } else {
        snprintf(sub_focus, sizeof(sub_focus), "HIGH-SPEED DESKTOP DISPATCH BUS NOMINAL");
    }
    cairo_move_to(cr, lx, ly + 26.0);
    cairo_show_text(cr, sub_focus);

    // 4. Center Section: EXPANDED PROMINENT COMMAND SEARCH CONSOLE
    double search_w = 680.0;
    double search_x = pad_x + ((panel_w - search_w) * 0.5);
    double search_y = cur_y + 14.0;
    double search_h = 36.0;
    gboolean is_hov = g_app.is_search_hovered;

    cairo_save(cr);
    cairo_new_path(cr);
    double sc = 6.0;
    cairo_move_to(cr, search_x + sc, search_y);
    cairo_line_to(cr, search_x + search_w - sc, search_y);
    cairo_line_to(cr, search_x + search_w, search_y + sc);
    cairo_line_to(cr, search_x + search_w, search_y + search_h - sc);
    cairo_line_to(cr, search_x + search_w - sc, search_y + search_h);
    cairo_line_to(cr, search_x + sc, search_y + search_h);
    cairo_line_to(cr, search_x, search_y + search_h - sc);
    cairo_line_to(cr, search_x, search_y + sc);
    cairo_close_path(cr);

    if (is_hov) {
        cairo_set_source_rgba(cr, 0.02, 0.10, 0.18, 0.96);
    } else {
        cairo_set_source_rgba(cr, 0.015, 0.045, 0.085, 0.75);
    }
    cairo_fill_preserve(cr);

    if (is_hov) {
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 1.0);
        cairo_set_line_width(cr, 1.5);
    } else {
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.50);
        cairo_set_line_width(cr, 1.0);
    }
    cairo_stroke(cr);

    // Decorative chamfer corner ticks on search bar
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, is_hov ? 1.0 : 0.75);
    cairo_set_line_width(cr, 1.8);
    // Top-left tick
    cairo_move_to(cr, search_x + 4.0, search_y);
    cairo_line_to(cr, search_x, search_y + 4.0);
    cairo_stroke(cr);
    // Top-right tick
    cairo_move_to(cr, search_x + search_w - 4.0, search_y);
    cairo_line_to(cr, search_x + search_w, search_y + 4.0);
    cairo_stroke(cr);
    // Bottom-left tick
    cairo_move_to(cr, search_x, search_y + search_h - 4.0);
    cairo_line_to(cr, search_x + 4.0, search_y + search_h);
    cairo_stroke(cr);
    // Bottom-right tick
    cairo_move_to(cr, search_x + search_w - 4.0, search_y + search_h);
    cairo_line_to(cr, search_x + search_w, search_y + search_h - 4.0);
    cairo_stroke(cr);

    // Search Glyph & Placeholder Text
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 10.5);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, is_hov ? 1.0 : 0.90);
    cairo_move_to(cr, search_x + 18.0, search_y + 22.5);
    cairo_show_text(cr, "⌕");

    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 8.2);
    if (is_hov) {
        cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 1.0);
    } else {
        cairo_set_source_rgba(cr, COLOR_CYAN_R * 0.92, COLOR_CYAN_G * 0.92, COLOR_CYAN_B * 0.95, 0.88);
    }
    cairo_move_to(cr, search_x + 36.0, search_y + 22.0);
    cairo_show_text(cr, "SEARCH APPLICATIONS, FILES, SYSTEM COMMANDS...");

    // Right shortcut badge on search pill
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 7.5);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, is_hov ? 0.95 : 0.60);
    const char *sc_tag = "[ SUPER + SPACE ]";
    cairo_text_extents_t scte;
    cairo_text_extents(cr, sc_tag, &scte);
    cairo_move_to(cr, search_x + search_w - scte.width - 16.0, search_y + 22.0);
    cairo_show_text(cr, sc_tag);

    cairo_restore(cr);

    // 5. Right Section: AVIONICS STATUS & SHORTCUTS
    double rx = pad_x + panel_w - 380.0;
    double ry = cur_y + 12.0;

    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.90);
    cairo_move_to(cr, rx, ry + 10.0);
    cairo_show_text(cr, "AVIONICS DISPATCH // NIRI WAYLAND");

    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 7.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.2, COLOR_TEXT_MUTED_G + 0.2, COLOR_TEXT_MUTED_B + 0.2, 0.85);
    cairo_move_to(cr, rx, ry + 26.0);
    cairo_show_text(cr, "LAUNCH: [SUPER+SPACE] // RADAR: [SUPER+D]");

    // Spacecraft Sequential Power-On Awakening Overlay (Starts at 0.4s)
    solaris_draw_awakening_overlay(cr, pad_x, cur_y, panel_w, panel_h, 0.4, 0.9, "COMMAND CONDUIT");

    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    (void)user_data;
    double x = event->x;
    double y = event->y;

    double pad_x = 8.0;
    double cur_y = 6.0;
    double panel_w = (double)gtk_widget_get_allocated_width(widget) - 16.0;
    double search_w = 680.0;
    double search_x = pad_x + ((panel_w - search_w) * 0.5);
    double search_y = cur_y + 14.0;
    double search_h = 36.0;

    gboolean old_hov = g_app.is_search_hovered;
    g_app.is_search_hovered = (x >= search_x && x <= search_x + search_w && y >= search_y && y <= search_y + search_h);

    if (old_hov != g_app.is_search_hovered) {
        gtk_widget_queue_draw(widget);
    }
    return FALSE;
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)user_data;
    if (event->button == 1 && g_app.is_search_hovered) {
        g_spawn_command_line_async("fuzzel", NULL);
        g_timeout_add(100, (GSourceFunc)on_draw, widget);
    }
    return FALSE;
}

static gboolean on_anim_tick(gpointer user_data) {
    (void)user_data;
    g_app.time_sec += 0.05;
    if (g_app.drawing_area) {
        gtk_widget_queue_draw(g_app.drawing_area);
    }
    return G_SOURCE_CONTINUE;
}

static gboolean on_telem_tick(gpointer user_data) {
    (void)user_data;
    update_focus_context(&g_app);
    solaris_trim_memory();
    if (g_app.drawing_area) {
        gtk_widget_queue_draw(g_app.drawing_area);
    }
    return G_SOURCE_CONTINUE;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    solaris_load_css();

    memset(&g_app, 0, sizeof(g_app));

    g_app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    solaris_setup_layer_window(g_app.window, "solaris-bg-conduit",
                                1674, 76,
                                TRUE, FALSE, TRUE, FALSE,
                                704, 0, 16, 0);

    g_app.drawing_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(g_app.window), g_app.drawing_area);

    gtk_widget_add_events(g_app.drawing_area,
                          GDK_POINTER_MOTION_MASK |
                          GDK_BUTTON_PRESS_MASK |
                          GDK_LEAVE_NOTIFY_MASK);

    g_signal_connect(g_app.drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(g_app.drawing_area, "motion-notify-event", G_CALLBACK(on_motion_notify), NULL);
    g_signal_connect(g_app.drawing_area, "button-press-event", G_CALLBACK(on_button_press), NULL);

    update_focus_context(&g_app);

    g_app.anim_timer_id = g_timeout_add(50, on_anim_tick, NULL);
    g_app.telem_timer_id = g_timeout_add(250, on_telem_tick, NULL);

    gtk_widget_show_all(g_app.window);
    gtk_main();
    return 0;
}
