#include "orbital_core.h"
#include <gdk/gdkkeysyms.h>
#include <ctype.h>

static void draw_tech_box(cairo_t *cr, double x, double y, double w, double h, double c) {
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

void terminal_init(holographic_terminal_t *term) {
    memset(term, 0, sizeof(*term));
    term->cursor_pos = 0;
    term->scroll_offset = 0;
    term->has_focus = TRUE;

    terminal_write_line(term, "╔══════════════════════════════════════════════════════════════╗");
    terminal_write_line(term, "║ 🛸 SOLARIS // REMOTE SSH TERMINAL (TAILSCALE MESH)          ║");
    terminal_write_line(term, "║ REMOTE PEER: venkat-server // TAILSCALE IP: 100.110.135.100  ║");
    terminal_write_line(term, "║ SSH TARGET : venkat-server@100.110.135.100 [PORT 22]        ║");
    terminal_write_line(term, "╚══════════════════════════════════════════════════════════════╝");
    terminal_write_line(term, "Type 'ssh' or click [SSH] to launch interactive SSH session.");
    terminal_write_line(term, "Type 'ts' for Tailscale status. Press [ESC] to fold terminal.");
    terminal_write_line(term, "");
}

void terminal_write_line(holographic_terminal_t *term, const char *line) {
    if (term->line_count < TERMINAL_MAX_LINES) {
        snprintf(term->lines[term->line_count], TERMINAL_LINE_LEN, "%s", line);
        term->line_count++;
    } else {
        // Shift lines up
        for (int i = 0; i < TERMINAL_MAX_LINES - 1; i++) {
            memcpy(term->lines[i], term->lines[i + 1], TERMINAL_LINE_LEN);
        }
        snprintf(term->lines[TERMINAL_MAX_LINES - 1], TERMINAL_LINE_LEN, "%s", line);
    }
}

void terminal_execute_command(holographic_terminal_t *term, const char *cmd, server_telemetry_t *tel) {
    if (!cmd || cmd[0] == '\0') return;

    char echo_buf[TERMINAL_LINE_LEN];
    snprintf(echo_buf, sizeof(echo_buf), "solaris@core:~$ %s", cmd);
    terminal_write_line(term, echo_buf);

    // Trim whitespace
    char trimmed[128];
    snprintf(trimmed, sizeof(trimmed), "%s", cmd);
    char *p = trimmed;
    while (isspace(*p)) p++;
    char *end = p + strlen(p) - 1;
    while (end > p && isspace(*end)) *end-- = '\0';

    if (strcmp(p, "clear") == 0) {
        term->line_count = 0;
        return;
    } else if (strcmp(p, "ssh") == 0 || strcmp(p, "connect") == 0 || strcmp(p, "venkat-server") == 0) {
        terminal_write_line(term, "Initiating Tailscale SSH connection to venkat-server...");
        terminal_write_line(term, "Destination: venkat-server@100.110.135.100 [Port 22]");
        terminal_write_line(term, "Spawning interactive SSH session in Kitty terminal...");
        system("kitty -e ssh venkat-server@100.110.135.100 & disown");
        return;
    } else if (strcmp(p, "ts") == 0 || strcmp(p, "tailscale") == 0) {
        terminal_write_line(term, "TAILSCALE MESH STATUS:");
        FILE *fp = popen("tailscale status 2>/dev/null | grep -E 'venkat-server|solaris'", "r");
        if (fp) {
            char buf[256];
            while (fgets(buf, sizeof(buf), fp)) {
                buf[strcspn(buf, "\r\n")] = '\0';
                terminal_write_line(term, buf);
            }
            pclose(fp);
        }
        return;
    } else if (strcmp(p, "ping") == 0) {
        terminal_write_line(term, "Pinging venkat-server (100.110.135.100)...");
        FILE *fp = popen("tailscale ping -c 2 100.110.135.100 2>&1", "r");
        if (fp) {
            char buf[256];
            while (fgets(buf, sizeof(buf), fp)) {
                buf[strcspn(buf, "\r\n")] = '\0';
                terminal_write_line(term, buf);
            }
            pclose(fp);
        }
        return;
    } else if (strcmp(p, "help") == 0) {
        terminal_write_line(term, "AVAILABLE COMMANDS:");
        terminal_write_line(term, "  ssh / connect        - Launch interactive SSH to venkat-server@100.110.135.100");
        terminal_write_line(term, "  ts / tailscale       - Show Tailscale peer connection status");
        terminal_write_line(term, "  ping                 - Ping venkat-server via Tailscale mesh");
        terminal_write_line(term, "  sysinfo / fastfetch  - Display system summary & hardware specs");
        terminal_write_line(term, "  top / btop / ps      - Show active processes & CPU load");
        terminal_write_line(term, "  df / disk            - Storage & NVMe partition status");
        terminal_write_line(term, "  net / ip             - Network interfaces & bandwidth");
        terminal_write_line(term, "  temp / thermal       - Thermal sensor telemetry");
        terminal_write_line(term, "  uptime               - System uptime and load averages");
        terminal_write_line(term, "  kitty                - Launch external dedicated terminal");
        terminal_write_line(term, "  clear                - Clear terminal buffer");
        terminal_write_line(term, "  exit                 - Close holographic terminal");
        terminal_write_line(term, "  <shell-cmd>          - Execute any Linux bash command");
        return;
    } else if (strcmp(p, "sysinfo") == 0 || strcmp(p, "fastfetch") == 0 || strcmp(p, "neofetch") == 0) {
        char buf[256];
        terminal_write_line(term, "─── SYSTEM MATRIX ───────────────────────────────────");
        snprintf(buf, sizeof(buf), "  OS      : Solaris OS (Arch Linux Base)"); terminal_write_line(term, buf);
        snprintf(buf, sizeof(buf), "  HOST    : %.40s", tel->hostname); terminal_write_line(term, buf);
        snprintf(buf, sizeof(buf), "  KERNEL  : %.80s", tel->kernel_os); terminal_write_line(term, buf);
        snprintf(buf, sizeof(buf), "  UPTIME  : %.40s", tel->uptime_str); terminal_write_line(term, buf);
        snprintf(buf, sizeof(buf), "  MEMORY  : %.1f GB / %.1f GB (%.1f%%)", tel->mem_used_gb, tel->mem_total_gb, tel->mem_percent); terminal_write_line(term, buf);
        snprintf(buf, sizeof(buf), "  CPU TEMP: %.1f°C [NOMINAL]", tel->cpu_temp); terminal_write_line(term, buf);
        terminal_write_line(term, "─────────────────────────────────────────────────────");
        return;
    } else if (strcmp(p, "top") == 0 || strcmp(p, "btop") == 0 || strcmp(p, "ps") == 0) {
        char buf[128];
        terminal_write_line(term, "PID    USER     %CPU  %MEM  COMMAND");
        FILE *fp = popen("ps -eo pid,user,%cpu,%mem,comm --sort=-%cpu | head -n 6 | tail -n 5", "r");
        if (fp) {
            while (fgets(buf, sizeof(buf), fp)) {
                buf[strcspn(buf, "\r\n")] = '\0';
                terminal_write_line(term, buf);
            }
            pclose(fp);
        } else {
            terminal_write_line(term, "  Process telemetry unavailable.");
        }
        return;
    } else if (strcmp(p, "df") == 0 || strcmp(p, "disk") == 0) {
        char buf[128];
        terminal_write_line(term, "FILESYSTEM      SIZE  USED AVAIL USE% MOUNTED ON");
        FILE *fp = popen("df -h / /home 2>/dev/null | tail -n +2", "r");
        if (fp) {
            while (fgets(buf, sizeof(buf), fp)) {
                buf[strcspn(buf, "\r\n")] = '\0';
                terminal_write_line(term, buf);
            }
            pclose(fp);
        }
        return;
    } else if (strcmp(p, "net") == 0 || strcmp(p, "ip") == 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "NETWORK THROUGHPUT: RX: %.1f KB/s | TX: %.1f KB/s", tel->net_rx_kbs, tel->net_tx_kbs);
        terminal_write_line(term, buf);
        FILE *fp = popen("ip -br addr show | head -n 4", "r");
        if (fp) {
            while (fgets(buf, sizeof(buf), fp)) {
                buf[strcspn(buf, "\r\n")] = '\0';
                terminal_write_line(term, buf);
            }
            pclose(fp);
        }
        return;
    } else if (strcmp(p, "uptime") == 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "UPTIME: %s // CPU LOAD: %.1f%%", tel->uptime_str, tel->cpu_usage);
        terminal_write_line(term, buf);
        return;
    } else if (strcmp(p, "kitty") == 0) {
        terminal_write_line(term, "Spawning external kitty terminal session...");
        system("kitty & disown");
        return;
    } else if (strcmp(p, "exit") == 0) {
        terminal_write_line(term, "Closing holographic terminal...");
        return;
    } else {
        // Execute arbitrary bash command
        char exec_cmd[256];
        snprintf(exec_cmd, sizeof(exec_cmd), "%s 2>&1 | head -n 8", p);
        FILE *fp = popen(exec_cmd, "r");
        if (fp) {
            char buf[TERMINAL_LINE_LEN];
            gboolean got_output = FALSE;
            while (fgets(buf, sizeof(buf), fp)) {
                buf[strcspn(buf, "\r\n")] = '\0';
                terminal_write_line(term, buf);
                got_output = TRUE;
            }
            pclose(fp);
            if (!got_output) {
                terminal_write_line(term, "[Command completed with no output]");
            }
        } else {
            terminal_write_line(term, "bash: command execution failed");
        }
    }
}

