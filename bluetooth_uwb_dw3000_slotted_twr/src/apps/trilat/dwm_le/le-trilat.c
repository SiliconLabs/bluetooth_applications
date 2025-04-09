/**
 * LEAPS - Low Energy Accurate Positioning System.
 *
 * Location Engine - Trilateration.
 * Based on codes from Decawave with improvements by LEAPS.
 *
 * Copyright (c) 2016-2018, Decawave Ltd, Dublin, Ireland. All rights reserved.
 * Copyright (c) 2016-2018, LEAPS. All rights reserved.
 *
 */

#if defined(CFG_LE_TRILAT) && (CFG_LE_TRILAT == 1)

#if defined(CFG_LE_TRILAT_DEBUG) && (CFG_LE_TRILAT_DEBUG == 1)
#include <stdio.h>
#endif /* CFG_LE_TRILAT_DEBUG */
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "le-trilat.h"

#if defined(CFG_LE_TRILAT_UTILS) && (CFG_LE_TRILAT_UTILS == 1)
static trilat_solver_t *_trilat_solver = NULL;
#endif /* CFG_LE_TRILAT_UTILS */

/* Largest nonnegative number still considered zero */
#define MAXZERO                            0.001

#define ERR_TRIL_CONCENTRIC                -1
#define ERR_TRIL_COLINEAR_2SOLUTIONS       -2
#define ERR_TRIL_SQRTNEGNUMB               -3
#define ERR_TRIL_NOINTERSECTION_SPHERE4    -4
#define ERR_TRIL_NEEDMORESPHERE            -5

#define CM_ERR_ADDED                       (10)

#define CFG_LEGACY                         1

#include "port_dw3000.h"
#include "port_common.h"
#include "deca_dbg.h"
#define TRILAT_MALLOC                      pvPortMalloc
#define TRILAT_FREE                        vPortFree

/* Return the difference of two vectors, (vector1 - vector2). */
static vec3d_t vdiff(const vec3d_t vector1, const vec3d_t vector2)
{
  vec3d_t v;

  v.x = vector1.x - vector2.x;
  v.y = vector1.y - vector2.y;
  v.z = vector1.z - vector2.z;

  return v;
}

/* Return the sum of two vectors. */
static vec3d_t vsum(const vec3d_t vector1, const vec3d_t vector2)
{
  vec3d_t v;

  v.x = vector1.x + vector2.x;
  v.y = vector1.y + vector2.y;
  v.z = vector1.z + vector2.z;

  return v;
}

/* Multiply vector by a number. */
static vec3d_t vmul(const vec3d_t vector, const double n)
{
  vec3d_t v;

  v.x = vector.x * n;
  v.y = vector.y * n;
  v.z = vector.z * n;

  return v;
}

/* Divide vector by a number. */
static vec3d_t vdiv(const vec3d_t vector, const double n)
{
  vec3d_t v;

  v.x = vector.x / n;
  v.y = vector.y / n;
  v.z = vector.z / n;

  return v;
}

/* Return the Euclidean norm. */
#if 0
static double vdist(const vec3d_t v1, const vec3d_t v2)
{
  double xd = v1.x - v2.x;
  double yd = v1.y - v2.y;
  double zd = v1.z - v2.z;

  return sqrt(xd * xd + yd * yd + zd * zd);
}

#endif

/* Return the Euclidean norm. */
static double vnorm(const vec3d_t vector)
{
  return sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
}

/* Return the dot product of two vectors. */
static double dot(const vec3d_t vector1, const vec3d_t vector2)
{
  return vector1.x * vector2.x + vector1.y * vector2.y + vector1.z * vector2.z;
}

/* Replace vector with its cross product with another vector. */
static vec3d_t cross(const vec3d_t vector1, const vec3d_t vector2)
{
  vec3d_t v;

  v.x = vector1.y * vector2.z - vector1.z * vector2.y;
  v.y = vector1.z * vector2.x - vector1.x * vector2.z;
  v.z = vector1.x * vector2.y - vector1.y * vector2.x;

  return v;
}

/* Return the GDOP (Geometric Dilution of Precision) rate between 0-1.
 * Lower GDOP rate means better precision of intersection.
 */
static double gdoprate(const vec3d_t tag,
                       const vec3d_t p1,
                       const vec3d_t p2,
                       const vec3d_t p3)
{
  vec3d_t ex, t1, t2, t3;
  double h, gdop1, gdop2, gdop3, result;

  ex = vdiff(p1, tag);
  h = vnorm(ex);
  t1 = vdiv(ex, h);

  ex = vdiff(p2, tag);
  h = vnorm(ex);
  t2 = vdiv(ex, h);

  ex = vdiff(p3, tag);
  h = vnorm(ex);
  t3 = vdiv(ex, h);

  gdop1 = fabs(dot(t1, t2));
  gdop2 = fabs(dot(t2, t3));
  gdop3 = fabs(dot(t3, t1));

  if (gdop1 < gdop2) {
    result = gdop2;
  } else {
    result = gdop1;
  }
  if (result < gdop3) {
    result = gdop3;
  }

  return result;
}

#if !defined(CFG_LEGACY) || (CFG_LEGACY == 0)

/* Intersecting a sphere sc with radius of r, with a line p1-p2.
 * Return zero if successful, negative error otherwise.
 * mu1 & mu2 are constant to find points of intersection.
 */
static int sphereline(const vec3d_t p1,
                      const vec3d_t p2,
                      const vec3d_t sc,
                      double r,
                      double *const mu1,
                      double *const mu2)
{
  double a, b, c;
  double bb4ac;
  vec3d_t dp;

  dp.x = p2.x - p1.x;
  dp.y = p2.y - p1.y;
  dp.z = p2.z - p1.z;

  a = dp.x * dp.x + dp.y * dp.y + dp.z * dp.z;

  b = 2 * (dp.x * (p1.x - sc.x) + dp.y * (p1.y - sc.y) + dp.z * (p1.z - sc.z));

  c = sc.x * sc.x + sc.y * sc.y + sc.z * sc.z;
  c += p1.x * p1.x + p1.y * p1.y + p1.z * p1.z;
  c -= 2 * (sc.x * p1.x + sc.y * p1.y + sc.z * p1.z);
  c -= r * r;

  bb4ac = b * b - 4 * a * c;

  if ((fabs(a) == 0) || (bb4ac < 0)) {
    *mu1 = 0;
    *mu2 = 0;
    return -1;
  }

  *mu1 = (-b + sqrt(bb4ac)) / (2 * a);
  *mu2 = (-b - sqrt(bb4ac)) / (2 * a);

  return 0;
}

