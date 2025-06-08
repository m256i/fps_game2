#ifndef RENDERER_CAMERA_H_
#define RENDERER_CAMERA_H_

#include <common.h>
#include <math/vec3.h>
#include <math/mat4.h>
#include <math/angles.h>
#include <math/math.h>

#define CAMERA_INITIAL_YAW         -90.f
#define CAMERA_INITIAL_PITCH       0.f
#define CAMERA_INITIAL_SPEED       5.6f
#define CAMERA_INITIAL_SENSITIVITY 0.2f
#define CAMERA_INITIAL_FOV         100.f
#define CAMERA_INITIAL_FRONT       ((vec3){.x = 0.f, .y = 1.f, .z = -1.f})
#define CAMERA_WORLD_UP            ((vec3){.x = 0.f, .y = 1.f, .z = 0.f})

typedef struct {
  vec3 position;
  vec3 front;
  vec3 up;
  vec3 right;

  f32 yaw;
  f32 pitch;
  f32 movement_speed;
  f32 mouse_sense;
  f32 fov;
} game_camera;

u0 camera_update_vectors(game_camera *const _cam) {
  vec3 front;
  front.x     = cosf(DEG2RAD(_cam->yaw) * cosf(DEG2RAD(_cam->pitch)));
  front.y     = sinf(DEG2RAD(_cam->pitch));
  front.z     = sinf(DEG2RAD(_cam->yaw)) * cosf(DEG2RAD(_cam->pitch));
  _cam->front = vec3_normalize(&front);
  /* front now used as storage, disregard name */
  front       = vec3_cross(&front, &CAMERA_WORLD_UP);
  _cam->right = vec3_normalize(&front);
  front       = vec3_cross(&front, &_cam->front);
  _cam->up    = vec3_normalize(&front);
}

u0 camera_process_mouse_movement(
  game_camera *_cam,
  f32          _xoffset,
  f32          _yoffset
) {
  _xoffset    *= _cam->mouse_sense;
  _yoffset    *= _cam->mouse_sense;
  _cam->yaw   += _xoffset;
  _cam->pitch += _yoffset;

  _cam->pitch = clamp(_cam->pitch, -89.0f, 89.0f);
  camera_update_vectors(_cam);
}

u0 camera_get_view_matrix(mat4 *const _m0, const game_camera *const _cam) {
  vec3 centr = vec3_add(&_cam->position, &_cam->front);
  mat4_look_at(_m0, &_cam->position, &centr, &_cam->up);
}

u0 camera_initialize(
  game_camera *const _cam,
  const vec3         _position,
  const vec3         _up,
  const f32          _yaw,
  const f32          _pitch
) {
  _cam->front          = CAMERA_INITIAL_FRONT;
  _cam->movement_speed = CAMERA_INITIAL_SPEED;
  _cam->mouse_sense    = CAMERA_INITIAL_SENSITIVITY;
  _cam->fov            = CAMERA_INITIAL_FOV;
  _cam->position       = _position;
  _cam->up             = _up;
  _cam->yaw            = _yaw;
  _cam->pitch          = _pitch;
}

#endif // RENDERER_CAMERA_H_
