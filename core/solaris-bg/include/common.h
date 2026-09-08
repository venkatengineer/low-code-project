#ifndef SOLARIS_COMMON_H
#define SOLARIS_COMMON_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <cairo.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/statvfs.h>
#include <malloc.h>

static inline void solaris_trim_memory(void) {
    malloc_trim(0);
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Visual Aesthetic Palette (Dark Extraterrestrial Industrial)
#define COLOR_BG_PANEL_R 0.024
#define COLOR_BG_PANEL_G 0.038
#define COLOR_BG_PANEL_B 0.055
#define COLOR_BG_PANEL_A 0.880

#define COLOR_BORDER_R   0.000
#define COLOR_BORDER_G   0.651
#define COLOR_BORDER_B   0.984
#define COLOR_BORDER_A   0.350

#define COLOR_CYAN_R     0.000
#define COLOR_CYAN_G     0.898
#define COLOR_CYAN_B     1.000

#define COLOR_GREEN_R    0.000
#define COLOR_GREEN_G    1.000
#define COLOR_GREEN_B    0.400

#define COLOR_VIOLET_R   0.620
#define COLOR_VIOLET_G   0.360
#define COLOR_VIOLET_B   1.000

#define COLOR_ALERT_R    1.000
#define COLOR_ALERT_G    0.200
#define COLOR_ALERT_B    0.350

#define COLOR_TEXT_MUTED_R 0.380
#define COLOR_TEXT_MUTED_G 0.500
#define COLOR_TEXT_MUTED_B 0.620

#define COLOR_TEXT_BRIGHT_R 0.900
#define COLOR_TEXT_BRIGHT_G 0.950
#define COLOR_TEXT_BRIGHT_B 1.000

// Helper to initialize layer shell for bottom background positioning
void solaris_setup_layer_window(GtkWidget *window, const char *title,
                                int width, int height,
                                gboolean anchor_top, gboolean anchor_bottom,
                                gboolean anchor_left, gboolean anchor_right,
                                int margin_top, int margin_bottom,
                                int margin_left, int margin_right);

void solaris_setup_layer_window_for_monitor(GtkWidget *window, GdkMonitor *monitor, const char *title,
                                            int width, int height,
                                            gboolean anchor_top, gboolean anchor_bottom,
                                            gboolean anchor_left, gboolean anchor_right,
                                            int margin_top, int margin_bottom,
                                            int margin_left, int margin_right);

// CSS loader
void solaris_load_css(void);

#endif // SOLARIS_COMMON_H
