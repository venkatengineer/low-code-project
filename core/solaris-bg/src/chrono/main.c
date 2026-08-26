#include "common.h"
#include "telemetry.h"
#include "cairo_utils.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    char time_str[32];
    char date_str[64];
    char stardate_str[32];
    char uptime_str[32];
    guint timer_id;
} chrono_app_t;

static chrono_app_t g_app;

static void update_time_data(void) {
    time_t rawtime;
    struct tm *info;
    time(&rawtime);
    info = localtime(&rawtime);

    strftime(g_app.time_str, sizeof(g_app.time_str), "%H:%M:%S", info);
    strftime(g_app.date_str, sizeof(g_app.date_str), "%a, %d %b %Y", info);

    // Stardate calculation
    int day_of_year = info->tm_yday + 1;
    snprintf(g_app.stardate_str, sizeof(g_app.stardate_str), "SD %d.%03d", 1900 + info->tm_year, day_of_year);

    // Read uptime
    FILE *fp = fopen("/proc/uptime", "r");
    if (fp) {
        double up = 0;
        if (fscanf(fp, "%lf", &up) == 1) {
            int hrs = (int)up / 3600;
            int mins = ((int)up % 3600) / 60;
            int secs = (int)up % 60;
            snprintf(g_app.uptime_str, sizeof(g_app.uptime_str), "UPTIME %02d:%02d:%02d", hrs, mins, secs);
        }
        fclose(fp);
    }
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);

    double pad_x = 6.0;
    double cur_y = 6.0;
    double panel_w = (double)width - 12.0;
    double panel_h = (double)height - 12.0;

    cairo_draw_beveled_panel(cr, pad_x, cur_y, panel_w, panel_h, 6.0);

    // Center Clock
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

    double center_x = pad_x + panel_w / 2.0;

    // Time
    cairo_set_font_size(cr, 18.0);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 1.0);
    cairo_text_extents_t text_ext;
    cairo_text_extents(cr, g_app.time_str, &text_ext);
    cairo_move_to(cr, center_x - text_ext.width / 2.0, cur_y + 24.0);
    cairo_show_text(cr, g_app.time_str);

    // Date & Stardate
    char full_date[128];
    snprintf(full_date, sizeof(full_date), "%s // %s", g_app.stardate_str, g_app.date_str);
    cairo_set_font_size(cr, 8.5);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.95);
    cairo_text_extents(cr, full_date, &text_ext);
    cairo_move_to(cr, center_x - text_ext.width / 2.0, cur_y + 38.0);
    cairo_show_text(cr, full_date);

    // Left brand
    cairo_set_font_size(cr, 9.5);
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 1.0);
    cairo_move_to(cr, pad_x + 14.0, cur_y + 22.0);
    cairo_show_text(cr, "⬡ SOLARIS OS v2.8.7");

    cairo_set_font_size(cr, 8.0);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.9);
    cairo_move_to(cr, pad_x + 14.0, cur_y + 36.0);
    cairo_show_text(cr, "ARCH LINUX // WAYLAND");

    // Right Uptime & Status
    cairo_set_font_size(cr, 9.0);
    cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.9);
    cairo_text_extents(cr, g_app.uptime_str, &text_ext);
    cairo_move_to(cr, pad_x + panel_w - text_ext.width - 14.0, cur_y + 22.0);
    cairo_show_text(cr, g_app.uptime_str);

    cairo_set_font_size(cr, 8.0);
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.95);
    const char *status_str = "● ALL SYSTEMS ONLINE";
    cairo_text_extents(cr, status_str, &text_ext);
    cairo_move_to(cr, pad_x + panel_w - text_ext.width - 14.0, cur_y + 36.0);
    cairo_show_text(cr, status_str);

    return FALSE;
}

static gboolean on_tick(gpointer user_data) {
    update_time_data();
    if (g_app.drawing_area) {
        gtk_widget_queue_draw(g_app.drawing_area);
    }
    return G_SOURCE_CONTINUE;
}

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    solaris_load_css();

    memset(&g_app, 0, sizeof(g_app));
    update_time_data();

    g_app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    solaris_setup_layer_window(g_app.window, "solaris-bg-chrono",
                               640, 54,
                               TRUE, FALSE, FALSE, FALSE, // Top-Center anchor
                               4, 0, 0, 0);

    g_app.drawing_area = gtk_drawing_area_new();
    g_signal_connect(g_app.drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    gtk_container_add(GTK_CONTAINER(g_app.window), g_app.drawing_area);

    g_app.timer_id = g_timeout_add(1000, on_tick, NULL);

    gtk_widget_show_all(g_app.window);
    gtk_main();
    return 0;
}
