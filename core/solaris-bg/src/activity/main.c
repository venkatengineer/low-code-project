#include "common.h"
#include "cairo_utils.h"
#include "telemetry.h"
#include <dirent.h>
#include <sys/stat.h>

#define MAX_EVENTS 32

typedef enum {
    EVENT_SUCCESS = 0,
    EVENT_WARN,
    EVENT_ERROR,
    EVENT_INFO
} event_type_t;

typedef struct {
    char time_str[16];
    char title[64];
    char subtitle[128];
    char action_cmd[256];
    event_type_t type;
} mission_event_t;

#define MAX_ACTIVITY_MONITORS 8

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    GtkWidget *windows[MAX_ACTIVITY_MONITORS];
    GtkWidget *drawing_areas[MAX_ACTIVITY_MONITORS];
    int view_count;
    mission_event_t events[MAX_EVENTS];
    int event_count;
    int scroll_offset;
    int hovered_idx;
    double time_sec;
    guint telem_timer_id;
    guint anim_timer_id;
} activity_app_t;

static activity_app_t g_app;

static void redraw_all_views(void) {
    for (int i = 0; i < g_app.view_count; i++) {
        if (g_app.drawing_areas[i]) {
            gtk_widget_queue_draw(g_app.drawing_areas[i]);
        }
    }
}