void terminal_handle_key(holographic_terminal_t *term, guint keyval, server_telemetry_t *tel, orbital_core_t *core) {
    if (keyval == GDK_KEY_Escape) {
        orbital_core_set_state(core, CORE_STATE_SERVER_CORE);
        return;
    }

    if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
        if (strlen(term->current_input) > 0) {
            if (strcmp(term->current_input, "exit") == 0) {
                orbital_core_set_state(core, CORE_STATE_SERVER_CORE);
                term->current_input[0] = '\0';
                term->cursor_pos = 0;
                return;
            }
            terminal_execute_command(term, term->current_input, tel);
            snprintf(term->last_command, sizeof(term->last_command), "%s", term->current_input);
            term->current_input[0] = '\0';
            term->cursor_pos = 0;
        } else {
            terminal_write_line(term, "solaris@core:~$ ");
        }
        return;
    }

    if (keyval == GDK_KEY_BackSpace) {
        int len = strlen(term->current_input);
        if (len > 0) {
            term->current_input[len - 1] = '\0';
            term->cursor_pos = len - 1;
        }
        return;
    }

    if (keyval == GDK_KEY_Up) {
        if (strlen(term->last_command) > 0) {
            snprintf(term->current_input, sizeof(term->current_input), "%s", term->last_command);
            term->cursor_pos = strlen(term->current_input);
        }
        return;
    }

    // Normal printable character
    if (keyval >= 0x20 && keyval <= 0x7e) {
        int len = strlen(term->current_input);
        if (len < TERMINAL_CMD_LEN - 2) {
            term->current_input[len] = (char)keyval;
            term->current_input[len + 1] = '\0';
            term->cursor_pos = len + 1;
        }
    }
}

