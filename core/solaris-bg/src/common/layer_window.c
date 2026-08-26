#include "common.h"

void solaris_setup_layer_window(GtkWidget *window, const char *title,
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

void solaris_load_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    char css_path[512];
    const char *home = getenv("HOME");
    snprintf(css_path, sizeof(css_path), "%s/.local/share/solaris/style/solaris.css", home ? home : "");
    GError *error = NULL;

    if (g_file_test(css_path, G_FILE_TEST_EXISTS)) {
        gtk_css_provider_load_from_path(provider, css_path, &error);
    } else {
        const char *fallback_css =
            "window { background-color: transparent; }"
            "* { font-family: 'JetBrainsMono Nerd Font', monospace; }";
        gtk_css_provider_load_from_data(provider, fallback_css, -1, NULL);
    }

    if (error) {
        g_warning("Failed to load CSS: %s", error->message);
        g_error_free(error);
    }

    GdkScreen *screen = gdk_screen_get_default();
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}
