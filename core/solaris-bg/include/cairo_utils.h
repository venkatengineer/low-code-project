#ifndef SOLARIS_CAIRO_UTILS_H
#define SOLARIS_CAIRO_UTILS_H

#include <cairo.h>
#include <glib.h>
#include "telemetry.h"

// Draw beveled alien technical panel background with decorative conduits
void cairo_draw_beveled_panel(cairo_t *cr, double x, double y, double w, double h, double chamfer);

// Draw alien panel header with title and status indicator
void cairo_draw_module_header(cairo_t *cr, double x, double y, double w, const char *title, double r, double g, double b);

// Draw high-tech Sci-Fi Speedometer / Tachometer (CPU)
void cairo_draw_sci_fi_speedometer(cairo_t *cr, double cx, double cy, double radius,
                                   double usage_pct, double freq_ghz, double temp_c,
                                   const char *model_name, double phase);

// Draw high-tech Spacecraft Warp Core Reactor (CPU)
void cairo_draw_spacecraft_warp_core(cairo_t *cr, double cx, double cy, double radius,
                                     double usage_pct, double freq_ghz, double phase,
                                     const char *model_name);

// Draw Multi-Core Plasma Fuel Injector Array
void cairo_draw_plasma_injectors(cairo_t *cr, double x, double y, double w, double h,
                                 int core_count, double usage_pct, double phase);

// Draw Quantum Memory Buffer Matrix (RAM & SWAP)
void cairo_draw_quantum_hex_ram_core(cairo_t *cr, double x, double y, double w, double h,
                                     double used_gb, double total_gb, double pct,
                                     double swap_used, double swap_total, double swap_pct,
                                     double phase);

void cairo_draw_quantum_memory_matrix(cairo_t *cr, double x, double y, double w, double h,
                                      double used_gb, double total_gb, double pct,
                                      double swap_used, double swap_total, double swap_pct,
                                      double phase);

// Draw Attractive Cybernetic GPU Tachometer HUD (RTX 5050 GPU)
void cairo_draw_cyber_gpu_tachometer(cairo_t *cr, double x, double y, double w, double h,
                                     double usage_pct, double temp_c,
                                     double vram_used, double vram_total,
                                     const char *model, double phase);

// Draw Tensor Warp Compute Core (RTX 5050 GPU Hexagonal Engine)
void cairo_draw_tensor_gpu_core(cairo_t *cr, double x, double y, double w, double h,
                                double usage_pct, double temp_c,
                                double vram_used, double vram_total,
                                const char *model, double phase);

// Draw Spacecraft Storage Canister Pods
void cairo_draw_spacecraft_storage_pod(cairo_t *cr, double x, double y, double w, double h,
                                       const char *sector_label, double used_gb, double total_gb,
                                       double pct, double r, double g, double b);

// Draw Aerospace Multi-Cell Plasma Fuel Battery Core (Battery Widget)
void cairo_draw_aerospace_power_core(cairo_t *cr, double x, double y, double w, double h,
                                     double bat_pct, gboolean ac_online,
                                     const char *status, const char *profile,
                                     double phase);

// Draw Spacecraft Antimatter / Fusion Battery Reactor Core (Battery Widget)
void cairo_draw_spacecraft_power_core(cairo_t *cr, double cx, double cy, double radius,
                                      double bat_pct, gboolean ac_online,
                                      const char *status, const char *profile,
                                      double phase);

// Draw Active Cooling Fan Turbine with rotating aerofoil blades
void cairo_draw_cooling_turbine(cairo_t *cr, double cx, double cy, double radius,
                                int rpm, double temp_c, double phase);

// Draw eDEX-UI inspired 3D Rotating Holographic Earth Globe with Wireframe Projection & Beacons
void cairo_draw_edex_globe(cairo_t *cr, double cx, double cy, double radius,
                           double time_sec, const wifi_scan_telemetry_t *scan);

// Draw 3D Multi-Axis Gyroscopic Quantum Resonance Radar (Bluetooth Scanner)
void cairo_draw_quantum_bt_gyroscope(cairo_t *cr, double cx, double cy, double radius,
                                     double time_sec);

// Draw tactical deep-space sweep radar with real discovered RF targets
void cairo_draw_tactical_radar_deepspace(cairo_t *cr, double cx, double cy, double radius,
                                        double angle_rad, double time_sec,
                                        const wifi_scan_telemetry_t *wifi_scan);

// Draw circular ring gauge
void cairo_draw_ring_gauge(cairo_t *cr, double cx, double cy, double radius, double thickness,
                           double pct, const char *center_text,
                           double r, double g, double b);

// Draw sparkline / telemetry history graph
void cairo_draw_sparkline(cairo_t *cr, double x, double y, double w, double h,
                          const double *data, int count, double min_val, double max_val,
                          double r, double g, double b);

// Draw segmented progress bar
void cairo_draw_segmented_bar(cairo_t *cr, double x, double y, double w, double h,
                              double pct, int segments,
                              double r, double g, double b);

// Draw mini network/RF radar
void cairo_draw_mini_radar(cairo_t *cr, double cx, double cy, double radius,
                           double angle_rad, double r, double g, double b);

// ==============================================================================
// 🛸 SPACECRAFT SEQUENTIAL POWER-ON AWAKENING ENGINE
// ==============================================================================
double solaris_get_awakening_elapsed(void);
double solaris_calc_awakening_illumination(double start_delay, double duration);
void solaris_draw_awakening_overlay(cairo_t *cr, double x, double y, double w, double h,
                                   double start_delay, double duration, const char *module_tag);

#endif // SOLARIS_CAIRO_UTILS_H