#endif /* !CFG_LEGACY */

/* Return TRIL_3SPHERES if it is performed using 3 spheres and return
 * TRIL_4SPHERES if it is performed using 4 spheres
 * For TRIL_3SPHERES, there are two solutions: result1 and result2
 * For TRIL_4SPHERES, there is only one solution: best_solution
 *
 * Return negative number for other errors
 *
 * To force the function to work with only 3 spheres, provide a duplicate of
 * any sphere at any place among p1, p2, p3 or p4.
 *
 * The last parameter is the largest nonnegative number considered zero;
 * it is somewhat analogous to machine epsilon (but inclusive).
 */
int trilateration(vec3d_t *const result1,
                  vec3d_t *const result2,
                  vec3d_t *const best_solution,
                  const vec3d_t p1, const double r1,
                  const vec3d_t p2, const double r2,
                  const vec3d_t p3, const double r3,
                  const vec3d_t p4, const double r4,
                  const double maxzero)
{
  (void) best_solution;
  (void) p4; (void) r4;
  vec3d_t    ex, ey, ez, t1, t2;
  double    h, i, j, x, y, z, t;
#if !defined(CFG_LEGACY) || (CFG_LEGACY == 0)
  vec3d_t    t3;
  double    mu1, mu2, mu;
  int result;
#endif /* CFG_LEGACY */

  /*********** FINDING TWO POINTS FROM THE FIRST THREE SPHERES **********/

  // if there are at least 2 concentric spheres within the first 3 spheres
  // then the calculation may not continue, drop it with error -1

  /* h = |p3 - p1|, ex = (p3 - p1) / |p3 - p1| */
  ex = vdiff(p3, p1);   // vector p13
  h = vnorm(ex);   // scalar p13
  if (h <= maxzero) {
    /* p1 and p3 are concentric, not good to obtain a precise intersection point
     */
    // diag_printf("concentric13 return -1\n");
    return ERR_TRIL_CONCENTRIC;
  }

  /* h = |p3 - p2|, ex = (p3 - p2) / |p3 - p2| */
  ex = vdiff(p3, p2);   // vector p23
  h = vnorm(ex);   // scalar p23
  if (h <= maxzero) {
    /* p2 and p3 are concentric, not good to obtain a precise intersection point
     */
    // diag_printf("concentric23 return -1\n");
    return ERR_TRIL_CONCENTRIC;
  }

  /* h = |p2 - p1|, ex = (p2 - p1) / |p2 - p1| */
  ex = vdiff(p2, p1);   // vector p12
  h = vnorm(ex);   // scalar p12
  if (h <= maxzero) {
    /* p1 and p2 are concentric, not good to obtain a precise intersection point
     */
    // diag_printf("concentric12 return -1\n");
    return ERR_TRIL_CONCENTRIC;
  }
  ex = vdiv(ex, h);   // unit vector ex with respect to p1 (new coordinate
                      //   system)

  /* t1 = p3 - p1, t2 = ex (ex . (p3 - p1)) */
  t1 = vdiff(p3, p1);   // vector p13
  i = dot(ex, t1);   // the scalar of t1 on the ex direction
  t2 = vmul(ex, i);   // colinear vector to p13 with the length of i

  /* ey = (t1 - t2), t = |t1 - t2| */
  ey = vdiff(t1, t2);   // vector t21 perpendicular to t1
  t = vnorm(ey);   // scalar t21
  if (t > maxzero) {
    /* ey = (t1 - t2) / |t1 - t2| */
    ey = vdiv(ey, t);     // unit vector ey with respect to p1 (new coordinate
                          //   system)

    /* j = ey . (p3 - p1) */
    j = dot(ey, t1);     // scalar t1 on the ey direction
  } else {
    j = 0.0;
  }

  /* Note: t <= maxzero implies j = 0.0. */
  if (fabs(j) <= maxzero) {
    /* Is point p1 + (r1 along the axis) the intersection? */
    t2 = vsum(p1, vmul(ex, r1));
    if ((fabs(vnorm(vdiff(p2, t2)) - r2) <= maxzero)
        && (fabs(vnorm(vdiff(p3, t2)) - r3) <= maxzero)) {
      /* Yes, t2 is the only intersection point. */
      if (result1) {
        *result1 = t2;
      }
      if (result2) {
        *result2 = t2;
      }
      return TRIL_3SPHERES;
    }

    /* Is point p1 - (r1 along the axis) the intersection? */
    t2 = vsum(p1, vmul(ex, -r1));
    if ((fabs(vnorm(vdiff(p2, t2)) - r2) <= maxzero)
        && (fabs(vnorm(vdiff(p3, t2)) - r3) <= maxzero)) {
      /* Yes, t2 is the only intersection point. */
      if (result1) {
        *result1 = t2;
      }
      if (result2) {
        *result2 = t2;
      }
      return TRIL_3SPHERES;
    }

    /* p1, p2 and p3 are colinear with more than one solution */
    return ERR_TRIL_COLINEAR_2SOLUTIONS;
  }

  /* ez = ex x ey */
  ez = cross(ex, ey);   // unit vector ez with respect to p1 (new coordinate
                        //   system)

  x = (r1 * r1 - r2 * r2) / (2 * h) + h / 2;
  y = (r1 * r1 - r3 * r3 + i * i) / (2 * j) + j / 2 - x * i / j;
  z = r1 * r1 - x * x - y * y;
  if (z < -maxzero) {
    /* The solution is invalid, square root of negative number */
    return ERR_TRIL_SQRTNEGNUMB;
  } else
  if (z > 0.0) {
    z = sqrt(z);
  } else {
    z = 0.0;
  }

  /* t2 = p1 + x ex + y ey */
  t2 = vsum(p1, vmul(ex, x));
  t2 = vsum(t2, vmul(ey, y));

  /* result1 = p1 + x ex + y ey + z ez */
  if (result1) {
    *result1 = vsum(t2, vmul(ez, z));
  }

  /* result1 = p1 + x ex + y ey - z ez */
  if (result2) {
    *result2 = vsum(t2, vmul(ez, -z));
  }

  /*********** END OF FINDING TWO POINTS FROM THE FIRST THREE SPHERES
   *   **********/
  /********* RESULT1 AND RESULT2 ARE SOLUTIONS, OTHERWISE RETURN ERROR
   *   *********/

#if defined(CFG_LEGACY) && (CFG_LEGACY == 1)
  return TRIL_3SPHERES;
#else

  /************* FINDING ONE SOLUTION BY INTRODUCING ONE MORE SPHERE
   *   ***********/

  // check for concentricness of sphere 4 to sphere 1, 2 and 3
  // if it is concentric to one of them, then sphere 4 cannot be used
  // to determine the best solution and return -1

  /* h = |p4 - p1|, ex = (p4 - p1) / |p4 - p1| */
  ex = vdiff(p4, p1);   // vector p14
  h = vnorm(ex);   // scalar p14
  if (h <= maxzero) {
    /* p1 and p4 are concentric, not good to obtain a precise intersection point
     */
    // diag_printf("concentric14 return 0\n");
    return TRIL_3SPHERES;
  }

  /* h = |p4 - p2|, ex = (p4 - p2) / |p4 - p2| */
  ex = vdiff(p4, p2);   // vector p24
  h = vnorm(ex);   // scalar p24
  if (h <= maxzero) {
    /* p2 and p4 are concentric, not good to obtain a precise intersection point
     */
    // diag_printf("concentric24 return 0\n");
    return TRIL_3SPHERES;
  }

  /* h = |p4 - p3|, ex = (p4 - p3) / |p4 - p3| */
  ex = vdiff(p4, p3);   // vector p34
  h = vnorm(ex);   // scalar p34
  if (h <= maxzero) {
    /* p3 and p4 are concentric, not good to obtain a precise intersection point
     */
    // diag_printf("concentric34 return 0\n");
    return TRIL_3SPHERES;
  }

  // if sphere 4 is not concentric to any sphere, then best solution can be
  //   obtained

  /* find i as the distance of result1 to p4 */
  t3 = vdiff(*result1, p4);
  i = vnorm(t3);

  /* find h as the distance of result2 to p4 */
  t3 = vdiff(*result2, p4);
  h = vnorm(t3);

  /* pick the result1 as the nearest point to the center of sphere 4 */
  if (i > h) {
    *best_solution = *result1;
    *result1 = *result2;
    *result2 = *best_solution;
  }

  int count4 = 0;
  double rr4 = r4;
  result = 1;

  /* intersect result1-result2 vector with sphere 4 */
  while (result && count4 < 10)
  {
    result = sphereline(*result1, *result2, p4, rr4, &mu1, &mu2);
    rr4 += 0.1;
    count4++;
  }

  if (result) {
    /* No intersection between sphere 4 and the line with the gradient of
     *   result1-result2! */
    *best_solution = *result1;     // result1 is the closer solution to sphere 4
    // return ERR_TRIL_NOINTERSECTION_SPHERE4;
  } else {
    if ((mu1 < 0) && (mu2 < 0)) {
      /* if both mu1 and mu2 are less than 0
       * result1-result2 line segment is outside sphere 4 with no intersection
       */
      if (fabs(mu1) <= fabs(mu2)) {
        mu = mu1;
      } else {
        mu = mu2;
      }

      /* h = |result2 - result1|, ex = (result2 - result1) / |result2 - result1|
       */
      ex = vdiff(*result2, *result1);       // vector result1-result2
      h = vnorm(ex);       // scalar result1-result2
      ex = vdiv(ex, h);       // unit vector ex with respect to result1 (new
                              //   coordinate system)

      /* 50-50 error correction for mu */
      mu = 0.5 * mu;

      /* t2 points to the intersection */
      t2 = vmul(ex, mu * h);
      t2 = vsum(*result1, t2);

      /* the best solution = t2 */
      *best_solution = t2;
    } else if (((mu1 < 0) && (mu2 > 1)) || ((mu2 < 0) && (mu1 > 1))) {
      /* if mu1 is less than zero and mu2 is greater than 1,
       * or the other way around
       * result1-result2 line segment is inside sphere 4 with no intersection
       */
      if (mu1 > mu2) {
        mu = mu1;
      } else {
        mu = mu2;
      }

      /* h = |result2 - result1|, ex = (result2 - result1) / |result2 - result1|
       */
      ex = vdiff(*result2, *result1);       // vector result1-result2
      h = vnorm(ex);       // scalar result1-result2
      ex = vdiv(ex, h);       // unit vector ex with respect to result1 (new
                              //   coordinate system)

      /* t2 points to the intersection */
      t2 = vmul(ex, mu * h);
      t2 = vsum(*result1, t2);

      /* vector t2-result2 with 50-50 error correction on the length of t3 */
      t3 = vmul(vdiff(*result2, t2), 0.5);

      /* the best solution = t2 + t3 */
      *best_solution = vsum(t2, t3);
    } else if ((((mu1 > 0) && (mu1 < 1)) && ((mu2 < 0) || (mu2 > 1)))
               || (((mu2 > 0) && (mu2 < 1)) && ((mu1 < 0) || (mu1 > 1)))) {
      /* if one mu is between 0 to 1 and the other is not */
      /* result1-result2 line segment intersects sphere 4 at one point */
      if ((mu1 >= 0) && (mu1 <= 1)) {
        mu = mu1;
      } else {
        mu = mu2;
      }

      /* add or subtract with 0.5*mu to distribute error equally onto every
       *   sphere */
      if (mu <= 0.5) {
        mu -= 0.5 * mu;
      } else {
        mu -= 0.5 * (1 - mu);
      }

      /* h = |result2 - result1|, ex = (result2 - result1) / |result2 - result1|
       */
      ex = vdiff(*result2, *result1);       // vector result1-result2
      h = vnorm(ex);       // scalar result1-result2
      ex = vdiv(ex, h);       // unit vector ex with respect to result1 (new
                              //   coordinate system)

      /* t2 points to the intersection */
      t2 = vmul(ex, mu * h);
      t2 = vsum(*result1, t2);

      /* the best solution = t2 */
      *best_solution = t2;
    } else if (mu1 == mu2) {
      /* if both mu1 and mu2 are between 0 and 1, and mu1 = mu2 */
      /* result1-result2 line segment is tangential to sphere 4 at one point */
      mu = mu1;

      /* add or subtract with 0.5*mu to distribute error equally onto every
       *   sphere */
      if (mu <= 0.25) {
        mu -= 0.5 * mu;
      } else if (mu <= 0.5) {
        mu -= 0.5 * (0.5 - mu);
      } else if (mu <= 0.75) {
        mu -= 0.5 * (mu - 0.5);
      } else {
        mu -= 0.5 * (1 - mu);
      }

      /* h = |result2 - result1|, ex = (result2 - result1) / |result2 - result1|
       */
      ex = vdiff(*result2, *result1);       // vector result1-result2
      h = vnorm(ex);       // scalar result1-result2
      ex = vdiv(ex, h);       // unit vector ex with respect to result1 (new
                              //   coordinate system)

      /* t2 points to the intersection */
      t2 = vmul(ex, mu * h);
      t2 = vsum(*result1, t2);

      /* the best solution = t2 */
      *best_solution = t2;
    } else {
      /* if both mu1 and mu2 are between 0 and 1 */
      /* result1-result2 line segment intersects sphere 4 at two points */

      // return ERR_TRIL_NEEDMORESPHERE;

      mu = mu1 + mu2;

      /* h = |result2 - result1|, ex = (result2 - result1) / |result2 - result1|
       */
      ex = vdiff(*result2, *result1);       // vector result1-result2
      h = vnorm(ex);       // scalar result1-result2
      ex = vdiv(ex, h);       // unit vector ex with respect to result1 (new
                              //   coordinate system)

      /* 50-50 error correction for mu */
      mu = 0.5 * mu;

      /* t2 points to the intersection */
      t2 = vmul(ex, mu * h);
      t2 = vsum(*result1, t2);

      /* the best solution = t2 */
      *best_solution = t2;
    }
  }

  return TRIL_4SPHERES;

  /******* END OF FINDING ONE SOLUTION BY INTRODUCING ONE MORE SPHERE ********/
#endif
}

