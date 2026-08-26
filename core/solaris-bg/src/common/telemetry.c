#include "telemetry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <math.h>
#include <glib.h>

// Thread-safe cached GPU data
static gpu_telemetry_t g_cached_gpu;
static GMutex g_gpu_mutex;
static gboolean g_gpu_thread_started = FALSE;

// Thread-safe cached Wi-Fi scan targets
static wifi_scan_telemetry_t g_cached_wifi_scan;
static GMutex g_wifi_mutex;
static gboolean g_wifi_thread_started = FALSE;

static double get_real_cpu_temp(void) {
    // 1. Try hwmon coretemp / alienware_wmi
    const char *hwmon_paths[] = {
        "/sys/class/hwmon/hwmon5/temp1_input", // alienware_wmi CPU
        "/sys/class/hwmon/hwmon8/temp1_input", // coretemp Package id 0
        "/sys/class/hwmon/hwmon3/temp1_input", // dell_ddv CPU
        "/sys/class/hwmon/hwmon4/temp1_input", // dell_smm CPU
        NULL
    };
    for (int i = 0; hwmon_paths[i]; i++) {
        FILE *fp = fopen(hwmon_paths[i], "r");
        if (fp) {
            long t = 0;
            if (fscanf(fp, "%ld", &t) == 1 && t > 10000 && t < 120000) {
                fclose(fp);
                return (double)t / 1000.0;
            }
            fclose(fp);
        }
    }

    // 2. Try thermal zones
    for (int z = 1; z <= 12; z++) {
        char ttype_path[64], tval_path[64];
        snprintf(ttype_path, sizeof(ttype_path), "/sys/class/thermal/thermal_zone%d/type", z);
        snprintf(tval_path, sizeof(tval_path), "/sys/class/thermal/thermal_zone%d/temp", z);
        FILE *tfp = fopen(ttype_path, "r");
        if (tfp) {
            char type_str[64];
            if (fgets(type_str, sizeof(type_str), tfp)) {
                if (strstr(type_str, "pkg_temp") || strstr(type_str, "TCPU") || strstr(type_str, "SEN1")) {
                    fclose(tfp);
                    FILE *vfp = fopen(tval_path, "r");
                    if (vfp) {
                        long t = 0;
                        if (fscanf(vfp, "%ld", &t) == 1 && t > 10000 && t < 120000) {
                            fclose(vfp);
                            return (double)t / 1000.0;
                        }
                        fclose(vfp);
                    }
                    continue;
                }
            }
            fclose(tfp);
        }
    }
    return 64.0;
}

static int get_real_fan_rpm(void) {
    const char *fan_paths[] = {
        "/sys/class/hwmon/hwmon5/fan1_input",
        "/sys/class/hwmon/hwmon3/fan1_input",
        "/sys/class/hwmon/hwmon4/fan1_input",
        NULL
    };
    for (int i = 0; fan_paths[i]; i++) {
        FILE *fp = fopen(fan_paths[i], "r");
        if (fp) {
            int rpm = 0;
            if (fscanf(fp, "%d", &rpm) == 1 && rpm > 0) {
                fclose(fp);
                return rpm;
            }
            fclose(fp);
        }
    }
    return 3750;
}

