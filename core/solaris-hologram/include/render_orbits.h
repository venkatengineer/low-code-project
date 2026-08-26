#ifndef SOLARIS_HOLOGRAM_RENDER_ORBITS_H
#define SOLARIS_HOLOGRAM_RENDER_ORBITS_H

#include "common.h"
#include "model.h"

// Render back-half of orbits (behind the session core)
void render_orbital_rings_back(cairo_t *cr, const orbital_system_t *sys, double cx, double cy);

// Render front-half of orbits (in front of the session core)
void render_orbital_rings_front(cairo_t *cr, const orbital_system_t *sys, double cx, double cy);

// Render station labels and "+ New Station" button
void render_station_labels_and_controls(cairo_t *cr, const orbital_system_t *sys, double cx, double cy);

#endif // SOLARIS_HOLOGRAM_RENDER_ORBITS_H