#if !defined(CFG_LEGACY) || (CFG_LEGACY == 0)

/* This function calls trilateration to get the best solution.
 *
 * If any three spheres does not produce valid solution,
 * then each distance is increased to ensure intersection to happens.
 *
 * Return the selected trilateration mode between TRIL_3SPHERES or TRIL_4SPHERES
 * For TRIL_3SPHERES, there are two solutions: solution1 and solution2
 * For TRIL_4SPHERES, there is only one solution: best_solution
 *
 * nosolution_count = the number of failed attempt before intersection is found
 * by increasing the sphere diameter.
 */
int deca_3dlocate(vec3d_t    *const solution1,
                  vec3d_t    *const solution2,
                  vec3d_t    *const best_solution,
                  int        *const nosolution_count,
                  double    *const best_3derror,
                  double    *const best_gdoprate,
                  vec3d_t p1, double r1,
                  vec3d_t p2, double r2,
                  vec3d_t p3, double r3,
                  vec3d_t p4, double r4,
                  int *combination)
{
  vec3d_t    o1, o2, solution, ptemp;
  // vec3d_t    solution_compare1, solution_compare2;
  double    /*error_3dcompare1, error_3dcompare2,*/ rtemp;
  double    gdoprate_compare1, gdoprate_compare2;
  double    ovr_r1, ovr_r2, ovr_r3, ovr_r4;
  int        overlook_count, combination_counter;
  int        trilateration_errcounter, trilateration_mode34;
  int        success, concentric, result;

  trilateration_errcounter = 0;
  trilateration_mode34 = 0;

  combination_counter = 4;   /* four spheres combination */

  *best_gdoprate = 1;   /* put the worst gdoprate init */
  gdoprate_compare1 = 1; gdoprate_compare2 = 1;
  // solution_compare1.x = 0; solution_compare1.y = 0; solution_compare1.z = 0;
  // error_3dcompare1 = 0;

  do {
    success = 0;
    concentric = 0;
    overlook_count = 0;
    ovr_r1 = r1; ovr_r2 = r2; ovr_r3 = r3; ovr_r4 = r4;

    do {
      result = trilateration(&o1,
                             &o2,
                             &solution,
                             p1,
                             ovr_r1,
                             p2,
                             ovr_r2,
                             p3,
                             ovr_r3,
                             p4,
                             ovr_r4,
                             MAXZERO);

      switch (result)
      {
        case TRIL_3SPHERES:         // 3 spheres are used to get the result
          trilateration_mode34 = TRIL_3SPHERES;
          success = 1;
          break;

        case TRIL_4SPHERES:         // 4 spheres are used to get the result
          trilateration_mode34 = TRIL_4SPHERES;
          success = 1;
          break;

        case ERR_TRIL_CONCENTRIC:
          concentric = 1;
          break;

        default:         // any other return value goes here
          ovr_r1 += 0.10;
          ovr_r2 += 0.10;
          ovr_r3 += 0.10;
          ovr_r4 += 0.10;
          overlook_count++;
          break;
      }

      // qDebug() << "while(!success)" << overlook_count << concentric <<
      //   "result" << result;
    } while (!success && (overlook_count <= CM_ERR_ADDED) && !concentric);

//        if(success)
//            qDebug() << "Location" << ovr_r1 << ovr_r2 << ovr_r3 << ovr_r4 <<
//   "+err=" << overlook_count;
//        else
//            qDebug() << "No Location" << ovr_r1 << ovr_r2 << ovr_r3 << ovr_r4
//   << "+err=" << overlook_count;

    if (success) {
      switch (result)
      {
        case TRIL_3SPHERES:
          *solution1 = o1;
          *solution2 = o2;
          *nosolution_count = overlook_count;

          combination_counter = 0;
          break;

        case TRIL_4SPHERES:
          /* calculate the new gdop */
          gdoprate_compare1 = gdoprate(solution, p1, p2, p3);

          /* compare and swap with the better result */
          if (gdoprate_compare1 <= gdoprate_compare2) {
            *solution1 = o1;
            *solution2 = o2;
            *best_solution = solution;
            *nosolution_count = overlook_count;
            *best_3derror =
              sqrt((vnorm(vdiff(solution,
                                p1)) - r1) * (vnorm(vdiff(solution, p1)) - r1)
                   + (vnorm(vdiff(solution,
                                  p2)) - r2) * (vnorm(vdiff(solution, p2)) - r2)
                   + (vnorm(vdiff(solution,
                                  p3)) - r3) * (vnorm(vdiff(solution, p3)) - r3)
                   + (vnorm(vdiff(solution,
                                  p4)) - r4)
                   * (vnorm(vdiff(solution, p4)) - r4));
            *best_gdoprate = gdoprate_compare1;

            /* save the previous result */
            // solution_compare2 = solution_compare1;
            // error_3dcompare2 = error_3dcompare1;
            gdoprate_compare2 = gdoprate_compare1;
          }
          *combination = 5 - combination_counter;

          ptemp = p1; p1 = p2; p2 = p3; p3 = p4; p4 = ptemp;
          rtemp = r1; r1 = r2; r2 = r3; r3 = r4; r4 = rtemp;
          combination_counter--;
          break;

        default:
          break;
      }
    } else {
      // trilateration_errcounter++;
      trilateration_errcounter = 4;
      combination_counter = 0;
    }

    // ptemp = p1; p1 = p2; p2 = p3; p3 = p4; p4 = ptemp;
    // rtemp = r1; r1 = r2; r2 = r3; r3 = r4; r4 = rtemp;
    // combination_counter--;
    // qDebug() << "while(combination_counter)" << combination_counter;
  } while (combination_counter);

  // if it gives error for all 4 sphere combinations then no valid result is
  //   given
  // otherwise return the trilateration mode used
  if (trilateration_errcounter >= 4) {
    return -1;
  } else {
    return trilateration_mode34;
  }
}