static gpointer gpu_worker_func(gpointer user_data) {
    while (TRUE) {
        FILE *fp = popen("nvidia-smi --query-gpu=utilization.gpu,temperature.gpu,memory.used,memory.total,name --format=csv,noheader,nounits 2>/dev/null", "r");
        if (fp) {
            char line[256];
            if (fgets(line, sizeof(line), fp)) {
                char *p1 = strtok(line, ",");
                char *p2 = strtok(NULL, ",");
                char *p3 = strtok(NULL, ",");
                char *p4 = strtok(NULL, ",");
                char *p5 = strtok(NULL, "\n");

                g_mutex_lock(&g_gpu_mutex);
                if (p1) g_cached_gpu.usage_pct = atof(p1);
                if (p2) g_cached_gpu.temp_c = atof(p2);
                if (p3) g_cached_gpu.vram_used_gb = atof(p3) / 1024.0;
                if (p4) g_cached_gpu.vram_total_gb = atof(p4) / 1024.0;
                if (g_cached_gpu.vram_total_gb > 0) {
                    g_cached_gpu.vram_pct = (g_cached_gpu.vram_used_gb / g_cached_gpu.vram_total_gb) * 100.0;
                }
                if (p5) {
                    while (*p5 == ' ') p5++;
                    snprintf(g_cached_gpu.model_name, sizeof(g_cached_gpu.model_name), "%s", p5);
                }
                g_mutex_unlock(&g_gpu_mutex);
            }
            pclose(fp);
        } else {
            // Fallback hwmon GPU sensor
            FILE *gfp = fopen("/sys/class/hwmon/hwmon5/temp2_input", "r");
            if (gfp) {
                long gt = 0;
                if (fscanf(gfp, "%ld", &gt) == 1) {
                    g_mutex_lock(&g_gpu_mutex);
                    g_cached_gpu.temp_c = (double)gt / 1000.0;
                    g_mutex_unlock(&g_gpu_mutex);
                }
                fclose(gfp);
            }
        }
        g_usleep(2000000); // 2-second background polling
    }
    return NULL;
}

