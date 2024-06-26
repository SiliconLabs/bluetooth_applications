/**
 * LEAPS - Low Energy Accurate Positioning System.
 *
 * Mathematic utilities.
 *
 * Copyright (c) 2016-2018, LEAPS. All rights reserved.
 *
 */

#ifndef _DWM_MATH_H_
#define _DWM_MATH_H_

#include <math.h>
#include <stdint.h>

#define POW(x)                         ((x) * (x))
#ifndef MIN
#define MIN(a, b)                      (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)                      (((a) > (b)) ? (a) : (b))
#endif
#ifndef ABS
#define ABS(x)                         (((x) < 0) ? (-(x)) : (x))
#endif

#ifndef M_PI
#define M_PI                  3.14159265358979323846
#endif
#define DEG2RAD(a)                     ((a) * 0.01745329252) /* /180.0*M_PI */
#define RAD2DEG(a)                     ((a) * 57.2957795131) /* 180.0/M_PI */

#define NANF                  0x7ff80000

/* Millimeters to/from Centimeters conversion */
#define MM_TO_M(mm)                    ((mm) / 1000)
#define M_TO_MM(cm)                    ((cm) * 1000)

#define SPEED_OF_LIGHT        (299702547.0)    /* in m/s in air */

#define MOVING_AVERAGE(o, n, k)        (((k) * (o) + (n)) / ((k) + 1))

typedef union
{
  float value;
  uint32_t word;      /* Assuming 32 bit int. */
} ieee_float_shape_t;

/* Get a 32 bit int from a float. */
#define GET_FLOAT_WORD(i, d) \
  do {                       \
    ieee_float_shape_t gf_u; \
    gf_u.value = (d);        \
    (i) = gf_u.word;         \
  } while (0);

/* Set a float from a 32 bit int. */
#define SET_FLOAT_WORD(d, i) \
  do {                       \
    ieee_float_shape_t sf_u; \
    sf_u.word = (i);         \
    (d) = sf_u.value;        \
  } while (0);

struct vec3d {
  double x;
  double y;
  double z;
};

typedef struct vec3d vec3d_t;

/**
 * Calculate factorial of number n
 *
 * @param[in] n    Number to calculate
 * @return     Returns factorial value
 */
unsigned long fact(int n);

/**
 * Calculate fast log2
 *
 * @param[in] n    Value
 * @return     Returns log2 of value
 */
float flog2(float val);

/**
 * Calculate fast log10
 *
 * @param[in] n    Value
 * @return     Returns log10 of value
 */
float flog10(float val);

/**
 * Calculate distance between two 3D points
 *
 * @param[in] p0  Pointer to point 1
 * @param[in] p1  Pointer to point 2
 * @return     Returns distance between the points
 */
double get_dist(const vec3d_t *p0, const vec3d_t *p1);

/**
 * Calculate distance between two 3D points given by XYZ
 *
 * @param[in] x0  X value of point 1
 * @param[in] y0  Y value of point 1
 * @param[in] z0  Z value of point 1
 * @param[in] x1  X value of point 2
 * @param[in] y1  Y value of point 2
 * @param[in] z1  Z value of point 2
 * @return     Returns distance between the points
 */
double get_dist_xyz(const double x0, const double y0, const double z0,
                    const double x1, const double y1, const double z1);

/**
 * Returns true if given point is inside the polygon defined by an array of
 *   points
 *
 * @param[in] n   Number of points
 * @param[in] x   Array containing X coordinates
 * @param[in] y   Array containing Y coordinates
 * @param[in] tx  X coordinate of test point
 * @param[in] ty  Y coordinate of test point
 * @return  Returns true if given point is inside the polygon defined by an
 *   array of points
 */
int poly_contains_point(int n, float *x, float *y, float tx, float ty);

#endif /* _DWM_MATH_H_ */