int GetLocation(vec3d_t *best_solution,
                int use4thAnchor,
                vec3d_t *anchorArray,
                int *distanceArray)
{
  vec3d_t    o1, o2, p1, p2, p3, p4;
  double    r1 = 0, r2 = 0, r3 = 0, r4 = 0, best_3derror, best_gdoprate;
  int        result;
  int     error, combination;

//     vec3d_t    t3;
//     double    dist1, dist2;

  /* Anchors coordinate */
  p1.x = anchorArray[0].x;
  p1.y = anchorArray[0].y;
  p1.z = anchorArray[0].z;

  p2.x = anchorArray[1].x;
  p2.y = anchorArray[1].y;
  p2.z = anchorArray[1].z;

  p3.x = anchorArray[2].x;
  p3.y = anchorArray[2].y;
  p3.z = anchorArray[2].z;

  p4.x = anchorArray[3].x;
  p4.y = anchorArray[3].y;
  p4.z = anchorArray[3].z; // 4th same as 1st - only 3 used for trilateration

  r1 = (double) distanceArray[0] / 1000.0;
  r2 = (double) distanceArray[1] / 1000.0;
  r3 = (double) distanceArray[2] / 1000.0;

  r4 = (double) distanceArray[3] / 1000.0;

  // qDebug() << "GetLocation" << r1 << r2 << r3 << r4;

  // r4 = r1;

  /* get the best location using 3 or 4 spheres and keep it as
   *   know_best_location */
  result = deca_3dlocate(&o1,
                         &o2,
                         best_solution,
                         &error,
                         &best_3derror,
                         &best_gdoprate,
                         p1,
                         r1,
                         p2,
                         r2,
                         p3,
                         r3,
                         p4,
                         r4,
                         &combination);

  // qDebug() << "GetLocation" << result << "sol1: " << o1.x << o1.y << o1.z <<
  //   " sol2: " << o2.x << o2.y << o2.z;

  if (result >= 0) {
//         if (use4thAnchor == 1) //if have 4 ranging results, then use 4th
//   anchor to pick solution closest to it
//         {
//                 double diff1, diff2;
//                 /* find dist1 as the distance of o1 to known_best_location */
//                 t3 = vdiff(o1, anchorArray[3]);
//                 dist1 = vnorm(t3);
//
//                 t3 = vdiff(o2, anchorArray[3]);
//                 dist2 = vnorm(t3);
//
//                 /* find the distance closest to received range measurement
//   from 4th anchor */
//                 diff1 = fabs(r4 - dist1);
//                 diff2 = fabs(r4 - dist2);
//
//                 /* pick the closest match to the 4th anchor range */
//                 if (diff1 < diff2) *best_solution = o1; else *best_solution =
//   o2;
//         }
//         else
//         {
    // assume tag is below the anchors (1, 2, and 3)
    if (o1.z < p1.z) {
      *best_solution = o1;
    } else {
      *best_solution = o2;
    }
//         }
  }

  if (result >= 0) {
    return result;
  }

  // return error
  return -1;
}