static unsigned int hash_str(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static gpointer wifi_scan_worker_func(gpointer user_data) {
    while (TRUE) {
        // Query nmcli tabular list without active scanning freeze
        FILE *fp = popen("nmcli -t -f IN-USE,SSID,SIGNAL,SECURITY,CHAN,FREQ dev wifi list --rescan no 2>/dev/null", "r");
        if (fp) {
            char line[512];
            wifi_scan_telemetry_t scan_temp;
            memset(&scan_temp, 0, sizeof(scan_temp));
            int count = 0;

            while (fgets(line, sizeof(line), fp) && count < MAX_WIFI_TARGETS) {
                char *nl = strchr(line, '\n');
                if (nl) *nl = '\0';
                if (line[0] == '\0') continue;

                gboolean is_active = (line[0] == '*');

                // Split by colon
                gchar **tokens = g_strsplit(line, ":", -1);
                int num_tokens = g_strv_length(tokens);

                if (num_tokens >= 4) {
                    char ssid[64] = {0};
                    snprintf(ssid, sizeof(ssid), "%s", tokens[1]);

                    if (ssid[0] != '\0' && strcmp(ssid, "--") != 0) {
                        int signal = atoi(tokens[2]);
                        if (signal <= 0) signal = 50;

                        char security[32] = "WPA2";
                        if (num_tokens >= 4 && strlen(tokens[3]) > 0) {
                            snprintf(security, sizeof(security), "%s", tokens[3]);
                        }

                        char band[16] = "2.4 GHz";
                        if (num_tokens >= 6) {
                            int freq = atoi(tokens[5]);
                            if (freq > 4000 || strstr(tokens[5], "51") || strstr(tokens[5], "52") || strstr(tokens[5], "57") || strstr(tokens[5], "58")) {
                                snprintf(band, sizeof(band), "5 GHz");
                            }
                        }

                        // Check duplicate SSID: update signal if duplicate
                        int existing_idx = -1;
                        for (int k = 0; k < count; k++) {
                            if (strcmp(scan_temp.targets[k].ssid, ssid) == 0) {
                                existing_idx = k;
                                break;
                            }
                        }

                        if (existing_idx >= 0) {
                            if (is_active) scan_temp.targets[existing_idx].is_active = TRUE;
                            if (signal > scan_temp.targets[existing_idx].signal_pct) {
                                scan_temp.targets[existing_idx].signal_pct = signal;
                                scan_temp.targets[existing_idx].signal_dbm = -100 + (signal / 2);
                                snprintf(scan_temp.targets[existing_idx].band, sizeof(scan_temp.targets[existing_idx].band), "%s", band);
                            }
                        } else {
                            wifi_target_t *tgt = &scan_temp.targets[count];
                            snprintf(tgt->ssid, sizeof(tgt->ssid), "%s", ssid);
                            snprintf(tgt->band, sizeof(tgt->band), "%s", band);
                            snprintf(tgt->security, sizeof(tgt->security), "%s", security);
                            tgt->signal_pct = signal;
                            tgt->signal_dbm = -100 + (signal / 2);
                            tgt->is_active = is_active;

                            if (is_active) {
                                snprintf(scan_temp.active_ssid, sizeof(scan_temp.active_ssid), "%s", ssid);
                                scan_temp.active_signal = signal;
                            }

                            // Distance mapped to signal RSSI
                            tgt->radar_dist = 0.90 - ((double)signal / 100.0) * 0.65;
                            if (tgt->radar_dist < 0.22) tgt->radar_dist = 0.22;
                            if (tgt->radar_dist > 0.88) tgt->radar_dist = 0.88;

                            unsigned int h = hash_str(ssid);
                            tgt->radar_angle = (double)(h % 360) * (M_PI / 180.0);

                            count++;
                        }
                    }
                }
                g_strfreev(tokens);
            }
            pclose(fp);

            scan_temp.target_count = count;

            g_mutex_lock(&g_wifi_mutex);
            memcpy(&g_cached_wifi_scan, &scan_temp, sizeof(wifi_scan_telemetry_t));
            g_mutex_unlock(&g_wifi_mutex);
        }
        g_usleep(3000000); // 3-second background polling
    }
    return NULL;
}

void init_telemetry(void) {
    if (!g_gpu_thread_started) {
        g_mutex_init(&g_gpu_mutex);
        memset(&g_cached_gpu, 0, sizeof(g_cached_gpu));
        g_cached_gpu.usage_pct = 0.0;
        g_cached_gpu.temp_c = 58.0;
        g_cached_gpu.vram_used_gb = 1.4;
        g_cached_gpu.vram_total_gb = 8.0;
        g_cached_gpu.vram_pct = 17.5;
        snprintf(g_cached_gpu.model_name, sizeof(g_cached_gpu.model_name), "NVIDIA RTX 5050");

        g_thread_new("gpu-telemetry-worker", gpu_worker_func, NULL);
        g_gpu_thread_started = TRUE;
    }

    if (!g_wifi_thread_started) {
        g_mutex_init(&g_wifi_mutex);
        memset(&g_cached_wifi_scan, 0, sizeof(g_cached_wifi_scan));
        g_thread_new("wifi-scan-worker", wifi_scan_worker_func, NULL);
        g_wifi_thread_started = TRUE;
    }
}

void update_wifi_scan_telemetry(wifi_scan_telemetry_t *scan) {
    if (!g_wifi_thread_started) {
        init_telemetry();
    }

    g_mutex_lock(&g_wifi_mutex);
    memcpy(scan, &g_cached_wifi_scan, sizeof(wifi_scan_telemetry_t));
    g_mutex_unlock(&g_wifi_mutex);

    // Fallback if initial scan is populating
    if (scan->target_count == 0) {
        scan->target_count = 4;
        snprintf(scan->targets[0].ssid, sizeof(scan->targets[0].ssid), "VIJAYANAND5G");
        snprintf(scan->targets[0].band, sizeof(scan->targets[0].band), "5 GHz");
        snprintf(scan->targets[0].security, sizeof(scan->targets[0].security), "WPA2");
        scan->targets[0].signal_pct = 100;
        scan->targets[0].signal_dbm = -35;
        scan->targets[0].is_active = TRUE;
        scan->targets[0].radar_dist = 0.25;
        scan->targets[0].radar_angle = 0.85;

        snprintf(scan->targets[1].ssid, sizeof(scan->targets[1].ssid), "P.G.VIJAY ANAND");
        snprintf(scan->targets[1].band, sizeof(scan->targets[1].band), "2.4 GHz");
        snprintf(scan->targets[1].security, sizeof(scan->targets[1].security), "WPA2");
        scan->targets[1].signal_pct = 80;
        scan->targets[1].signal_dbm = -55;
        scan->targets[1].is_active = FALSE;
        scan->targets[1].radar_dist = 0.45;
        scan->targets[1].radar_angle = 2.40;

        snprintf(scan->targets[2].ssid, sizeof(scan->targets[2].ssid), "ACT-ai_103742");
        snprintf(scan->targets[2].band, sizeof(scan->targets[2].band), "2.4 GHz");
        snprintf(scan->targets[2].security, sizeof(scan->targets[2].security), "WPA2");
        scan->targets[2].signal_pct = 47;
        scan->targets[2].signal_dbm = -72;
        scan->targets[2].is_active = FALSE;
        scan->targets[2].radar_dist = 0.65;
        scan->targets[2].radar_angle = 4.60;

        snprintf(scan->targets[3].ssid, sizeof(scan->targets[3].ssid), "Dr.N.Rathika");
        snprintf(scan->targets[3].band, sizeof(scan->targets[3].band), "2.4 GHz");
        snprintf(scan->targets[3].security, sizeof(scan->targets[3].security), "WPA2");
        scan->targets[3].signal_pct = 44;
        scan->targets[3].signal_dbm = -75;
        scan->targets[3].is_active = FALSE;
        scan->targets[3].radar_dist = 0.72;
        scan->targets[3].radar_angle = 3.30;

        snprintf(scan->active_ssid, sizeof(scan->active_ssid), "VIJAYANAND5G");
        scan->active_signal = 100;
    }
}

void trigger_wifi_connect(const char *ssid) {
    if (!ssid || ssid[0] == '\0') return;

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "kitty --class=solaris-net-connect --title='🛸 SOLARIS COMM MATRIX' -e bash -c 'echo -e \"\\033[1;36m========================================================\\033[0m\\n🛸 SOLARIS RF COMM // CONNECTING TO [ %s ]\\n========================================================\\033[0m\"; nmcli --ask dev wifi connect \"%s\"; echo -e \"\\n\\033[1;32m[Solaris] Connection sequence completed. Press Enter to close.\\033[0m\"; read -r' &",
             ssid, ssid);
    g_spawn_command_line_async(cmd, NULL);
}

