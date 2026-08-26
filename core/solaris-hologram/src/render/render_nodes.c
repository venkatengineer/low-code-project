#include "render_nodes.h"
#include "math3d.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

typedef struct {
    const window_info_t *win;
    double depth;
} node_sort_entry_t;

static int compare_node_depth(const void *a, const void *b) {
    const node_sort_entry_t *ea = (const node_sort_entry_t *)a;
    const node_sort_entry_t *eb = (const node_sort_entry_t *)b;
    if (ea->depth > eb->depth) return -1; // Further away draws first
    if (ea->depth < eb->depth) return 1;
    return 0;
}

static void draw_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r, r, -M_PI * 0.5, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI * 0.5);
    cairo_arc(cr, x + r, y + h - r, r, M_PI * 0.5, M_PI);
    cairo_arc(cr, x + r, y + r, r, M_PI, M_PI * 1.5);
    cairo_close_path(cr);
}

void render_application_nodes(cairo_t *cr, const orbital_system_t *sys, double cx, double cy) {
    if (sys->window_count == 0) return;

    // Collect and sort nodes by 3D depth
    node_sort_entry_t entries[MAX_WINDOWS];
    int count = 0;
    for (int i = 0; i < sys->window_count; i++) {
        if (sys->windows[i].screen_scale > 0.1) {
            entries[count].win = &sys->windows[i];
            entries[count].depth = sys->windows[i].screen_depth;
            count++;
        }
    }
    qsort(entries, count, sizeof(node_sort_entry_t), compare_node_depth);

    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *name_font = pango_font_description_from_string("JetBrainsMono Nerd Font Bold 7");
    PangoFontDescription *glyph_font = pango_font_description_from_string("JetBrainsMono Nerd Font 9");

    for (int k = 0; k < count; k++) {
        const window_info_t *win = entries[k].win;
        double sc = win->screen_scale;
        double x = win->screen_pos.x + win->drag_offset_x;
        double y = win->screen_pos.y + win->drag_offset_y;

        gboolean is_foc = win->is_focused;
        gboolean is_hov = (sys->hovered_window_id == win->id);
        gboolean is_act_ws = (win->workspace_idx == sys->active_workspace_idx);

        color_rgb_t sc_node = get_station_color(win->workspace_idx);

        // 1. Draw Sleek Fiber-Optic Tether to Orbit Ring
        const workspace_info_t *ws = NULL;
        for (int w = 0; w < sys->workspace_count; w++) {
            if (sys->workspaces[w].idx == win->workspace_idx) {
                ws = &sys->workspaces[w];
                break;
            }
        }
        if (ws) {
            double r = ws->current_radius;
            double ang = win->orbit_angle;
            vec3_t anchor_3d = vec3_create(
                r * cos(ang),
                ws->y_height + sin(ws->inclination) * r * sin(ang),
                r * sin(ang)
            );
            vec2_t anchor_sp;
            if (project_to_screen(anchor_3d, &sys->camera, cx, cy, &anchor_sp, NULL, NULL)) {
                // Subtle laser tether
                double tether_alpha = is_foc ? 0.80 : (is_hov ? 0.60 : 0.25);
                cairo_set_source_rgba(cr, sc_node.r, sc_node.g, sc_node.b, tether_alpha);
                cairo_set_line_width(cr, (is_foc ? 1.4 : 0.8) * sc);
                cairo_move_to(cr, anchor_sp.x, anchor_sp.y);
                cairo_line_to(cr, x, y);
                cairo_stroke(cr);

                // Tiny glowing anchor pip on the orbital ring
                cairo_set_source_rgba(cr, sc_node.r, sc_node.g, sc_node.b, is_foc ? 1.0 : 0.7);
                cairo_arc(cr, anchor_sp.x, anchor_sp.y, 2.0 * sc, 0, 2 * M_PI);
                cairo_fill(cr);
            }
        }

        // Orb Dimensions
        double orb_r = 13.0 * sc;
        cairo_save(cr);

        // 2. Focused / Hover Halo Pulse
        if (is_foc || win->focus_anim > 0.01 || is_hov) {
            double glow_amt = is_foc ? 1.0 : (is_hov ? 0.8 : win->focus_anim);
            double halo_r = orb_r + (12.0 * glow_amt * sc);
            cairo_pattern_t *halo = cairo_pattern_create_radial(x, y, orb_r * 0.5, x, y, halo_r);
            cairo_pattern_add_color_stop_rgba(halo, 0.0, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.45 * glow_amt);
            cairo_pattern_add_color_stop_rgba(halo, 0.6, sc_node.r, sc_node.g, sc_node.b, 0.20 * glow_amt);
            cairo_pattern_add_color_stop_rgba(halo, 1.0, 0, 0, 0, 0);
            cairo_set_source(cr, halo);
            cairo_arc(cr, x, y, halo_r, 0, 2 * M_PI);
            cairo_fill(cr);
            cairo_pattern_destroy(halo);

            // Rotating quantum resonance brackets for focused window
            if (is_foc) {
                double rot_foc = sys->time_sec * 1.8;
                cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.90);
                cairo_set_line_width(cr, 1.2 * sc);
                for (int b = 0; b < 3; b++) {
                    double a_start = rot_foc + (b * (2.0 * M_PI / 3.0));
                    cairo_arc(cr, x, y, orb_r + (4.0 * sc), a_start, a_start + 0.9);
                    cairo_stroke(cr);
                }
            }
        }

        // 3. Circular Satellite Orb Lens Body
        cairo_arc(cr, x, y, orb_r, 0, 2 * M_PI);
        if (is_foc) {
            cairo_set_source_rgba(cr, 0.02, 0.12, 0.22, 0.95);
        } else if (is_hov) {
            cairo_set_source_rgba(cr, 0.04, 0.14, 0.26, 0.92);
        } else if (is_act_ws) {
            cairo_set_source_rgba(cr, 0.02, 0.08, 0.16, 0.88);
        } else {
            cairo_set_source_rgba(cr, 0.01, 0.05, 0.10, 0.80);
        }
        cairo_fill_preserve(cr);

        // Neon Satellite Rim
        if (is_foc) {
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 1.0);
            cairo_set_line_width(cr, 1.8 * sc);
        } else if (is_hov) {
            cairo_set_source_rgba(cr, sc_node.r, sc_node.g, sc_node.b, 0.95);
            cairo_set_line_width(cr, 1.4 * sc);
        } else if (is_act_ws) {
            cairo_set_source_rgba(cr, sc_node.r, sc_node.g, sc_node.b, 0.70);
            cairo_set_line_width(cr, 1.1 * sc);
        } else {
            cairo_set_source_rgba(cr, sc_node.r, sc_node.g, sc_node.b, 0.35);
            cairo_set_line_width(cr, 0.85 * sc);
        }
        cairo_stroke(cr);

        // 4. Centered Application Icon / Glyph
        if (win->icon_pixbuf) {
            cairo_save(cr);
            double pix_w = gdk_pixbuf_get_width(win->icon_pixbuf);
            double pix_h = gdk_pixbuf_get_height(win->icon_pixbuf);
            double target_icon_size = 15.0 * sc;
            cairo_translate(cr, x - (target_icon_size * 0.5), y - (target_icon_size * 0.5));
            cairo_scale(cr, target_icon_size / pix_w, target_icon_size / pix_h);
            gdk_cairo_set_source_pixbuf(cr, win->icon_pixbuf, 0, 0);
            cairo_paint_with_alpha(cr, (is_foc ? 1.0 : (is_act_ws ? 0.95 : 0.75)));
            cairo_restore(cr);
        } else {
            pango_layout_set_font_description(layout, glyph_font);
            pango_layout_set_text(layout, win->glyph, -1);
            if (is_foc) {
                cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 1.0);
            } else if (is_act_ws) {
                cairo_set_source_rgba(cr, sc_node.r, sc_node.g, sc_node.b, 0.95);
            } else {
                cairo_set_source_rgba(cr, sc_node.r, sc_node.g, sc_node.b, 0.75);
            }
            int gw, gh;
            pango_layout_get_pixel_size(layout, &gw, &gh);
            cairo_move_to(cr, x - (gw * 0.5), y - (gh * 0.5));
            pango_cairo_show_layout(cr, layout);
        }

        // 5. Activity Jewel Dot on Satellite
        double dot_angle = -M_PI * 0.25;
        double dot_x = x + ((orb_r - 2.0 * sc) * cos(dot_angle));
        double dot_y = y + ((orb_r - 2.0 * sc) * sin(dot_angle));
        if (win->is_urgent) {
            cairo_set_source_rgba(cr, COLOR_ALERT_R, 0.2, 0.2, 1.0);
            cairo_arc(cr, dot_x, dot_y, 2.8 * sc, 0, 2 * M_PI);
            cairo_fill(cr);
        } else if (is_foc) {
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 1.0);
            cairo_arc(cr, dot_x, dot_y, 2.2 * sc, 0, 2 * M_PI);
            cairo_fill(cr);
        } else if (is_act_ws) {
            cairo_set_source_rgba(cr, sc_node.r, sc_node.g, sc_node.b, 0.85);
            cairo_arc(cr, dot_x, dot_y, 1.8 * sc, 0, 2 * M_PI);
            cairo_fill(cr);
        }

        // 6. Dynamic Minimal Tooltip Pill (Only on Hover or Focused Window)
        if (is_foc || is_hov) {
            pango_layout_set_font_description(layout, name_font);
            pango_layout_set_text(layout, win->display_name, -1);
            int nw, nh;
            pango_layout_get_pixel_size(layout, &nw, &nh);

            double pill_x = x + orb_r + (6.0 * sc);
            double pill_y = y - (nh * 0.5) - (2.0 * sc);
            double pill_w = nw + (14.0 * sc);
            double pill_h = nh + (4.0 * sc);

            // Translucent glass capsule
            draw_rounded_rect(cr, pill_x, pill_y, pill_w, pill_h, 3.5 * sc);
            cairo_set_source_rgba(cr, 0.02, 0.08, 0.16, 0.92);
            cairo_fill_preserve(cr);

            cairo_set_source_rgba(cr, is_foc ? COLOR_CYAN_R : sc_node.r,
                                      is_foc ? COLOR_CYAN_G : sc_node.g,
                                      is_foc ? COLOR_CYAN_B : sc_node.b,
                                      is_foc ? 0.95 : 0.70);
            cairo_set_line_width(cr, 1.0 * sc);
            cairo_stroke(cr);

            // Pill text
            cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 1.0);
            cairo_move_to(cr, pill_x + (7.0 * sc), pill_y + (2.0 * sc));
            pango_cairo_show_layout(cr, layout);
        }

        cairo_restore(cr);
    }

    pango_font_description_free(name_font);
    pango_font_description_free(glyph_font);
    g_object_unref(layout);
}
