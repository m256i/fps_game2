#ifndef RENDERER_CAMERA_H_
#define RENDERER_CAMERA_H_

#include <common.h>
#include <math/vec3.h>
#include <math/mat4.h>
#include <math/quat4.h>
#include <math/angles.h>
#include <math/math.h>

#define CAMERA_INITIAL_YAW         -90.f
#define CAMERA_INITIAL_PITCH       0.f
#define CAMERA_INITIAL_SPEED       5.6f
#define CAMERA_INITIAL_SENSITIVITY 0.2f
#define CAMERA_INITIAL_FOV         9.f
#define CAMERA_INITIAL_FRONT       ((vec3){.x = 0.f, .y = 1.f, .z = -1.f})
#define CAMERA_WORLD_UP            ((vec3){.x = 0.f, .y = 1.f, .z = 0.f})
#define CAMERA_WORLD_FORWARD       ((vec3){.x = 0.f, .y = 0.f, .z = -1.f})
#define CAMERA_WORLD_RIGHT         ((vec3){.x = 1.f, .y = 0.f, .z = 0.f})

typedef struct {
  vec3 position;
  vec3 vec_front, vec_up, vec_right;
  f32  yaw, pitch; // in degree
  f32  movement_speed;
  f32  mouse_sense;
  f32  fov;
} game_camera;

static inline void update_vectors(game_camera *cam) {
  cam->vec_front.x = cosf(DEG2RAD(cam->yaw)) * cosf(DEG2RAD(cam->pitch));
  cam->vec_front.y = sinf(DEG2RAD(cam->pitch));
  cam->vec_front.z = sinf(DEG2RAD(cam->yaw)) * cosf(DEG2RAD(cam->pitch));

  vec3_normalize_inplace(&cam->vec_front);
  vec3 cross = vec3_cross(&cam->vec_front, &CAMERA_WORLD_UP);
  vec3_normalize_inplace(&cross);
  cam->vec_right = cross;
  cross          = vec3_cross(&cam->vec_right, &cam->vec_front);
  vec3_normalize_inplace(&cross);
  cam->vec_up = cross;
}

static inline void
camera_initialize(game_camera *cam, vec3 position, f32 yaw_deg, f32 pitch_deg) {
  cam->position       = position;
  cam->yaw            = yaw_deg;
  cam->pitch          = pitch_deg;
  cam->movement_speed = CAMERA_INITIAL_SPEED;
  cam->mouse_sense    = CAMERA_INITIAL_SENSITIVITY;
  cam->fov            = CAMERA_INITIAL_FOV;

  cam->vec_front = CAMERA_INITIAL_FRONT;
  cam->vec_up    = CAMERA_WORLD_UP;
}

static inline void
camera_process_mouse_movement(game_camera *cam, float xoffset, float yoffset) {
  xoffset *= cam->mouse_sense;
  yoffset *= cam->mouse_sense;

  cam->yaw   += xoffset;
  cam->pitch += yoffset;

  cam->pitch = clampf(cam->pitch, -89.f, 89.f);
  update_vectors(cam);
}

static inline void camera_get_view_matrix(mat4 *view, const game_camera *cam) {
  vec3 forward = vec3_add(&cam->position, &cam->vec_front);
  mat4_look_at(view, &cam->position, &forward, &cam->vec_up);
}

#endif // RENDERER_CAMERA_H_