void update_cpu_telemetry(cpu_telemetry_t *cpu) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return;

    char line[256];
    if (fgets(line, sizeof(line), fp)) {
        unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
        if (sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
                   &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) >= 4) {
            unsigned long long total = user + nice + system + idle + iowait + irq + softirq + steal;
            unsigned long long active = total - (idle + iowait);

            if (cpu->prev_total > 0 && total > cpu->prev_total) {
                unsigned long long total_d = total - cpu->prev_total;
                unsigned long long active_d = active - (cpu->prev_total - cpu->prev_idle);
                cpu->usage_pct = ((double)active_d / (double)total_d) * 100.0;
                if (cpu->usage_pct < 0.0) cpu->usage_pct = 0.0;
                if (cpu->usage_pct > 100.0) cpu->usage_pct = 100.0;
            } else {
                cpu->usage_pct = 6.0;
            }
            cpu->prev_total = total;
            cpu->prev_idle = idle + iowait;
        }
    }
    fclose(fp);

    // Read CPU Frequency
    double freq_sum = 0.0;
    int freq_count = 0;
    for (int i = 0; i < 32; i++) {
        char freq_path[128];
        snprintf(freq_path, sizeof(freq_path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", i);
        FILE *ffp = fopen(freq_path, "r");
        if (ffp) {
            long freq_khz = 0;
            if (fscanf(ffp, "%ld", &freq_khz) == 1) {
                freq_sum += (double)freq_khz / 1000000.0;
                freq_count++;
            }
            fclose(ffp);
        }
    }
    if (freq_count > 0) {
        cpu->freq_ghz = freq_sum / freq_count;
        cpu->core_count = freq_count;
    } else {
        cpu->freq_ghz = 3.20;
        cpu->core_count = 8;
    }

    // Read CPU Model
    if (cpu->model_name[0] == '\0') {
        FILE *cinfo = fopen("/proc/cpuinfo", "r");
        if (cinfo) {
            while (fgets(line, sizeof(line), cinfo)) {
                if (strncmp(line, "model name", 10) == 0) {
                    char *colon = strchr(line, ':');
                    if (colon) {
                        colon++;
                        while (*colon == ' ' || *colon == '\t') colon++;
                        snprintf(cpu->model_name, sizeof(cpu->model_name), "%s", colon);
                        char *nl = strchr(cpu->model_name, '\n');
                        if (nl) *nl = '\0';
                        break;
                    }
                }
            }
            fclose(cinfo);
        }
        if (cpu->model_name[0] == '\0') {
            snprintf(cpu->model_name, sizeof(cpu->model_name), "Intel Core Ultra / Extraterrestrial Core");
        }
    }

    cpu->plasma_phase += 0.08 + (cpu->usage_pct * 0.003);
    if (cpu->plasma_phase > 2.0 * M_PI) cpu->plasma_phase -= 2.0 * M_PI;

    // Update history
    if (cpu->history_count < MAX_HISTORY_POINTS) {
        cpu->history[cpu->history_count++] = cpu->usage_pct;
    } else {
        memmove(&cpu->history[0], &cpu->history[1], sizeof(double) * (MAX_HISTORY_POINTS - 1));
        cpu->history[MAX_HISTORY_POINTS - 1] = cpu->usage_pct;
    }
}

void update_mem_telemetry(mem_telemetry_t *mem) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return;

    char line[128];
    unsigned long long total_kb = 0, free_kb = 0, avail_kb = 0, buffers_kb = 0, cached_kb = 0;
    unsigned long long swap_total_kb = 0, swap_free_kb = 0;

    while (fgets(line, sizeof(line), fp)) {
        sscanf(line, "MemTotal: %llu kB", &total_kb);
        sscanf(line, "MemFree: %llu kB", &free_kb);
        sscanf(line, "MemAvailable: %llu kB", &avail_kb);
        sscanf(line, "Buffers: %llu kB", &buffers_kb);
        sscanf(line, "Cached: %llu kB", &cached_kb);
        sscanf(line, "SwapTotal: %llu kB", &swap_total_kb);
        sscanf(line, "SwapFree: %llu kB", &swap_free_kb);
    }
    fclose(fp);

    if (total_kb > 0) {
        mem->total_gb = (double)total_kb / (1024.0 * 1024.0);
        unsigned long long used_kb = total_kb - avail_kb;
        mem->used_gb = (double)used_kb / (1024.0 * 1024.0);
        mem->free_gb = (double)avail_kb / (1024.0 * 1024.0);
        mem->usage_pct = ((double)used_kb / (double)total_kb) * 100.0;
    }

    if (swap_total_kb > 0) {
        mem->swap_total_gb = (double)swap_total_kb / (1024.0 * 1024.0);
        unsigned long long swap_used_kb = swap_total_kb - swap_free_kb;
        mem->swap_used_gb = (double)swap_used_kb / (1024.0 * 1024.0);
        mem->swap_pct = ((double)swap_used_kb / (double)swap_total_kb) * 100.0;
    } else {
        mem->swap_total_gb = 0.0;
        mem->swap_used_gb = 0.0;
        mem->swap_pct = 0.0;
    }

    mem->flux_phase += 0.05;
    if (mem->flux_phase > 2.0 * M_PI) mem->flux_phase -= 2.0 * M_PI;

    if (mem->history_count < MAX_HISTORY_POINTS) {
        mem->history[mem->history_count++] = mem->usage_pct;
    } else {
        memmove(&mem->history[0], &mem->history[1], sizeof(double) * (MAX_HISTORY_POINTS - 1));
        mem->history[MAX_HISTORY_POINTS - 1] = mem->usage_pct;
    }
}

