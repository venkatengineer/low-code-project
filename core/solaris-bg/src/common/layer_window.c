#include "common.h"

void solaris_setup_layer_window_for_monitor(GtkWidget *window, GdkMonitor *monitor, const char *title,
                                            int width, int height,
                                            gboolean anchor_top, gboolean anchor_bottom,
                                            gboolean anchor_left, gboolean anchor_right,
                                            int margin_top, int margin_bottom,
                                            int margin_left, int margin_right) {
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_widget_set_size_request(window, width, height);

    // Set RGBA colormap for true background transparency
    GdkScreen *screen = gtk_widget_get_screen(window);
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual != NULL && gdk_screen_is_composited(screen)) {
        gtk_widget_set_visual(window, visual);
    }
    gtk_widget_set_app_paintable(window, TRUE);

    // Initialize layer shell
    gtk_layer_init_for_window(GTK_WINDOW(window));
    if (monitor) {
        gtk_layer_set_monitor(GTK_WINDOW(window), monitor);
    }
    // Run on BOTTOM layer (beneath all windows, directly over wallpaper)
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_BOTTOM);

    // EXTREMELY IMPORTANT: Keyboard interactivity MUST be FALSE so the user can type in Kitty/terminal
    gtk_layer_set_keyboard_interactivity(GTK_WINDOW(window), FALSE);

    // Set namespace
    gtk_layer_set_namespace(GTK_WINDOW(window), "solaris-bg");

    // Set anchors
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, anchor_top);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, anchor_bottom);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, anchor_left);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, anchor_right);

    // Set margins
    if (anchor_top) gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, margin_top);
    if (anchor_bottom) gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, margin_bottom);
    if (anchor_left) gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, margin_left);
    if (anchor_right) gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, margin_right);
}

void solaris_setup_layer_window(GtkWidget *window, const char *title,
                                int width, int height,
                                gboolean anchor_top, gboolean anchor_bottom,
                                gboolean anchor_left, gboolean anchor_right,
                                int margin_top, int margin_bottom,
                                int margin_left, int margin_right) {
    solaris_setup_layer_window_for_monitor(window, NULL, title, width, height,
                                           anchor_top, anchor_bottom,
                                           anchor_left, anchor_right,
                                           margin_top, margin_bottom,
                                           margin_left, margin_right);
}

void solaris_load_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *home = getenv("HOME");
    char p1[512], p2[512], p3[512];
    snprintf(p1, sizeof(p1), "%s/.local/share/solaris/style/solaris.css", home ? home : "");
    snprintf(p2, sizeof(p2), "%s/solaris-os/core/solaris-bg/style/solaris.css", home ? home : "");
    snprintf(p3, sizeof(p3), "%s/Projects/solaris-bg/style/solaris.css", home ? home : "");

    const char *candidates[] = { p1, p2, p3, NULL };
    gboolean loaded = FALSE;
    GError *error = NULL;

    for (int i = 0; candidates[i] != NULL; i++) {
        if (g_file_test(candidates[i], G_FILE_TEST_EXISTS)) {
            if (gtk_css_provider_load_from_path(provider, candidates[i], &error)) {
                loaded = TRUE;
                break;
            }
            if (error) {
                g_error_free(error);
                error = NULL;
            }
        }
    }

    if (!loaded) {
        const char *fallback_css =
            "* { font-family: 'JetBrainsMono Nerd Font', monospace; font-weight: 500; }\n"
            "window { background-color: transparent; }\n"
            "button.solaris-btn {\n"
            "    background: linear-gradient(180deg, rgba(16, 26, 38, 0.85) 0%, rgba(6, 12, 18, 0.95) 100%);\n"
            "    border: 1px solid rgba(0, 166, 251, 0.4);\n"
            "    border-radius: 3px;\n"
            "    color: #e0f2fe;\n"
            "    padding: 6px 12px;\n"
            "    font-size: 11px;\n"
            "    font-weight: 600;\n"
            "    letter-spacing: 0.5px;\n"
            "    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.6);\n"
            "}\n"
            "button.solaris-btn:hover {\n"
            "    background: linear-gradient(180deg, rgba(24, 42, 60, 0.95) 0%, rgba(12, 22, 32, 1.0) 100%);\n"
            "    border-color: #00e5ff;\n"
            "    color: #00e5ff;\n"
            "    box-shadow: 0 0 12px rgba(0, 229, 255, 0.35);\n"
            "}\n"
            "button.solaris-btn.danger {\n"
            "    border-color: rgba(255, 42, 95, 0.5);\n"
            "    color: #ff6b8b;\n"
            "}\n"
            "button.solaris-btn.render-active {\n"
            "    background: linear-gradient(180deg, rgba(80, 10, 45, 0.95) 0%, rgba(35, 4, 20, 0.98) 100%);\n"
            "    border-color: #ff2a8d;\n"
            "    color: #ff94c2;\n"
            "}\n"
            ".dock-bar {\n"
            "    background: linear-gradient(180deg, rgba(10, 16, 24, 0.88) 0%, rgba(4, 8, 12, 0.94) 100%);\n"
            "    border: 1px solid rgba(0, 166, 251, 0.35);\n"
            "    border-radius: 6px;\n"
            "    padding: 4px 10px;\n"
            "    box-shadow: 0 4px 20px rgba(0, 5, 10, 0.8);\n"
            "}\n";
        gtk_css_provider_load_from_data(provider, fallback_css, -1, NULL);
    }

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
