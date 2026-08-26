#include "math3d.h"

vec3_t vec3_create(double x, double y, double z) {
    vec3_t v = {x, y, z};
    return v;
}

vec3_t vec3_add(vec3_t a, vec3_t b) {
    vec3_t v = {a.x + b.x, a.y + b.y, a.z + b.z};
    return v;
}

vec3_t vec3_sub(vec3_t a, vec3_t b) {
    vec3_t v = {a.x - b.x, a.y - b.y, a.z - b.z};
    return v;
}

vec3_t vec3_scale(vec3_t v, double s) {
    vec3_t res = {v.x * s, v.y * s, v.z * s};
    return res;
}

double vec3_length(vec3_t v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec3_t vec3_normalize(vec3_t v) {
    double len = vec3_length(v);
    if (len > 1e-6) {
        return vec3_scale(v, 1.0 / len);
    }
    return vec3_create(0, 0, 0);
}

vec3_t vec3_rotate_euler(vec3_t p, double pitch, double yaw, double roll) {
    // 1. Yaw rotation around Y axis (horizontal azimuth)
    double cos_y = cos(yaw);
    double sin_y = sin(yaw);
    double x1 = p.x * cos_y + p.z * sin_y;
    double y1 = p.y;
    double z1 = -p.x * sin_y + p.z * cos_y;

    // 2. Pitch rotation around X axis (vertical elevation)
    double cos_p = cos(pitch);
    double sin_p = sin(pitch);
    double x2 = x1;
    double y2 = y1 * cos_p - z1 * sin_p;
    double z2 = y1 * sin_p + z1 * cos_p;

    // 3. Roll rotation around Z axis
    if (fabs(roll) > 1e-5) {
        double cos_r = cos(roll);
        double sin_r = sin(roll);
        double x3 = x2 * cos_r - y2 * sin_r;
        double y3 = x2 * sin_r + y2 * cos_r;
        double z3 = z2;
        return vec3_create(x3, y3, z3);
    }

    return vec3_create(x2, y2, z2);
}

void camera3d_init(camera3d_t *cam) {
    // Default optimal isometric-perspective command bridge view
    // Pitch ~ 55 degrees down, Yaw ~ 15 degrees angle for 3D depth
    cam->pitch = 0.95;            // ~54.4 degrees
    cam->yaw = 0.25;              // ~14.3 degrees
    cam->roll = 0.0;
    cam->distance = 780.0;
    cam->fov = 680.0;
    cam->pan_x = 0.0;
    cam->pan_y = 0.0;

    cam->target_pitch = cam->pitch;
    cam->target_yaw = cam->yaw;
    cam->target_distance = cam->distance;
    cam->target_pan_x = cam->pan_x;
    cam->target_pan_y = cam->pan_y;
}

void camera3d_reset(camera3d_t *cam) {
    cam->target_pitch = 0.95;
    cam->target_yaw = 0.25;
    cam->target_distance = 780.0;
    cam->target_pan_x = 0.0;
    cam->target_pan_y = 0.0;
}

void camera3d_update(camera3d_t *cam, double dt) {
    double lerp_factor = 1.0 - exp(-12.0 * dt);
    cam->pitch += (cam->target_pitch - cam->pitch) * lerp_factor;
    cam->yaw += (cam->target_yaw - cam->yaw) * lerp_factor;
    cam->distance += (cam->target_distance - cam->distance) * lerp_factor;
    cam->pan_x += (cam->target_pan_x - cam->pan_x) * lerp_factor;
    cam->pan_y += (cam->target_pan_y - cam->pan_y) * lerp_factor;

    // Clamp pitch to avoid inversion (e.g. 10 deg to 85 deg)
    if (cam->pitch < 0.15) cam->pitch = 0.15;
    if (cam->pitch > 1.50) cam->pitch = 1.50;
    if (cam->target_pitch < 0.15) cam->target_pitch = 0.15;
    if (cam->target_pitch > 1.50) cam->target_pitch = 1.50;

    // Clamp distance (zoom limit)
    if (cam->distance < 400.0) cam->distance = 400.0;
    if (cam->distance > 1500.0) cam->distance = 1500.0;
    if (cam->target_distance < 400.0) cam->target_distance = 400.0;
    if (cam->target_distance > 1500.0) cam->target_distance = 1500.0;
}

gboolean project_to_screen(vec3_t world_pos, const camera3d_t *cam,
                           double screen_center_x, double screen_center_y,
                           vec2_t *screen_pos, double *out_scale, double *out_depth) {
    // 1. Apply camera orientation
    vec3_t rot = vec3_rotate_euler(world_pos, cam->pitch, cam->yaw, cam->roll);

    // 2. Camera distance offset along Z
    double z_cam = rot.z + cam->distance;
    if (z_cam < 100.0) {
        // Point is too close or behind the camera
        return FALSE;
    }

    // 3. Perspective divide
    double scale = cam->fov / z_cam;
    
    // Invert Y because Cairo Y-axis points downwards, whereas standard 3D Y-axis points upwards
    double sx = screen_center_x + cam->pan_x + (rot.x * scale);
    double sy = screen_center_y + cam->pan_y - (rot.y * scale);

    if (screen_pos) {
        screen_pos->x = sx;
        screen_pos->y = sy;
    }
    if (out_scale) {
        *out_scale = scale;
    }
    if (out_depth) {
        *out_depth = z_cam;
    }

    return TRUE;
}

double distance_point_to_segment(double px, double py, double x1, double y1, double x2, double y2) {
    double l2 = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
    if (l2 < 1e-6) {
        return sqrt((px - x1) * (px - x1) + (py - y1) * (py - y1));
    }
    double t = ((px - x1) * (x2 - x1) + (py - y1) * (y2 - y1)) / l2;
    t = fmax(0.0, fmin(1.0, t));
    double proj_x = x1 + t * (x2 - x1);
    double proj_y = y1 + t * (y2 - y1);
    return sqrt((px - proj_x) * (px - proj_x) + (py - proj_y) * (py - proj_y));
}
