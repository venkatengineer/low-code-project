#ifndef SOLARIS_HOLOGRAM_RENDER_HUD_H
#define SOLARIS_HOLOGRAM_RENDER_HUD_H

#include "common.h"
#include "model.h"

// Render technical grid, scanlines, and HUD corner brackets
void render_hud_background_and_grid(cairo_t *cr, const orbital_system_t *sys, double w, double h);

// Render interactive floating telemetry card on hover
void render_hud_hover_card(cairo_t *cr, const orbital_system_t *sys, double mx, double my, double w, double h);

// Render system link connection status banner
void render_hud_connection_status(cairo_t *cr, const orbital_system_t *sys, double w, double h);

#endif // SOLARIS_HOLOGRAM_RENDER_HUD_H
