#ifndef SOLARIS_TELEMETRY_H
#define SOLARIS_TELEMETRY_H

#include "common.h"

typedef struct {
    double cpu_usage;       // 0.0 to 100.0%
    double cpu_temp;        // Celsius
    double mem_used_gb;     // GB
    double mem_total_gb;    // GB
    double mem_percent;     // 0.0 to 100.0%
    double net_rx_kbs;      // KB/s
    double net_tx_kbs;      // KB/s
    double disk_read_mbs;   // MB/s
    double disk_write_mbs;  // MB/s
    char hostname[64];
    char kernel_os[128];
    char uptime_str[64];
    gboolean is_live;       // TRUE if real metrics were read successfully
    gboolean disk_active;   // TRUE if active read/write in current sample

    // Tailscale Remote Server Recognition (venkat-server @ 100.110.135.100)
    char remote_host[64];       // "venkat-server"
    char remote_ip[64];         // "100.110.135.100"
    char remote_user[64];       // "venkat-server"
    char remote_direct_addr[64];// "192.168.29.105:41641"
    double remote_ping_ms;      // Ping latency (ms)
    gboolean remote_online;     // TRUE if recognized and online on Tailscale
    gboolean remote_is_tailscale; // TRUE if connected via Tailscale
    double ts_check_timer;

    // Internal counters for deltas
    unsigned long long prev_cpu_idle;
    unsigned long long prev_cpu_total;
    unsigned long long prev_net_rx;
    unsigned long long prev_net_tx;
    unsigned long long prev_disk_sectors;
    struct timeval prev_sample_tv;
} server_telemetry_t;

void telemetry_init(server_telemetry_t *tel);
void telemetry_update(server_telemetry_t *tel);

#endif // SOLARIS_TELEMETRY_H
