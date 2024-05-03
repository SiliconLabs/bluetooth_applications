/***************************************************************************//**
 * @file peps_follower.c
 * @brief PEPS Follower functionality
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: LicenseRef-MSLA
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement
 * By installing, copying or otherwise using this software, you agree to the
 * terms of the MSLA.
 *
 ******************************************************************************/
#include "peps_follower.h"

#include "env.h"
#include "var.h"
#include "ui.h"
#include "lcd.h"

#include "lin/sl_lin.h"

#include "app_log.h"

#include <em_gpio.h>

#include <inttypes.h>

typedef enum conn_analyzer_command {
  CONN_ANALYZER_IDLE,
  CONN_ANALYZER_START,
  CONN_ANALYZER_STOP,
  CONN_ANALYZER_RESULT,
} conn_analyzer_command_t;

#define RSSI_HISTORY 7

static unsigned char var1[8] =
{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
static unsigned char var2[8] =
{ 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18 };

static volatile peps_cmd_read_connparams_state_t connparams_state = {
  .sequence_no = 0,
  .ack_bits = 0,
};

static uint32_t connparam_access_address;
static uint8_t connparam_role;
static uint32_t connparam_crc_init;
static uint16_t connparam_interval;
static uint16_t connparam_supervision_timeout;
static uint8_t connparam_central_clock_accuracy;
static uint8_t connparam_central_phy;
static uint8_t connparam_peripheral_phy;
static uint8_t connparam_channel_selection_algorithm;
static uint8_t connparam_hop;
static sl_bt_connection_channel_map_t connparam_channel_map;
static uint8_t connparam_channel;
static uint16_t connparam_event_counter;
static int32_t connparam_start_time_us;
static uint8_t connparam_analyzer_handle;

static volatile conn_analyzer_command_t conn_analyzer_command =
  CONN_ANALYZER_IDLE;
static volatile float angle;
static volatile float distance;

static volatile peps_cmd_read_rssi_t curr_rssi = {
  .status = 0,
  .central_rssi = -128,
  .peripheral_rssi = -128,
};

static int8_t central_rssi_history[RSSI_HISTORY];
static int8_t peripheral_rssi_history[RSSI_HISTORY];
static int rssi_elems = 0;
static int rssi_ptr = 0;

static int rssi_compare(const void *a, const void *b)
{
  int8_t left = *(const int8_t *)a;
  int8_t right = *(const int8_t *)b;

  if (left < right) {
    return -1;
  } else if (left > right) {
    return 1;
  } else {
    return 0;
  }
}

static void process_connparams_1(const void *arg);
static void process_connparams_2(const void *arg);
static void process_connparams_3(const void *arg);
static void process_connparams_4(const void *arg);
static void process_connparams_5(const void *arg);

typedef void (*process_connparms_t)(const void *param);
static process_connparms_t process_connparams[5] = {
  process_connparams_1,
  process_connparams_2,
  process_connparams_3,
  process_connparams_4,
  process_connparams_5,
};

static void cmd_broadcast_disconnect(uint8_t frame_id,
                                     bool writable,
                                     uint8_t *data,
                                     int len,
                                     bool success)
{
  (void)frame_id;
  (void)writable;
  (void)data;
  (void)len;

  if (!success) {
    app_log_level(APP_LOG_LEVEL_WARNING, "Failed broadcast of disconnect\n");
  } else {
    app_log_level(APP_LOG_LEVEL_INFO, "Disconnect has been broadcast\n");

    conn_analyzer_command = CONN_ANALYZER_STOP;
  }
}

static void cmd_broadcast_connparams(uint8_t frame_id,
                                     bool writable,
                                     uint8_t *data,
                                     int len,
                                     bool success)
{
  (void)writable;
  (void)len;

  if (!success) {
    app_log_level(APP_LOG_LEVEL_WARNING,
                  "Failed broadcast of connparam %u\n",
                  frame_id);
    connparams_state.ack_bits = 0;
    connparams_state.sequence_no = 0;
  } else {
    app_log_level(APP_LOG_LEVEL_INFO,
                  "Connparam %u has been broadcast\n",
                  frame_id);

    process_connparams[frame_id - 1](data);

    connparams_state.ack_bits |= (1U << (frame_id - 1U));

    if (connparams_state.ack_bits == PEPS_ACK_ALL_CONNPARAMS) {
      GPIO_PinOutToggle(MONACT_PORT, MONACT_PIN);
      conn_analyzer_command = CONN_ANALYZER_START;
    }
  }

  sl_lin_slave_update_readable_endpoint(PEPS_ADDR_READ_CONNPARAMS_STATE(
                                          env_base_address),
                                        (const uint8_t *)&connparams_state);
}

static void process_connparams_1(const void *arg)
{
  const peps_cmd_broadcast_connparams_1_t *param =
    (const peps_cmd_broadcast_connparams_1_t *)arg;

  if (connparams_state.sequence_no != param->sequence_no) {
    connparams_state.sequence_no = param->sequence_no;
    connparams_state.ack_bits = 0;
  }

  connparam_access_address = param->access_address;
  connparam_central_phy = param->central_phy;
  connparam_peripheral_phy = param->peripheral_phy;
  connparam_channel_selection_algorithm = param->channel_selection_algorithm;
}

static void process_connparams_2(const void *arg)
{
  const peps_cmd_broadcast_connparams_2_t *param =
    (const peps_cmd_broadcast_connparams_2_t *)arg;

  if (connparams_state.sequence_no != param->sequence_no) {
    connparams_state.sequence_no = param->sequence_no;
    connparams_state.ack_bits = 0;
  }

  connparam_crc_init = param->crc_init;
  connparam_role = param->role;
}

static void process_connparams_3(const void *arg)
{
  const peps_cmd_broadcast_connparams_3_t *param =
    (const peps_cmd_broadcast_connparams_3_t *)arg;

  if (connparams_state.sequence_no != param->sequence_no) {
    connparams_state.sequence_no = param->sequence_no;
    connparams_state.ack_bits = 0;
  }

  connparam_channel_map = param->channel_map;
  connparam_channel = param->channel;
  connparam_hop = param->hop;
}

static void process_connparams_4(const void *arg)
{
  const peps_cmd_broadcast_connparams_4_t *param =
    (const peps_cmd_broadcast_connparams_4_t *)arg;

  if (connparams_state.sequence_no != param->sequence_no) {
    connparams_state.sequence_no = param->sequence_no;
    connparams_state.ack_bits = 0;
  }

  connparam_interval = param->interval;
  connparam_supervision_timeout = param->supervision_timeout;
  connparam_event_counter = param->event_counter;
  connparam_central_clock_accuracy = param->central_clock_accuracy;
}

static void process_connparams_5(const void *arg)
{
  const peps_cmd_broadcast_connparams_5_t *param =
    (const peps_cmd_broadcast_connparams_5_t *)arg;

  if (connparams_state.sequence_no != param->sequence_no) {
    connparams_state.sequence_no = param->sequence_no;
    connparams_state.ack_bits = 0;
  }

  connparam_start_time_us = param->start_time_us;
}

static void cmd_read_connparams_state(uint8_t frame_id,
                                      bool writable,
                                      uint8_t *data,
                                      int len,
                                      bool success)
{
  (void)frame_id;
  (void)writable;
  (void)data;
  (void)len;

  if (success) {
    // app_log_level(APP_LOG_LEVEL_INFO, "Connparams state has been read\n");
  } else {
    app_log_level(APP_LOG_LEVEL_WARNING,
                  "Failed request to read connparams state\n");
  }

  // a state read means that the current iteration has ended
  connparams_state.ack_bits = 0;
}

static void cmd_read_var1(uint8_t frame_id,
                          bool writable,
                          uint8_t *data,
                          int len,
                          bool success)
{
  (void)frame_id;
  (void)writable;
  (void)data;
  (void)len;

  if (success) {
    app_log_level(APP_LOG_LEVEL_INFO, "var1 has been read\n");
  } else {
    app_log_level(APP_LOG_LEVEL_WARNING, "Failed request to read var1\n");
  }
}

static void cmd_read_var2(uint8_t frame_id,
                          bool writable,
                          uint8_t *data,
                          int len,
                          bool success)
{
  (void)frame_id;
  (void)writable;
  (void)data;
  (void)len;

  if (success) {
    app_log_level(APP_LOG_LEVEL_INFO, "var2 has been read\n");
  } else {
    app_log_level(APP_LOG_LEVEL_WARNING, "Failed request to read var2\n");
  }
}

static void cmd_read_rssi(uint8_t frame_id,
                          bool writable,
                          uint8_t *data,
                          int len,
                          bool success)
{
  (void)frame_id;
  (void)writable;
  (void)len;

  if (success) {
    curr_rssi = *(peps_cmd_read_rssi_t *)data;
    // conn_analyzer_command = CONN_ANALYZER_RESULT;
    // app_log_level(APP_LOG_LEVEL_INFO, "RSSI read completed\n");
  } else {
    app_log_level(APP_LOG_LEVEL_WARNING, "RSSI read failed\n");
  }
}

static void cmd_write_var2(uint8_t frame_id,
                           bool writable,
                           uint8_t *data,
                           int len,
                           bool success)
{
  (void)writable;
  (void)len;

  if (success) {
    sl_lin_slave_update_readable_endpoint(frame_id, data);
    app_log_level(APP_LOG_LEVEL_INFO, "Updated var2\n");
  } else {
    app_log_level(APP_LOG_LEVEL_WARNING, "Failed request to update var2\n");
  }
}

static void cmd_write_result(uint8_t frame_id,
                             bool writable,
                             uint8_t *data,
                             int len,
                             bool success)
{
  (void)frame_id;
  (void)writable;
  (void)len;
  peps_cmd_write_result_t *result = (peps_cmd_write_result_t *)data;

  if (success) {
    distance = result->distance;
    angle = result->angle;
    conn_analyzer_command = CONN_ANALYZER_RESULT;
    // app_log_level(APP_LOG_LEVEL_INFO, "Result update completed.\n");
  } else {
    app_log_level(APP_LOG_LEVEL_WARNING,
                  "Failed request to update the result\n");
  }
}

sl_status_t peps_follower_update_rssi(int8_t central_rssi,
                                      int8_t peripheral_rssi)
{
  CORE_DECLARE_IRQ_STATE;

  CORE_ENTER_CRITICAL();

  central_rssi_history[rssi_ptr] = central_rssi;
  peripheral_rssi_history[rssi_ptr] = peripheral_rssi;
  if (rssi_elems < RSSI_HISTORY) {
    rssi_elems++;
  }

  CORE_EXIT_CRITICAL();

  rssi_ptr = (rssi_ptr + 1) % RSSI_HISTORY;

  return SL_STATUS_OK;
}

static void cmd_broadcast_latch_rssi(uint8_t frame_id,
                                     bool writable,
                                     uint8_t *data,
                                     int len,
                                     bool success)
{
  (void)frame_id;
  (void)writable;
  (void)data;
  (void)len;

  if (!success) {
    app_log_level(APP_LOG_LEVEL_WARNING, "Failed broadcast of latch\n");
    return;
  }

  peps_cmd_read_rssi_t new_rssi;

  new_rssi.status = curr_rssi.status;

  if (rssi_elems) {
    int8_t rssi_work[RSSI_HISTORY];

    // TODO: should move this out of atomic context...
    // however RSSI_HISTORY shall be small enough anyway to be useful
    // and for low values the runtime should be low enough to keep this here

    memcpy(rssi_work, central_rssi_history, sizeof(int8_t) * rssi_elems);
    qsort(rssi_work, sizeof(int8_t), rssi_elems, rssi_compare);
    new_rssi.central_rssi = rssi_work[(rssi_elems + 1) / 2];

    memcpy(rssi_work, peripheral_rssi_history, sizeof(int8_t) * rssi_elems);
    qsort(rssi_work, sizeof(int8_t), rssi_elems, rssi_compare);
    new_rssi.peripheral_rssi = rssi_work[(rssi_elems + 1) / 2];
  } else {
    new_rssi.central_rssi = -128;
    new_rssi.peripheral_rssi = -128;
  }

  sl_lin_slave_update_readable_endpoint(PEPS_ADDR_READ_RSSI(env_base_address),
                                        (const uint8_t *)&new_rssi);
}

sl_status_t peps_follower_init(void)
{
  sl_status_t ret;

  GPIO_PinModeSet(MONACT_PORT, MONACT_PIN, gpioModePushPull, 0);

#if defined(MODEM_DOUT_PORT) && defined(PRS_MODEM_DOUT)
  PRS_Setup(MODEM_DOUT_PRS,
            prsTypeDefault,
            PRS_RACL_ACTIVE,
            MODEM_DOUT_PORT,
            MODEM_DOUT_PIN,
            MODEM_DOUT_LOC);
#endif

  ret = sl_lin_slave_register_writable_endpoint(
    PEPS_ADDR_BROADCAST_CONNPARAMS_1,
    sizeof(
      peps_cmd_broadcast_connparams_1_t),
    cmd_broadcast_connparams,
    true);
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to register %s!\n",
                  "PEPS_ADDR_BROADCAST_CONNPARAMS_1");
    return ret;
  }

  ret = sl_lin_slave_register_writable_endpoint(
    PEPS_ADDR_BROADCAST_CONNPARAMS_2,
    sizeof(
      peps_cmd_broadcast_connparams_2_t),
    cmd_broadcast_connparams,
    true);
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to register %s!\n",
                  "PEPS_ADDR_BROADCAST_CONNPARAMS_2");
    return ret;
  }

  ret = sl_lin_slave_register_writable_endpoint(
    PEPS_ADDR_BROADCAST_CONNPARAMS_3,
    sizeof(
      peps_cmd_broadcast_connparams_3_t),
    cmd_broadcast_connparams,
    true);
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to register %s!\n",
                  "PEPS_ADDR_BROADCAST_CONNPARAMS_3");
    return ret;
  }

  ret = sl_lin_slave_register_writable_endpoint(
    PEPS_ADDR_BROADCAST_CONNPARAMS_4,
    sizeof(
      peps_cmd_broadcast_connparams_4_t),
    cmd_broadcast_connparams,
    true);
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to register %s!\n",
                  "PEPS_ADDR_BROADCAST_CONNPARAMS_4");
    return ret;
  }

  ret = sl_lin_slave_register_writable_endpoint(
    PEPS_ADDR_BROADCAST_CONNPARAMS_5,
    sizeof(
      peps_cmd_broadcast_connparams_5_t),
    cmd_broadcast_connparams,
    true);
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to register %s!\n",
                  "PEPS_ADDR_BROADCAST_CONNPARAMS_5");
    return ret;
  }

  ret = sl_lin_slave_register_writable_endpoint(PEPS_ADDR_BROADCAST_DISCONNECT,
                                                sizeof(
                                                  peps_cmd_broadcast_disconnect_t),
                                                cmd_broadcast_disconnect,
                                                true);
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to register %s!\n",
                  "PEPS_ADDR_BROADCAST_DISCONNECT");
    return ret;
  }

  ret =
    sl_lin_slave_register_readable_endpoint(PEPS_ADDR_READ_VAR1(
                                              env_base_address),
                                            sizeof(peps_cmd_read_var1_t),
                                            cmd_read_var1,
                                            var1,
                                            true);
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to register %s!\n",
                  "PEPS_ADDR_READ_VAR1");
    return ret;
  }

  ret =
    sl_lin_slave_register_readable_endpoint(PEPS_ADDR_READ_VAR2(
                                              env_base_address),
                                            sizeof(peps_cmd_read_var2_t),
                                            cmd_read_var2,
                                            var2,
                                            true);
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to register %s!\n",
                  "PEPS_ADDR_READ_VAR2");
    return ret;
  }

  ret =
    sl_lin_slave_register_writable_endpoint(PEPS_ADDR_WRITE_VAR2(
                                              env_base_address),
                                            sizeof(peps_cmd_write_var2_t),
                                            cmd_write_var2,
                                            true);
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to register %s!\n",
                  "PEPS_ADDR_WRITE_VAR2");
    return ret;
  }

  ret =
    sl_lin_slave_register_readable_endpoint(PEPS_ADDR_READ_RSSI(
                                              env_base_address),
                                            sizeof(peps_cmd_read_rssi_t),
                                            cmd_read_rssi,
                                            (const uint8_t *)&curr_rssi,
                                            true);
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to register %s!\n",
                  "PEPS_ADDR_READ_RSSI");
    return ret;
  }

  ret =
    sl_lin_slave_register_readable_endpoint(PEPS_ADDR_READ_CONNPARAMS_STATE(
                                              env_base_address),
                                            sizeof(
                                              peps_cmd_read_connparams_state_t),
                                            cmd_read_connparams_state,
                                            (const uint8_t *)&connparams_state,
                                            true);
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to register %s!\n",
                  "PEPS_ADDR_READ_CONNPARAMS_STATE");
    return ret;
  }

  ret =
    sl_lin_slave_register_writable_endpoint(PEPS_ADDR_WRITE_RESULT(
                                              env_base_address),
                                            sizeof(peps_cmd_write_result_t),
                                            cmd_write_result,
                                            true);
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to register %s!\n",
                  "PEPS_ADDR_WRITE_RESULT");
    return ret;
  }

  ret = sl_lin_slave_register_writable_endpoint(PEPS_ADDR_BROADCAST_LATCH_RSSI,
                                                sizeof(
                                                  peps_cmd_broadcast_latch_rssi_t),
                                                cmd_broadcast_latch_rssi,
                                                true);
  if (ret != SL_STATUS_OK) {
    app_log_level(APP_LOG_LEVEL_ERROR,
                  "Failed to register %s!\n",
                  "PEPS_ADDR_BROADCAST_LATCH_RSSI");
    return ret;
  }

  sl_lin_slave_init();

  return SL_STATUS_OK;
}