void update_gpu_telemetry(gpu_telemetry_t *gpu) {
    if (!g_gpu_thread_started) {
        init_telemetry();
    }

    g_mutex_lock(&g_gpu_mutex);
    gpu->usage_pct = g_cached_gpu.usage_pct;
    gpu->temp_c = g_cached_gpu.temp_c;
    gpu->vram_used_gb = g_cached_gpu.vram_used_gb;
    gpu->vram_total_gb = g_cached_gpu.vram_total_gb;
    gpu->vram_pct = g_cached_gpu.vram_pct;
    snprintf(gpu->model_name, sizeof(gpu->model_name), "%s", g_cached_gpu.model_name);
    g_mutex_unlock(&g_gpu_mutex);

    gpu->tensor_phase += 0.06;
    if (gpu->tensor_phase > 2.0 * M_PI) gpu->tensor_phase -= 2.0 * M_PI;

    if (gpu->history_count < MAX_HISTORY_POINTS) {
        gpu->history[gpu->history_count++] = gpu->usage_pct;
    } else {
        memmove(&gpu->history[0], &gpu->history[1], sizeof(double) * (MAX_HISTORY_POINTS - 1));
        gpu->history[MAX_HISTORY_POINTS - 1] = gpu->usage_pct;
    }
}

void update_disk_telemetry(disk_telemetry_t *disk) {
    struct statvfs sv;
    if (statvfs("/", &sv) == 0) {
        double total = (double)(sv.f_blocks * sv.f_frsize) / (1024.0 * 1024.0 * 1024.0);
        double free_b = (double)(sv.f_bfree * sv.f_frsize) / (1024.0 * 1024.0 * 1024.0);
        disk->root_total_gb = total;
        disk->root_used_gb = total - free_b;
        disk->root_pct = (total > 0) ? (disk->root_used_gb / total) * 100.0 : 0.0;
    }

    if (statvfs("/home", &sv) == 0) {
        double total = (double)(sv.f_blocks * sv.f_frsize) / (1024.0 * 1024.0 * 1024.0);
        double free_b = (double)(sv.f_bfree * sv.f_frsize) / (1024.0 * 1024.0 * 1024.0);
        disk->home_total_gb = total;
        disk->home_used_gb = total - free_b;
        disk->home_pct = (total > 0) ? (disk->home_used_gb / total) * 100.0 : 0.0;
    }
}

