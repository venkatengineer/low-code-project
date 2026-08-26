#include "common.h"
#include "telemetry.h"
#include "cairo_utils.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    mpris_telemetry_t mpris;
    guint anim_timer_id;
    guint telem_timer_id;
} audio_app_t;

static audio_app_t g_app;

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);

    double pad_x = 8.0;
    double cur_y = 8.0;
    double panel_w = (double)width - 16.0;
    double panel_h = (double)height - 16.0;

    cairo_draw_beveled_panel(cr, pad_x, cur_y, panel_w, panel_h, 8.0);
    cairo_draw_module_header(cr, pad_x, cur_y, panel_w, "AETHERIS // AUDIO MATRIX", COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // Track Title & Artist
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 11.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.95);
    cairo_move_to(cr, pad_x + 14.0, cur_y + 44.0);
    cairo_show_text(cr, g_app.mpris.title);

    cairo_set_font_size(cr, 9.5);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.85);
    cairo_move_to(cr, pad_x + 14.0, cur_y + 60.0);
    cairo_show_text(cr, g_app.mpris.artist);

    // Status glyph (Playing / Paused)
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.9);
    cairo_arc(cr, pad_x + panel_w - 18.0, cur_y + 42.0, 3.5, 0, 2 * M_PI);
    cairo_fill(cr);

    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.9);
    cairo_move_to(cr, pad_x + panel_w - 68.0, cur_y + 45.0);
    cairo_show_text(cr, g_app.mpris.status);

    // Progress Bar
    cairo_draw_segmented_bar(cr, pad_x + 14.0, cur_y + 70.0, panel_w - 28.0, 4.0,
                             g_app.mpris.progress_pct, 30,
                             COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // 24-Band Extraterrestrial Equalizer Spectrum Bars
    double spec_y = cur_y + 84.0;
    double spec_h = 44.0;
    int num_bars = 24;
    double bar_gap = 3.0;
    double bar_w = ((panel_w - 28.0) - (num_bars - 1) * bar_gap) / (double)num_bars;

    for (int i = 0; i < num_bars; i++) {
        double bx = pad_x + 14.0 + i * (bar_w + bar_gap);
        double bh = g_app.mpris.spectrum[i] * spec_h;
        if (bh < 3.0) bh = 3.0;
        double by = spec_y + (spec_h - bh);

        // Gradient from cyan to violet
        cairo_pattern_t *bpat = cairo_pattern_create_linear(bx, spec_y, bx, spec_y + spec_h);
        cairo_pattern_add_color_stop_rgba(bpat, 0.0, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
        cairo_pattern_add_color_stop_rgba(bpat, 1.0, COLOR_VIOLET_R, COLOR_VIOLET_G, COLOR_VIOLET_B, 0.7);
        cairo_set_source(cr, bpat);
        cairo_rectangle(cr, bx, by, bar_w, bh);
        cairo_fill(cr);
        cairo_pattern_destroy(bpat);

        // Cap dot
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.9);
        cairo_rectangle(cr, bx, by - 2.0, bar_w, 1.5);
        cairo_fill(cr);
    }

    return FALSE;
}

static gboolean on_anim_tick(gpointer user_data) {
    update_mpris_telemetry(&g_app.mpris);
    if (g_app.drawing_area) {
        gtk_widget_queue_draw(g_app.drawing_area);
    }
    return G_SOURCE_CONTINUE;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    solaris_load_css();

    memset(&g_app, 0, sizeof(g_app));
    init_telemetry();
    update_mpris_telemetry(&g_app.mpris);

    g_app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    solaris_setup_layer_window(g_app.window, "solaris-bg-audio",
                               490, 160,
                               TRUE, FALSE, FALSE, TRUE, // Top-Right anchor (stacked cleanly below bluetooth)
                               334, 0, 0, 16);

    g_app.drawing_area = gtk_drawing_area_new();
    g_signal_connect(g_app.drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    gtk_container_add(GTK_CONTAINER(g_app.window), g_app.drawing_area);

    g_app.anim_timer_id = g_timeout_add(40, on_anim_tick, NULL); // 25 FPS spectrum anim

    gtk_widget_show_all(g_app.window);
    gtk_main();
    return 0;
}