#endif /* !CFG_LEGACY */

#define CFG_FSQRT    0

#if defined(CFG_FSQRT) && (CFG_FSQRT == 1)
static float fsqrt(float x)
{
  ieee_float_shape_t t;

  t.value = x;

  t.word = 0x5f3759df - (t.word >> 1);
  t.value = t.value * (1.5f - (0.5f * x * t.value * t.value));

  return (1 / t.value);
}

#define _sqrt(x)    fsqrt(x)
#else
#define _sqrt(x)    sqrt(x)
#endif /* CFG_FSQRT */

static double _get_dist(const vec3d_t *p0, const vec3d_t *p1)
{
#if defined(CFG_FSQRT) && (CFG_FSQRT == 1)
  return _sqrt(POW(p0->x - p1->x) + POW(p0->y - p1->y) + POW(p0->z - p1->z));
#else
  float x1, x2, y1, y2, z1, z2;
  float fpow;
  float frv;

  x1 = p0->x;
  x2 = p1->x;
  y1 = p0->y;
  y2 = p1->y;
  z1 = p0->z;
  z2 = p1->z;

  fpow = POW(x1 - x2) + POW(y1 - y2) + POW(z1 - z2);
  frv = _sqrt(fpow);

  return frv;
#endif /* CFG_FSQRT */
}

#if defined(CFG_LE_TRILAT_QSORT) && (CFG_LE_TRILAT_QSORT == 1)
static int cmpdiff(const void *a, const void *b)
{
  trilat_result_t *a_res = (trilat_result_t *)a;
  trilat_result_t *b_res = (trilat_result_t *)b;

  return (fabs(a_res->e) > fabs(b_res->e));
}

static void trilat_solver_sort_res(trilat_result_t *res, int res_cnt)
{
  qsort(res, res_cnt, sizeof(trilat_result_t), cmpdiff);
}

#else

