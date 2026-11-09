#include "telemetry.h"
#include <sys/utsname.h>
#include <dirent.h>

void telemetry_init(server_telemetry_t *tel) {
    memset(tel, 0, sizeof(*tel));
    tel->is_live = TRUE;

    // Hostname
    if (gethostname(tel->hostname, sizeof(tel->hostname) - 1) != 0) {
        snprintf(tel->hostname, sizeof(tel->hostname), "solaris-core");
    }

    // Kernel & OS
    struct utsname un;
    if (uname(&un) == 0) {
        snprintf(tel->kernel_os, sizeof(tel->kernel_os), "%.30s %.40s %.20s", un.sysname, un.release, un.machine);
    } else {
        snprintf(tel->kernel_os, sizeof(tel->kernel_os), "Linux (Unknown Kernel)");
    }

    // Default Remote Server Recognition (venkat-server @ 100.110.135.100 via Tailscale)
    snprintf(tel->remote_host, sizeof(tel->remote_host), "venkat-server");
    snprintf(tel->remote_ip, sizeof(tel->remote_ip), "100.110.135.100");
    snprintf(tel->remote_user, sizeof(tel->remote_user), "venkat-server");
    snprintf(tel->remote_direct_addr, sizeof(tel->remote_direct_addr), "192.168.29.105:41641");
    tel->remote_ping_ms = 3.2;
    tel->remote_online = TRUE;
    tel->remote_is_tailscale = TRUE;

    gettimeofday(&tel->prev_sample_tv, NULL);
    telemetry_update(tel);
}