void update_net_telemetry(net_telemetry_t *net) {
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) return;

    char line[256];
    unsigned long long rx = 0, tx = 0;
    char ifname[32] = {0};

    if (!fgets(line, sizeof(line), fp)) { fclose(fp); return; }
    if (!fgets(line, sizeof(line), fp)) { fclose(fp); return; }

    while (fgets(line, sizeof(line), fp)) {
        char name[32];
        unsigned long long r_b, r_p, r_e, r_d, r_f, r_fr, r_c, r_m;
        unsigned long long t_b, t_p, t_e, t_d, t_f, t_c, t_ca, t_co;
        if (sscanf(line, " %31[^:]: %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                   name, &r_b, &r_p, &r_e, &r_d, &r_f, &r_fr, &r_c, &r_m,
                   &t_b, &t_p, &t_e, &t_d, &t_f, &t_c, &t_ca, &t_co) >= 10) {
            if (strcmp(name, "lo") != 0 && r_b > 0) {
                snprintf(ifname, sizeof(ifname), "%s", name);
                rx = r_b;
                tx = t_b;
                break;
            }
        }
    }
    fclose(fp);

    if (ifname[0] != '\0') {
        snprintf(net->iface, sizeof(net->iface), "%s", ifname);
    } else {
        snprintf(net->iface, sizeof(net->iface), "wlp3s0");
    }

    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);
    if (net->prev_time.tv_sec > 0 && rx >= net->prev_rx_bytes) {
        double dt = (cur_time.tv_sec - net->prev_time.tv_sec) + (cur_time.tv_usec - net->prev_time.tv_usec) / 1000000.0;
        if (dt > 0.05) {
            net->rx_kbs = ((double)(rx - net->prev_rx_bytes) / 1024.0) / dt;
            net->tx_kbs = ((double)(tx - net->prev_tx_bytes) / 1024.0) / dt;
        }
    }
    net->prev_rx_bytes = rx;
    net->prev_tx_bytes = tx;
    net->prev_time = cur_time;

    FILE *nfp = popen("nmcli -t -f NAME,TYPE connection show --active 2>/dev/null | grep ':wifi' | head -n 1", "r");
    if (nfp) {
        char nline[64];
        if (fgets(nline, sizeof(nline), nfp)) {
            char *colon = strchr(nline, ':');
            if (colon) *colon = '\0';
            char *nl = strchr(nline, '\n');
            if (nl) *nl = '\0';
            if (nline[0] != '\0') {
                snprintf(net->ssid, sizeof(net->ssid), "%s", nline);
                net->signal_pct = 100;
                net->signal_dbm = -35;
            }
        }
        pclose(nfp);
    }
    if (net->ssid[0] == '\0') {
        snprintf(net->ssid, sizeof(net->ssid), "VIJAYANAND5G");
        net->signal_pct = 100;
        net->signal_dbm = -35;
    }

    if (net->history_count < MAX_HISTORY_POINTS) {
        net->history_rx[net->history_count++] = net->rx_kbs;
    } else {
        memmove(&net->history_rx[0], &net->history_rx[1], sizeof(double) * (MAX_HISTORY_POINTS - 1));
        net->history_rx[MAX_HISTORY_POINTS - 1] = net->rx_kbs;
    }
}

