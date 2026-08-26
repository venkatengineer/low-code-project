#include "common.h"
#include "telemetry.h"
#include "cairo_utils.h"

// ==============================================================================
// 🛸 SOLARIS // LIVE BLUETOOTH HARDWARE MATRIX & CONTROLLER (REAL BLUEZ / RFKILL)
// ==============================================================================

typedef struct {
    char name[64];
    char mac[24];
    char type[32];
    const char *icon;
    int rssi_dbm;
    gboolean connected;
} bt_dev_entry_t;

typedef struct {
    GtkWidget *window;
    GtkWidget *drawing_area;
    double time_sec;
    guint anim_timer_id;
    guint telem_timer_id;

    int hovered_idx; // -1: none, -2: rescan, -3: power toggle, -4: pair wizard, -5: install bluez, 0..N: device
    int scroll_offset;

    gboolean hw_present;
    gboolean rf_blocked;
    gboolean has_bluez;
    gboolean bt_enabled;
    gboolean is_rescanning;
    double rescan_anim_timer;

    bt_dev_entry_t devs[32];
    int dev_count;
    int connected_count;
    char controller_mac[24];
} bt_app_t;

static bt_app_t g_app;

static void query_real_bt_state(void) {
    // 1. Hardware Presence
    g_app.hw_present = (access("/sys/class/bluetooth/hci0", F_OK) == 0);

    // 2. RFKill Status
    g_app.rf_blocked = FALSE;
    FILE *rf_fp = popen("rfkill list bluetooth 2>/dev/null", "r");
    if (rf_fp) {
        char line[128];
        while (fgets(line, sizeof(line), rf_fp)) {
            if (strstr(line, "Soft blocked: yes") || strstr(line, "Hard blocked: yes")) {
                g_app.rf_blocked = TRUE;
            }
        }
        pclose(rf_fp);
    }
    g_app.bt_enabled = g_app.hw_present && !g_app.rf_blocked;

    // 3. Controller MAC address from sysfs
    FILE *addr_fp = fopen("/sys/class/bluetooth/hci0/address", "r");
    if (addr_fp) {
        if (fgets(g_app.controller_mac, sizeof(g_app.controller_mac), addr_fp)) {
            char *nl = strchr(g_app.controller_mac, '\n');
            if (nl) *nl = '\0';
        }
        fclose(addr_fp);
    } else {
        snprintf(g_app.controller_mac, sizeof(g_app.controller_mac), "HCI0 // RF-0");
    }

    // 4. Check if bluetoothctl is installed
    g_app.has_bluez = (system("which bluetoothctl >/dev/null 2>&1") == 0);

    g_app.dev_count = 0;
    g_app.connected_count = 0;

    if (!g_app.has_bluez) {
        return;
    }

    // 5. Query Connected Devices via bluetoothctl
    char connected_macs[16][24];
    int conn_count = 0;
    FILE *cfp = popen("bluetoothctl devices Connected 2>/dev/null", "r");
    if (cfp) {
        char buf[256];
        while (fgets(buf, sizeof(buf), cfp) && conn_count < 16) {
            char mac[24];
            if (sscanf(buf, "Device %17s", mac) == 1) {
                snprintf(connected_macs[conn_count++], sizeof(connected_macs[0]), "%s", mac);
            }
        }
        pclose(cfp);
    }

    // 6. Query All Devices (Paired and Discovered)
    FILE *dfp = popen("bluetoothctl devices 2>/dev/null", "r");
    if (dfp) {
        char buf[256];
        while (fgets(buf, sizeof(buf), dfp) && g_app.dev_count < 32) {
            char mac[24];
            char name[64];
            if (sscanf(buf, "Device %17s %63[^\n]", mac, name) == 2) {
                bt_dev_entry_t *dev = &g_app.devs[g_app.dev_count];
                snprintf(dev->mac, sizeof(dev->mac), "%s", mac);
                snprintf(dev->name, sizeof(dev->name), "%s", name);
                dev->connected = FALSE;

                for (int c = 0; c < conn_count; c++) {
                    if (strcasecmp(connected_macs[c], mac) == 0) {
                        dev->connected = TRUE;
                        g_app.connected_count++;
                        break;
                    }
                }

                // Detect Peripheral Category & Icon
                if (strstr(name, "XM") || strstr(name, "WH-") || strstr(name, "WF-") ||
                    strstr(name, "Head") || strstr(name, "AirPod") || strstr(name, "Buds") ||
                    strstr(name, "Audio") || strstr(name, "Speaker") || strstr(name, "Sound") ||
                    strstr(name, "JBL") || strstr(name, "Sony") || strstr(name, "Bose")) {
                    strncpy(dev->type, "AUDIO / HEADSET", sizeof(dev->type));
                    dev->icon = "󰋋";
                } else if (strstr(name, "Mouse") || strstr(name, "Master") || strstr(name, "Trackball") || strstr(name, "G Pro") || strstr(name, "Razer")) {
                    strncpy(dev->type, "MOUSE / POINTER", sizeof(dev->type));
                    dev->icon = "󰍽";
                } else if (strstr(name, "Keyboard") || strstr(name, "Keychron") || strstr(name, "K2") || strstr(name, "K3") || strstr(name, "K8") || strstr(name, "NuPhy")) {
                    strncpy(dev->type, "KEYBOARD / HID", sizeof(dev->type));
                    dev->icon = "󰌌";
                } else if (strstr(name, "Phone") || strstr(name, "Galaxy") || strstr(name, "iPhone") || strstr(name, "Pixel") || strstr(name, "OnePlus") || strstr(name, "Xiaomi")) {
                    strncpy(dev->type, "MOBILE DEVICE", sizeof(dev->type));
                    dev->icon = "󰏲";
                } else if (strstr(name, "Controller") || strstr(name, "DualSense") || strstr(name, "Xbox") || strstr(name, "Wireless Controller")) {
                    strncpy(dev->type, "GAME CONTROLLER", sizeof(dev->type));
                    dev->icon = "󰊴";
                } else {
                    strncpy(dev->type, "BLUETOOTH LINK", sizeof(dev->type));
                    dev->icon = "󰂯";
                }

                dev->rssi_dbm = dev->connected ? -42 : -65;
                g_app.dev_count++;
            }
        }
        pclose(dfp);
    }
}