void peps_follower_tick(void)
{
  sl_status_t ret;

  switch (conn_analyzer_command)
  {
    case CONN_ANALYZER_IDLE:
      break;

    case CONN_ANALYZER_START:
      conn_analyzer_command = CONN_ANALYZER_IDLE;

      // time offset between the function calls due to the latency of LIN bus
      connparam_start_time_us = -47000;

      // a toggle on the slave side means that the parameters are about to be
      //   set
      // and the monitoring is about to be started
      GPIO_PinOutToggle(MONACT_PORT, MONACT_PIN);

      ret = sl_bt_connection_analyzer_start(connparam_access_address,
                                            connparam_crc_init,
                                            connparam_interval,
                                            connparam_supervision_timeout,
                                            connparam_central_clock_accuracy,
                                            connparam_central_phy,
                                            connparam_peripheral_phy,
                                            connparam_channel_selection_algorithm,
                                            connparam_hop,
                                            &connparam_channel_map,
                                            connparam_channel,
                                            connparam_event_counter,
                                            connparam_start_time_us,
                                            SL_BT_CONNECTION_ANALYZER_RELATIVE_TIME,
                                            // 0,
                                            &connparam_analyzer_handle);

      // a subsequent toggle on the slave means that the call has returned
      GPIO_PinOutToggle(MONACT_PORT, MONACT_PIN);

      if (ret == SL_STATUS_OK) {
        curr_rssi.status = 1;
      } else {
        app_log_level(APP_LOG_LEVEL_WARNING,
                      "Failed to start connection analyzer (%08" PRIx32 ")\n",
                      ret);
        connparams_state.ack_bits = 0;
      }

      break;

    case CONN_ANALYZER_STOP:
      conn_analyzer_command = CONN_ANALYZER_IDLE;
      if (curr_rssi.status) {
        curr_rssi.status = 0;
        sl_bt_connection_analyzer_stop(connparam_analyzer_handle);
        peps_follower_sniffer_stopped();
      }

      break;

    case CONN_ANALYZER_RESULT:
      conn_analyzer_command = CONN_ANALYZER_IDLE;
      ui_set_rssi(curr_rssi.central_rssi);
      ui_set_distance(distance);
      ui_set_angle(angle);
      lcd_update();

      break;

    default:;
  }
}

void peps_follower_sniffer_stopped(void)
{
  curr_rssi.status = 0;
  curr_rssi.central_rssi = -128;
  curr_rssi.peripheral_rssi = -128;

  rssi_elems = 0;
  rssi_ptr = 0;

  ui_clear_rssi();
  ui_clear_distance();
  ui_clear_angle();
  lcd_update();
}
