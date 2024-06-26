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

#ifndef _LE_TRILAT_H_
#define _LE_TRILAT_H_

#include "dwm-math.h"

#define CFG_TRILAT_MAX_CACHE           56
#define CFG_TRILAT_MAX_RESULTS         112
#define CFG_TRILAT_BN_PER_CALC         4
#define CFG_TRILAT_MEAS_PER_CALC       3
#define CFG_TRILAT_MAX_DIFF_M          5.0
#define CFG_TRILAT_MIN_DIFF_M          0.15
#define CFG_TRILAT_MIN_NEG_DIFF_M      -0.50
#define CFG_TRILAT_MIN_ERR_M           0.05
#define CFG_TRILAT_MAX_POS_ERR_M       0.5
#define CFG_TRILAT_MIN_RSSI            0

#define TRIL_3SPHERES                  3
#define TRIL_4SPHERES                  4

union trilat_mask_union {
  struct {
    uint8_t v[4];
  };

  uint32_t val;
};

typedef union trilat_mask_union trilat_mask_t;

struct trilat_result_struct {
  vec3d_t est;
  double e;
  trilat_mask_t mask;
};

typedef struct trilat_result_struct trilat_result_t;

struct trilat_struct {
  trilat_mask_t mask;
};

typedef struct trilat_struct trilat_t;

struct trilat_solver_struct {
  vec3d_t *bn_pos;
  unsigned int bn_cnt;

  vec3d_t min;
  vec3d_t max;
  vec3d_t center;

  trilat_t *trilat;
  int trilat_cnt;

  uint32_t chksum;
};

typedef struct trilat_solver_struct trilat_solver_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize matrices using base node positions
 *
 * \param trilat  Pointer to trilateration data structure
 * \param bn_pos  Pointer to base node position
 * \param bn_cnt  Number of base nodes
 * \return        Returns negative on an error else return 0
 */
int trilat_init(trilat_t *trilat, const vec3d_t *bn_pos, unsigned int bn_cnt);

/**
 * Solve quadratic equation
 *
 * \param trilat_solver  Pointer to trilateration solver data structure
 * \param trilat  Pointer to trilateration data structure
 * \param u     Cooefficient
 * \param meas  Pointer to measurement values
 * \return      Returns zero if one solution else sign of the discriminant
 */
double trilat_solv_quad(trilat_solver_t *trilat_solver,
                        trilat_t *trilat, double *u, const double *meas);

/**
 * Calculate position using trilateration and ranging measurements
 *
 * \param trilat_solver  Pointer to trilateration solver data structure
 * \param trilat   Pointer to trilateration data structure
 * \param est      Pointer to estimated positions
 * \param meas     Pointer to measurements
 * \param meas_cnt Number of measurements
 * \return         Returns negative on an error else return 0
 */
int trilat_get_pos(trilat_solver_t *trilat_solver, trilat_t *trilat,
                   vec3d_t *est, const double *meas, unsigned int meas_cnt);

/**
 * Initialize trilateration solver
 *
 * \param trilat_solver  Pointer to trilateration solver data structure
 * \param bn_pos       Pointer to base node position
 * \param bn_cnt       Number of base nodes
 * \return   Returns negative on an error else return 0
 */
int trilat_solver_init(trilat_solver_t *trilat_solver,
                       const vec3d_t *bn_pos,
                       unsigned int bn_cnt);

/**
 * Calculate position using trilateration and ranging measurements.
 * Select the best solution and indicate position quality.
 *
 * \param trilat_solver  Pointer to trilateration data structure
 * \param mn_pos       Pointer to estimated position
 * \param qf           Pointer to position quality factor
 * \param meas         Pointer to measurements
 * \param meas_cnt     Number of measurements
 * \return   Returns negative on an error else return 0
 */
int trilat_solver_get_pos(trilat_solver_t *trilat_solver,
                          vec3d_t *mn_pos,
                          uint8_t *qf,
                          const double *meas,
                          unsigned int meas_cnt);

#if defined(CFG_LE_TRILAT_UTILS) && (CFG_LE_TRILAT_UTILS == 1)

/**
 * Trilateration - use provided positions and measurements to calculate
 * unknown position
 *
 * \param bn_pos  Pointer to known positions
 * \param meas    Pointer to measurements
 * \param cnt     Number of measurements
 * \param pos_est Estimated position
 * \param qf      Position quality factor for the estimated position
 * \return        Returns negative if error occured else return zero
 */
int trilat_solve(vec3d_t *bn_pos,
                 double *meas,
                 int cnt,
                 vec3d_t *pos_est,
                 uint8_t *qf);

/**
 * Trilateration - reset the solver
 */
void trilat_reset(void);

#endif /* CFG_LE_TRILAT_UTILS */

#ifdef __cplusplus
}
#endif

#endif /* _LE_TRILAT_H_ */