void update_power_telemetry(power_telemetry_t *pwr) {
    FILE *fp = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    if (!fp) fp = fopen("/sys/class/power_supply/BAT1/capacity", "r");
    if (fp) {
        int cap = 0;
        if (fscanf(fp, "%d", &cap) == 1) pwr->bat_pct = cap;
        fclose(fp);
    } else {
        pwr->bat_pct = 82.0;
    }

    fp = fopen("/sys/class/power_supply/BAT0/status", "r");
    if (!fp) fp = fopen("/sys/class/power_supply/BAT1/status", "r");
    if (fp) {
        if (fgets(pwr->bat_status, sizeof(pwr->bat_status), fp)) {
            char *nl = strchr(pwr->bat_status, '\n');
            if (nl) *nl = '\0';
        }
        fclose(fp);
    } else {
        snprintf(pwr->bat_status, sizeof(pwr->bat_status), "Discharging");
    }

    fp = fopen("/sys/class/power_supply/AC/online", "r");
    if (!fp) fp = fopen("/sys/class/power_supply/ACAD/online", "r");
    if (!fp) fp = fopen("/sys/class/power_supply/ADP1/online", "r");
    if (fp) {
        int ac = 0;
        if (fscanf(fp, "%d", &ac) == 1) pwr->ac_online = (ac == 1);
        fclose(fp);
    } else {
        pwr->ac_online = FALSE;
    }

    // Real Hardware Temperatures and Fans
    pwr->temp_c = get_real_cpu_temp();
    pwr->fan_rpm = get_real_fan_rpm();

    static time_t s_last_profile_check = 0;
    static char s_cached_profile[32] = "BALANCED";
    time_t now_pwr = time(NULL);
    if (now_pwr - s_last_profile_check >= 1) {
        s_last_profile_check = now_pwr;
        FILE *pfp = popen("powerprofilesctl get 2>/dev/null", "r");
        if (pfp) {
            char pbuf[32] = {0};
            if (fgets(pbuf, sizeof(pbuf), pfp)) {
                char *nl = strchr(pbuf, '\n');
                if (nl) *nl = '\0';
                if (pbuf[0] != '\0') {
                    if (strcasecmp(pbuf, "performance") == 0) {
                        snprintf(s_cached_profile, sizeof(s_cached_profile), "PERFORMANCE");
                    } else if (strcasecmp(pbuf, "power-saver") == 0 || strcasecmp(pbuf, "powersave") == 0) {
                        snprintf(s_cached_profile, sizeof(s_cached_profile), "POWER SAVER");
                    } else {
                        snprintf(s_cached_profile, sizeof(s_cached_profile), "BALANCED");
                    }
                }
            }
            pclose(pfp);
        }
    }
    snprintf(pwr->profile, sizeof(pwr->profile), "%s", s_cached_profile);
}

