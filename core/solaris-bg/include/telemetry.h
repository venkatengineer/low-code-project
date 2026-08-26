#ifndef SOLARIS_TELEMETRY_H
#define SOLARIS_TELEMETRY_H

#include <stdint.h>
#include <glib.h>

#define MAX_HISTORY_POINTS 60
#define MAX_WIFI_TARGETS 16

typedef struct {
    char ssid[64];
    char bssid[32];
    int signal_pct;
    int signal_dbm;
    char security[32];
    char band[16];
    gboolean is_active;
    double radar_angle;
    double radar_dist;
} wifi_target_t;

typedef struct {
    wifi_target_t targets[MAX_WIFI_TARGETS];
    int target_count;
    char active_ssid[64];
    int active_signal;
    gboolean scanning;
} wifi_scan_telemetry_t;

typedef struct {
    double usage_pct;
    double freq_ghz;
    int core_count;
    double core_usage[32];
    double history[MAX_HISTORY_POINTS];
    int history_count;
    char model_name[128];
    unsigned long long prev_total;
    unsigned long long prev_idle;
    unsigned long long prev_core_total[32];
    unsigned long long prev_core_idle[32];
    double plasma_phase;
} cpu_telemetry_t;

typedef struct {
    double total_gb;
    double used_gb;
    double free_gb;
    double usage_pct;
    double swap_total_gb;
    double swap_used_gb;
    double swap_pct;
    double history[MAX_HISTORY_POINTS];
    int history_count;
    double flux_phase;
} mem_telemetry_t;

typedef struct {
    double usage_pct;
    double temp_c;
    double vram_used_gb;
    double vram_total_gb;
    double vram_pct;
    char model_name[128];
    double history[MAX_HISTORY_POINTS];
    int history_count;
    double tensor_phase;
} gpu_telemetry_t;

typedef struct {
    double root_total_gb;
    double root_used_gb;
    double root_pct;
    double home_total_gb;
    double home_used_gb;
    double home_pct;
} disk_telemetry_t;

typedef struct {
    char iface[32];
    char ssid[64];
    char bssid[32];
    int signal_dbm;
    int signal_pct;
    double rx_kbs;
    double tx_kbs;
    double history_rx[MAX_HISTORY_POINTS];
    int history_count;
    unsigned long long prev_rx_bytes;
    unsigned long long prev_tx_bytes;
    struct timeval prev_time;
} net_telemetry_t;

typedef struct {
    double bat_pct;
    char bat_status[32];
    gboolean ac_online;
    double temp_c;
    int fan_rpm;
    char profile[32];
} power_telemetry_t;

typedef struct {
    char status[32];
    char title[128];
    char artist[128];
    char album[128];
    int64_t position_us;
    int64_t length_us;
    double progress_pct;
    double spectrum[24];
    double anim_phase;
} mpris_telemetry_t;

typedef struct {
    int active_ws;
    int total_ws;
    int window_count;
    gboolean ws_active[8];
} workspace_telemetry_t;

// Telemetry update functions
void init_telemetry(void);
void update_cpu_telemetry(cpu_telemetry_t *cpu);
void update_mem_telemetry(mem_telemetry_t *mem);
void update_gpu_telemetry(gpu_telemetry_t *gpu);
void update_disk_telemetry(disk_telemetry_t *disk);
void update_net_telemetry(net_telemetry_t *net);
void update_wifi_scan_telemetry(wifi_scan_telemetry_t *scan);
void trigger_wifi_connect(const char *ssid);
void update_power_telemetry(power_telemetry_t *pwr);
void update_mpris_telemetry(mpris_telemetry_t *mpris);
void update_workspace_telemetry(workspace_telemetry_t *ws);

#endif // SOLARIS_TELEMETRY_H