void telemetry_update(server_telemetry_t *tel) {
    struct timeval now;
    gettimeofday(&now, NULL);
    double dt = (now.tv_sec - tel->prev_sample_tv.tv_sec) +
                (now.tv_usec - tel->prev_sample_tv.tv_usec) * 1e-6;
    if (dt < 0.1) dt = 0.1;
    tel->prev_sample_tv = now;

    gboolean any_read_success = FALSE;

    // 1. CPU Usage (/proc/stat)
    FILE *fp_stat = fopen("/proc/stat", "r");
    if (fp_stat) {
        char line[256];
        if (fgets(line, sizeof(line), fp_stat)) {
            unsigned long long u, n, s, idle, iow, irq, sirq, steal;
            if (sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
                       &u, &n, &s, &idle, &iow, &irq, &sirq, &steal) >= 4) {
                unsigned long long total = u + n + s + idle + iow + irq + sirq + steal;
                if (tel->prev_cpu_total > 0 && total > tel->prev_cpu_total) {
                    unsigned long long diff_total = total - tel->prev_cpu_total;
                    unsigned long long diff_idle = idle - tel->prev_cpu_idle;
                    double usage = 100.0 * (1.0 - ((double)diff_idle / (double)diff_total));
                    if (usage < 0.0) usage = 0.0;
                    if (usage > 100.0) usage = 100.0;
                    tel->cpu_usage = usage;
                }
                tel->prev_cpu_total = total;
                tel->prev_cpu_idle = idle;
                any_read_success = TRUE;
            }
        }
        fclose(fp_stat);
    }

    // 2. Memory Usage (/proc/meminfo)
    FILE *fp_mem = fopen("/proc/meminfo", "r");
    if (fp_mem) {
        char key[64];
        unsigned long long val_kb;
        unsigned long long mem_total = 0, mem_avail = 0;
        char line[128];
        while (fgets(line, sizeof(line), fp_mem)) {
            if (sscanf(line, "%63s %llu", key, &val_kb) == 2) {
                if (strcmp(key, "MemTotal:") == 0) mem_total = val_kb;
                else if (strcmp(key, "MemAvailable:") == 0) mem_avail = val_kb;
            }
        }
        fclose(fp_mem);
        if (mem_total > 0) {
            tel->mem_total_gb = (double)mem_total / (1024.0 * 1024.0);
            tel->mem_used_gb = (double)(mem_total - mem_avail) / (1024.0 * 1024.0);
            tel->mem_percent = 100.0 * (tel->mem_used_gb / tel->mem_total_gb);
            any_read_success = TRUE;
        }
    }

    // 3. CPU Temperature
    double temp_c = 0.0;
    gboolean temp_found = FALSE;
    // Try thermal_zone files
    for (int tz = 0; tz < 8; tz++) {
        char path[128];
        snprintf(path, sizeof(path), "/sys/class/thermal/thermal_zone%d/temp", tz);
        FILE *fp_temp = fopen(path, "r");
        if (fp_temp) {
            long raw_temp = 0;
            if (fscanf(fp_temp, "%ld", &raw_temp) == 1) {
                double t = (double)raw_temp / 1000.0;
                if (t > 20.0 && t < 120.0) {
                    temp_c = t;
                    temp_found = TRUE;
                    fclose(fp_temp);
                    break;
                }
            }
            fclose(fp_temp);
        }
    }
    if (temp_found) {
        tel->cpu_temp = temp_c;
        any_read_success = TRUE;
    } else if (tel->cpu_temp <= 0.0) {
        tel->cpu_temp = 55.0; // fallback reasonable estimate
    }

    // 4. Network Rates (/proc/net/dev)
    FILE *fp_net = fopen("/proc/net/dev", "r");
    if (fp_net) {
        char line[256];
        unsigned long long total_rx = 0, total_tx = 0;
        // Skip header lines
        if (fgets(line, sizeof(line), fp_net) && fgets(line, sizeof(line), fp_net)) {
            while (fgets(line, sizeof(line), fp_net)) {
                char ifname[32];
                unsigned long long rx = 0, tx = 0;
                char *colon = strchr(line, ':');
                if (colon) {
                    *colon = ' ';
                    if (sscanf(line, "%31s %llu %*u %*u %*u %*u %*u %*u %*u %llu",
                               ifname, &rx, &tx) >= 3) {
                        if (strcmp(ifname, "lo") != 0) {
                            total_rx += rx;
                            total_tx += tx;
                        }
                    }
                }
            }
        }
        fclose(fp_net);

        if (tel->prev_net_rx > 0 && total_rx >= tel->prev_net_rx) {
            tel->net_rx_kbs = (double)(total_rx - tel->prev_net_rx) / (dt * 1024.0);
            tel->net_tx_kbs = (double)(total_tx - tel->prev_net_tx) / (dt * 1024.0);
        }
        tel->prev_net_rx = total_rx;
        tel->prev_net_tx = total_tx;
        any_read_success = TRUE;
    }

    // 5. Disk Activity (/proc/diskstats)
    FILE *fp_disk = fopen("/proc/diskstats", "r");
    if (fp_disk) {
        char line[256];
        unsigned long long sectors_total = 0;
        while (fgets(line, sizeof(line), fp_disk)) {
            char dev[32];
            unsigned long long rd_sec = 0, wr_sec = 0;
            if (sscanf(line, "%*d %*d %31s %*u %*u %llu %*u %*u %*u %llu",
                       dev, &rd_sec, &wr_sec) >= 3) {
                // Focus on nvme0n1, sda, vda
                if (strncmp(dev, "nvme0n1", 7) == 0 || strncmp(dev, "sda", 3) == 0 || strncmp(dev, "vda", 3) == 0) {
                    sectors_total += (rd_sec + wr_sec);
                }
            }
        }
        fclose(fp_disk);

        if (tel->prev_disk_sectors > 0 && sectors_total >= tel->prev_disk_sectors) {
            double diff_mb = (double)(sectors_total - tel->prev_disk_sectors) * 512.0 / (1024.0 * 1024.0);
            tel->disk_read_mbs = diff_mb / dt;
            tel->disk_active = (tel->disk_read_mbs > 0.1);
        } else {
            tel->disk_active = FALSE;
        }
        tel->prev_disk_sectors = sectors_total;
        any_read_success = TRUE;
    }

    // 6. Uptime (/proc/uptime)
    FILE *fp_up = fopen("/proc/uptime", "r");
    if (fp_up) {
        double up_sec = 0.0;
        if (fscanf(fp_up, "%lf", &up_sec) == 1) {
            int hrs = (int)(up_sec / 3600);
            int mins = (int)(((long)up_sec % 3600) / 60);
            int secs = (int)((long)up_sec % 60);
            snprintf(tel->uptime_str, sizeof(tel->uptime_str), "%02dh %02dm %02ds", hrs, mins, secs);
            any_read_success = TRUE;
        }
        fclose(fp_up);
    }

    // 7. Tailscale Remote Server Recognition (venkat-server @ 100.110.135.100)
    tel->ts_check_timer += dt;
    if (tel->ts_check_timer >= 2.0) {
        tel->ts_check_timer = 0.0;
        FILE *fp_ts = popen("tailscale status 2>/dev/null | grep '100.110.135.100'", "r");
        if (fp_ts) {
            char ts_line[256];
            if (fgets(ts_line, sizeof(ts_line), fp_ts)) {
                tel->remote_online = (strstr(ts_line, "active") != NULL ||
                                      strstr(ts_line, "direct") != NULL ||
                                      strstr(ts_line, "idle") != NULL);
                tel->remote_is_tailscale = TRUE;
                char *dir = strstr(ts_line, "direct ");
                if (dir) {
                    sscanf(dir, "direct %63[^, ]", tel->remote_direct_addr);
                }
            } else {
                tel->remote_online = FALSE;
            }
            pclose(fp_ts);
        }
    }

    tel->is_live = any_read_success;
}