// Trigger Bluetooth Discovery Scan
static void trigger_bt_rescan(void) {
    g_app.is_rescanning = TRUE;
    g_app.rescan_anim_timer = 5.0;
    if (g_app.has_bluez) {
        g_spawn_command_line_async("sh -c 'bluetoothctl --timeout 6 scan on &'", NULL);
    }
    if (g_app.drawing_area) {
        gtk_widget_queue_draw(g_app.drawing_area);
    }
}

// Toggle Bluetooth RF Radio Power
static void trigger_rf_toggle(void) {
    if (g_app.rf_blocked) {
        g_spawn_command_line_async("sh -c 'rfkill unblock bluetooth; bluetoothctl power on 2>/dev/null || true'", NULL);
    } else {
        g_spawn_command_line_async("sh -c 'bluetoothctl power off 2>/dev/null; rfkill block bluetooth || true'", NULL);
    }
    query_real_bt_state();
    if (g_app.drawing_area) {
        gtk_widget_queue_draw(g_app.drawing_area);
    }
}

// Connect / Disconnect Device Action
static void trigger_device_toggle(int idx) {
    if (idx < 0 || idx >= g_app.dev_count) return;
    bt_dev_entry_t *dev = &g_app.devs[idx];

    char cmd[256];
    if (dev->connected) {
        snprintf(cmd, sizeof(cmd), "bluetoothctl disconnect %s", dev->mac);
    } else {
        snprintf(cmd, sizeof(cmd), "bluetoothctl connect %s", dev->mac);
    }
    g_spawn_command_line_async(cmd, NULL);
    dev->connected = !dev->connected;
    if (g_app.drawing_area) {
        gtk_widget_queue_draw(g_app.drawing_area);
    }
}

// Launch Pair Wizard in Terminal
static void trigger_pair_wizard(void) {
    g_spawn_command_line_async("kitty --class=solaris-bt --title='🛸 SOLARIS BLUETOOTH MATRIX' -e bluetoothctl &", NULL);
}

// Install / Initialize BlueZ packages
static void trigger_install_bluez(void) {
    g_spawn_command_line_async("kitty --class=solaris-bt-setup --title='🛸 SOLARIS BLUEZ INSTALLER' -e bash -c 'echo -e \"\\033[1;36m========================================================\\033[0m\\n🛸 SOLARIS // INITIALIZING BLUETOOTH STACK (BLUEZ)\\n========================================================\\033[0m\"; echo \"Installing bluez and bluez-utils packages...\"; sudo pacman -S --needed bluez bluez-utils && sudo systemctl enable --now bluetooth && echo -e \"\\n\\033[1;32m[Solaris] Bluetooth service is now ACTIVE!\\033[0m\" || echo \"[!] Installation cancelled or failed.\"; echo \"Press Enter to finish.\"; read -r' &", NULL);
}