void update_mpris_telemetry(mpris_telemetry_t *mpris) {
    static time_t s_last_mpris_query = 0;
    time_t now = time(NULL);

    if (now - s_last_mpris_query >= 1) {
        s_last_mpris_query = now;
        FILE *fp = popen("playerctl metadata --format '{{status}}|{{artist}}|{{title}}|{{album}}|{{position}}|{{mpris:length}}' 2>/dev/null", "r");
        gboolean found = FALSE;
        if (fp) {
            char line[512];
            if (fgets(line, sizeof(line), fp)) {
                char *st = strtok(line, "|");
                char *ar = strtok(NULL, "|");
                char *ti = strtok(NULL, "|");
                char *al = strtok(NULL, "|");
                char *pos = strtok(NULL, "|");
                char *len = strtok(NULL, "\n");

                if (st) snprintf(mpris->status, sizeof(mpris->status), "%s", st);
                if (ar) snprintf(mpris->artist, sizeof(mpris->artist), "%s", ar);
                if (ti) snprintf(mpris->title, sizeof(mpris->title), "%s", ti);
                if (al) snprintf(mpris->album, sizeof(mpris->album), "%s", al);
                if (pos) mpris->position_us = atoll(pos);
                if (len) mpris->length_us = atoll(len);

                if (mpris->length_us > 0) {
                    mpris->progress_pct = ((double)mpris->position_us / (double)mpris->length_us) * 100.0;
                }
                found = TRUE;
            }
            pclose(fp);
        }

        if (!found || mpris->title[0] == '\0') {
            snprintf(mpris->status, sizeof(mpris->status), "Idle");
            snprintf(mpris->title, sizeof(mpris->title), "OFFLINE");
            snprintf(mpris->artist, sizeof(mpris->artist), "NO ACTIVE CARRIER");
            snprintf(mpris->album, sizeof(mpris->album), "SOLARIS CARRIER");
            mpris->progress_pct = 0.0;
        }
    }

    mpris->anim_phase += 0.15;
    if (mpris->anim_phase > 2.0 * M_PI) mpris->anim_phase -= 2.0 * M_PI;
}

void update_workspace_telemetry(workspace_telemetry_t *ws) {
    ws->total_ws = 6;
    for (int i = 0; i < 6; i++) {
        ws->ws_active[i] = FALSE;
    }
    ws->active_ws = 1;

    FILE *fp = popen("niri msg workspaces 2>/dev/null", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "*")) {
                int num = 0;
                if (sscanf(line, " * %d", &num) == 1 && num >= 1 && num <= 6) {
                    ws->active_ws = num;
                    ws->ws_active[num - 1] = TRUE;
                }
            } else {
                int num = 0;
                if (sscanf(line, "   %d", &num) == 1 && num >= 1 && num <= 6) {
                    ws->ws_active[num - 1] = TRUE;
                }
            }
        }
        pclose(fp);
    }
}
