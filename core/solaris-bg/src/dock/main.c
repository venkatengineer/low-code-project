#include "common.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *render_btn;
    GtkWidget *render_icon;
    GtkWidget *render_label;
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

static gboolean update_dock_status(gpointer user_data) {
    (void)user_data;
    gboolean is_active = (access("/tmp/solaris_rendering_mode", F_OK) == 0);
    if (is_active != g_app.render_active) {
        g_app.render_active = is_active;
        if (g_app.render_btn && g_app.render_label) {
            GtkStyleContext *ctx = gtk_widget_get_style_context(g_app.render_btn);
            if (is_active) {
                gtk_style_context_add_class(ctx, "render-active");
                gtk_label_set_text(GTK_LABEL(g_app.render_label), "RENDER ON");
            } else {
                gtk_style_context_remove_class(ctx, "render-active");
                gtk_label_set_text(GTK_LABEL(g_app.render_label), "RENDER");
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

    g_app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    solaris_setup_layer_window(g_app.window, "solaris-bg-dock",
                                790, 48,
                                FALSE, TRUE, FALSE, FALSE, // Bottom-Center anchor
                                0, 16, 0, 0);

    g_app.box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(g_app.box), "dock-bar");
    gtk_container_add(GTK_CONTAINER(g_app.window), g_app.box);

    // Add buttons
    gtk_box_pack_start(GTK_BOX(g_app.box), create_dock_btn("", "FILES", "solaris-explorer", FALSE), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(g_app.box), create_dock_btn("", "TERMINAL", "kitty -1", FALSE), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(g_app.box), create_dock_btn("", "BROWSER", "vivaldi", FALSE), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(g_app.box), create_dock_btn("", "APPS", "fuzzel", FALSE), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(g_app.box), create_dock_btn("", "PROCESS", "kitty -1 --class=xenomon-hud -e xenomon", FALSE), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(g_app.box), create_dock_btn("", "COMMAND", "kitty -1 --class=alien-cmd-hud -e alien-cmd", FALSE), TRUE, TRUE, 0);

    // Render Mode Toggle Button
    g_app.render_btn = gtk_button_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(g_app.render_btn), "solaris-btn");
    GtkWidget *r_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    g_app.render_icon = gtk_label_new("󰑈");
    g_app.render_label = gtk_label_new("RENDER");
    gtk_box_pack_start(GTK_BOX(r_hbox), g_app.render_icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(r_hbox), g_app.render_label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(g_app.render_btn), r_hbox);
    g_signal_connect(g_app.render_btn, "clicked", G_CALLBACK(on_action_clicked), (gpointer)"solaris-render-mode toggle");
    gtk_box_pack_start(GTK_BOX(g_app.box), g_app.render_btn, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(g_app.box), create_dock_btn("", "CAPTURE", "niri msg action screenshot", FALSE), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(g_app.box), create_dock_btn("", "LOCK", "solaris-lock", TRUE), TRUE, TRUE, 0);

    update_dock_status(NULL);
    g_app.timer_id = g_timeout_add(1000, update_dock_status, NULL);

    gtk_widget_show_all(g_app.window);
    gtk_main();
    return 0;
}