static void format_file_size(off_t bytes, char *buf, size_t buf_size) {
    if (bytes >= 1024 * 1024 * 1024) {
        snprintf(buf, buf_size, "%.1f GB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
    } else if (bytes >= 1024 * 1024) {
        snprintf(buf, buf_size, "%.1f MB", (double)bytes / (1024.0 * 1024.0));
    } else if (bytes >= 1024) {
        snprintf(buf, buf_size, "%ld KB", bytes / 1024);
    } else {
        snprintf(buf, buf_size, "%ld B", bytes);
    }
}

static void refresh_mission_events(void) {
    int count = 0;
    time_t now = time(NULL);

    // 1. Check Recent Downloads (~/Downloads)
    const char *home = getenv("HOME");
    char dl_path[256];
    snprintf(dl_path, sizeof(dl_path), "%s/Downloads", home ? home : "/home/venkatengineer");

    DIR *dir = opendir(dl_path);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && count < MAX_EVENTS) {
            if (entry->d_name[0] == '.') continue;

            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", dl_path, entry->d_name);

            struct stat st;
            if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
                double age_hours = difftime(now, st.st_mtime) / 3600.0;
                if (age_hours <= 120.0) {
                    struct tm *tm_info = localtime(&st.st_mtime);
                    char sz_buf[32];
                    format_file_size(st.st_size, sz_buf, sizeof(sz_buf));

                    strftime(g_app.events[count].time_str, sizeof(g_app.events[count].time_str), "%H:%M", tm_info);
                    snprintf(g_app.events[count].title, sizeof(g_app.events[count].title), "DOWNLOAD COMPLETE");
                    snprintf(g_app.events[count].subtitle, sizeof(g_app.events[count].subtitle), "%.24s // %s", entry->d_name, sz_buf);
                    snprintf(g_app.events[count].action_cmd, sizeof(g_app.events[count].action_cmd), "nautilus '%.230s'", dl_path);
                    g_app.events[count].type = EVENT_SUCCESS;
                    count++;
                }
            }
        }
        closedir(dir);
    }

    // 2. Check Package Updates (pacman / checkupdates)
    FILE *fp = popen("checkupdates 2>/dev/null | wc -l || pacman -Qu 2>/dev/null | wc -l", "r");
    if (fp) {
        int pkg_updates = 0;
        if (fscanf(fp, "%d", &pkg_updates) == 1 && pkg_updates > 0 && count < MAX_EVENTS) {
            struct tm *tm_info = localtime(&now);
            strftime(g_app.events[count].time_str, sizeof(g_app.events[count].time_str), "%H:%M", tm_info);
            snprintf(g_app.events[count].title, sizeof(g_app.events[count].title), "PACKAGE UPDATE");
            snprintf(g_app.events[count].subtitle, sizeof(g_app.events[count].subtitle), "%d PACKAGES READY // PACMAN", pkg_updates);
            snprintf(g_app.events[count].action_cmd, sizeof(g_app.events[count].action_cmd), "kitty --hold -e sudo pacman -Syu");
            g_app.events[count].type = EVENT_WARN;
            count++;
        }
        pclose(fp);
    }

    // 3. Check Git Projects Activity
    char proj1[512], proj2[512];
    snprintf(proj1, sizeof(proj1), "%s/Projects/solaris-hologram", home ? home : "");
    snprintf(proj2, sizeof(proj2), "%s/Projects/solaris-bg", home ? home : "");
    const char *proj_dirs[] = { proj1, proj2, NULL };
    for (int p = 0; proj_dirs[p] != NULL && count < MAX_EVENTS; p++) {
        char git_cmd[512];
        snprintf(git_cmd, sizeof(git_cmd), "git -C \"%s\" log -1 --format=\"%%h|%%ct|%%s\" 2>/dev/null", proj_dirs[p]);
        FILE *gfp = popen(git_cmd, "r");
        if (gfp) {
            char line[256];
            if (fgets(line, sizeof(line), gfp)) {
                char *hash = strtok(line, "|");
                char *ct_str = strtok(NULL, "|");
                char *msg = strtok(NULL, "\n");
                if (hash && ct_str && msg) {
                    time_t commit_time = (time_t)atol(ct_str);
                    double age_hrs = difftime(now, commit_time) / 3600.0;
                    if (age_hrs <= 72.0) {
                        struct tm *tm_info = localtime(&commit_time);
                        strftime(g_app.events[count].time_str, sizeof(g_app.events[count].time_str), "%H:%M", tm_info);
                        snprintf(g_app.events[count].title, sizeof(g_app.events[count].title), "GIT COMMIT // %s", hash);
                        snprintf(g_app.events[count].subtitle, sizeof(g_app.events[count].subtitle), "%.30s", msg);
                        snprintf(g_app.events[count].action_cmd, sizeof(g_app.events[count].action_cmd), "kitty --directory=\"%s\"", proj_dirs[p]);
                        g_app.events[count].type = EVENT_SUCCESS;
                        count++;
                    }
                }
            }
            pclose(gfp);
        }
    }

    // 4. Check Failed Systemd Units
    FILE *sfp = popen("systemctl --user --failed --no-legend 2>/dev/null | head -n 2", "r");
    if (sfp) {
        char s_line[256];
        while (fgets(s_line, sizeof(s_line), sfp) && count < MAX_EVENTS) {
            char unit_name[128];
            if (sscanf(s_line, "%127s", unit_name) == 1 && unit_name[0] != '\0') {
                struct tm *tm_info = localtime(&now);
                strftime(g_app.events[count].time_str, sizeof(g_app.events[count].time_str), "%H:%M", tm_info);
                snprintf(g_app.events[count].title, sizeof(g_app.events[count].title), "SERVICE FAILED");
                snprintf(g_app.events[count].subtitle, sizeof(g_app.events[count].subtitle), "%.30s // SYSTEMD", unit_name);
                snprintf(g_app.events[count].action_cmd, sizeof(g_app.events[count].action_cmd), "kitty -e systemctl --user status %s", unit_name);
                g_app.events[count].type = EVENT_ERROR;
                count++;
            }
        }
        pclose(sfp);
    }

    g_app.event_count = count;
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
    double cur_y = 8.0;
    double panel_w = (double)width - 16.0;
    double panel_h = (double)height - 16.0;

    // 1. Spacecraft Beveled Deck Panel
    cairo_draw_beveled_panel(cr, pad_x, cur_y, panel_w, panel_h, 8.0);
    cairo_draw_module_header(cr, pad_x, cur_y, panel_w, "MISSION QUEUE // ACTIVITY STREAM", COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // 2. Physical Inter-Module Connecting Conduits
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.40);
    cairo_set_line_width(cr, 1.2);
    // Top conduit (connects up to Wi-Fi radar)
    cairo_move_to(cr, pad_x + 40.0, cur_y);
    cairo_line_to(cr, pad_x + 40.0, 0);
    cairo_stroke(cr);
    cairo_arc(cr, pad_x + 40.0, cur_y, 2.0, 0, 2 * M_PI);
    cairo_fill(cr);

    // Right conduit (connects toward Central Hologram)
    cairo_move_to(cr, pad_x + panel_w, cur_y + 120.0);
    cairo_line_to(cr, width, cur_y + 120.0);
    cairo_stroke(cr);
    cairo_arc(cr, pad_x + panel_w, cur_y + 120.0, 2.0, 0, 2 * M_PI);
    cairo_fill(cr);

    // Bottom conduit (connects down to Command Conduit)
    cairo_move_to(cr, pad_x + 80.0, cur_y + panel_h);
    cairo_line_to(cr, pad_x + 80.0, height);
    cairo_stroke(cr);
    cairo_arc(cr, pad_x + 80.0, cur_y + panel_h, 2.0, 0, 2 * M_PI);
    cairo_fill(cr);

    // 3. Status Bar Sub-header
    double content_y = cur_y + 36.0;
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 7.5);

    char queue_status[64];
    if (g_app.event_count > 0) {
        snprintf(queue_status, sizeof(queue_status), "ACTIVE SIGNALS: %d EVENTS // LIVE STREAM", g_app.event_count);
        cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.85);
    } else {
        snprintf(queue_status, sizeof(queue_status), "DISPATCH IDLE // SYSTEM NOMINAL");
        cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.65);
    }
    cairo_move_to(cr, pad_x + 14.0, content_y);
    cairo_show_text(cr, queue_status);

    // Thin separator line
    content_y += 8.0;
    cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.25);
    cairo_set_line_width(cr, 0.8);
    cairo_move_to(cr, pad_x + 12.0, content_y);
    cairo_line_to(cr, pad_x + panel_w - 12.0, content_y);
    cairo_stroke(cr);

    content_y += 10.0;

    // 4. Render Mission Event Feed
    if (g_app.event_count == 0) {
        double box_h = 160.0;
        double box_y = content_y + 40.0;
        double box_w = panel_w - 40.0;
        double box_x = pad_x + 20.0;

        cairo_set_source_rgba(cr, 0.02, 0.06, 0.12, 0.40);
        cairo_rectangle(cr, box_x, box_y, box_w, box_h);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.25);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);

        cairo_arc(cr, box_x + box_w * 0.5, box_y + 45.0, 18.0, 0, 2 * M_PI);
        cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.35);
        cairo_stroke(cr);
        cairo_arc(cr, box_x + box_w * 0.5, box_y + 45.0, 4.0, 0, 2 * M_PI);
        cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.70);
        cairo_fill(cr);

        cairo_set_font_size(cr, 8.5);
        cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.90);
        cairo_text_extents_t te;
        const char *t1 = "MISSION QUEUE // IDLE";
        cairo_text_extents(cr, t1, &te);
        cairo_move_to(cr, box_x + (box_w - te.width) * 0.5, box_y + 85.0);
        cairo_show_text(cr, t1);

        cairo_set_font_size(cr, 7.5);
        cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.85);
        const char *t2 = "● SYSTEM NOMINAL";
        cairo_text_extents(cr, t2, &te);
        cairo_move_to(cr, box_x + (box_w - te.width) * 0.5, box_y + 105.0);
        cairo_show_text(cr, t2);

        cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.65);
        const char *t3 = "NO PENDING EVENTS";
        cairo_text_extents(cr, t3, &te);
        cairo_move_to(cr, box_x + (box_w - te.width) * 0.5, box_y + 125.0);
        cairo_show_text(cr, t3);
    } else {
        double card_h = 44.0;
        double card_w = panel_w - 24.0;
        double start_x = pad_x + 12.0;

        int max_visible = 4;
        for (int i = 0; i < max_visible && (i + g_app.scroll_offset) < g_app.event_count; i++) {
            int idx = i + g_app.scroll_offset;
            const mission_event_t *ev = &g_app.events[idx];
            double cy = content_y + (i * (card_h + 5.0));
            if (cy + card_h > cur_y + panel_h - 6.0) break;

            gboolean is_hov = (g_app.hovered_idx == idx);

            cairo_save(cr);
            cairo_new_path(cr);
            double c = 4.0;
            cairo_move_to(cr, start_x + c, cy);
            cairo_line_to(cr, start_x + card_w - c, cy);
            cairo_line_to(cr, start_x + card_w, cy + c);
            cairo_line_to(cr, start_x + card_w, cy + card_h - c);
            cairo_line_to(cr, start_x + card_w - c, cy + card_h);
            cairo_line_to(cr, start_x + c, cy + card_h);
            cairo_line_to(cr, start_x, cy + card_h - c);
            cairo_line_to(cr, start_x, cy + c);
            cairo_close_path(cr);

            if (is_hov) {
                cairo_set_source_rgba(cr, 0.03, 0.10, 0.18, 0.90);
            } else {
                cairo_set_source_rgba(cr, 0.015, 0.040, 0.075, 0.65);
            }
            cairo_fill_preserve(cr);

            double sr = COLOR_GREEN_R, sg = COLOR_GREEN_G, sb = COLOR_GREEN_B;
            if (ev->type == EVENT_WARN) {
                sr = 1.0; sg = 0.67; sb = 0.0;
            } else if (ev->type == EVENT_ERROR) {
                sr = COLOR_ALERT_R; sg = COLOR_ALERT_G; sb = COLOR_ALERT_B;
            } else if (ev->type == EVENT_INFO) {
                sr = COLOR_CYAN_R; sg = COLOR_CYAN_G; sb = COLOR_CYAN_B;
            }

            if (is_hov) {
                cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
                cairo_set_line_width(cr, 1.4);
            } else {
                cairo_set_source_rgba(cr, sr, sg, sb, 0.35);
                cairo_set_line_width(cr, 0.9);
            }
            cairo_stroke(cr);

            cairo_set_source_rgba(cr, sr, sg, sb, is_hov ? 1.0 : 0.75);
            cairo_rectangle(cr, start_x + 1.0, cy + 3.0, 2.5, card_h - 6.0);
            cairo_fill(cr);

            cairo_arc(cr, start_x + 10.0, cy + 15.0, 2.5, 0, 2 * M_PI);
            cairo_fill(cr);

            cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 8.0);
            cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.98);

            char line1[128];
            snprintf(line1, sizeof(line1), "%s  %s", ev->time_str, ev->title);
            cairo_move_to(cr, start_x + 18.0, cy + 16.0);
            cairo_show_text(cr, line1);

            cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
            cairo_set_font_size(cr, 7.2);
            cairo_set_source_rgba(cr, sr * 0.9, sg * 0.9, sb * 0.9, 0.85);
            cairo_move_to(cr, start_x + 18.0, cy + 32.0);
            cairo_show_text(cr, ev->subtitle);

            cairo_restore(cr);
        }
    }

    // Spacecraft Sequential Power-On Awakening Overlay (Starts at 1.0s)
    solaris_draw_awakening_overlay(cr, pad_x, cur_y, panel_w, panel_h, 1.0, 0.8, "AVIONICS MATRIX");

    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    (void)user_data;
    double x = event->x;
    double y = event->y;

    double pad_x = 8.0;
    double cur_y = 8.0;
    double panel_w = (double)gtk_widget_get_allocated_width(widget) - 16.0;
    double content_y = cur_y + 54.0;
    double card_h = 44.0;
    double card_w = panel_w - 24.0;
    double start_x = pad_x + 12.0;

    int old_hov = g_app.hovered_idx;
    g_app.hovered_idx = -1;

    for (int i = 0; i < 4 && (i + g_app.scroll_offset) < g_app.event_count; i++) {
        int idx = i + g_app.scroll_offset;
        double cy = content_y + (i * (card_h + 5.0));
        if (x >= start_x && x <= start_x + card_w && y >= cy && y <= cy + card_h) {
            g_app.hovered_idx = idx;
            break;
        }
    }

    if (old_hov != g_app.hovered_idx) {
        redraw_all_views();
    }
    return FALSE;
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)user_data;
    if (event->button == 1 && g_app.hovered_idx >= 0 && g_app.hovered_idx < g_app.event_count) {
        const mission_event_t *ev = &g_app.events[g_app.hovered_idx];
        if (ev->action_cmd[0] != '\0') {
            g_spawn_command_line_async(ev->action_cmd, NULL);
        }
    }
    return FALSE;
}