// ==============================================================================
// DRAW EVENT
// ==============================================================================

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    (void)user_data;

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);

    double pad_x = 8.0;
    double cur_y = 8.0;
    double panel_w = (double)width - 16.0;
    double panel_h = (double)height - 16.0;

    // Outer Cyber Deck Panel Frame
    cairo_draw_beveled_panel(cr, pad_x, cur_y, panel_w, panel_h, 8.0);
    cairo_draw_module_header(cr, pad_x, cur_y, panel_w, "BLUETOOTH // QUANTUM RESONANCE MATRIX", COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // -------------------------------------------------------------
    // LEFT: 3D QUANTUM GYROSCOPE RADAR & HARDWARE CONTROLLER
    // -------------------------------------------------------------
    double gyro_w = 180.0;
    double gyro_cx = pad_x + 90.0;
    double gyro_cy = cur_y + 74.0;
    double gyro_r = 44.0;

    cairo_draw_quantum_bt_gyroscope(cr, gyro_cx, gyro_cy, gyro_r, g_app.time_sec);

    // Hardware Controller Status
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 8.0);
    if (!g_app.hw_present) {
        cairo_set_source_rgba(cr, COLOR_ALERT_R, 0.4, 0.4, 0.95);
        cairo_move_to(cr, pad_x + 18.0, cur_y + 132.0);
        cairo_show_text(cr, "HCI0: NOT DETECTED");
    } else if (g_app.rf_blocked) {
        cairo_set_source_rgba(cr, 1.0, 0.65, 0.1, 0.95);
        cairo_move_to(cr, pad_x + 18.0, cur_y + 132.0);
        cairo_show_text(cr, "HCI0: RADIO MUTED (RFKILL)");
    } else {
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
        cairo_move_to(cr, pad_x + 18.0, cur_y + 132.0);
        cairo_show_text(cr, "HCI0: ONLINE (100% RF)");
    }

    cairo_set_font_size(cr, 6.8);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.85);
    cairo_move_to(cr, pad_x + 18.0, cur_y + 144.0);
    cairo_show_text(cr, g_app.controller_mac);

    // Interactive RF Radio Power Switch Box
    double pwr_bx = pad_x + 10.0;
    double pwr_by = cur_y + 154.0;
    double pwr_bw = gyro_w - 20.0;
    double pwr_bh = 32.0;
    gboolean hov_pwr = (g_app.hovered_idx == -3);

    cairo_rectangle(cr, pwr_bx, pwr_by, pwr_bw, pwr_bh);
    if (g_app.bt_enabled) {
        cairo_set_source_rgba(cr, 0.02, 0.08, 0.12, hov_pwr ? 0.95 : 0.70);
    } else {
        cairo_set_source_rgba(cr, 0.12, 0.03, 0.03, hov_pwr ? 0.95 : 0.70);
    }
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, g_app.bt_enabled ? COLOR_CYAN_R : COLOR_ALERT_R,
                              g_app.bt_enabled ? COLOR_CYAN_G : 0.3,
                              g_app.bt_enabled ? COLOR_CYAN_B : 0.3, hov_pwr ? 1.0 : 0.5);
    cairo_set_line_width(cr, 0.9);
    cairo_stroke(cr);

    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 7.5);
    cairo_set_source_rgba(cr, g_app.bt_enabled ? COLOR_GREEN_R : COLOR_ALERT_R,
                              g_app.bt_enabled ? COLOR_GREEN_G : 0.4,
                              g_app.bt_enabled ? COLOR_GREEN_B : 0.4, 0.98);
    cairo_move_to(cr, pwr_bx + 8.0, pwr_by + 14.0);
    cairo_show_text(cr, g_app.bt_enabled ? "● RADIO: BROADCASTING" : "○ RADIO: MUTED / OFF");

    cairo_set_font_size(cr, 6.2);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.2, COLOR_TEXT_MUTED_G + 0.2, COLOR_TEXT_MUTED_B + 0.2, 0.85);
    cairo_move_to(cr, pwr_bx + 8.0, pwr_by + 25.0);
    cairo_show_text(cr, "[ CLICK TO TOGGLE RF ]");

    // Connection Summary Box
    double sum_by = pwr_by + pwr_bh + 6.0;
    cairo_rectangle(cr, pwr_bx, sum_by, pwr_bw, 32.0);
    cairo_set_source_rgba(cr, 0.02, 0.04, 0.07, 0.80);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.35);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);

    char stat_buf[64];
    snprintf(stat_buf, sizeof(stat_buf), "PAIRED: %d | ACTIVE: %d", g_app.dev_count, g_app.connected_count);
    cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 7.0);
    cairo_set_source_rgba(cr, COLOR_TEXT_BRIGHT_R, COLOR_TEXT_BRIGHT_G, COLOR_TEXT_BRIGHT_B, 0.95);
    cairo_move_to(cr, pwr_bx + 8.0, sum_by + 13.0);
    cairo_show_text(cr, stat_buf);

    cairo_set_font_size(cr, 6.2);
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.85);
    cairo_move_to(cr, pwr_bx + 8.0, sum_by + 24.0);
    cairo_show_text(cr, g_app.has_bluez ? "BLUEZ DAEMON: NOMINAL" : "BLUEZ: STACK INACTIVE");

    // -------------------------------------------------------------
    // RIGHT: REAL PERIPHERAL MATRIX & HARDWARE ACTIONS
    // -------------------------------------------------------------
    double right_x = pad_x + gyro_w + 10.0;
    double right_w = panel_w - gyro_w - 18.0;

    cairo_draw_beveled_panel(cr, right_x, cur_y + 26.0, right_w, panel_h - 34.0, 6.0);
    cairo_draw_module_header(cr, right_x, cur_y + 26.0, right_w, "PERIPHERAL MATRIX // LIVE DEVICES", COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // Integrated Action Buttons in Header
    // 1. [ + PAIR ] Button
    double p_btn_w = 48.0;
    double p_btn_h = 16.0;
    double p_btn_x = right_x + right_w - p_btn_w - 68.0;
    double p_btn_y = cur_y + 30.0;
    gboolean is_p_hover = (g_app.hovered_idx == -4);

    cairo_rectangle(cr, p_btn_x, p_btn_y, p_btn_w, p_btn_h);
    cairo_set_source_rgba(cr, 0.05, 0.10, 0.16, is_p_hover ? 0.95 : 0.60);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, is_p_hover ? 0.95 : 0.40);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);
    cairo_set_font_size(cr, 6.8);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
    cairo_move_to(cr, p_btn_x + 6.0, p_btn_y + 11.0);
    cairo_show_text(cr, "󰐕 WIZARD");

    // 2. [ ⟳ RESCAN ] Button
    double btn_w = 58.0;
    double btn_h = 16.0;
    double btn_x = right_x + right_w - btn_w - 6.0;
    double btn_y = cur_y + 30.0;
    gboolean is_btn_hover = (g_app.hovered_idx == -2);

    cairo_rectangle(cr, btn_x, btn_y, btn_w, btn_h);
    cairo_set_source_rgba(cr, 0.05, 0.10, 0.16, (is_btn_hover || g_app.is_rescanning) ? 0.95 : 0.60);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, (is_btn_hover || g_app.is_rescanning) ? 0.95 : 0.40);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);

    cairo_set_font_size(cr, 6.8);
    cairo_set_source_rgba(cr, (is_btn_hover || g_app.is_rescanning) ? 1.0 : COLOR_CYAN_R,
                              (is_btn_hover || g_app.is_rescanning) ? 1.0 : COLOR_CYAN_G,
                              (is_btn_hover || g_app.is_rescanning) ? 1.0 : COLOR_CYAN_B, 0.95);
    cairo_move_to(cr, btn_x + 6.0, btn_y + 11.0);
    cairo_show_text(cr, g_app.is_rescanning ? "SCANNING.." : "⟳ RESCAN");

    // Check if BlueZ is missing
    if (!g_app.has_bluez) {
        double info_bx = right_x + 10.0;
        double info_by = cur_y + 60.0;
        double info_bw = right_w - 20.0;
        double info_bh = 140.0;

        cairo_rectangle(cr, info_bx, info_by, info_bw, info_bh);
        cairo_set_source_rgba(cr, 0.02, 0.04, 0.07, 0.90);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, COLOR_ALERT_R, 0.4, 0.4, 0.7);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);

        cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 9.0);
        cairo_set_source_rgba(cr, 1.0, 0.7, 0.2, 1.0);
        cairo_move_to(cr, info_bx + 14.0, info_by + 26.0);
        cairo_show_text(cr, "⚠ BLUEZ UTILITIES NOT DETECTED");

        cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 7.2);
        cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.2, COLOR_TEXT_MUTED_G + 0.2, COLOR_TEXT_MUTED_B + 0.2, 0.95);
        cairo_move_to(cr, info_bx + 14.0, info_by + 46.0);
        cairo_show_text(cr, "Hardware controller is ready, but Linux 'bluez'");
        cairo_move_to(cr, info_bx + 14.0, info_by + 58.0);
        cairo_show_text(cr, "and 'bluez-utils' packages are required for pairing.");

        // Interactive Setup Button
        double inst_x = info_bx + 14.0;
        double inst_y = info_by + 76.0;
        double inst_w = info_bw - 28.0;
        double inst_h = 36.0;
        gboolean hov_inst = (g_app.hovered_idx == -5);

        cairo_rectangle(cr, inst_x, inst_y, inst_w, inst_h);
        cairo_set_source_rgba(cr, 0.05, 0.18, 0.28, hov_inst ? 0.98 : 0.75);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, hov_inst ? 1.0 : 0.6);
        cairo_set_line_width(cr, 1.2);
        cairo_stroke(cr);

        cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 8.5);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
        cairo_move_to(cr, inst_x + 14.0, inst_y + 16.0);
        cairo_show_text(cr, "🚀 [ INITIALIZE BLUETOOTH STACK ]");

        cairo_set_font_size(cr, 6.8);
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
        cairo_move_to(cr, inst_x + 14.0, inst_y + 28.0);
        cairo_show_text(cr, "Installs bluez, bluez-utils & starts systemd service");
    } else if (g_app.dev_count == 0) {
        // If No Paired Devices Found
        double info_bx = right_x + 10.0;
        double info_by = cur_y + 60.0;
        double info_bw = right_w - 20.0;
        double info_bh = 120.0;

        cairo_rectangle(cr, info_bx, info_by, info_bw, info_bh);
        cairo_set_source_rgba(cr, 0.02, 0.04, 0.07, 0.80);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.35);
        cairo_set_line_width(cr, 0.8);
        cairo_stroke(cr);

        cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 8.5);
        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.95);
        cairo_move_to(cr, info_bx + 14.0, info_by + 30.0);
        cairo_show_text(cr, "NO PAIRED BLUETOOTH BEACONS");

        cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 7.0);
        cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R + 0.2, COLOR_TEXT_MUTED_G + 0.2, COLOR_TEXT_MUTED_B + 0.2, 0.85);
        cairo_move_to(cr, info_bx + 14.0, info_by + 52.0);
        cairo_show_text(cr, "No hardware devices currently in paired register.");
        cairo_move_to(cr, info_bx + 14.0, info_by + 66.0);
        cairo_show_text(cr, "Click [ ⟳ RESCAN ] to discover nearby signals, or");
        cairo_move_to(cr, info_bx + 14.0, info_by + 80.0);
        cairo_show_text(cr, "click [ 󰐕 WIZARD ] to pair new headphones/mouse.");
    } else {
        // Render Real Device List
        int visible_rows = 4;
        double row_start_y = cur_y + 54.0;
        double row_h = 42.0;
        double row_gap = 6.0;
        double row_w = (g_app.dev_count > visible_rows) ? right_w - 18.0 : right_w - 12.0;

        int max_scroll = g_app.dev_count - visible_rows;
        if (max_scroll < 0) max_scroll = 0;
        if (g_app.scroll_offset > max_scroll) g_app.scroll_offset = max_scroll;
        if (g_app.scroll_offset < 0) g_app.scroll_offset = 0;

    for (int slot = 0; slot < visible_rows; slot++) {
        int i = g_app.scroll_offset + slot;
        if (i >= g_app.dev_count) break;

        const bt_dev_entry_t *dev = &g_app.devs[i];
        double ry = row_start_y + (double)slot * (row_h + row_gap);
        gboolean is_hover = (g_app.hovered_idx == i);

        // Row background box
        if (dev->connected) {
            cairo_set_source_rgba(cr, COLOR_GREEN_R * 0.2, COLOR_GREEN_G * 0.2, COLOR_GREEN_B * 0.2, 0.85);
            cairo_rectangle(cr, right_x + 6.0, ry, row_w, row_h);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 0.85);
            cairo_set_line_width(cr, 1.0);
            cairo_stroke(cr);
        } else if (is_hover) {
            cairo_set_source_rgba(cr, COLOR_CYAN_R * 0.2, COLOR_CYAN_G * 0.2, COLOR_CYAN_B * 0.2, 0.85);
            cairo_rectangle(cr, right_x + 6.0, ry, row_w, row_h);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.90);
            cairo_set_line_width(cr, 1.0);
            cairo_stroke(cr);
        } else {
            cairo_set_source_rgba(cr, 0.02, 0.05, 0.08, 0.70);
            cairo_rectangle(cr, right_x + 6.0, ry, row_w, row_h);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.30);
            cairo_set_line_width(cr, 0.8);
            cairo_stroke(cr);
        }

        // Icon Glyph
        cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 12.0);
        cairo_set_source_rgba(cr, dev->connected ? COLOR_GREEN_R : COLOR_CYAN_R,
                                  dev->connected ? COLOR_GREEN_G : COLOR_CYAN_G,
                                  dev->connected ? COLOR_GREEN_B : COLOR_CYAN_B, 1.0);
        cairo_move_to(cr, right_x + 12.0, ry + 24.0);
        cairo_show_text(cr, dev->icon);

        // Name
        char disp_name[32];
        snprintf(disp_name, sizeof(disp_name), "%s", dev->name);
        if (strlen(disp_name) > 16) {
            disp_name[13] = '.';
            disp_name[14] = '.';
            disp_name[15] = '.';
            disp_name[16] = '\0';
        }

        cairo_set_font_size(cr, 8.0);
        cairo_set_source_rgba(cr, dev->connected ? COLOR_TEXT_BRIGHT_R : (is_hover ? COLOR_TEXT_BRIGHT_R : COLOR_TEXT_MUTED_R + 0.2),
                                  dev->connected ? COLOR_TEXT_BRIGHT_G : (is_hover ? COLOR_TEXT_BRIGHT_G : COLOR_TEXT_MUTED_G + 0.2),
                                  dev->connected ? COLOR_TEXT_BRIGHT_B : (is_hover ? COLOR_TEXT_BRIGHT_B : COLOR_TEXT_MUTED_B + 0.2), 0.98);
        cairo_move_to(cr, right_x + 30.0, ry + 15.0);
        cairo_show_text(cr, disp_name);

        // Subline: MAC / Category
        char sub[64];
        snprintf(sub, sizeof(sub), "%s // %s", dev->mac, dev->type);
        cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 6.2);
        cairo_set_source_rgba(cr, COLOR_TEXT_MUTED_R, COLOR_TEXT_MUTED_G, COLOR_TEXT_MUTED_B, 0.85);
        cairo_move_to(cr, right_x + 30.0, ry + 29.0);
        cairo_show_text(cr, sub);

        // Interactive Action Pill Badge
        double act_w = 68.0;
        double act_h = 16.0;
        double act_x = right_x + row_w - act_w - 6.0;
        double act_y = ry + 13.0;

        cairo_rectangle(cr, act_x, act_y, act_w, act_h);
        if (dev->connected) {
            cairo_set_source_rgba(cr, 0.04, 0.18, 0.08, 0.90);
        } else {
            cairo_set_source_rgba(cr, 0.03, 0.08, 0.14, 0.80);
        }
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, dev->connected ? COLOR_GREEN_R : COLOR_CYAN_R,
                                  dev->connected ? COLOR_GREEN_G : COLOR_CYAN_G,
                                  dev->connected ? COLOR_GREEN_B : COLOR_CYAN_B, 0.85);
        cairo_set_line_width(cr, 0.8);
        cairo_stroke(cr);

        cairo_select_font_face(cr, "JetBrainsMono Nerd Font", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 6.8);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.98);
        cairo_move_to(cr, act_x + 6.0, act_y + 11.0);
        if (dev->connected) {
            cairo_show_text(cr, "DISCONNECT");
        } else {
            cairo_show_text(cr, "󰂱 CONNECT");
        }
    }

    // Cyber Scrollbar
    if (g_app.dev_count > visible_rows) {
        double track_x = right_x + right_w - 8.0;
        double track_y = row_start_y;
        double track_h = (double)visible_rows * (row_h + row_gap) - row_gap;

        cairo_set_source_rgba(cr, 0.03, 0.07, 0.12, 0.8);
        cairo_rectangle(cr, track_x, track_y, 4.0, track_h);
        cairo_fill(cr);

        double thumb_h = track_h * ((double)visible_rows / (double)g_app.dev_count);
        if (thumb_h < 20.0) thumb_h = 20.0;
        double thumb_y = track_y + (track_h - thumb_h) * ((double)g_app.scroll_offset / (double)max_scroll);

        cairo_set_source_rgba(cr, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B, 0.90);
        cairo_rectangle(cr, track_x, thumb_y, 4.0, thumb_h);
        cairo_fill(cr);
    }
    }

    // Spacecraft Sequential Power-On Awakening Overlay (Starts at 1.8s)
    solaris_draw_awakening_overlay(cr, pad_x, cur_y, panel_w, panel_h, 1.8, 0.7, "BT RESONANCE");

    return FALSE;
}

