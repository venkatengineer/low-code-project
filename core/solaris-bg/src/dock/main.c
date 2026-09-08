#include "common.h"

#define MAX_DOCK_MONITORS 8

typedef struct {
    GtkWidget *window;
    GtkWidget *render_btn;
    GtkWidget *render_label;
} dock_view_t;

typedef struct {
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *render_btn;
    GtkWidget *render_icon;
    GtkWidget *render_label;
    dock_view_t views[MAX_DOCK_MONITORS];
    int view_count;
    gboolean render_active;
    guint timer_id;
} dock_app_t;

static dock_app_t g_app;

static void on_action_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    const char *cmd = (const char *)user_data;
    if (cmd) {
        g_spawn_command_line_async(cmd, NULL);
    }
}

static GtkWidget *create_dock_btn(const char *icon, const char *label, const char *cmd, gboolean danger) {
    GtkWidget *btn = gtk_button_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(btn), "solaris-btn");
    if (danger) {
        gtk_style_context_add_class(gtk_widget_get_style_context(btn), "danger");
    }

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *lbl_icon = gtk_label_new(icon);
    GtkWidget *lbl_text = gtk_label_new(label);

    gtk_box_pack_start(GTK_BOX(hbox), lbl_icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), lbl_text, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(btn), hbox);

    g_signal_connect(btn, "clicked", G_CALLBACK(on_action_clicked), (gpointer)cmd);
    return btn;
}

static GtkWidget *create_dock_bar(dock_view_t *view) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(box), "dock-bar");

    // Add buttons
    gtk_box_pack_start(GTK_BOX(box), create_dock_btn("", "FILES", "solaris-explorer", FALSE), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), create_dock_btn("", "TERMINAL", "kitty -1", FALSE), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), create_dock_btn("", "BROWSER", "vivaldi", FALSE), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), create_dock_btn("", "APPS", "fuzzel", FALSE), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), create_dock_btn("", "PROCESS", "kitty -1 --class=xenomon-hud -e xenomon", FALSE), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), create_dock_btn("", "COMMAND", "kitty -1 --class=alien-cmd-hud -e alien-cmd", FALSE), TRUE, TRUE, 0);

    // Render Mode Toggle Button
    view->render_btn = gtk_button_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(view->render_btn), "solaris-btn");
    GtkWidget *r_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *r_icon = gtk_label_new("󰑈");
    view->render_label = gtk_label_new("RENDER");
    gtk_box_pack_start(GTK_BOX(r_hbox), r_icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(r_hbox), view->render_label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(view->render_btn), r_hbox);
    g_signal_connect(view->render_btn, "clicked", G_CALLBACK(on_action_clicked), (gpointer)"solaris-render-mode toggle");
    gtk_box_pack_start(GTK_BOX(box), view->render_btn, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(box), create_dock_btn("", "CAPTURE", "niri msg action screenshot", FALSE), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), create_dock_btn("", "LOCK", "solaris-lock", TRUE), TRUE, TRUE, 0);

    return box;
}

static gboolean update_dock_status(gpointer user_data) {
    (void)user_data;
    gboolean is_active = (access("/tmp/solaris_rendering_mode", F_OK) == 0);
    if (is_active != g_app.render_active) {
        g_app.render_active = is_active;
        for (int i = 0; i < g_app.view_count; i++) {
            if (g_app.views[i].render_btn && g_app.views[i].render_label) {
                GtkStyleContext *ctx = gtk_widget_get_style_context(g_app.views[i].render_btn);
                if (is_active) {
                    gtk_style_context_add_class(ctx, "render-active");
                    gtk_label_set_text(GTK_LABEL(g_app.views[i].render_label), "RENDER ON");
                } else {
                    gtk_style_context_remove_class(ctx, "render-active");
                    gtk_label_set_text(GTK_LABEL(g_app.views[i].render_label), "RENDER");
                }
            }
        }
    }
    solaris_trim_memory();
    return G_SOURCE_CONTINUE;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    solaris_load_css();

    memset(&g_app, 0, sizeof(g_app));

    GdkDisplay *display = gdk_display_get_default();
    int n_monitors = display ? gdk_display_get_n_monitors(display) : 1;
    g_app.view_count = 0;

    for (int i = 0; i < n_monitors && i < MAX_DOCK_MONITORS; i++) {
        GdkMonitor *mon = display ? gdk_display_get_monitor(display, i) : NULL;
        GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        solaris_setup_layer_window_for_monitor(win, mon, "solaris-bg-dock",
                                               790, 48,
                                               FALSE, TRUE, FALSE, FALSE, // Bottom-Center anchor
                                               0, 16, 0, 0);

        g_app.views[g_app.view_count].window = win;
        GtkWidget *box = create_dock_bar(&g_app.views[g_app.view_count]);
        gtk_container_add(GTK_CONTAINER(win), box);
        gtk_widget_show_all(win);
        g_app.view_count++;
    }

    if (g_app.view_count > 0) {
        g_app.window = g_app.views[0].window;
        g_app.render_btn = g_app.views[0].render_btn;
        g_app.render_label = g_app.views[0].render_label;
    }

    update_dock_status(NULL);
    g_app.timer_id = g_timeout_add(1000, update_dock_status, NULL);

    gtk_main();
    return 0;
}