static void trilat_solver_sort_res(trilat_result_t *res, int res_cnt)
{
  trilat_result_t tres;
  int i, j;

  for (i = 1; i < res_cnt; i++) {
    for (j = 0; j < res_cnt - 1; j++) {
      if (fabs(res[j].e) > fabs(res[i].e)) {
        memcpy(&tres, &res[i], sizeof(trilat_result_t));
        memcpy(&res[i], &res[j], sizeof(trilat_result_t));
        memcpy(&res[j], &tres, sizeof(trilat_result_t));
      }
    }
  }
}

#endif /* CFG_LE_TRILAT_QSORT */

#if defined(CFG_LE_TRILAT_DEBUG) && (CFG_LE_TRILAT_DEBUG == 1)
static void trilat_results_dump2(trilat_solver_t *trilat_solver,
                                 trilat_result_t *res,
                                 trilat_mask_t *mask,
                                 int res_cnt,
                                 vec3d_t *est,
                                 double *min_e,
                                 double *max_e,
                                 double *r)
{
  static int first = 1;

  if (first) {
    first = 0;
    diag_printf("res:tot | variant | dmeas | "
                "est[0]=x:y:z / err | est[1]=x:y:z / err | est=x:y:z / err\n");
  }

  diag_printf("%3d:%3d | %d:%d:%d | %7.3f:%7.3f:%7.3f | "
              "%7.3f:%7.3f:%7.3f / <%7.3f;%7.3f> | "
              "%7.3f:%7.3f:%7.3f / <%7.3f;%7.3f> | ",
              res_cnt, trilat_solver->trilat_cnt,
              mask->v[0], mask->v[1], mask->v[2],
              r[0], r[1], r[2],
              est[0].x, est[0].y, est[0].z, min_e[0], max_e[0],
              est[1].x, est[1].y, est[1].z, min_e[1], max_e[1]);

  if (res) {
    diag_printf("%7.3f:%7.3f:%7.3f / %7.3f\n",
                res->est.x, res->est.y, res->est.z, res->e);
  } else {
    diag_printf("rejected\n");
  }
}

/**
 * Dump possible solutions
 *
 * \param trilat_res  Pointer to solution structure
 * \param cnt       Number of solutions
 */
static void trilat_results_dump(trilat_result_t *trilat_res, int cnt)
{
  int i;

  diag_printf("res | %d\n", cnt);
  for (i = 0; i < cnt; i++) {
    diag_printf("%3d | %d:%d:%d | %7.3f:%7.3f:%7.3f / %7.3f\n", i,
                trilat_res[i].mask.v[0], trilat_res[i].mask.v[1],
                trilat_res[i].mask.v[2],
                trilat_res[i].est.x, trilat_res[i].est.y,
                trilat_res[i].est.z, trilat_res[i].e);
  }
}

#else
static void trilat_results_dump2(trilat_solver_t *trilat_solver,
                                 trilat_result_t *res,
                                 trilat_mask_t *mask, int res_cnt,
                                 vec3d_t *est,
                                 double *min_e, double *max_e, double *r)
{
  (void) trilat_solver; (void) res; (void) mask;
  (void) res_cnt; (void) est; (void) *min_e; (void) max_e; (void) r;
}

static void trilat_results_dump(trilat_result_t *trilat_res, int cnt)
{
  (void) trilat_res; (void) cnt;
}

#endif /* CFG_LE_TRILAT_DEBUG */

/* Described in header file */
int trilat_solver_init(trilat_solver_t *trilat_solver,
                       const vec3d_t *bn_pos,
                       unsigned int bn_cnt)
{
  vec3d_t _bn_pos[CFG_TRILAT_MEAS_PER_CALC];
  int a[CFG_TRILAT_MEAS_PER_CALC];
  trilat_t *trilat;
  int trilat_cnt = 0;
  int i;

  /* Reset the solver */
  memset(trilat_solver, 0, sizeof(trilat_solver_t));

  /* Make a local copy of base node positions */
  trilat_solver->bn_pos = (vec3d_t *)TRILAT_MALLOC(sizeof(vec3d_t) * bn_cnt);
  if (trilat_solver->bn_pos == NULL) {
    return -10;
  }
  memcpy(trilat_solver->bn_pos, bn_pos, sizeof(vec3d_t) * bn_cnt);
  trilat_solver->bn_cnt = bn_cnt;

  trilat_solver->trilat_cnt = fact(bn_cnt) / (fact(bn_cnt - 3) * 6);

  if (trilat_solver->trilat_cnt > CFG_TRILAT_MAX_CACHE) {
    trilat_solver->trilat_cnt = CFG_TRILAT_MAX_CACHE;
  }

  trilat_solver->trilat = (trilat_t *)TRILAT_MALLOC(
    sizeof(trilat_t) * trilat_solver->trilat_cnt);
  if (trilat_solver->trilat == NULL) {
    TRILAT_FREE(trilat_solver->bn_pos);
    return -11;
  }

  /* Find the boundary of covered area */
  for (i = 0; i < (int)bn_cnt; i++) {
    if (trilat_solver->bn_pos[i].x < trilat_solver->min.x) {
      trilat_solver->min.x = trilat_solver->bn_pos[i].x;
    }
    if (trilat_solver->bn_pos[i].x > trilat_solver->max.x) {
      trilat_solver->max.x = trilat_solver->bn_pos[i].x;
    }

    if (trilat_solver->bn_pos[i].y < trilat_solver->min.y) {
      trilat_solver->min.y = trilat_solver->bn_pos[i].y;
    }
    if (trilat_solver->bn_pos[i].y > trilat_solver->max.y) {
      trilat_solver->max.y = trilat_solver->bn_pos[i].y;
    }

    if (trilat_solver->bn_pos[i].z < trilat_solver->min.z) {
      trilat_solver->min.z = trilat_solver->bn_pos[i].z;
    }
    if (trilat_solver->bn_pos[i].z > trilat_solver->max.z) {
      trilat_solver->max.z = trilat_solver->bn_pos[i].z;
    }
  }

  trilat_solver->center.x = (trilat_solver->max.x + trilat_solver->min.x) / 2;
  trilat_solver->center.y = (trilat_solver->max.y + trilat_solver->min.y) / 2;
  trilat_solver->center.z = (trilat_solver->max.z + trilat_solver->min.z) / 2;

  if (trilat_solver->center.z) {
    trilat_solver->min.z -= 1.0;
    trilat_solver->max.z += 1.0;
  }

  /* Create cache */
  for (a[0] = 0; a[0] < (int)bn_cnt; a[0]++) {
    for (a[1] = a[0] + 1; a[1] < (int)bn_cnt; a[1]++) {
      for (a[2] = a[1] + 1; a[2] < (int)bn_cnt; a[2]++) {
        if (trilat_cnt >= trilat_solver->trilat_cnt) {
          break;
        }

        trilat = &trilat_solver->trilat[trilat_cnt];
        memset(trilat, 0, sizeof(trilat_t));

        for (i = 0; i < CFG_TRILAT_MEAS_PER_CALC; i++) {
          memcpy(&_bn_pos[i], &trilat_solver->bn_pos[a[i]], sizeof(vec3d_t));
          trilat->mask.v[i] = a[i];
        }

#if defined(CFG_LE_TRILAT_DEBUG) && (CFG_LE_TRILAT_DEBUG == 1)
        diag_printf("%2d:anchor:x:y:z | variant %d:%d:%d\n",
                    trilat_cnt, a[0], a[1], a[2]);

        for (i = 0; i < CFG_TRILAT_MEAS_PER_CALC; i++) {
          diag_printf("%2d:%2d:%7.3f:%7.3f:%7.3f\n",
                      trilat_cnt, trilat->mask.v[i],
                      trilat_solver->bn_pos[trilat->mask.v[i]].x,
                      trilat_solver->bn_pos[trilat->mask.v[i]].y,
                      trilat_solver->bn_pos[trilat->mask.v[i]].z);
        }
        diag_printf("\n");
#endif /* CFG_LE_TRILAT_DEBUG */

        trilat_cnt++;
      }
    }
  }

  return 0;
}

