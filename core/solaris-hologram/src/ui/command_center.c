#include "command_center.h"
#include "math3d.h"
#include <gdk/gdkkeysyms.h>
#include <ctype.h>

void command_center_init(command_center_state_t *cc) {
    memset(cc, 0, sizeof(*cc));
    cc->search_selected_idx = 0;
}

static void draw_hud_box(cairo_t *cr, double x, double y, double w, double h, double c) {
    cairo_move_to(cr, x + c, y);
    cairo_line_to(cr, x + w - c, y);
    cairo_line_to(cr, x + w, y + c);
    cairo_line_to(cr, x + w, y + h - c);
    cairo_line_to(cr, x + w - c, y + h);
    cairo_line_to(cr, x + c, y + h);
    cairo_line_to(cr, x, y + h - c);
    cairo_line_to(cr, x, y + c);
    cairo_close_path(cr);
}

void command_center_render(cairo_t *cr, command_center_state_t *cc,
                           const orbital_system_t *sys, double w, double h) {
    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *title_font = pango_font_description_from_string("JetBrainsMono Nerd Font Bold 11");
    PangoFontDescription *tab_font = pango_font_description_from_string("JetBrainsMono Nerd Font Bold 8");
    PangoFontDescription *body_font = pango_font_description_from_string("JetBrainsMono Nerd Font 8");
    PangoFontDescription *small_font = pango_font_description_from_string("JetBrainsMono Nerd Font 7");

    // 1. Top Command Header Bar
    double bar_h = 48.0;
    cairo_save(cr);
    cairo_set_source_rgba(cr, 0.02, 0.05, 0.10, 0.92);
    cairo_rectangle(cr, 0, 0, w, bar_h);
    cairo_fill(cr);

    // Header bottom border
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.7);
    cairo_set_line_width(cr, 1.2);
    cairo_move_to(cr, 0, bar_h);
    cairo_line_to(cr, w, bar_h);
    cairo_stroke(cr);

    // System Title
    pango_layout_set_font_description(layout, title_font);
    pango_layout_set_text(layout, "🛸 SOLARIS // ORBITAL WORKSPACE COMMAND MATRIX", -1);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 1.0);
    cairo_move_to(cr, 24.0, 12.0);
    pango_cairo_show_layout(cr, layout);

    // Station Quick Tabs in Header Center
    pango_layout_set_font_description(layout, tab_font);
    double tab_start_x = w * 0.45;
    for (int i = 0; i < sys->workspace_count && i < 6; i++) {
        const workspace_info_t *ws = &sys->workspaces[i];
        double tw = 72.0;
        double tx = tab_start_x + (i * (tw + 6.0));
        double ty = 8.0;
        double th = 32.0;

        gboolean is_act = ws->is_active;
        draw_hud_box(cr, tx, ty, tw, th, 4.0);
        if (is_act) {
            cairo_set_source_rgba(cr, COLOR_CYAN_R * 0.2, COLOR_CYAN_G * 0.3, COLOR_CYAN_B * 0.4, 0.95);
        } else {
            cairo_set_source_rgba(cr, COLOR_DEEP_BLUE_R, COLOR_DEEP_BLUE_G, COLOR_DEEP_BLUE_B, 0.8);
        }
        cairo_fill_preserve(cr);

        cairo_set_source_rgba(cr, is_act ? COLOR_CYAN_R : COLOR_TEXT_MUTED_R,
                                  is_act ? COLOR_CYAN_G : COLOR_TEXT_MUTED_G,
                                  is_act ? COLOR_CYAN_B : COLOR_TEXT_MUTED_B,
                                  is_act ? 1.0 : 0.4);
        cairo_set_line_width(cr, is_act ? 1.4 : 0.8);
        cairo_stroke(cr);

        char t_buf[32];
        snprintf(t_buf, sizeof(t_buf), "STN %02d", ws->idx);
        pango_layout_set_text(layout, t_buf, -1);
        cairo_set_source_rgba(cr, is_act ? COLOR_TEXT_BRIGHT_R : COLOR_TEXT_MUTED_R + 0.3,
                                  is_act ? COLOR_TEXT_BRIGHT_G : COLOR_TEXT_MUTED_G + 0.3,
                                  is_act ? COLOR_TEXT_BRIGHT_B : COLOR_TEXT_MUTED_B + 0.3,
                                  1.0);
        cairo_move_to(cr, tx + 14.0, ty + 8.0);
        pango_cairo_show_layout(cr, layout);
    }

    // Close HUD button on top right
    double close_x = w - 120.0;
    double close_y = 10.0;
    draw_hud_box(cr, close_x, close_y, 100.0, 28.0, 4.0);
    cairo_set_source_rgba(cr, COLOR_ALERT_R * 0.2, 0.05, 0.1, 0.85);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_ALERT_R, 0.3, 0.3, 0.8);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    pango_layout_set_font_description(layout, tab_font);
    pango_layout_set_text(layout, "[ESC] CLOSE", -1);
    cairo_set_source_rgba(cr, COLOR_ALERT_R, 0.6, 0.6, 1.0);
    cairo_move_to(cr, close_x + 10.0, close_y + 6.0);
    pango_cairo_show_layout(cr, layout);
    cairo_restore(cr);

    // 2. Left Search & Window Matrix Rail
    double rail_w = 320.0;
    double rail_x = 24.0;
    double rail_y = bar_h + 18.0;
    double rail_h = h - rail_y - 48.0;

    cairo_save(cr);
    draw_hud_box(cr, rail_x, rail_y, rail_w, rail_h, 6.0);
    cairo_set_source_rgba(cr, 0.02, 0.06, 0.11, 0.90);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_TEAL_R, COLOR_TEAL_G, COLOR_TEAL_B, 0.5);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    // Search Input Bar
    double search_box_x = rail_x + 12.0;
    double search_box_y = rail_y + 12.0;
    double search_box_w = rail_w - 24.0;
    double search_box_h = 32.0;

    draw_hud_box(cr, search_box_x, search_box_y, search_box_w, search_box_h, 4.0);
    cairo_set_source_rgba(cr, 0.01, 0.03, 0.06, 0.95);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.8);
    cairo_set_line_width(cr, 1.2);
    cairo_stroke(cr);

    pango_layout_set_font_description(layout, body_font);
    char s_disp[160];
    if (cc->search_query[0] != '\0') {
        snprintf(s_disp, sizeof(s_disp), " %s█", cc->search_query);
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 1.0);
    } else {
        snprintf(s_disp, sizeof(s_disp), " Type to search open consoles...");
        cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.7);
    }
    pango_layout_set_text(layout, s_disp, -1);
    cairo_move_to(cr, search_box_x + 10.0, search_box_y + 8.0);
    pango_cairo_show_layout(cr, layout);

    // Filtered Window Items List
    double list_y = search_box_y + search_box_h + 14.0;
    int match_idx = 0;

    for (int i = 0; i < sys->window_count; i++) {
        const window_info_t *win = &sys->windows[i];

        // Filter by search query if provided
        if (cc->search_query[0] != '\0') {
            char l_query[128], l_title[256], l_app[128];
            for (size_t q = 0; cc->search_query[q] && q < sizeof(l_query)-1; q++) l_query[q] = tolower(cc->search_query[q]);
            l_query[strlen(cc->search_query)] = '\0';
            for (size_t q = 0; win->title[q] && q < sizeof(l_title)-1; q++) l_title[q] = tolower(win->title[q]);
            l_title[strlen(win->title)] = '\0';
            for (size_t q = 0; win->app_id[q] && q < sizeof(l_app)-1; q++) l_app[q] = tolower(win->app_id[q]);
            l_app[strlen(win->app_id)] = '\0';

            if (!strstr(l_title, l_query) && !strstr(l_app, l_query)) {
                continue;
            }
        }

        if (list_y + 44.0 > rail_y + rail_h - 10.0) break;

        gboolean is_sel = (match_idx == cc->search_selected_idx);
        gboolean is_foc = win->is_focused;

        double item_x = rail_x + 12.0;
        double item_w = rail_w - 24.0;
        double item_h = 38.0;

        draw_hud_box(cr, item_x, list_y, item_w, item_h, 4.0);
        if (is_sel) {
            cairo_set_source_rgba(cr, COLOR_CYAN_R * 0.25, COLOR_CYAN_G * 0.35, COLOR_CYAN_B * 0.45, 0.95);
        } else if (is_foc) {
            cairo_set_source_rgba(cr, 0.03, 0.12, 0.20, 0.85);
        } else {
            cairo_set_source_rgba(cr, 0.02, 0.05, 0.09, 0.75);
        }
        cairo_fill_preserve(cr);

        cairo_set_source_rgba(cr, is_sel ? COLOR_CYAN_R : (is_foc ? COLOR_TEAL_R : COLOR_TEXT_MUTED_R),
                                  is_sel ? COLOR_CYAN_G : (is_foc ? COLOR_TEAL_G : COLOR_TEXT_MUTED_G),
                                  is_sel ? COLOR_CYAN_B : (is_foc ? COLOR_TEAL_B : COLOR_TEXT_MUTED_B),
                                  is_sel ? 0.95 : 0.4);
        cairo_set_line_width(cr, is_sel ? 1.4 : 0.8);
        cairo_stroke(cr);

        // App Icon / Glyph
        PangoFontDescription *item_font = pango_font_description_from_string("JetBrainsMono Nerd Font Bold 8");
        pango_layout_set_font_description(layout, item_font);
        char item_h_buf[96];
        snprintf(item_h_buf, sizeof(item_h_buf), "%s %s", win->glyph, win->display_name);
        pango_layout_set_text(layout, item_h_buf, -1);
        cairo_set_source_rgba(cr, is_sel ? COLOR_TEXT_BRIGHT_R : (is_foc ? COLOR_CYAN_R : COLOR_TEXT_MUTED_R + 0.3),
                                  is_sel ? COLOR_TEXT_BRIGHT_G : (is_foc ? COLOR_CYAN_G : COLOR_TEXT_MUTED_G + 0.3),
                                  is_sel ? COLOR_TEXT_BRIGHT_B : (is_foc ? COLOR_CYAN_B : COLOR_TEXT_MUTED_B + 0.3),
                                  1.0);
        cairo_move_to(cr, item_x + 8.0, list_y + 4.0);
        pango_cairo_show_layout(cr, layout);

        // Title preview & station tag
        pango_layout_set_font_description(layout, small_font);
        char sub_buf[128];
        snprintf(sub_buf, sizeof(sub_buf), "WS #%d | %.24s", win->workspace_idx, win->title);
        pango_layout_set_text(layout, sub_buf, -1);
        cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.8);
        cairo_move_to(cr, item_x + 8.0, list_y + 20.0);
        pango_cairo_show_layout(cr, layout);
        pango_font_description_free(item_font);

        list_y += item_h + 6.0;
        match_idx++;
    }

    cairo_restore(cr);

    // 3. Window Inspector Modal Card (if an application is inspected)
    if (cc->inspected_window_id != 0) {
        const window_info_t *win = NULL;
        for (int i = 0; i < sys->window_count; i++) {
            if (sys->windows[i].id == cc->inspected_window_id) {
                win = &sys->windows[i];
                break;
            }
        }
        if (win) {
            double insp_w = 340.0;
            double insp_h = 240.0;
            double insp_x = w - insp_w - 24.0;
            double insp_y = bar_h + 18.0;

            cairo_save(cr);
            draw_hud_box(cr, insp_x, insp_y, insp_w, insp_h, 6.0);
            cairo_set_source_rgba(cr, 0.02, 0.07, 0.14, 0.96);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
            cairo_set_line_width(cr, 1.4);
            cairo_stroke(cr);

            // Inspector Header
            pango_layout_set_font_description(layout, title_font);
            char h_text[64];
            snprintf(h_text, sizeof(h_text), "INSPECTOR // %s", win->display_name);
            pango_layout_set_text(layout, h_text, -1);
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 1.0);
            cairo_move_to(cr, insp_x + 14.0, insp_y + 12.0);
            pango_cairo_show_layout(cr, layout);

            // Inspector Details
            pango_layout_set_font_description(layout, body_font);
            char dt_buf[256];
            snprintf(dt_buf, sizeof(dt_buf),
                     "TITLE: %.30s\nAPP ID: %s\nPROCESS ID: %d\nCURRENT STATION: %d\nLAYOUT: %s (%dx%d)",
                     win->title, win->app_id, win->pid, win->workspace_idx,
                     win->is_floating ? "FLOATING" : "TILED",
                     win->tile_width, win->tile_height);
            pango_layout_set_text(layout, dt_buf, -1);
            cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.9);
            cairo_move_to(cr, insp_x + 14.0, insp_y + 38.0);
            pango_cairo_show_layout(cr, layout);

            // Transfer Buttons
            pango_layout_set_font_description(layout, small_font);
            pango_layout_set_text(layout, "TRANSFER CONSOLE TO STATION:", -1);
            cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.2, COLOR_TEXT_MUTED_G + 0.2, COLOR_TEXT_MUTED_B + 0.2, 0.9);
            cairo_move_to(cr, insp_x + 14.0, insp_y + 140.0);
            pango_cairo_show_layout(cr, layout);

            for (int s = 1; s <= 6; s++) {
                double sb_x = insp_x + 14.0 + ((s - 1) * 50.0);
                double sb_y = insp_y + 160.0;
                draw_hud_box(cr, sb_x, sb_y, 44.0, 24.0, 3.0);
                cairo_set_source_rgba(cr, COLOR_DEEP_BLUE_R, COLOR_DEEP_BLUE_G, COLOR_DEEP_BLUE_B, 0.9);
                cairo_fill_preserve(cr);
                cairo_set_source_rgba(cr, COLOR_TEAL_R, COLOR_TEAL_G, COLOR_TEAL_B, 0.6);
                cairo_set_line_width(cr, 1.0);
                cairo_stroke(cr);

                char s_tag[16];
                snprintf(s_tag, sizeof(s_tag), "STN %d", s);
                pango_layout_set_text(layout, s_tag, -1);
                cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.9);
                cairo_move_to(cr, sb_x + 6.0, sb_y + 5.0);
                pango_cairo_show_layout(cr, layout);
            }

            // Close Console Button
            double close_btn_x = insp_x + 14.0;
            double close_btn_y = insp_y + 196.0;
            draw_hud_box(cr, close_btn_x, close_btn_y, 140.0, 26.0, 3.0);
            cairo_set_source_rgba(cr, COLOR_ALERT_R * 0.25, 0.05, 0.1, 0.85);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, COLOR_ALERT_R, 0.3, 0.3, 0.9);
            cairo_set_line_width(cr, 1.0);
            cairo_stroke(cr);

            pango_layout_set_text(layout, "DECOMMISSION CONSOLE", -1);
            cairo_set_source_rgba(cr, COLOR_ALERT_R, 0.6, 0.6, 1.0);
            cairo_move_to(cr, close_btn_x + 8.0, close_btn_y + 6.0);
            pango_cairo_show_layout(cr, layout);

            cairo_restore(cr);
        }
    }

    // 4. Bottom Footer Navigation Legend
    double foot_h = 32.0;
    double foot_y = h - foot_h;
    cairo_save(cr);
    cairo_set_source_rgba(cr, 0.02, 0.04, 0.08, 0.95);
    cairo_rectangle(cr, 0, foot_y, w, foot_h);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, COLOR_TEAL_R, COLOR_TEAL_G, COLOR_TEAL_B, 0.4);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, 0, foot_y);
    cairo_line_to(cr, w, foot_y);
    cairo_stroke(cr);

    pango_layout_set_font_description(layout, small_font);
    pango_layout_set_text(layout, "KEYS: [ARROWS] Navigate | [1-6] Jump Station | [ENTER] Focus | [/] Search | [DRAG] Rotate 3D Space | [SCROLL] Zoom | [ESC] Exit", -1);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.2, COLOR_TEXT_MUTED_G + 0.2, COLOR_TEXT_MUTED_B + 0.2, 0.9);
    cairo_move_to(cr, 24.0, foot_y + 8.0);
    pango_cairo_show_layout(cr, layout);
    cairo_restore(cr);

    pango_font_description_free(title_font);
    pango_font_description_free(tab_font);
    pango_font_description_free(body_font);
    pango_font_description_free(small_font);
    g_object_unref(layout);
}

