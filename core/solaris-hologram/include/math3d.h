#ifndef SOLARIS_HOLOGRAM_MATH3D_H
#define SOLARIS_HOLOGRAM_MATH3D_H

#include "common.h"

struct vec3_s {
    double x;
    double y;
    double z;
};

typedef struct {
    double x;
    double y;
} vec2_t;

struct mat4_s {
    double m[4][4];
};

struct camera3d_s {
    double pitch;       // Rotation around X axis (elevation, rad)
    double yaw;         // Rotation around Y axis (azimuth, rad)
    double roll;        // Rotation around Z axis (rad)
    double distance;    // Camera distance (zoom)
    double fov;         // Field of view parameter / focal length
    double pan_x;       // Screen pan offset X
    double pan_y;       // Screen pan offset Y
    
    // Target values for smooth spring/damping interpolation
    double target_pitch;
    double target_yaw;
    double target_distance;
    double target_pan_x;
    double target_pan_y;
};

// 3D vector operations
vec3_t vec3_create(double x, double y, double z);
vec3_t vec3_add(vec3_t a, vec3_t b);
vec3_t vec3_sub(vec3_t a, vec3_t b);
vec3_t vec3_scale(vec3_t v, double s);
double vec3_length(vec3_t v);
vec3_t vec3_normalize(vec3_t v);

// 3D rotation & transformation
vec3_t vec3_rotate_euler(vec3_t p, double pitch, double yaw, double roll);

// Camera initialization and smoothing
void camera3d_init(camera3d_t *cam);
void camera3d_update(camera3d_t *cam, double dt);
void camera3d_reset(camera3d_t *cam);

// Perspective projection: Transforms 3D world coordinate to 2D screen coordinate
// Returns TRUE if point is in front of camera, FALSE if behind clipping plane
gboolean project_to_screen(vec3_t world_pos, const camera3d_t *cam,
                           double screen_center_x, double screen_center_y,
                           vec2_t *screen_pos, double *out_scale, double *out_depth);

// Hit testing
double distance_point_to_segment(double px, double py, double x1, double y1, double x2, double y2);

#endif // SOLARIS_HOLOGRAM_MATH3D_H