/* Described in header file */
int trilat_solver_get_pos(trilat_solver_t *trilat_solver,
                          vec3d_t *mn_pos,
                          uint8_t *qf,
                          const double *meas,
                          unsigned int meas_cnt)
{
  trilat_result_t *res;
  int res_cnt = 0;
  int res_tot;
  double _r[4];
  double _e;
  double _mae[2];
  double _mie[2];
  double _rej[2];
  vec3d_t est[2];
  int mres_cnt = 0;
  int rc = 0;
  int mi = 0;
  int i, j, k;
  int rv = 0;

  /* Total number of solutions */
  res_tot = trilat_solver->trilat_cnt * 2;
  if (res_tot > CFG_TRILAT_MAX_RESULTS) {
    res_tot = CFG_TRILAT_MAX_RESULTS;
  }
  res = (trilat_result_t *)TRILAT_MALLOC(sizeof(trilat_result_t) * res_tot);
  if (res == NULL) {
    return -20;
  }

  /* Run through all trilat variants and calculate positions */
  for (k = 0; (k < trilat_solver->trilat_cnt) && (res_cnt < res_tot); k++) {
    /* Get variants of the measurements */
    _r[0] = meas[trilat_solver->trilat[k].mask.v[0]];
    _r[1] = meas[trilat_solver->trilat[k].mask.v[1]];
    _r[2] = meas[trilat_solver->trilat[k].mask.v[2]];

    /* Calculate position for the given variant */
#if 0
    rv = trilat_get_pos(trilat_solver, &trilat_solver->trilat[k],
                        est, _r, CFG_TRILAT_MEAS_PER_CALC);
#else
    vec3d_t solution;
    int result;
    int overlook_count = 0;
    int concentric = 0;
    double _ar[4];

    memset(_ar, 0, sizeof(_ar));

    do {
      result = trilateration(&est[0], &est[1], &solution,
                             trilat_solver->bn_pos[trilat_solver->trilat[k].mask
                                                   .v[0]], _r[0] + _ar[0],
                             trilat_solver->bn_pos[trilat_solver->trilat[k].mask
                                                   .v[1]], _r[1] + _ar[1],
                             trilat_solver->bn_pos[trilat_solver->trilat[k].mask
                                                   .v[2]], _r[2] + _ar[2],
                             trilat_solver->bn_pos[trilat_solver->trilat[k].mask
                                                   .v[0]], _r[0] + _ar[0],
                             MAXZERO);

      switch (result) {
        case TRIL_3SPHERES:
          rv = 0;
          break;

        case ERR_TRIL_CONCENTRIC:
          rv = -101;
          concentric = 1;
          break;

        default:
          rv = -100;
          _ar[0] += 0.10;
          _ar[1] += 0.10;
          _ar[2] += 0.10;
          overlook_count++;
          break;
      }
    } while ((rv < 0) && (overlook_count <= CM_ERR_ADDED) && !concentric);
#endif

    if (rv < 0) {
      /* No solution found */
      continue;
    }

    for (i = 0; i < 2; i++) {
      _mae[i] = 0;
      _rej[i] = 0;
      _mie[i] = -1;

      for (j = 0; j < (int)meas_cnt; j++) {
        _e = meas[j] - _get_dist(&est[i], &trilat_solver->bn_pos[j]);

        if (_e < CFG_TRILAT_MIN_NEG_DIFF_M) {
          _rej[i] = 1;
          _mae[i] = _e;
          break;
        }

        if ((j == trilat_solver->trilat[k].mask.v[0])
            || (j == trilat_solver->trilat[k].mask.v[1])
            || (j == trilat_solver->trilat[k].mask.v[2])) {
          if (fabs(_e) > fabs(_mae[i])) {
            _mae[i] = _e;
          }
        } else {
          if ((_mie[i] == -1) || (fabs(_e) < fabs(_mie[i]))) {
            _mie[i] = _e;

            if (fabs(_mie[i]) == 0) {
              break;
            }
          }
        }
      }
    }

    if ((_rej[0] && _rej[1])
        || ((fabs(_mie[0]) > CFG_TRILAT_MAX_DIFF_M)
            && (fabs(_mie[1]) > CFG_TRILAT_MAX_DIFF_M))) {
      trilat_results_dump2(trilat_solver,
                           NULL,
                           &trilat_solver->trilat[k].mask,
                           res_cnt,
                           est,
                           _mie,
                           _mae,
                           _r);
      continue;
    }

    _mie[0] = floor(_mie[0] * 1000000) / 1000000;
    _mie[1] = floor(_mie[1] * 1000000) / 1000000;
    if ((est[0].z >= trilat_solver->min.z)
        && (est[0].z <= trilat_solver->max.z)
        && (est[1].z >= trilat_solver->min.z)
        && (est[1].z <= trilat_solver->max.z)) {
      if (fabs(_mie[0] - _mie[1]) < CFG_TRILAT_MIN_ERR_M) {
        if (fabs(est[0].z - trilat_solver->center.z)
            <= fabs(est[1].z - trilat_solver->center.z)) {
          i = 0;
        } else {
          i = 1;
        }
      } else if (fabs(_mie[0]) < fabs(_mie[1])) {
        i = 0;
      } else {
        i = 1;
      }
    } else if ((est[0].z >= trilat_solver->min.z)
               && (est[0].z <= trilat_solver->max.z)) {
      i = 0;
    } else if ((est[1].z >= trilat_solver->min.z)
               && (est[1].z <= trilat_solver->max.z)) {
      i = 1;
    } else if (fabs(_mie[0]) <= fabs(_mie[1])) {
      i = 0;
    } else {
      i = 1;
    }

    memcpy(&res[res_cnt].est, &est[i], sizeof(vec3d_t));
    res[res_cnt].e = _mie[i];

    res[res_cnt].mask.val = trilat_solver->trilat[k].mask.val;

    trilat_results_dump2(trilat_solver,
                         &res[res_cnt],
                         &trilat_solver->trilat[k].mask,
                         res_cnt,
                         est,
                         _mie,
                         _mae,
                         _r);

    res_cnt++;
  }

  if (res_cnt <= 0) {
    rv = -21;
    goto out;
  }

  rv = 0;
  trilat_solver_sort_res(res, res_cnt);
  trilat_results_dump(res, res_cnt);

  for (i = 0; (i < res_cnt) && (res_cnt > 1); i++) {
    if (fabs((fabs(res[0].e) - fabs(res[i].e))) > CFG_TRILAT_MIN_DIFF_M) {
      break;
    }

    rc = 1;
    memcpy(&est[0], &res[i].est, sizeof(vec3d_t));
    for (j = i + 1; j < res_cnt; j++) {
      if (_get_dist(&res[i].est, &res[j].est) > CFG_TRILAT_MIN_DIFF_M) {
        continue;
      }

      est[0].x += res[j].est.x;
      est[0].y += res[j].est.y;
      est[0].z += res[j].est.z;
      res[j].e = CFG_TRILAT_MAX_DIFF_M * 2;
      rc++;
    }

    if (rc > mres_cnt) {
      mres_cnt = rc;
      mi = i;
    }

    if (rc > 1) {
      res[i].est.x = est[0].x / rc;
      res[i].est.x = est[0].x / rc;
      res[i].est.x = est[0].x / rc;
      res[i].mask.val = rc;
    }
  }

  trilat_results_dump(res, (i) ? i : 1);

  mn_pos->x = res[mi].est.x;
  mn_pos->y = res[mi].est.y;
  mn_pos->z = res[mi].est.z;

#if 0
  if ((res_cnt == 1) && (meas_cnt == CFG_TRILAT_MEAS_PER_CALC)) {
    *qf = 50;
  } else {
    if (fabs(res[mi].e) > CFG_TRILAT_MAX_POS_ERR_M) {
      *qf = 50;
    } else {
      *qf = 100 - res[mi].e / CFG_TRILAT_MAX_POS_ERR_M * 50.0f;
    }
  }
#else
  double gdoprate_min = 1;
  double gdop;

  for (k = 0; k < trilat_solver->trilat_cnt; k++) {
    gdop = gdoprate(*mn_pos,
                    trilat_solver->bn_pos[trilat_solver->trilat[k].mask.v[0]],
                    trilat_solver->bn_pos[trilat_solver->trilat[k].mask.v[1]],
                    trilat_solver->bn_pos[trilat_solver->trilat[k].mask.v[2]]);
    if (gdop < gdoprate_min) {
      gdoprate_min = gdop;
    }
  }

  *qf = gdoprate_min * 100;
#endif

  if (*qf > 100) {
    *qf = 100;
  }

#if defined(CFG_LE_TRILAT_DEBUG) && (CFG_LE_TRILAT_DEBUG == 1)
  diag_printf("est(x:y:z:qf:diff) = %7.3f:%7.3f:%7.3f:%6d:%7.3f\n\n",
              mn_pos->x, mn_pos->y, mn_pos->z, *qf, res[mi].e);
#endif /* CFG_LE_TRILAT_DEBUG */

  out:
  TRILAT_FREE(res);

  return rv;
}

