#ifndef SOLARIS_HOLOGRAM_RENDER_NODES_H
#define SOLARIS_HOLOGRAM_RENDER_NODES_H

#include "common.h"
#include "model.h"

// Render all application window nodes with z-sorting and tether beams
void render_application_nodes(cairo_t *cr, const orbital_system_t *sys, double cx, double cy);

#endif // SOLARIS_HOLOGRAM_RENDER_NODES_H