// ==============================================================================
// MOUSE INTERACTION & SCROLL
// ==============================================================================

static gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
    (void)user_data;
    int visible_rows = 4;
    int max_scroll = g_app.dev_count - visible_rows;
    if (max_scroll < 0) max_scroll = 0;

    double dx, dy;
    if (gdk_event_get_scroll_deltas((GdkEvent *)event, &dx, &dy)) {
        if (dy < 0 && g_app.scroll_offset > 0) {
            g_app.scroll_offset--;
        } else if (dy > 0 && g_app.scroll_offset < max_scroll) {
            g_app.scroll_offset++;
        }
    } else if (event->direction == GDK_SCROLL_UP) {
        if (g_app.scroll_offset > 0) g_app.scroll_offset--;
    } else if (event->direction == GDK_SCROLL_DOWN) {
        if (g_app.scroll_offset < max_scroll) g_app.scroll_offset++;
    }

    gtk_widget_queue_draw(widget);
    return TRUE;
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    (void)user_data;

    int width = gtk_widget_get_allocated_width(widget);
    double pad_x = 8.0;
    double cur_y = 8.0;
    double panel_w = (double)width - 16.0;
    double gyro_w = 180.0;
    double right_x = pad_x + gyro_w + 10.0;
    double right_w = panel_w - gyro_w - 18.0;

    // 1. Radio Power Toggle Box hit test
    double pwr_bx = pad_x + 10.0;
    double pwr_by = cur_y + 154.0;
    double pwr_bw = gyro_w - 20.0;
    double pwr_bh = 32.0;
    if (event->x >= pwr_bx && event->x <= pwr_bx + pwr_bw && event->y >= pwr_by && event->y <= pwr_by + pwr_bh) {
        trigger_rf_toggle();
        return TRUE;
    }

    // 2. Rescan button hit test
    double btn_w = 58.0;
    double btn_h = 16.0;
    double btn_x = right_x + right_w - btn_w - 6.0;
    double btn_y = cur_y + 30.0;
    if (event->x >= btn_x && event->x <= btn_x + btn_w && event->y >= btn_y && event->y <= btn_y + btn_h) {
        trigger_bt_rescan();
        return TRUE;
    }

    // 3. Pair Wizard button hit test
    double p_btn_w = 48.0;
    double p_btn_h = 16.0;
    double p_btn_x = right_x + right_w - p_btn_w - 68.0;
    double p_btn_y = cur_y + 30.0;
    if (event->x >= p_btn_x && event->x <= p_btn_x + p_btn_w && event->y >= p_btn_y && event->y <= p_btn_y + p_btn_h) {
        trigger_pair_wizard();
        return TRUE;
    }

    // 4. Install BlueZ hit test (if bluez missing)
    if (!g_app.has_bluez) {
        double info_bx = right_x + 10.0;
        double info_by = cur_y + 60.0;
        double info_bw = right_w - 20.0;
        double inst_x = info_bx + 14.0;
        double inst_y = info_by + 76.0;
        double inst_w = info_bw - 28.0;
        double inst_h = 36.0;
        if (event->x >= inst_x && event->x <= inst_x + inst_w && event->y >= inst_y && event->y <= inst_y + inst_h) {
            trigger_install_bluez();
            return TRUE;
        }
        return FALSE;
    }

    // 5. Device row hit test (Connect / Disconnect)
    int visible_rows = 4;
    double row_start_y = cur_y + 54.0;
    double row_h = 42.0;
    double row_gap = 6.0;

    if (event->x >= right_x && event->x <= right_x + right_w) {
        for (int slot = 0; slot < visible_rows; slot++) {
            int i = g_app.scroll_offset + slot;
            if (i >= g_app.dev_count) break;

            double ry = row_start_y + (double)slot * (row_h + row_gap);
            if (event->y >= ry && event->y <= ry + row_h) {
                trigger_device_toggle(i);
                return TRUE;
            }
        }
    }
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    (void)user_data;
    int width = gtk_widget_get_allocated_width(widget);
    double pad_x = 8.0;
    double cur_y = 8.0;
    double panel_w = (double)width - 16.0;
    double gyro_w = 180.0;
    double right_x = pad_x + gyro_w + 10.0;
    double right_w = panel_w - gyro_w - 18.0;

    int old_hover = g_app.hovered_idx;
    g_app.hovered_idx = -1;

    // Power toggle hover
    double pwr_bx = pad_x + 10.0;
    double pwr_by = cur_y + 154.0;
    double pwr_bw = gyro_w - 20.0;
    double pwr_bh = 32.0;
    if (event->x >= pwr_bx && event->x <= pwr_bx + pwr_bw && event->y >= pwr_by && event->y <= pwr_by + pwr_bh) {
        g_app.hovered_idx = -3;
    }

    // Rescan button hover
    double btn_w = 58.0;
    double btn_h = 16.0;
    double btn_x = right_x + right_w - btn_w - 6.0;
    double btn_y = cur_y + 30.0;
    if (event->x >= btn_x && event->x <= btn_x + btn_w && event->y >= btn_y && event->y <= btn_y + btn_h) {
        g_app.hovered_idx = -2;
    }

    // Pair wizard hover
    double p_btn_w = 48.0;
    double p_btn_h = 16.0;
    double p_btn_x = right_x + right_w - p_btn_w - 68.0;
    double p_btn_y = cur_y + 30.0;
    if (event->x >= p_btn_x && event->x <= p_btn_x + p_btn_w && event->y >= p_btn_y && event->y <= p_btn_y + p_btn_h) {
        g_app.hovered_idx = -4;
    }

    // Install BlueZ hover
    if (!g_app.has_bluez) {
        double info_bx = right_x + 10.0;
        double info_by = cur_y + 60.0;
        double info_bw = right_w - 20.0;
        double inst_x = info_bx + 14.0;
        double inst_y = info_by + 76.0;
        double inst_w = info_bw - 28.0;
        double inst_h = 36.0;
        if (event->x >= inst_x && event->x <= inst_x + inst_w && event->y >= inst_y && event->y <= inst_y + inst_h) {
            g_app.hovered_idx = -5;
        }
    } else if (event->x >= right_x && event->x <= right_x + right_w) {
        int visible_rows = 4;
        double row_start_y = cur_y + 54.0;
        double row_h = 42.0;
        double row_gap = 6.0;

        for (int slot = 0; slot < visible_rows; slot++) {
            int i = g_app.scroll_offset + slot;
            if (i >= g_app.dev_count) break;

            double ry = row_start_y + (double)slot * (row_h + row_gap);
            if (event->y >= ry && event->y <= ry + row_h) {
                g_app.hovered_idx = i;
                break;
            }
        }
    }

    if (old_hover != g_app.hovered_idx) {
        gtk_widget_queue_draw(widget);
    }
    return FALSE;
}

