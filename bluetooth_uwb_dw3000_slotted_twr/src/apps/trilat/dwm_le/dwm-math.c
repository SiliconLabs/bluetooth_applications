/**
 * LEAPS - Low Energy Accurate Positioning System.
 *
 * Mathematic utilities.
 *
 * Copyright (c) 2016-2018, LEAPS. All rights reserved.
 *
 */

#include "dwm-math.h"

/* Described in header file */
unsigned long fact(int n)
{
  unsigned long rv = 1;
  int i;

  for (i = 1; i <= n; i++) {
    rv *= i;
  }

  return rv;
}

/* Described in header file */
float flog2(float val)
{
  ieee_float_shape_t gf_u = { val };
  float tmp = gf_u.word;

  tmp *= 1.0 / (1 << 23);

  return tmp - 126.94269504f;
}

/* Described in header file */
float flog10(float val)
{
  return 0.30102999566f * flog2(val);
}

static float fsqrt(float x)
{
  ieee_float_shape_t t;

  t.value = x;

  t.word = 0x5f3759df - (t.word >> 1);
  t.value = t.value * (1.5f - (0.5f * x * t.value * t.value));

  return (1 / t.value);
}

/* Described in header file */
double get_dist(const vec3d_t *p0, const vec3d_t *p1)
{
  return fsqrt(POW(p0->x - p1->x) + POW(p0->y - p1->y) + POW(p0->z - p1->z));
}

/* Described in header file */
double get_dist_xyz(const double x0, const double y0, const double z0,
                    const double x1, const double y1, const double z1)
{
  return fsqrt((float)(POW(x0 - x1) + POW(y0 - y1) + POW(z0 - z1)));
}

/* Described in header file */
int poly_contains_point(int n, float *x, float *y, float tx, float ty)
{
  int i, j, c = 0;

  for (i = 0, j = n - 1; i < n; j = i++) {
    if (((y[i] > ty) != (y[j] > ty))
        && (tx < (x[j] - x[i]) * (ty - y[i]) / (y[j] - y[i]) + x[i])) {
      c = !c;
    }
  }

  return c;
}