static gboolean on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
    (void)user_data;
    if (event->direction == GDK_SCROLL_UP) {
        if (g_app.scroll_offset > 0) {
            g_app.scroll_offset--;
            redraw_all_views();
        }
    } else if (event->direction == GDK_SCROLL_DOWN) {
        if (g_app.scroll_offset + 4 < g_app.event_count) {
            g_app.scroll_offset++;
            redraw_all_views();
        }
    }
    return FALSE;
}

static gboolean on_anim_tick(gpointer user_data) {
    (void)user_data;
    g_app.time_sec += 0.05;
    redraw_all_views();
    return G_SOURCE_CONTINUE;
}

static gboolean on_telem_tick(gpointer user_data) {
    (void)user_data;
    refresh_mission_events();
    solaris_trim_memory();
    redraw_all_views();
    return G_SOURCE_CONTINUE;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    solaris_load_css();

    memset(&g_app, 0, sizeof(g_app));
    g_app.hovered_idx = -1;

    GdkDisplay *display = gdk_display_get_default();
    int n_monitors = display ? gdk_display_get_n_monitors(display) : 1;
    g_app.view_count = 0;

    for (int i = 0; i < n_monitors && i < MAX_ACTIVITY_MONITORS; i++) {
        GdkMonitor *mon = display ? gdk_display_get_monitor(display, i) : NULL;
        GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        solaris_setup_layer_window_for_monitor(win, mon, "solaris-bg-activity",
                                                460, 330,
                                                TRUE, FALSE, TRUE, FALSE,
                                                374, 0, 16, 0);

        GtkWidget *da = gtk_drawing_area_new();
        gtk_container_add(GTK_CONTAINER(win), da);

        gtk_widget_add_events(da,
                              GDK_POINTER_MOTION_MASK |
                              GDK_BUTTON_PRESS_MASK |
                              GDK_SCROLL_MASK |
                              GDK_LEAVE_NOTIFY_MASK);

        g_signal_connect(da, "draw", G_CALLBACK(on_draw), NULL);
        g_signal_connect(da, "motion-notify-event", G_CALLBACK(on_motion_notify), NULL);
        g_signal_connect(da, "button-press-event", G_CALLBACK(on_button_press), NULL);
        g_signal_connect(da, "scroll-event", G_CALLBACK(on_scroll), NULL);

        gtk_widget_show_all(win);

        g_app.windows[g_app.view_count] = win;
        g_app.drawing_areas[g_app.view_count] = da;
        g_app.view_count++;
    }

    if (g_app.view_count > 0) {
        g_app.window = g_app.windows[0];
        g_app.drawing_area = g_app.drawing_areas[0];
    }

    refresh_mission_events();

    g_app.anim_timer_id = g_timeout_add(50, on_anim_tick, NULL);
    g_app.telem_timer_id = g_timeout_add(4000, on_telem_tick, NULL);

    gtk_main();
    return 0;
}