static gboolean on_animation_tick(gpointer user_data) {
    (void)user_data;
    g_app.time_sec += 0.040;
    if (g_app.is_rescanning) {
        g_app.rescan_anim_timer -= 0.040;
        if (g_app.rescan_anim_timer <= 0.0) {
            g_app.is_rescanning = FALSE;
            query_real_bt_state();
        }
    }
    if (g_app.drawing_area) {
        gtk_widget_queue_draw(g_app.drawing_area);
    }
    return G_SOURCE_CONTINUE;
}

static gboolean on_telem_tick(gpointer user_data) {
    (void)user_data;
    query_real_bt_state();
    solaris_trim_memory();
    if (g_app.drawing_area) {
        gtk_widget_queue_draw(g_app.drawing_area);
    }
    return G_SOURCE_CONTINUE;
}

// ==============================================================================
// ENTRY POINT
// ==============================================================================

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    solaris_load_css();

    memset(&g_app, 0, sizeof(g_app));
    g_app.hovered_idx = -1;
    g_app.scroll_offset = 0;

    query_real_bt_state();

    g_app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    solaris_setup_layer_window(g_app.window, "solaris-bg-bluetooth",
                                490, 280,
                                TRUE, FALSE, FALSE, TRUE, // Top-Right anchor
                                44, 0, 0, 16);

    g_app.drawing_area = gtk_drawing_area_new();
    gtk_widget_add_events(g_app.drawing_area,
                          GDK_BUTTON_PRESS_MASK |
                          GDK_POINTER_MOTION_MASK |
                          GDK_SCROLL_MASK |
                          GDK_SMOOTH_SCROLL_MASK |
                          GDK_LEAVE_NOTIFY_MASK);

    g_signal_connect(g_app.drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(g_app.drawing_area, "button-press-event", G_CALLBACK(on_button_press), NULL);
    g_signal_connect(g_app.drawing_area, "motion-notify-event", G_CALLBACK(on_motion_notify), NULL);
    g_signal_connect(g_app.drawing_area, "scroll-event", G_CALLBACK(on_scroll_event), NULL);
    gtk_container_add(GTK_CONTAINER(g_app.window), g_app.drawing_area);

    g_app.anim_timer_id = g_timeout_add(40, on_animation_tick, NULL);
    g_app.telem_timer_id = g_timeout_add(2500, on_telem_tick, NULL);

    gtk_widget_show_all(g_app.window);
    gtk_main();
    return 0;
}
