/***************************************************************************//**
 * @file peps_leader.c
 * @brief PEPS Leader functionality
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#include "peps_leader.h"

#include "app_log.h"

#include "env.h"
#include "var.h"
#include "ui.h"
#include "app.h"

#include "lin/sl_lin_master.h"

#include "sl_cli.h"
#include "sl_cli_instances.h"
#include "sl_cli_arguments.h"
#include "sl_cli_handles.h"

#include "sl_bt_api.h"

#include <string.h>
#include <math.h>

#include <matrix_functions.h>

#include <em_gpio.h>
#include "system.h"

#include <inttypes.h>

#ifndef M_PI
#define M_PI                3.14159265358979323846
#endif

#define PEPS_BAUD           20000

sl_status_t peps_leader_broadcast_connparams_1(
  const peps_cmd_broadcast_connparams_1_t *param)
{
  sl_status_t ret;

  ret = sl_lin_master_transmit(PEPS_ADDR_BROADCAST_CONNPARAMS_1,
                               (const uint8_t *)param,
                               sizeof(param),
                               true,
                               false,
                               true);

  return ret;
}

sl_status_t peps_leader_broadcast_connparams_2(
  const peps_cmd_broadcast_connparams_2_t *param)
{
  sl_status_t ret;

  ret = sl_lin_master_transmit(PEPS_ADDR_BROADCAST_CONNPARAMS_2,
                               (const uint8_t *)param,
                               sizeof(param),
                               true,
                               false,
                               true);

  return ret;
}

sl_status_t peps_leader_broadcast_connparams_3(
  const peps_cmd_broadcast_connparams_3_t *param)
{
  sl_status_t ret;

  ret = sl_lin_master_transmit(PEPS_ADDR_BROADCAST_CONNPARAMS_3,
                               (const uint8_t *)param,
                               sizeof(param),
                               true,
                               false,
                               true);

  return ret;
}

sl_status_t peps_leader_broadcast_connparams_4(
  const peps_cmd_broadcast_connparams_4_t *param)
{
  sl_status_t ret;

  ret = sl_lin_master_transmit(PEPS_ADDR_BROADCAST_CONNPARAMS_4,
                               (const uint8_t *)param,
                               sizeof(param),
                               true,
                               false,
                               true);

  return ret;
}

sl_status_t peps_leader_broadcast_connparams_5(
  const peps_cmd_broadcast_connparams_5_t *param)
{
  sl_status_t ret;

  ret = sl_lin_master_transmit(PEPS_ADDR_BROADCAST_CONNPARAMS_5,
                               (const uint8_t *)param,
                               sizeof(param),
                               true,
                               false,
                               true);

  return ret;
}

sl_status_t peps_leader_broadcast_disconnect(void)
{
  sl_status_t ret;
  peps_cmd_broadcast_disconnect_t disconnect;

  disconnect.dummy = 42;

  ret = sl_lin_master_transmit(PEPS_ADDR_BROADCAST_DISCONNECT,
                               (const uint8_t *)&disconnect,
                               sizeof(disconnect),
                               true,
                               false,
                               true);

  return ret;
}

sl_status_t peps_leader_broadcast_latch_rssi(void)
{
  sl_status_t ret;
  peps_cmd_broadcast_latch_rssi_t latch_rssi;

  latch_rssi.dummy = 42;

  ret = sl_lin_master_transmit(PEPS_ADDR_BROADCAST_LATCH_RSSI,
                               (const uint8_t *)&latch_rssi,
                               sizeof(latch_rssi),
                               true,
                               false,
                               true);

  return ret;
}

sl_status_t peps_leader_read_connparams_state(unsigned char base,
                                              peps_cmd_read_connparams_state_t *param)
{
  sl_status_t ret;
  uint8_t address = PEPS_ADDR_READ_CONNPARAMS_STATE(base);

  if (address > SL_LIN_MAX_ENDPOINT) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Address is out of range!\n");
    return SL_STATUS_INVALID_RANGE;
  }

  ret = sl_lin_master_request(address,
                              (uint8_t *)param,
                              sizeof(*param),
                              true,
                              true);

  return ret;
}

sl_status_t peps_leader_read_var1(unsigned char base,
                                  peps_cmd_read_var1_t *param)
{
  sl_status_t ret;
  uint8_t address = PEPS_ADDR_READ_VAR1(base);

  if (address > SL_LIN_MAX_ENDPOINT) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Address is out of range!\n");
    return SL_STATUS_INVALID_RANGE;
  }

  ret = sl_lin_master_request(address,
                              (uint8_t *)param,
                              sizeof(*param),
                              true,
                              true);

  return ret;
}

sl_status_t peps_leader_read_var2(unsigned char base,
                                  peps_cmd_read_var2_t *param)
{
  sl_status_t ret;
  uint8_t address = PEPS_ADDR_READ_VAR2(base);

  if (address > SL_LIN_MAX_ENDPOINT) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Address is out of range!\n");
    return SL_STATUS_INVALID_RANGE;
  }

  ret = sl_lin_master_request(address,
                              (uint8_t *)param,
                              sizeof(*param),
                              true,
                              true);

  return ret;
}

sl_status_t peps_leader_write_var2(unsigned char base,
                                   const peps_cmd_write_var2_t *param)
{
  sl_status_t ret;
  uint8_t address = PEPS_ADDR_WRITE_VAR2(base);

  if (address > SL_LIN_MAX_ENDPOINT) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Address is out of range!\n");
    return SL_STATUS_INVALID_RANGE;
  }

  ret = sl_lin_master_transmit(address,
                               (const uint8_t *)param,
                               sizeof(param),
                               true,
                               false,
                               true);

  return ret;
}

sl_status_t peps_leader_read_rssi(unsigned char base,
                                  int8_t *central_rssi,
                                  int8_t *peripheral_rssi)
{
  sl_status_t ret;
  peps_cmd_read_rssi_t rssi;
  uint8_t address = PEPS_ADDR_READ_RSSI(base);

  if (address > SL_LIN_MAX_ENDPOINT) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Address is out of range!\n");
    return SL_STATUS_INVALID_RANGE;
  }

  ret = sl_lin_master_request(address,
                              (uint8_t *)&rssi,
                              sizeof(rssi),
                              true,
                              true);
  if ((ret == SL_STATUS_OK) && (rssi.status == 0)) {
    ret = SL_STATUS_INVALID_STATE;
  }

  if (central_rssi) {
    *central_rssi = rssi.central_rssi;
  }

  if (peripheral_rssi) {
    *peripheral_rssi = rssi.peripheral_rssi;
  }

  return ret;
}

sl_status_t peps_leader_write_result(unsigned char base,
                                     float distance,
                                     float angle)
{
  sl_status_t ret;
  peps_cmd_write_result_t write_result;
  uint8_t address = PEPS_ADDR_WRITE_RESULT(base);

  if (address > SL_LIN_MAX_ENDPOINT) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Address is out of range!\n");
    return SL_STATUS_INVALID_RANGE;
  }

  write_result.distance = distance;
  write_result.angle = angle;

  ret = sl_lin_master_transmit(address,
                               (uint8_t *)&write_result,
                               sizeof(write_result),
                               true,
                               false,
                               true);

  return ret;
}

static sl_status_t peps_calibrate_location(env_device_location_t location)
{
  var_location_record_t *loc = &var_location_record[location];
  double sum_x = 0.0;
  double sum_y = 0.0;
  double sum_xy = 0.0;
  double sum_x2 = 0.0;
  double coeff, offset;
  unsigned int i;
  sl_status_t ret;

  if (loc->measurements < 3) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Too few measurements to calibrate!\n");
    return SL_STATUS_NOT_AVAILABLE;
  }

  for (i = 0; i < loc->measurements; i++)
  {
    var_location_measurement_t *meas = &loc->measurement[i];
    double x, y;

    x = -10.0 * log10(meas->distance);
    y = meas->central_rssi;

    sum_x += x;
    sum_y += y;
    sum_xy += x * y;
    sum_x2 += x * x;
  }

  coeff = (loc->measurements * sum_xy - sum_x * sum_y)
          / (loc->measurements * sum_x2 - sum_x * sum_x);
  offset = (coeff * sum_x - sum_y) / loc->measurements;
  app_log_level(APP_LOG_LEVEL_INFO,
                "Coefficient: %.8f Offset: %.8f\n",
                coeff,
                offset);

  ret = env_location_set_calib(location, coeff, offset);
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to store calibration parameters!\n");
  }

  return ret;
}

#ifdef DEBUG_CALC
static void print_matrix(const char *name, arm_matrix_instance_f64 *mat)
{
  unsigned int i, j;

  printf("---- begin matrix %s[%u][%u]\n", name, mat->numRows, mat->numCols);

  if (mat->numRows > 10) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Suspicious number of rows: %u!\n",
                  mat->numRows);
    return;
  }

  if (mat->numCols > 10) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Suspicious number of cols: %u!\n",
                  mat->numCols);
    return;
  }

  for (i = 0; i < mat->numRows; i++)
  {
    printf("%+.4f", mat->pData[i * mat->numCols + 0]);

    for (j = 1; j < mat->numCols; j++)
    {
      printf("  %+.4f", mat->pData[i * mat->numCols + j]);
    }

    printf("\n");
  }

  printf("---- end matrix %s[%u][%u]\n", name, mat->numRows, mat->numCols);
}

#endif

static sl_status_t peps_calculate_location(float *x, float *y)
{
  // local RSSI is out of sync, so skip it for now...

  double A[ENV_LOCATION_LAST - 2][2];
  arm_matrix_instance_f64 mat_A =
  { .numRows = ENV_LOCATION_LAST - 2, .numCols = 2, .pData = (float64_t *)A, };
  double A_t[2][ENV_LOCATION_LAST - 2];
  arm_matrix_instance_f64 mat_A_t =
  { .numRows = 2, .numCols = ENV_LOCATION_LAST - 2, .pData = (float64_t *)A_t,
  };
  double b[ENV_LOCATION_LAST - 2];
  arm_matrix_instance_f64 mat_b =
  { .numRows = ENV_LOCATION_LAST - 2, .numCols = 1, .pData = (float64_t *)b, };
  double m[2][2];
  arm_matrix_instance_f64 mat_m =
  { .numRows = 2, .numCols = 2, .pData = (float64_t *)m, };
  double m_i[2][2];
  arm_matrix_instance_f64 mat_m_i =
  { .numRows = 2, .numCols = 2, .pData = (float64_t *)m_i, };
  double m2[2][ENV_LOCATION_LAST - 2];
  arm_matrix_instance_f64 mat_m2 =
  { .numRows = 2, .numCols = ENV_LOCATION_LAST - 2, .pData = (float64_t *)m2, };
  double result[2];
  arm_matrix_instance_f64 mat_result =
  { .numRows = 2, .numCols = 1, .pData = (float64_t *)result, };
  unsigned int i;
  int ret;

  for (i = 0; i < (ENV_LOCATION_LAST - 1); i++)
  {
    var_distance[i] =
      pow(10.0,
          (var_central_rssi[i] + env_location_params[i].offset)
          / (-10.0 * env_location_params[i].coeff));
  }

  for (i = 0; i < (ENV_LOCATION_LAST - 2); i++)
  {
    A[i][0] = 2
              * (env_location_params[i].x
                 - env_location_params[ENV_LOCATION_LAST - 2].x);
    A[i][1] = 2
              * (env_location_params[i].y
                 - env_location_params[ENV_LOCATION_LAST - 2].y);
    b[i] = env_location_params[i].x * env_location_params[i].x
           - env_location_params[ENV_LOCATION_LAST - 2].x
           * env_location_params[ENV_LOCATION_LAST - 2].x
           + env_location_params[i].y * env_location_params[i].y
           - env_location_params[ENV_LOCATION_LAST - 2].y
           * env_location_params[ENV_LOCATION_LAST - 2].y
           + var_distance[i] * var_distance[i]
           - var_distance[ENV_LOCATION_LAST - 2]
           * var_distance[ENV_LOCATION_LAST - 2];
  }

#ifdef DEBUG_CALC
  print_matrix("mat_A", &mat_A);
  print_matrix("mat_b", &mat_b);
#endif
  if (arm_mat_trans_f64(&mat_A, &mat_A_t) != ARM_MATH_SUCCESS) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Failed to transpose matrix!\n");
    return SL_STATUS_FAIL;
  }
#ifdef DEBUG_CALC
  print_matrix("mat_A_t", &mat_A_t);
#endif

  if (arm_mat_mult_f64(&mat_A_t, &mat_A, &mat_m) != ARM_MATH_SUCCESS) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Failed to multiply matrix!\n");
    return SL_STATUS_FAIL;
  }
#ifdef DEBUG_CALC
  print_matrix("mat_m", &mat_m);
#endif

  if ((ret = arm_mat_inverse_f64(&mat_m, &mat_m_i)) != ARM_MATH_SUCCESS) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Failed to invert matrix! (%d)\n", ret);
    return SL_STATUS_FAIL;
  }
#ifdef DEBUG_CALC
  print_matrix("mat_m_i", &mat_m_i);
#endif

  if (arm_mat_mult_f64(&mat_m_i, &mat_A_t, &mat_m2) != ARM_MATH_SUCCESS) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Failed to multiply matrix!\n");
    return SL_STATUS_FAIL;
  }
#ifdef DEBUG_CALC
  print_matrix("mat_m2", &mat_m2);
#endif

  if (arm_mat_mult_f64(&mat_m2, &mat_b, &mat_result) != ARM_MATH_SUCCESS) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Failed to multiply matrix!\n");
    return SL_STATUS_FAIL;
  }
#ifdef DEBUG_CALC
  print_matrix("mat_result", &mat_result);
#endif

  double dx = result[0] - env_location_params[ENV_LOCATION_CENTER].x;
  double dy = result[1] - env_location_params[ENV_LOCATION_CENTER].y;
  var_distance[ENV_LOCATION_CENTER] = sqrt(dx * dx + dy * dy);

  // TODO: should return an absolute position compared to 0,0 origin or
  // a relative one (dx, dy) compared to the center device's position
  // (in case that's not at the origin) ?
  *x = result[0];
  *y = result[1];

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    var_angle[i] =
      (atan2(result[1] - env_location_params[i].y,
             result[0] - env_location_params[i].x) * 180) / M_PI;
#ifdef DEBUG_CALC
    app_log_level(APP_LOG_LEVEL_DEBUG,
                  "Distance from %s is %+.4f, angle is %+.4f\n",
                  env_device_location_names[i],
                  var_distance[i],
                  var_angle[i]);
#endif
  }

  return SL_STATUS_OK;
}

static void peps_read_var1_help(void)
{
  app_log_level(APP_LOG_LEVEL_INFO, "Syntax: peps read var1 <address>\n");

  app_log_nl();
}

static void peps_read_var1(sl_cli_command_arg_t *arguments)
{
  peps_cmd_read_var1_t var1;
  unsigned long addr;
  char *endp = NULL;
  sl_status_t ret;

  if (sl_cli_get_argument_count(arguments) != 1) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_read_var1_help();
    return;
  }

  addr = strtoul(sl_cli_get_argument_string(arguments, 0), &endp, 0);
  if (addr > (SL_LIN_MAX_ENDPOINT + 1 - PEPS_OFS_READ_VAR1)) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Base address is out of range!\n");
    peps_read_var1_help();
    return;
  }

  ret = peps_leader_read_var1(addr, &var1);
  if (ret == SL_STATUS_OK) {
    char output[100];
    int ptr = 0;
    unsigned int i;

    output[ptr++] = '<';
    ptr +=
      snprintf(&output[ptr], sizeof(output) - 1 - ptr, "%02x", var1.data[0]);
    for (i = 1; i < sizeof(var1); i++)
    {
      ptr +=
        snprintf(&output[ptr], sizeof(output) - 1 - ptr, "%02x", var1.data[i]);
    }
    output[ptr++] = '>';
    output[ptr] = '\0';

    app_log_level(APP_LOG_LEVEL_INFO, "%s\n", output);
  }

  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_read_var2_help(void)
{
  app_log_level(APP_LOG_LEVEL_INFO, "Syntax: peps read var2 <address>\n");

  app_log_nl();
}

static void peps_read_var2(sl_cli_command_arg_t *arguments)
{
  peps_cmd_read_var2_t var2;
  unsigned long addr;
  char *endp = NULL;
  sl_status_t ret;

  if (sl_cli_get_argument_count(arguments) != 1) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_read_var2_help();
    return;
  }

  addr = strtoul(sl_cli_get_argument_string(arguments, 0), &endp, 0);
  if (addr > (SL_LIN_MAX_ENDPOINT + 1 - PEPS_OFS_READ_VAR2)) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Base address is out of range!\n");
    peps_read_var2_help();
    return;
  }

  ret = peps_leader_read_var2(addr, &var2);
  if (ret == SL_STATUS_OK) {
    char output[100];
    int ptr = 0;
    unsigned int i;

    output[ptr++] = '<';
    ptr +=
      snprintf(&output[ptr], sizeof(output) - 1 - ptr, "%02x", var2.data[0]);
    for (i = 1; i < sizeof(var2); i++)
    {
      ptr +=
        snprintf(&output[ptr], sizeof(output) - 1 - ptr, "%02x", var2.data[i]);
    }
    output[ptr++] = '>';
    output[ptr] = '\0';

    app_log_level(APP_LOG_LEVEL_INFO, "%s\n", output);
  }

  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_write_var2_help(void)
{
  app_log_level(APP_LOG_LEVEL_INFO,
                "Syntax: peps write var2 <address> <hex data>\n");

  app_log_nl();
}

static void peps_write_var2(sl_cli_command_arg_t *arguments)
{
  uint8_t *data;
  unsigned long addr;
  char *endp = NULL;
  size_t size;
  sl_status_t ret;

  if (sl_cli_get_argument_count(arguments) != 2) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_write_var2_help();
    return;
  }

  addr = strtoul(sl_cli_get_argument_string(arguments, 0), &endp, 0);
  if (addr > (SL_LIN_MAX_ENDPOINT + 1 - PEPS_OFS_WRITE_VAR2)) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Base address is out of range!\n");
    peps_write_var2_help();
    return;
  }

  data = sl_cli_get_argument_hex(arguments, 1, &size);
  if (size != sizeof(peps_cmd_write_var2_t)) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Invalid number of data bytes, must be %u!\n",
                  sizeof(peps_cmd_write_var2_t));
    peps_write_var2_help();
  }

  ret = peps_leader_write_var2(addr, (const peps_cmd_write_var2_t *)data);
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_location_measure_help(void)
{
  unsigned int i;

  app_log_level(APP_LOG_LEVEL_INFO,
                "Syntax: peps location measure <location> <distance>\n");
  app_log_level(APP_LOG_LEVEL_INFO,
                "Where the <location> is one of the following:\n");

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    app_log_level(APP_LOG_LEVEL_INFO, "\t%s\n", env_device_location_names[i]);
  }

  app_log_nl();
}

static void peps_location_measure(sl_cli_command_arg_t *arguments)
{
  env_device_location_t location;
  unsigned char addr;
  float distance;
  int8_t central_rssi = -128;
  int8_t peripheral_rssi = -128;
  char *endp = NULL;
  sl_status_t ret;
  unsigned int i;

  if (sl_cli_get_argument_count(arguments) != 2) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_location_measure_help();
    return;
  }

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    if (strcmp(env_device_location_names[i],
               sl_cli_get_argument_string(arguments, 0)) == 0) {
      break;
    }
  }

  if (i >= ENV_LOCATION_LAST) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid location!\n");
    peps_location_measure_help();
    return;
  }

  location = (env_device_location_t)i;
  addr = env_location[location];

  distance = strtod(sl_cli_get_argument_string(arguments, 1), &endp);
  if (endp && *endp) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "%s shall be a valid floating point value!\n",
                  "distance");
    peps_location_measure_help();
    return;
  }

  ret = peps_leader_broadcast_latch_rssi();
  if (ret == SL_STATUS_OK) {
    ret = peps_leader_read_rssi(addr, &central_rssi, &peripheral_rssi);
  }

  if (ret == SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_INFO,
                  "Distance: %.2f Central RSSI: %+3d dBm Peripheral RSSI: %+3d dBm\n",
                  distance,
                  central_rssi,
                  peripheral_rssi);

    if ((central_rssi > -128)
        && (peripheral_rssi > -128)
        && (var_location_measurement_add(location, distance, central_rssi,
                                         peripheral_rssi) == false)) {
      app_log_level(APP_LOG_LEVEL_ERROR, "Failed to register measurement!\n");
      ret = SL_STATUS_FAIL;
    }
  }

  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_location_forget_help(void)
{
  unsigned int i;

  app_log_level(APP_LOG_LEVEL_INFO,
                "Syntax: peps location forget <location> <distance>\n");
  app_log_level(APP_LOG_LEVEL_INFO,
                "Where the <location> is one of the following:\n");

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    app_log_level(APP_LOG_LEVEL_INFO, "\t%s\n", env_device_location_names[i]);
  }

  app_log_nl();
}

static void peps_location_forget(sl_cli_command_arg_t *arguments)
{
  env_device_location_t location;
  float distance;
  char *endp = NULL;
  sl_status_t ret = SL_STATUS_OK;
  unsigned int i;

  if (sl_cli_get_argument_count(arguments) != 2) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_location_forget_help();
    return;
  }

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    if (strcmp(env_device_location_names[i],
               sl_cli_get_argument_string(arguments, 0)) == 0) {
      break;
    }
  }

  if (i >= ENV_LOCATION_LAST) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid location!\n");
    peps_location_forget_help();
    return;
  }

  location = (env_device_location_t)i;

  distance = strtod(sl_cli_get_argument_string(arguments, 1), &endp);
  if (endp && *endp) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "%s shall be a valid floating point value!\n",
                  "distance");
    peps_location_forget_help();
    return;
  }

  if (var_location_measurement_remove(location, distance) == false) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Failed to remove measurement!\n");
    ret = SL_STATUS_FAIL;
  }

  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_location_flush_help(void)
{
  unsigned int i;

  app_log_level(APP_LOG_LEVEL_INFO, "Syntax: peps location flush <location>\n");
  app_log_level(APP_LOG_LEVEL_INFO,
                "Where the <location> is one of the following:\n");

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    app_log_level(APP_LOG_LEVEL_INFO, "\t%s\n", env_device_location_names[i]);
  }

  app_log_nl();
}

static void peps_location_flush(sl_cli_command_arg_t *arguments)
{
  env_device_location_t location;
  sl_status_t ret = SL_STATUS_OK;
  unsigned int i;

  if (sl_cli_get_argument_count(arguments) != 1) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_location_flush_help();
    return;
  }

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    if (strcmp(env_device_location_names[i],
               sl_cli_get_argument_string(arguments, 0)) == 0) {
      break;
    }
  }

  if (i >= ENV_LOCATION_LAST) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid location!\n");
    peps_location_flush_help();
    return;
  }

  location = (env_device_location_t)i;

  if (var_location_measurement_flush(location) == false) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Failed to remove measurements!\n");
    ret = SL_STATUS_FAIL;
  }

  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_location_list_help(void)
{
  unsigned int i;

  app_log_level(APP_LOG_LEVEL_INFO, "Syntax: peps location list <location>\n");
  app_log_level(APP_LOG_LEVEL_INFO,
                "Where the <location> is one of the following:\n");

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    app_log_level(APP_LOG_LEVEL_INFO, "\t%s\n", env_device_location_names[i]);
  }

  app_log_nl();
}

static void peps_location_list(sl_cli_command_arg_t *arguments)
{
  env_device_location_t location;
  var_location_record_t *loc;
  sl_status_t ret = SL_STATUS_OK;
  unsigned int i;

  if (sl_cli_get_argument_count(arguments) != 1) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_location_list_help();
    return;
  }

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    if (strcmp(env_device_location_names[i],
               sl_cli_get_argument_string(arguments, 0)) == 0) {
      break;
    }
  }

  if (i >= ENV_LOCATION_LAST) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid location!\n");
    peps_location_list_help();
    return;
  }

  location = (env_device_location_t)i;
  loc = &var_location_record[location];

  for (i = 0; i < loc->measurements; i++)
  {
    var_location_measurement_t *meas = &loc->measurement[i];

    app_log_level(APP_LOG_LEVEL_INFO,
                  "  Distance: %.2f RSSI: %+3d dBm\n",
                  meas->distance,
                  meas->central_rssi);
  }

  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_location_calibrate_help(void)
{
  unsigned int i;

  app_log_level(APP_LOG_LEVEL_INFO, "Syntax: peps location calib <location>\n");
  app_log_level(APP_LOG_LEVEL_INFO,
                "Where the <location> is one of the following:\n");

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    app_log_level(APP_LOG_LEVEL_INFO, "\t%s\n", env_device_location_names[i]);
  }

  app_log_nl();
}

static void peps_location_calibrate(sl_cli_command_arg_t *arguments)
{
  env_device_location_t location;
  sl_status_t ret;
  unsigned int i;

  if (sl_cli_get_argument_count(arguments) != 1) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_location_calibrate_help();
    return;
  }

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    if (strcmp(env_device_location_names[i],
               sl_cli_get_argument_string(arguments, 0)) == 0) {
      break;
    }
  }

  if (i >= ENV_LOCATION_LAST) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid location!\n");
    peps_location_calibrate_help();
    return;
  }

  location = (env_device_location_t)i;
  ret = peps_calibrate_location(location);

  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

sl_status_t peps_leader_update_location(void)
{
  sl_status_t ret;
  float x, y;
  unsigned int i;

  ret = peps_calculate_location(&x, &y);
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Failed to calculate location!\n");
    return ret;
  }

  app_log_level(APP_LOG_LEVEL_INFO, "  X: %+.4f Y: %+.4f\n", x, y);

  ui_set_distance(var_distance[ENV_LOCATION_CENTER]);
  ui_set_angle(var_angle[ENV_LOCATION_CENTER]);

  for (i = 0; i < ENV_LOCATION_LAST - 1; i++)
  {
    peps_leader_write_result(env_location[i], var_distance[i], var_angle[i]);
  }

  return SL_STATUS_OK;
}

static void peps_location_calculate(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  sl_status_t ret;

  ret = peps_leader_update_location();
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_follower_add_help(void)
{
  unsigned int i;

  app_log_level(APP_LOG_LEVEL_INFO,
                "Syntax: peps follower add <name> <type> <address>\n");
  app_log_level(APP_LOG_LEVEL_INFO,
                "Where the <type> is one of the following:\n");

  for (i = 0; i < ENV_DEVICE_LAST; i++)
  {
    app_log_level(APP_LOG_LEVEL_INFO, "\t%s\n", env_device_type_names[i]);
  }

  app_log_nl();
}

static void peps_follower_add(sl_cli_command_arg_t *arguments)
{
  env_device_name_t padded_name;
  env_device_type_t type;
  unsigned long addr;
  char *endp = NULL;
  sl_status_t ret;
  unsigned int i;

  if (sl_cli_get_argument_count(arguments) != 3) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_follower_add_help();
    return;
  }

  env_pad_name_from(padded_name, sl_cli_get_argument_string(arguments, 0));

  for (i = 0; i < ENV_DEVICE_LAST; i++)
  {
    if (strcmp(env_device_type_names[i],
               sl_cli_get_argument_string(arguments, 1)) == 0) {
      break;
    }
  }

  if (i >= ENV_DEVICE_LAST) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid device type!\n");
    peps_follower_add_help();
    return;
  }

  type = (env_device_type_t)i;

  addr = strtoul(sl_cli_get_argument_string(arguments, 2), &endp, 0);
  if (addr > SL_LIN_MAX_ENDPOINT) {
    // TODO: check if the range specified by the type fits!
    app_log_level(APP_LOG_LEVEL_ERROR, "Base address is out of range!\n");
    peps_follower_add_help();
    return;
  }

  ret = env_add_follower(addr, type, padded_name);
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_follower_remove_name_help(void)
{
  app_log_level(APP_LOG_LEVEL_INFO,
                "Syntax: peps follower remove name <name>\n");
  app_log_nl();
}

static void peps_follower_remove_name(sl_cli_command_arg_t *arguments)
{
  env_device_name_t padded_name;
  sl_status_t ret;

  if (sl_cli_get_argument_count(arguments) != 1) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_follower_remove_name_help();
    return;
  }

  env_pad_name_from(padded_name, sl_cli_get_argument_string(arguments, 0));

  ret = env_remove_follower_by_name(padded_name);
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_follower_remove_address_help(void)
{
  app_log_level(APP_LOG_LEVEL_INFO,
                "Syntax: peps follower remove address <address>\n");
  app_log_nl();
}

static void peps_follower_remove_address(sl_cli_command_arg_t *arguments)
{
  unsigned long addr;
  char *endp = NULL;
  sl_status_t ret;

  if (sl_cli_get_argument_count(arguments) != 1) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_follower_remove_address_help();
    return;
  }

  addr = strtoul(sl_cli_get_argument_string(arguments, 0), &endp, 0);
  if (addr > SL_LIN_MAX_ENDPOINT) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Base address is out of range!\n");
    peps_follower_remove_address_help();
    return;
  }

  ret = env_remove_follower_by_address(addr);
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_follower_remove_all(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  sl_status_t ret;

  ret = env_remove_followers();
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_follower_list(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  unsigned int i;

  for (i = 0; i < env_follower_count; i++)
  {
    app_log_level(APP_LOG_LEVEL_INFO,
                  "Address: 0x%02x Type: %-13s Name: %-.*s\n",
                  env_follower_address[i],
                  env_device_type_names[env_follower_type[i]],
                  sizeof(env_device_name),
                  env_follower_name[i]);
  }

  app_log_nl();
}

static void peps_follower_location_add_address_help(void)
{
  unsigned int i;

  app_log_level(APP_LOG_LEVEL_INFO,
                "Syntax: peps follower location add address <address> <location>\n");
  app_log_level(APP_LOG_LEVEL_INFO,
                "Where the <location> is one of the following:\n");

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    app_log_level(APP_LOG_LEVEL_INFO, "\t%s\n", env_device_location_names[i]);
  }

  app_log_nl();
}

static void peps_follower_location_add_address(sl_cli_command_arg_t *arguments)
{
  unsigned long addr;
  env_device_location_t location;
  unsigned int i;
  char *endp = NULL;
  sl_status_t ret;

  if (sl_cli_get_argument_count(arguments) != 2) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_follower_location_add_address_help();
    return;
  }

  addr = strtoul(sl_cli_get_argument_string(arguments, 0), &endp, 0);
  if (addr > SL_LIN_MAX_ENDPOINT) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Base address is out of range!\n");
    peps_follower_location_add_address_help();
    return;
  }

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    if (strcmp(env_device_location_names[i],
               sl_cli_get_argument_string(arguments, 1)) == 0) {
      break;
    }
  }

  if (i >= ENV_LOCATION_LAST) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid location!\n");
    peps_follower_location_add_address_help();
    return;
  }

  location = (env_device_location_t)i;

  ret = env_add_location_by_address(location, addr);
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_follower_location_add_name_help(void)
{
  unsigned int i;

  app_log_level(APP_LOG_LEVEL_INFO,
                "Syntax: peps follower location add name <name> <location>\n");
  app_log_level(APP_LOG_LEVEL_INFO,
                "Where the <location> is one of the following:\n");

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    app_log_level(APP_LOG_LEVEL_INFO, "\t%s\n", env_device_location_names[i]);
  }

  app_log_nl();
}

static void peps_follower_location_add_name(sl_cli_command_arg_t *arguments)
{
  env_device_name_t padded_name;
  env_device_location_t location;
  unsigned int i;
  sl_status_t ret;

  if (sl_cli_get_argument_count(arguments) != 2) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_follower_location_add_name_help();
    return;
  }

  env_pad_name_from(padded_name, sl_cli_get_argument_string(arguments, 0));

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    if (strcmp(env_device_location_names[i],
               sl_cli_get_argument_string(arguments, 1)) == 0) {
      break;
    }
  }

  if (i >= ENV_LOCATION_LAST) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid location!\n");
    peps_follower_location_add_name_help();
    return;
  }

  location = (env_device_location_t)i;

  ret = env_add_location_by_name(location, padded_name);
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_follower_location_remove_address_help(void)
{
  app_log_level(APP_LOG_LEVEL_INFO,
                "Syntax: peps follower location remove address <address>\n");

  app_log_nl();
}

static void peps_follower_location_remove_address(
  sl_cli_command_arg_t *arguments)
{
  unsigned long addr;
  char *endp = NULL;
  sl_status_t ret;

  if (sl_cli_get_argument_count(arguments) != 1) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_follower_location_remove_address_help();
    return;
  }

  addr = strtoul(sl_cli_get_argument_string(arguments, 0), &endp, 0);
  if (addr > SL_LIN_MAX_ENDPOINT) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Base address is out of range!\n");
    peps_follower_location_remove_address_help();
    return;
  }

  ret = env_remove_location_by_address(addr);
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_follower_location_remove_name_help(void)
{
  app_log_level(APP_LOG_LEVEL_INFO,
                "Syntax: peps follower location remove name <name>\n");

  app_log_nl();
}

static void peps_follower_location_remove_name(sl_cli_command_arg_t *arguments)
{
  env_device_name_t padded_name;
  sl_status_t ret;

  if (sl_cli_get_argument_count(arguments) != 1) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_follower_location_remove_name_help();
    return;
  }

  env_pad_name_from(padded_name, sl_cli_get_argument_string(arguments, 0));

  ret = env_remove_location_by_name(padded_name);
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_follower_location_remove_location_help(void)
{
  unsigned int i;

  app_log_level(APP_LOG_LEVEL_INFO,
                "Syntax: peps follower location remove location <location>\n");
  app_log_level(APP_LOG_LEVEL_INFO,
                "Where the <location> is one of the following:\n");

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    app_log_level(APP_LOG_LEVEL_INFO, "\t%s\n", env_device_location_names[i]);
  }

  app_log_nl();
}

static void peps_follower_location_remove_location(
  sl_cli_command_arg_t *arguments)
{
  env_device_location_t location;
  unsigned int i;
  sl_status_t ret;

  if (sl_cli_get_argument_count(arguments) != 1) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_follower_location_remove_location_help();
    return;
  }

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    if (strcmp(env_device_location_names[i],
               sl_cli_get_argument_string(arguments, 0)) == 0) {
      break;
    }
  }

  if (i >= ENV_LOCATION_LAST) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid location!\n");
    peps_follower_location_remove_location_help();
    return;
  }

  location = (env_device_location_t)i;

  ret = env_remove_location(location);
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_follower_location_remove_all(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  sl_status_t ret;

  ret = env_remove_locations();
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_follower_location_list(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  unsigned int i;

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    unsigned char address = env_location[i];
    const char *name = NULL;
    env_device_type_t type = ENV_DEVICE_NONE;
    unsigned int j;

    for (j = 0; j < env_follower_count; j++)
    {
      if (env_follower_address[j] == address) {
        name = env_follower_name[j];
        type = env_follower_type[j];
        break;
      }
    }

    app_log_level(APP_LOG_LEVEL_INFO,
                  "Location: %-12s Address: 0x%02x Type: %-13s Name: %-.*s X: %+.4f Y: %+.4f Coeff: %+.4f Offset: %+.4f\n",
                  env_device_location_names[i],
                  address,
                  env_device_type_names[type],
                  sizeof(env_device_name),
                  name ? name : "<none>",
                  env_location_params[i].x,
                  env_location_params[i].y,
                  env_location_params[i].coeff,
                  env_location_params[i].offset);
  }

  app_log_nl();
}

static void peps_follower_location_set_pos_help(void)
{
  unsigned int i;

  app_log_level(APP_LOG_LEVEL_INFO,
                "Syntax: peps follower location set pos <location> <x> <y>\n");
  app_log_level(APP_LOG_LEVEL_INFO,
                "Where the <location> is one of the following:\n");

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    app_log_level(APP_LOG_LEVEL_INFO, "\t%s\n", env_device_location_names[i]);
  }

  app_log_nl();
}

static void peps_follower_location_set_pos(sl_cli_command_arg_t *arguments)
{
  env_device_location_t location;
  float x, y;
  char *endp = NULL;
  sl_status_t ret;
  unsigned int i;

  if (sl_cli_get_argument_count(arguments) != 3) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_follower_location_set_pos_help();
    return;
  }

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    if (strcmp(env_device_location_names[i],
               sl_cli_get_argument_string(arguments, 0)) == 0) {
      break;
    }
  }

  if (i >= ENV_LOCATION_LAST) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid location!\n");
    peps_follower_location_set_pos_help();
    return;
  }

  location = (env_device_location_t)i;

  x = strtod(sl_cli_get_argument_string(arguments, 1), &endp);
  if (endp && *endp) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "%s shall be a valid floating point value!\n",
                  "X coordinate");
    peps_follower_location_set_pos_help();
    return;
  }

  y = strtod(sl_cli_get_argument_string(arguments, 2), &endp);
  if (endp && *endp) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "%s shall be a valid floating point value!\n",
                  "Y coordinate");
    peps_follower_location_set_pos_help();
    return;
  }

  ret = env_location_set_coord(location, x, y);
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_follower_location_set_params_help(void)
{
  unsigned int i;

  app_log_level(APP_LOG_LEVEL_INFO,
                "Syntax: peps follower location set params <location> <coeff> <offset>\n");
  app_log_level(APP_LOG_LEVEL_INFO,
                "Where the <location> is one of the following:\n");

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    app_log_level(APP_LOG_LEVEL_INFO, "\t%s\n", env_device_location_names[i]);
  }

  app_log_nl();
}

static void peps_follower_location_set_params(sl_cli_command_arg_t *arguments)
{
  env_device_location_t location;
  float coeff, offset;
  char *endp = NULL;
  sl_status_t ret;
  unsigned int i;

  if (sl_cli_get_argument_count(arguments) != 3) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid number of arguments!\n");
    peps_follower_location_set_params_help();
    return;
  }

  for (i = 0; i < ENV_LOCATION_LAST; i++)
  {
    if (strcmp(env_device_location_names[i],
               sl_cli_get_argument_string(arguments, 0)) == 0) {
      break;
    }
  }

  if (i >= ENV_LOCATION_LAST) {
    app_log_level(APP_LOG_LEVEL_ERROR, "Invalid location!\n");
    peps_follower_location_set_params_help();
    return;
  }

  location = (env_device_location_t)i;

  coeff = strtod(sl_cli_get_argument_string(arguments, 1), &endp);
  if (endp && *endp) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "%s shall be a valid floating point value!\n",
                  "coeff");
    peps_follower_location_set_params_help();
    return;
  }

  offset = strtod(sl_cli_get_argument_string(arguments, 2), &endp);
  if (endp && *endp) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "%s shall be a valid floating point value!\n",
                  "offset");
    peps_follower_location_set_params_help();
    return;
  }

  ret = env_location_set_calib(location, coeff, offset);
  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret == SL_STATUS_OK) ? "Success." : "Failed!");
}

static void peps_stop(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  bool ret;

  ret = app_stop();

  app_log_level(APP_LOG_LEVEL_INFO, "%s\n", (ret) ? "Success." : "Failed!");
}

static void peps_start(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  bool ret;

  ret = app_start();

  app_log_level(APP_LOG_LEVEL_INFO,
                "%s\n",
                (ret) ? "Success." : "Failed!");
}

static const sl_cli_command_info_t cmd_peps_read_var1 = \
  SL_CLI_COMMAND(peps_read_var1,
                 "read var1 of a PEPS follower",
                 "<address> - address of follower to read from (0x... means hexadecimal)",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_peps_read_var2 = \
  SL_CLI_COMMAND(peps_read_var2,
                 "read var2 of a PEPS follower",
                 "<address> - address of follower to read from (0x... means hexadecimal)",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static sl_cli_command_entry_t cmdtable_peps_read[] = {
  { "var1", &cmd_peps_read_var1, false, },
  { "var2", &cmd_peps_read_var2, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_peps_read = \
  SL_CLI_COMMAND_GROUP(&cmdtable_peps_read,
                       "read data from PEPS followers");

static const sl_cli_command_info_t cmd_peps_write_var2 = \
  SL_CLI_COMMAND(peps_write_var2,
                 "write var2 of a PEPS follower",
                 "<address> - address of follower to write to (0x... means hexadecimal)" SL_CLI_UNIT_SEPARATOR
                 "<data> - data to write",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_HEX, SL_CLI_ARG_END,
                 });

static sl_cli_command_entry_t cmdtable_peps_write[] = {
  { "var2", &cmd_peps_write_var2, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_peps_write = \
  SL_CLI_COMMAND_GROUP(&cmdtable_peps_write,
                       "write data to PEPS followers");

static const sl_cli_command_info_t cmd_peps_location_measure = \
  SL_CLI_COMMAND(peps_location_measure,
                 "measure the RSSI of a PEPS follower for a specific distance for calibration purposes",
                 "<location> - location to measure on" SL_CLI_UNIT_SEPARATOR
                 "<distance> - distance to measure the RSSI for",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_peps_location_forget = \
  SL_CLI_COMMAND(peps_location_forget,
                 "remove a calibration-related measurement of a PEPS follower",
                 "<location> - location to remove a measurement from" SL_CLI_UNIT_SEPARATOR
                 "<distance> - distance to remove the measurement of",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_STRING, SL_CLI_ARG_END,
                 });

static const sl_cli_command_info_t cmd_peps_location_calibrate = \
  SL_CLI_COMMAND(peps_location_calibrate,
                 "calibrate a location's parameters based on former measurements",
                 "<location> - location to calibrate",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_peps_location_list = \
  SL_CLI_COMMAND(peps_location_list,
                 "list a location's calibration-related measurements",
                 "<location> - location to list the measurements of",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_peps_location_flush = \
  SL_CLI_COMMAND(peps_location_flush,
                 "flush a location's calibration-related measurements table",
                 "<location> - location to flush the measurements of",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_peps_location_calculate = \
  SL_CLI_COMMAND(peps_location_calculate,
                 "calculate the location",
                 "",
                 { SL_CLI_ARG_END, });

static sl_cli_command_entry_t cmdtable_peps_location[] = {
  { "measure", &cmd_peps_location_measure, false, },
  { "forget", &cmd_peps_location_forget, false, },
  { "calib", &cmd_peps_location_calibrate, false, },
  { "list", &cmd_peps_location_list, false, },
  { "flush", &cmd_peps_location_flush, false, },
  { "calc", &cmd_peps_location_calculate, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_peps_location = \
  SL_CLI_COMMAND_GROUP(&cmdtable_peps_location,
                       "PEPS location-related commands");

static const sl_cli_command_info_t cmd_peps_follower_add = \
  SL_CLI_COMMAND(peps_follower_add,
                 "adds a follower device to the LIN bus of a leader",
                 "<name> - name of the follower device" SL_CLI_UNIT_SEPARATOR
                 "<type> - type of the follower device" SL_CLI_UNIT_SEPARATOR
                 "<address> - address (0x... means hexadecimal)",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_STRING, SL_CLI_ARG_STRING,
                   SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_peps_follower_remove_name = \
  SL_CLI_COMMAND(peps_follower_remove_name,
                 "remove a follower device from the LIN bus of a leader by name",
                 "<name> - name to remove",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_peps_follower_remove_address = \
  SL_CLI_COMMAND(peps_follower_remove_address,
                 "remove a follower device from the LIN bus of a leader by address",
                 "<address> - address to remove (0x... means hexadecimal)",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_peps_follower_remove_all = \
  SL_CLI_COMMAND(peps_follower_remove_all,
                 "remove all followers",
                 "",
                 { SL_CLI_ARG_END, });

static sl_cli_command_entry_t cmdtable_peps_follower_remove[] = {
  { "name", &cmd_peps_follower_remove_name, false, },
  { "address", &cmd_peps_follower_remove_address, false, },
  { "all", &cmd_peps_follower_remove_all, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_peps_follower_remove = \
  SL_CLI_COMMAND_GROUP(&cmdtable_peps_follower_remove,
                       "remove a follower device from the LIN bus");

static const sl_cli_command_info_t cmd_peps_follower_list = \
  SL_CLI_COMMAND(peps_follower_list,
                 "list followers",
                 "",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_peps_follower_location_add_address = \
  SL_CLI_COMMAND(peps_follower_location_add_address,
                 "add a location by address",
                 "<address> - address to add a location for (0x... means hexadecimal)" SL_CLI_UNIT_SEPARATOR
                 "<location> - location to add",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_STRING, SL_CLI_ARG_END,
                 });

static const sl_cli_command_info_t cmd_peps_follower_location_add_name = \
  SL_CLI_COMMAND(peps_follower_location_add_name,
                 "add a location by name",
                 "<name> - name to add a location for" SL_CLI_UNIT_SEPARATOR
                 "<location> - location to add",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static sl_cli_command_entry_t cmdtable_peps_follower_location_add[] = {
  { "address", &cmd_peps_follower_location_add_address, false, },
  { "name", &cmd_peps_follower_location_add_name, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_peps_follower_location_add = \
  SL_CLI_COMMAND_GROUP(&cmdtable_peps_follower_location_add,
                       "add a location");

static const sl_cli_command_info_t cmd_peps_follower_location_remove_address = \
  SL_CLI_COMMAND(peps_follower_location_remove_address,
                 "remove a location by follower address",
                 "<address> - address to remove a location from (0x... means hexadecimal)",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_peps_follower_location_remove_name = \
  SL_CLI_COMMAND(peps_follower_location_remove_name,
                 "remove a location by follower name",
                 "<name> - name to remove a location from",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_peps_follower_location_remove_location = \
  SL_CLI_COMMAND(peps_follower_location_remove_location,
                 "remove a location by location name",
                 "<location> - location to remove",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_peps_follower_location_remove_all = \
  SL_CLI_COMMAND(peps_follower_location_remove_all,
                 "remove all locations",
                 "",
                 { SL_CLI_ARG_END, });

static sl_cli_command_entry_t cmdtable_peps_follower_location_remove[] = {
  { "address", &cmd_peps_follower_location_remove_address, false, },
  { "name", &cmd_peps_follower_location_remove_name, false, },
  { "location", &cmd_peps_follower_location_remove_location, false, },
  { "all", &cmd_peps_follower_location_remove_all, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_peps_follower_location_remove = \
  SL_CLI_COMMAND_GROUP(&cmdtable_peps_follower_location_remove,
                       "remove a location");

static const sl_cli_command_info_t cmd_peps_follower_location_list = \
  SL_CLI_COMMAND(peps_follower_location_list,
                 "list locations of devices",
                 "",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_peps_follower_location_set_pos = \
  SL_CLI_COMMAND(peps_follower_location_set_pos,
                 "set a location's position",
                 "<location> - location to set the position of" SL_CLI_UNIT_SEPARATOR
                 "<x> - X coordinate (0 is the center device)" SL_CLI_UNIT_SEPARATOR
                 "<y> - Y coordinate (0 is the center device)",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_STRING, SL_CLI_ARG_STRING,
                   SL_CLI_ARG_END,
                 });

static const sl_cli_command_info_t cmd_peps_follower_location_set_params = \
  SL_CLI_COMMAND(peps_follower_location_set_params,
                 "set a location's calibration parameters for the equation: RSSI = -(10 * <coeff> * log10(distance) + <offset>)",
                 "<location> - location to set the parameters of" SL_CLI_UNIT_SEPARATOR
                 "<coeff> - calibration coefficient" SL_CLI_UNIT_SEPARATOR
                 "<offset> - calibration offset",
                 { SL_CLI_ARG_STRING, SL_CLI_ARG_STRING, SL_CLI_ARG_STRING, SL_CLI_ARG_END,
                 });

static sl_cli_command_entry_t cmdtable_peps_follower_location_set[] = {
  { "pos", &cmd_peps_follower_location_set_pos, false, },
  { "params", &cmd_peps_follower_location_set_params, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_peps_follower_location_set = \
  SL_CLI_COMMAND_GROUP(&cmdtable_peps_follower_location_set,
                       "set location attributes");

static sl_cli_command_entry_t cmdtable_peps_follower_location[] = {
  { "add", &cmd_peps_follower_location_add, false, },
  { "remove", &cmd_peps_follower_location_remove, false, },
  { "list", &cmd_peps_follower_location_list, false, },
  { "set", &cmd_peps_follower_location_set, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_peps_follower_location = \
  SL_CLI_COMMAND_GROUP(&cmdtable_peps_follower_location,
                       "location-related commands");

static sl_cli_command_entry_t cmdtable_peps_follower[] = {
  { "add", &cmd_peps_follower_add, false, },
  { "remove", &cmd_peps_follower_remove, false, },
  { "list", &cmd_peps_follower_list, false, },
  { "location", &cmd_peps_follower_location, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_peps_follower = \
  SL_CLI_COMMAND_GROUP(&cmdtable_peps_follower,
                       "follower management commands");

static const sl_cli_command_info_t cmd_peps_stop = \
  SL_CLI_COMMAND(peps_stop,
                 "stop RSSI collection",
                 "",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd_peps_start = \
  SL_CLI_COMMAND(peps_start,
                 "start RSSI collection",
                 "",
                 { SL_CLI_ARG_END, });

static sl_cli_command_entry_t cmdtable_peps[] = {
  { "read", &cmd_peps_read, false, },
  { "write", &cmd_peps_write, false, },
  { "location", &cmd_peps_location, false, },
  { "follower", &cmd_peps_follower, false, },
  { "stop", &cmd_peps_stop, false, },
  { "start", &cmd_peps_start, false, },
  { NULL, NULL, false, },
};

static const sl_cli_command_info_t cmd_peps = \
  SL_CLI_COMMAND_GROUP(&cmdtable_peps,
                       "PEPS follower management");

static sl_cli_command_entry_t cmdtable[] = {
  { "peps", &cmd_peps, false, },
  { NULL, NULL, false, },
};

static sl_cli_command_group_t cmdgroup = {
  { NULL },
  false,
  cmdtable,
};

#if 0
#if !defined(PRS_MODEM_DOUT)
#if defined(PRS_MODEML_DOUT)
#define PRS_MODEM_DOUT                 PRS_MODEML_DOUT
#else
#define PRS_MODEM_DOUT                 0x5607
#endif
#endif

#if !defined(PRS_MODEM_DCLK)
#if defined(PRS_MODEML_DCLK)
#define PRS_MODEM_DCLK                 PRS_MODEML_DCLK
#else
#define PRS_MODEM_DCLK                 0x5606
#endif
#endif
#endif

sl_status_t peps_leader_init(void)
{
  GPIO_PinModeSet(MONACT_PORT, MONACT_PIN, gpioModePushPull, 0);

#if defined(MODEM_DOUT_PORT) && defined(PRS_MODEM_DOUT)
  PRS_Setup(MODEM_DOUT_PRS,
            prsTypeDefault,
            PRS_MODEM_DOUT,
            MODEM_DOUT_PORT,
            MODEM_DOUT_PIN,
            MODEM_DOUT_LOC);
  PRS_Setup(MODEM_DCLK_PRS,
            prsTypeDefault,
            PRS_MODEM_DCLK,
            MODEM_DCLK_PORT,
            MODEM_DCLK_PIN,
            MODEM_DCLK_LOC);
#endif

  sl_lin_master_init(PEPS_BAUD);

  sl_cli_command_add_command_group(sl_cli_inst_handle, &cmdgroup);

  return SL_STATUS_OK;
}

bool peps_leader_fetch_rssi_values(void)
{
  bool success = true;
  unsigned int i;

  if (peps_leader_broadcast_latch_rssi() != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_WARNING, "Failed to latch RSSI values!\n");
    return false;
  }

  for (i = 0; i < ENV_LOCATION_CENTER; i++)
  {
    unsigned char address = env_location[i];

    var_central_rssi[i] = -128;
    var_peripheral_rssi[i] = -128;

    if (address > SL_LIN_MAX_ENDPOINT) {
      success = false;
      continue;
    }

    if (peps_leader_read_rssi(address, &var_central_rssi[i],
                              &var_peripheral_rssi[i]) != SL_STATUS_OK) {
      app_log_level(APP_LOG_LEVEL_WARNING,
                    "Failed to fetch RSSI from %s, skipping!\n",
                    env_device_location_names[i]);
      success = false;
      continue;
    }

    app_log_level(APP_LOG_LEVEL_INFO,
                  "%s reported %+3d dBm Central RSSI, %+3d dBm Peripheral RSSI\n",
                  env_device_location_names[i],
                  var_central_rssi[i],
                  var_peripheral_rssi[i]);
  }

  return success;
}

static unsigned int connparams_seq_no = 0;

sl_status_t peps_leader_forward_connection_details(uint8_t connection)
{
  sl_status_t ret;
  peps_cmd_broadcast_connparams_1_t connparams_1;
  peps_cmd_broadcast_connparams_2_t connparams_2;
  peps_cmd_broadcast_connparams_3_t connparams_3;
  peps_cmd_broadcast_connparams_4_t connparams_4;
  peps_cmd_broadcast_connparams_5_t connparams_5;
  unsigned int i;
  void *data[5] = {
    &connparams_1,
    &connparams_2,
    &connparams_3,
    &connparams_4,
    &connparams_5,
  };
  unsigned int len[5] = {
    sizeof(connparams_1),
    sizeof(connparams_2),
    sizeof(connparams_3),
    sizeof(connparams_4),
    sizeof(connparams_5),
  };

  // dummy request to make sure the bus is hot when reaching the time-critical
  //   region
  peps_leader_broadcast_latch_rssi();

  // TIME CRITICAL BEGIN
  ret = sl_bt_connection_get_scheduling_details(
    connection,
    &connparams_1.access_address,
    &connparams_2.role,
    &connparams_2.crc_init,
    &connparams_4.interval,
    &connparams_4.supervision_timeout,
    &connparams_4.central_clock_accuracy,
    &connparams_1.central_phy,
    &connparams_1.peripheral_phy,
    &connparams_1.channel_selection_algorithm,
    &connparams_3.hop,
    &connparams_3.channel_map,
    &connparams_3.channel,
    &connparams_4.event_counter,
    &connparams_5.start_time_us);

  // a toggle on the master side means that the parameters have been fetched
  GPIO_PinOutToggle(MONACT_PORT, MONACT_PIN);

  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to get connection parameters!\n");
    return ret;
  }

  connparams_seq_no++;
  connparams_1.sequence_no = connparams_seq_no;
  connparams_2.sequence_no = connparams_seq_no;
  connparams_3.sequence_no = connparams_seq_no;
  connparams_4.sequence_no = connparams_seq_no;
  connparams_5.sequence_no = connparams_seq_no;

  for (i = 0; i < 5; i++)
  {
    ret = sl_lin_master_transmit(PEPS_ADDR_BROADCAST_CONNPARAMS_1 + i,
                                 (const uint8_t *)data[i], len[i],
                                 true, false, true);
    if (ret != SL_STATUS_OK) {
      app_log_level(APP_LOG_LEVEL_ERROR,
                    "Failed to broadcast connparams #%u (%08" PRIx32 ")!\n",
                    i,
                    ret);
      return ret;
    }
  }

  // TIME CRITICAL END

#if 0
  app_log_level(APP_LOG_LEVEL_INFO, "interval %u "
                                    "supervision_timeout %u "
                                    "cca %u "
                                    "central_phy %u "
                                    "peripheral_phy %u "
                                    "channel_selection_algorithm %u "
                                    "hop %u "
                                    "channel %u "
                                    "channel map %02x %02x %02x %02x %02x "
                                    "event_counter %u\n",
                connparams_4.interval,
                connparams_4.supervision_timeout,
                connparams_4.central_clock_accuracy,
                connparams_1.central_phy,
                connparams_1.peripheral_phy,
                connparams_1.channel_selection_algorithm,
                connparams_3.hop,
                connparams_3.channel,
                connparams_3.channel_map.data[0],
                connparams_3.channel_map.data[1],
                connparams_3.channel_map.data[2],
                connparams_3.channel_map.data[3],
                connparams_3.channel_map.data[4],
                connparams_4.event_counter);
#endif

  for (i = 0; i < env_follower_count; i++)
  {
    uint8_t address = env_follower_address[i];
    peps_cmd_read_connparams_state_t state;

    if (address == SL_LIN_INVALID_ENDPOINT) {
      continue;
    }

    ret = peps_leader_read_connparams_state(address, &state);
    if (ret != SL_STATUS_OK) {
      app_log_level(APP_LOG_LEVEL_ERROR,
                    "Failed to read connparams state of 0x%02x! (%08" PRIx32 ")\n",
                    address,
                    ret);
      // return ret;
      continue;
    }

    if (state.sequence_no != (connparams_seq_no & 0xff)) {
      app_log_level(APP_LOG_LEVEL_ERROR,
                    "Invalid sequence number reported by 0x%02x!\n",
                    address);
      // return SL_STATUS_INVALID_STATE;
      continue;
    }

    if (state.ack_bits != PEPS_ACK_ALL_CONNPARAMS) {
      app_log_level(APP_LOG_LEVEL_ERROR,
                    "Missing connparam reported by 0x%02x!\n",
                    address);
      // return SL_STATUS_INVALID_STATE;
      continue;
    }
  }

  return SL_STATUS_OK;
}
