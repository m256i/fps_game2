#ifndef MATH_ANGLES_H_
#define MATH_ANGLES_H_

#include <common.h>

#define DEG2RAD(x)                                                             \
  _Generic(                                                                    \
    (x),                                                                       \
    float: (x) * 0.0174532925f,                                                \
    double: (x) * 0.017453292519943295F,                                       \
    long double: (x) * 0.017453292519943295L,                                  \
    default: (x) * 0.017453292519943295                                        \
  )

#define RAD2DEG(x)                                                             \
  _Generic(                                                                    \
    (x),                                                                       \
    float: (x) * 57.295779513f,                                                \
    double: (x) * 57.295779513082320876,                                       \
    long double: (x) * 57.295779513082320876L,                                 \
    default: (x) * 57.295779513082320876                                       \
  )

#endif // MATH_ANGLES_H_