#if defined(CFG_LE_TRILAT_UTILS) && (CFG_LE_TRILAT_UTILS == 1)

/* Described in header file */
int trilat_solve(vec3d_t *bn_pos,
                 double *meas,
                 int cnt,
                 vec3d_t *pos_est,
                 uint8_t *qf)
{
#if defined(CFG_LEGACY) && (CFG_LEGACY == 1)
  int rv;

  if (_trilat_solver == NULL) {
    _trilat_solver = (trilat_solver_t *)TRILAT_MALLOC(sizeof(trilat_solver_t));
    if (_trilat_solver == NULL) {
      return -6;
    }

    // memset(_trilat_solver, 0, sizeof(trilat_solver_t));
  }

  rv = trilat_solver_init(_trilat_solver, bn_pos, cnt);
  if (rv < 0) {
    return rv;
  }

  rv = trilat_solver_get_pos(_trilat_solver, pos_est, qf, meas, cnt);

  trilat_reset();

  return rv;
#else
  int distances[4];
  int rv;
  int i;

  if (cnt < 3) {
    return -1;
  }

  for (i = 0; i < cnt; i++) {
    distances[i] = meas[i] * 1000;
  }

  rv = GetLocation(pos_est, cnt == 3 ? 0 : 1, bn_pos, distances);
  *qf = 11;

  return rv;
#endif
}

/* Described in header file */
void trilat_reset(void)
{
  if (_trilat_solver) {
    if (_trilat_solver->bn_pos) {
      TRILAT_FREE(_trilat_solver->bn_pos);
    }

    if (_trilat_solver->trilat) {
      TRILAT_FREE(_trilat_solver->trilat);
    }

//        memset(_trilat_solver, 0, sizeof(trilat_solver_t));

    TRILAT_FREE(_trilat_solver);

    _trilat_solver = NULL;
  }
}

#endif /* CFG_LE_TRILAT_UTILS */

#endif /* CFG_LE_TRILAT */