gboolean command_center_handle_key(command_center_state_t *cc, orbital_system_t *sys,
                                   niri_ipc_t *ipc, guint keyval) {
    if (keyval == GDK_KEY_Escape) {
        if (cc->inspected_window_id != 0) {
            cc->inspected_window_id = 0;
            return TRUE;
        }
        if (cc->search_query[0] != '\0') {
            cc->search_query[0] = '\0';
            return TRUE;
        }
        cc->is_active = FALSE;
        return TRUE;
    }

    // Number keys 1..6: Switch Station
    if (keyval >= GDK_KEY_1 && keyval <= GDK_KEY_6) {
        int stn = keyval - GDK_KEY_0;
        niri_ipc_focus_workspace_idx(ipc, stn);
        return TRUE;
    }

    // Up / Down arrows: navigate search list
    if (keyval == GDK_KEY_Up) {
        if (cc->search_selected_idx > 0) cc->search_selected_idx--;
        return TRUE;
    }
    if (keyval == GDK_KEY_Down) {
        cc->search_selected_idx++;
        return TRUE;
    }

    // Left / Right arrows: cycle active workspaces
    if (keyval == GDK_KEY_Left) {
        int next_ws = (sys->active_workspace_idx > 1) ? sys->active_workspace_idx - 1 : sys->workspace_count;
        niri_ipc_focus_workspace_idx(ipc, next_ws);
        return TRUE;
    }
    if (keyval == GDK_KEY_Right) {
        int next_ws = (sys->active_workspace_idx < sys->workspace_count) ? sys->active_workspace_idx + 1 : 1;
        niri_ipc_focus_workspace_idx(ipc, next_ws);
        return TRUE;
    }

    // Enter: Focus selected search window or workspace
    if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
        int cur_match = 0;
        for (int i = 0; i < sys->window_count; i++) {
            const window_info_t *win = &sys->windows[i];
            if (cc->search_query[0] != '\0') {
                char l_query[128], l_title[256], l_app[128];
                for (size_t q = 0; cc->search_query[q] && q < sizeof(l_query)-1; q++) l_query[q] = tolower(cc->search_query[q]);
                l_query[strlen(cc->search_query)] = '\0';
                for (size_t q = 0; win->title[q] && q < sizeof(l_title)-1; q++) l_title[q] = tolower(win->title[q]);
                l_title[strlen(win->title)] = '\0';
                for (size_t q = 0; win->app_id[q] && q < sizeof(l_app)-1; q++) l_app[q] = tolower(win->app_id[q]);
                l_app[strlen(win->app_id)] = '\0';
                if (!strstr(l_title, l_query) && !strstr(l_app, l_query)) continue;
            }
            if (cur_match == cc->search_selected_idx) {
                niri_ipc_focus_window(ipc, win->id);
                cc->is_active = FALSE;
                return TRUE;
            }
            cur_match++;
        }
        return TRUE;
    }

    // Backspace for search
    if (keyval == GDK_KEY_BackSpace) {
        size_t len = strlen(cc->search_query);
        if (len > 0) {
            cc->search_query[len - 1] = '\0';
            cc->search_selected_idx = 0;
        }
        return TRUE;
    }

    // Printable characters for search query
    if (keyval >= 32 && keyval <= 126) {
        size_t len = strlen(cc->search_query);
        if (len < sizeof(cc->search_query) - 2) {
            cc->search_query[len] = (char)keyval;
            cc->search_query[len + 1] = '\0';
            cc->search_selected_idx = 0;
        }
        return TRUE;
    }

    return FALSE;
}