void render_holographic_terminal(cairo_t *cr, orbital_core_t *core, const camera3d_t *cam,
                                 double cx, double cy, double w, double h) {
    double t = core->server_to_terminal_t;
    if (t <= 0.001) return;

    cairo_save(cr);

    // Easing for smooth unfolding scale & opacity
    double anim_scale = 0.70 + (0.30 * t);
    double anim_alpha = t;

    // Terminal geometry inside the central workspace (740x640)
    double tw = 620.0 * anim_scale;
    double th = 440.0 * anim_scale;
    double tx = cx - (tw * 0.5);
    double ty = cy - (th * 0.5) + 12.0;

    // 1. Holographic Emitter Projection Arms (from center core to 4 terminal corners)
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, anim_alpha * 0.35);
    cairo_set_line_width(cr, 1.0);

    vec2_t corners[4] = {
        {tx, ty},
        {tx + tw, ty},
        {tx + tw, ty + th},
        {tx, ty + th}
    };

    for (int i = 0; i < 4; i++) {
        cairo_move_to(cr, cx, cy);
        cairo_line_to(cr, corners[i].x, corners[i].y);
    }
    cairo_stroke(cr);

    // Targeting Reticle at center
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, anim_alpha * 0.5);
    cairo_arc(cr, cx, cy, 26.0 * (1.0 - (0.4 * t)), 0, 2 * M_PI);
    cairo_stroke(cr);

    // 2. Terminal Glass Background
    draw_tech_box(cr, tx, ty, tw, th, 6.0);
    cairo_set_source_rgba(cr, 0.015, 0.035, 0.065, anim_alpha * 0.94);
    cairo_fill_preserve(cr);

    // Terminal Border
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, anim_alpha * 0.85);
    cairo_set_line_width(cr, 1.4);
    cairo_stroke(cr);

    // Subtle Scanlines inside terminal window
    cairo_save(cr);
    draw_tech_box(cr, tx, ty, tw, th, 6.0);
    cairo_clip(cr);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, anim_alpha * 0.035);
    cairo_set_line_width(cr, 0.5);
    for (double sy = ty; sy < ty + th; sy += 3.5) {
        cairo_move_to(cr, tx, sy);
        cairo_line_to(cr, tx + tw, sy);
    }
    cairo_stroke(cr);
    cairo_restore(cr);

    // 3. Technical Corner Brackets
    double brk = 12.0;
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, anim_alpha * 0.95);
    cairo_set_line_width(cr, 2.0);
    // Top Left
    cairo_move_to(cr, tx, ty + brk); cairo_line_to(cr, tx, ty); cairo_line_to(cr, tx + brk, ty);
    // Top Right
    cairo_move_to(cr, tx + tw - brk, ty); cairo_line_to(cr, tx + tw, ty); cairo_line_to(cr, tx + tw, ty + brk);
    // Bottom Right
    cairo_move_to(cr, tx + tw, ty + th - brk); cairo_line_to(cr, tx + tw, ty + th); cairo_line_to(cr, tx + tw - brk, ty + th);
    // Bottom Left
    cairo_move_to(cr, tx + brk, ty + th); cairo_line_to(cr, tx, ty + th); cairo_line_to(cr, tx, ty + th - brk);
    cairo_stroke(cr);

    // 4. Terminal Header Bar
    double hdr_h = 32.0;
    cairo_set_source_rgba(cr, 0.025, 0.06, 0.12, anim_alpha * 0.96);
    cairo_rectangle(cr, tx + 1.0, ty + 1.0, tw - 2.0, hdr_h);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, anim_alpha * 0.6);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, tx, ty + hdr_h);
    cairo_line_to(cr, tx + tw, ty + hdr_h);
    cairo_stroke(cr);

    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *f_hdr = pango_font_description_from_string("JetBrainsMono Nerd Font Bold 8");
    PangoFontDescription *f_btn = pango_font_description_from_string("JetBrainsMono Nerd Font Bold 7.5");
    PangoFontDescription *f_term = pango_font_description_from_string("JetBrainsMono Nerd Font 8");

    // Title on Left
    pango_layout_set_font_description(layout, f_hdr);
    char hdr_txt[128];
    snprintf(hdr_txt, sizeof(hdr_txt), " SOLARIS TERMINAL // TARGET: venkat-server@100.110.135.100 // tty1");
    pango_layout_set_text(layout, hdr_txt, -1);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, anim_alpha * 1.0);
    cairo_move_to(cr, tx + 12.0, ty + 8.5);
    pango_cairo_show_layout(cr, layout);

    // Status pill
    double pill_x = tx + 370.0 * anim_scale;
    draw_tech_box(cr, pill_x, ty + 6.0, 78.0, 20.0, 3.0);
    cairo_set_source_rgba(cr, 0.0, 0.3, 0.15, anim_alpha * 0.85);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, anim_alpha * 0.9);
    cairo_set_line_width(cr, 0.9);
    cairo_stroke(cr);

    pango_layout_set_font_description(layout, f_btn);
    pango_layout_set_text(layout, "● TS DIRECT", -1);
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, anim_alpha * 1.0);
    cairo_move_to(cr, pill_x + 8.0, ty + 9.5);
    pango_cairo_show_layout(cr, layout);

    // Header Action Buttons: [ 󰒋 SSH ] [ - MIN ] [ ✕ CLOSE ]
    const char *h_btns[] = {"[ 󰒋 SSH ]", "[ - MIN ]", "[ ✕ CLOSE ]"};
    double h_btn_w = 66.0;
    for (int b = 0; b < 3; b++) {
        double bx = tx + tw - 12.0 - ((3 - b) * (h_btn_w + 4.0));
        double by = ty + 6.0;
        draw_tech_box(cr, bx, by, h_btn_w, 20.0, 3.0);
        if (b == 0) {
            // SSH Button (cyan / green highlight)
            cairo_set_source_rgba(cr, 0.0, 0.3, 0.2, anim_alpha * 0.85);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, anim_alpha * 0.95);
        } else if (b == 2) {
            // Close button (amber-red accent)
            cairo_set_source_rgba(cr, COLOR_ALERT_R * 0.25, 0.05, 0.10, anim_alpha * 0.85);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, COLOR_ALERT_R, 0.4, 0.4, anim_alpha * 0.9);
        } else {
            cairo_set_source_rgba(cr, COLOR_DEEP_BLUE_R, COLOR_DEEP_BLUE_G, COLOR_DEEP_BLUE_B, anim_alpha * 0.85);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, anim_alpha * 0.65);
        }
        cairo_set_line_width(cr, 0.8);
        cairo_stroke(cr);

        pango_layout_set_text(layout, h_btns[b], -1);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, anim_alpha * 0.95);
        cairo_move_to(cr, bx + 6.0, by + 4.0);
        pango_cairo_show_layout(cr, layout);
    }

    // 5. Quick Command Toolbar
    double bar_y = ty + hdr_h + 4.0;
    const char *q_cmds[] = {"[ 󰒋 SSH ]", "[ 󰛳 TS MESH ]", "[ 󰘳 SYSINFO ]", "[ 󰍛 TOP ]", "[ 󰘚 DISK ]", "[ 󰃢 CLEAR ]"};
    double q_w = (tw - 24.0) / 6.0;
    for (int q = 0; q < 6; q++) {
        double qx = tx + 12.0 + (q * q_w);
        draw_tech_box(cr, qx + 1.0, bar_y, q_w - 2.0, 20.0, 2.0);
        cairo_set_source_rgba(cr, 0.015, 0.04, 0.08, anim_alpha * 0.85);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, COLOR_TEAL_R, COLOR_TEAL_G, COLOR_TEAL_B, anim_alpha * 0.5);
        cairo_set_line_width(cr, 0.7);
        cairo_stroke(cr);

        pango_layout_set_font_description(layout, f_btn);
        pango_layout_set_text(layout, q_cmds[q], -1);
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, anim_alpha * 0.9);
        cairo_move_to(cr, qx + 5.0, bar_y + 4.5);
        pango_cairo_show_layout(cr, layout);
    }

    // 6. Terminal Output Console Buffer
    double out_x = tx + 14.0;
    double out_y = bar_y + 26.0;
    double out_w = tw - 28.0;
    double out_h = th - (out_y - ty) - 34.0;

    cairo_save(cr);
    cairo_rectangle(cr, out_x, out_y, out_w, out_h);
    cairo_clip(cr);

    pango_layout_set_font_description(layout, f_term);

    // Calculate how many lines fit
    const double LINE_SPACING = 15.0;
    int max_visible_lines = (int)(out_h / LINE_SPACING);
    int start_line = 0;
    if (core->terminal.line_count > max_visible_lines) {
        start_line = core->terminal.line_count - max_visible_lines;
    }

    double line_y = out_y;
    for (int i = start_line; i < core->terminal.line_count; i++) {
        const char *l_str = core->terminal.lines[i];
        if (strstr(l_str, "solaris@core:~$")) {
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, anim_alpha * 1.0);
        } else if (strstr(l_str, "═══") || strstr(l_str, "───")) {
            cairo_set_source_rgba(cr, COLOR_TEAL_R, COLOR_TEAL_G, COLOR_TEAL_B, anim_alpha * 0.7);
        } else if (strstr(l_str, "OS") || strstr(l_str, "HOST") || strstr(l_str, "CPU") || strstr(l_str, "PID")) {
            cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, anim_alpha * 0.95);
        } else {
            cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, anim_alpha * 0.88);
        }
        pango_layout_set_text(layout, l_str, -1);
        cairo_move_to(cr, out_x, line_y);
        pango_cairo_show_layout(cr, layout);
        line_y += LINE_SPACING;
    }
    cairo_restore(cr);

    // 7. Interactive Command Prompt Input Area
    double in_y = ty + th - 26.0;
    cairo_set_source_rgba(cr, 0.015, 0.04, 0.08, anim_alpha * 0.95);
    cairo_rectangle(cr, out_x, in_y, out_w, 20.0);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, anim_alpha * 0.7);
    cairo_set_line_width(cr, 0.9);
    cairo_stroke(cr);

    // Prompt indicator
    pango_layout_set_font_description(layout, f_hdr);
    pango_layout_set_text(layout, "solaris@core:~$ ", -1);
    cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, anim_alpha * 1.0);
    cairo_move_to(cr, out_x + 6.0, in_y + 4.0);
    pango_cairo_show_layout(cr, layout);

    int pw, ph;
    pango_layout_get_pixel_size(layout, &pw, &ph);

    // Current input string
    pango_layout_set_font_description(layout, f_term);
    pango_layout_set_text(layout, core->terminal.current_input, -1);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, anim_alpha * 0.95);
    cairo_move_to(cr, out_x + 6.0 + pw, in_y + 4.0);
    pango_cairo_show_layout(cr, layout);

    int iw, ih;
    pango_layout_get_pixel_size(layout, &iw, &ih);

    // Blinking Block Cursor
    if (fmod(core->terminal.cursor_blink_timer, 1.0) < 0.5) {
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, anim_alpha * 0.95);
        cairo_rectangle(cr, out_x + 6.0 + pw + iw + 1.0, in_y + 4.0, 7.0, 13.0);
        cairo_fill(cr);
    }

    g_object_unref(layout);
    pango_font_description_free(f_hdr);
    pango_font_description_free(f_btn);
    pango_font_description_free(f_term);

    cairo_restore(cr);
}