gboolean command_center_handle_click(command_center_state_t *cc, orbital_system_t *sys,
                                     niri_ipc_t *ipc, double mx, double my) {
    // 1. Top Close Button click
    if (my >= 8.0 && my <= 40.0 && mx >= 1706.0 - 140.0) {
        cc->is_active = FALSE;
        return TRUE;
    }

    // 2. Station Tabs click in header
    double tab_start_x = 1706.0 * 0.45;
    for (int i = 0; i < sys->workspace_count && i < 6; i++) {
        double tx = tab_start_x + (i * (72.0 + 6.0));
        if (mx >= tx && mx <= tx + 72.0 && my >= 8.0 && my <= 40.0) {
            niri_ipc_focus_workspace_idx(ipc, sys->workspaces[i].idx);
            return TRUE;
        }
    }

    // 3. Search Rail Items click
    double rail_x = 24.0;
    double rail_y = 48.0 + 18.0;
    double list_y = rail_y + 12.0 + 32.0 + 14.0;
    for (int i = 0; i < sys->window_count; i++) {
        const window_info_t *win = &sys->windows[i];
        if (mx >= rail_x + 12.0 && mx <= rail_x + 320.0 - 12.0 &&
            my >= list_y && my <= list_y + 38.0) {
            niri_ipc_focus_window(ipc, win->id);
            cc->inspected_window_id = win->id;
            return TRUE;
        }
        list_y += 38.0 + 6.0;
    }

    // 4. Inspector Transfer buttons click
    if (cc->inspected_window_id != 0) {
        double insp_w = 340.0;
        double insp_x = 1706.0 - insp_w - 24.0;
        double insp_y = 48.0 + 18.0;

        // Station transfer buttons
        for (int s = 1; s <= 6; s++) {
            double sb_x = insp_x + 14.0 + ((s - 1) * 50.0);
            double sb_y = insp_y + 160.0;
            if (mx >= sb_x && mx <= sb_x + 44.0 && my >= sb_y && my <= sb_y + 24.0) {
                niri_ipc_move_window_to_workspace(ipc, cc->inspected_window_id, s);
                return TRUE;
            }
        }

        // Close console button
        double cb_x = insp_x + 14.0;
        double cb_y = insp_y + 196.0;
        if (mx >= cb_x && mx <= cb_x + 140.0 && my >= cb_y && my <= cb_y + 26.0) {
            niri_ipc_close_window(ipc, cc->inspected_window_id);
            cc->inspected_window_id = 0;
            return TRUE;
        }
    }

    return FALSE;
}
