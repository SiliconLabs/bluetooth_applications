/***************************************************************************//**
 * @file
 * @brief Core application logic.
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
#include <sensor_nvm.h>
#include "sl_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "stdio.h"
#include "sl_sleeptimer.h"
#include "sl_simple_button_instances.h"
#include "sl_i2cspm_instances.h"
#include "bma400.h"
#include "mikroe_bma400_i2c.h"
#include "mikroe_bma400_i2c_config.h"
#include "sl_string.h"

/* Macro to determine count of activity change for each axis */
#define OPERATION_TIMEOUT_MS               (1000)

#define SENSOR_TIMER_EXT_SIG               0x01
// 2 main state
#define STATE_IDLE                         0
#define STATE_OPERATION                    1

// Advertising flags (common).
#define ADVERTISE_FLAGS_LENGTH             0x02
#define ADVERTISE_FLAGS_TYPE               0x01
#define ADVERTISE_FLAGS_DATA               0x06

// Complete local name.
#define DEVICE_NAME_LENGTH                 13
#define DEVICE_NAME_TYPE                   0x09
#define DEVICE_NAME                        "bma400_sensor"
// Manufacturer ID (0x02FF - Silicon Labs' company ID)
#define MANUF_ID                           0x02FF
// 1+2+8 bytes for type, company ID and the payload
#define MANUF_LENGTH                       5
#define MANUF_TYPE                         0xFF

SL_PACK_START(1)
typedef struct
{
  uint8_t len_flags;
  uint8_t type_flags;
  uint8_t val_flags;

  uint8_t len_manuf;
  uint8_t type_manuf;
  // First two bytes must contain the manufacturer ID (little-endian order)
  uint8_t company_LO;
  uint8_t company_HI;

  // The next bytes are freely configurable
  // using 1 bytes for operation time value, 1 byte to count
  uint8_t data_counter;
  uint8_t operation_time;

  // length of the name AD element is variable,
  // adding it last to keep things simple
  uint8_t len_name;
  uint8_t type_name;

  // NAME_MAX_LENGTH must be sized
  // so that total length of data does not exceed 31 bytes
  uint8_t name[DEVICE_NAME_LENGTH + 1];

  // These values are NOT included in the actual advertising payload,
  // just for bookkeeping
  char dummy;        // Space for null terminator
  uint8_t data_size; // Actual length of advertising data
} SL_ATTRIBUTE_PACKED advertising_packet_t;
SL_PACK_END()

// Advertising data.
static advertising_packet_t advertising_data = {
  .len_flags = ADVERTISE_FLAGS_LENGTH,
  .type_flags = ADVERTISE_FLAGS_TYPE,
  .val_flags = ADVERTISE_FLAGS_DATA,

  // manufacturer specific data
  .len_manuf = MANUF_LENGTH,
  .type_manuf = MANUF_TYPE,
  .company_LO = MANUF_ID & 0xFF,
  .company_HI = (MANUF_ID >> 8) & 0xFF,
  // for custom data
  .data_counter = 0,
  .operation_time = 0,

  // length of name element is the name string length + 1 for the AD type
  .len_name = DEVICE_NAME_LENGTH + 1,
  .type_name = DEVICE_NAME_TYPE,
  .name = DEVICE_NAME,

  // Calculate total length of advertising data
  .data_size = 3 + (1 + MANUF_LENGTH) + (1 + DEVICE_NAME_LENGTH + 1),
};

// static main_state = STATE_IDLE;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static uint8_t notification_status = 0;

// timer to check activity
static sl_sleeptimer_timer_handle_t sensor_timer;
// timer for measure operation time
static sl_sleeptimer_timer_handle_t operation_timer;

// variable for measure operation time
static uint32_t operation_current_tick = 0;
static uint32_t operation_last_tick = 0;
static bool operation_time_updated = false;

static uint32_t operation_time;

// interrupt status of bma400 sensor
uint16_t int_status;

// variable for bma400
struct bma400_dev bma;
struct bma400_sensor_data accel;
struct bma400_sensor_conf accel_setting[2] = { { 0 } };
struct bma400_int_enable int_en;

/**************************************************************************//**
 * Static functions declaration.
 *****************************************************************************/
// timer callback
static void sensor_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                  void *data);

static sl_status_t sensor_bma400_init(void);
static void sensor_timer_external_signal_handler(void);

static void update_adv_data(void);

void operation_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void) handle;
  (void) data;

  if (operation_current_tick != operation_last_tick) {
    operation_time += (sl_sleeptimer_tick_to_ms(operation_current_tick
                                                - operation_last_tick)) / 1000;
    operation_time_updated = true;
    operation_current_tick = 0;
    operation_last_tick = 0;
  }
}

void operation_detected(void)
{
  if (operation_last_tick == 0) {
    // Activity start point
    operation_last_tick = sl_sleeptimer_get_tick_count();
    if (operation_last_tick == 0) {
      operation_last_tick = 1;
    }
    operation_current_tick = operation_last_tick;
  } else {
    operation_current_tick = sl_sleeptimer_get_tick_count();
  }

  sl_sleeptimer_restart_timer_ms(&operation_timer,
                                 OPERATION_TIMEOUT_MS,
                                 operation_timer_callback,
                                 NULL,
                                 0,
                                 0);
}

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_status_t sc;

  sensor_config_nvm3_init();

  sc = sensor_bma400_init();
  if (sc == SL_STATUS_OK) {
    app_log("Sensor device init successful\r\n");
  }

  if (sl_simple_button_get_state(&sl_button_btn0) == SL_SIMPLE_BUTTON_PRESSED) {
    sc = sensor_nvm3_set_operation_time(0);
    app_assert_status(sc);
    app_log("reset operation time\r\n");
  }
  // delete old bonding
  sc = sl_bt_sm_delete_bondings();
  app_assert_status(sc);

  sc = sensor_nvm3_get_operation_time(&operation_time);
  app_assert_status(sc);

  sl_sleeptimer_start_timer_ms(&operation_timer,
                               OPERATION_TIMEOUT_MS,
                               operation_timer_callback,
                               NULL, 0, 0);
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      app_log("\r\n------ Silicon Labs BLE Operation time Measurement "
              "Demo: Sensor Role-----\r\n");
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);

      // Set custom advertising packet
      sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                            sl_bt_advertiser_advertising_data_packet,
                                            advertising_data.data_size,
                                            (uint8_t *)&advertising_data);
      app_assert_status(sc);

      // Enable connections
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);

      app_assert_status(sc);
      //
      sc = sl_sleeptimer_start_periodic_timer_ms(&sensor_timer,
                                                 100,
                                                 sensor_timer_callback,
                                                 NULL, 0, 0);
      app_assert_status(sc);

      // configure security before start advertise
      sc = sl_bt_sm_configure(0, sl_bt_sm_io_capability_noinputnooutput);
      app_assert_status(sc);
      sc = sl_bt_sm_set_bondable_mode(1);
      app_assert_status(sc);
      sc = sl_bt_sm_store_bonding_configuration(4, 0x2);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("connection opened\r\n");
      sl_bt_sm_increase_security(evt->data.evt_connection_opened.connection);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("connection closed\r\n");
      sc = sl_bt_sm_delete_bondings();
      app_assert_status(sc);

      // Set custom advertising payload
      sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                            sl_bt_advertiser_advertising_data_packet,
                                            advertising_data.data_size,
                                            (uint8_t *)&advertising_data);
      app_assert_status(sc);
      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    case sl_bt_evt_sm_bonded_id:
      app_log("sm_bonded event: bond handle %d and security mode %d\r\n",
              evt->data.evt_sm_bonded.bonding,
              evt->data.evt_sm_bonded.security_mode + 1);
      break;

    // -------------------------------
    case sl_bt_evt_gatt_server_user_read_request_id:
    {
      uint16_t sent_len;
      char data_send[10];

      sc = sensor_nvm3_get_operation_time(&operation_time);
      app_assert_status(sc);
      snprintf(data_send, 10, "%lu", operation_time);

      if (evt->data.evt_gatt_server_user_read_request.characteristic
          == gattdb_operation_time) {
        sc = sl_bt_gatt_server_send_user_read_response(
          evt->data.evt_gatt_server_user_read_request.connection,
          gattdb_operation_time,
          SL_STATUS_OK,
          sizeof(operation_time),
          (uint8_t *)data_send,
          &sent_len);
      }
    }
    break;

    // -------------------------------
    case sl_bt_evt_gatt_server_user_write_request_id:
      if ((evt->data.evt_gatt_server_user_write_request.characteristic
           == gattdb_operation_time)
          && (evt->data.evt_gatt_server_user_write_request.value.data[0]
              == 1)) {
        operation_time = 0;
        sensor_nvm3_set_operation_time(operation_time);
      }
      sc = sl_bt_gatt_server_send_user_write_response(
        evt->data.evt_gatt_server_user_write_request.connection,
        evt->data.evt_gatt_server_user_write_request.characteristic,
        SL_STATUS_OK);
      app_assert_status(sc);
      break;

    // -------------------------------
    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (evt->data.evt_gatt_server_characteristic_status.characteristic
          == gattdb_operation_time) {
        notification_status =
          evt->data.evt_gatt_server_characteristic_status.client_config_flags;
        if (notification_status) {
          char data_send[10];
          app_log("client turn on notification\r\n");
          snprintf(data_send, 10, "%lu", operation_time);
          sl_bt_gatt_server_notify_all(gattdb_operation_time,
                                       sl_strlen(data_send),
                                       (uint8_t *)data_send);
        } else {
          app_log("client turn off notification\r\n");
        }
      }
      break;
    // -------------------------------
    case sl_bt_evt_sm_bonding_failed_id:
      app_log("bonding failed, reason 0x%4X\r\n",
              evt->data.evt_sm_bonding_failed.reason);
      break;

    // -------------------------------
    case sl_bt_evt_system_external_signal_id:
      if (evt->data.evt_system_external_signal.extsignals
          == SENSOR_TIMER_EXT_SIG) {
        sensor_timer_external_signal_handler();
      }
      break;
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

/**************************************************************************//**
 * Static function definition.
 *****************************************************************************/
static sl_status_t sensor_bma400_init(void)
{
  sl_status_t sc;

  sc = bma400_i2c_init(sl_i2cspm_mikroe, MIKROE_BMA400_ADDR, &bma);
  app_assert_status(sc);

  sc = bma400_soft_reset(&bma);
  app_assert_status(sc);

  sc = bma400_init(&bma);
  app_assert_status(sc);

  accel_setting[0].type = BMA400_ACTIVITY_CHANGE_INT;
  accel_setting[1].type = BMA400_ACCEL;

  sc = bma400_get_sensor_conf(accel_setting, 2, &bma);
  app_assert_status(sc);

  accel_setting[0].param.act_ch.int_chan = BMA400_INT_CHANNEL_1;
  accel_setting[0].param.act_ch.axes_sel = BMA400_AXIS_XYZ_EN;
  accel_setting[0].param.act_ch.act_ch_ntps = BMA400_ACT_CH_SAMPLE_CNT_64;
  accel_setting[0].param.act_ch.data_source = BMA400_DATA_SRC_ACC_FILT1;
  accel_setting[0].param.act_ch.act_ch_thres = 5;

  accel_setting[1].param.accel.odr = BMA400_ODR_100HZ;
  accel_setting[1].param.accel.range = BMA400_RANGE_2G;
  accel_setting[1].param.accel.data_src = BMA400_DATA_SRC_ACCEL_FILT_1;

  /* Set the desired configurations to the sensor */
  sc = bma400_set_sensor_conf(accel_setting, 2, &bma);
  app_assert_status(sc);

  sc = bma400_set_power_mode(BMA400_MODE_NORMAL, &bma);
  app_assert_status(sc);

  int_en.type = BMA400_ACTIVITY_CHANGE_INT_EN;
  int_en.conf = BMA400_ENABLE;

  sc = bma400_enable_interrupt(&int_en, 1, &bma);
  app_assert_status(sc);

  return sc;
}

static void sensor_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                  void *data)
{
  (void) handle;
  (void) data;

  sl_bt_external_signal(SENSOR_TIMER_EXT_SIG);
}

static void update_adv_data(void)
{
  sl_status_t sc;

  // Update the two variable fields in the custom advertising packet
  advertising_data.operation_time = operation_time;
  advertising_data.data_counter++;
  // Set custom advertising payload
  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                        sl_bt_advertiser_advertising_data_packet,
                                        advertising_data.data_size,
                                        (uint8_t *)&advertising_data);

  app_assert_status(sc);
  app_log("The custom advertising set is updated\r\n");
}

static void sensor_timer_external_signal_handler(void)
{
  sl_status_t sc;
  char data_send[10];

  sc = bma400_get_interrupt_status(&int_status, &bma);
  app_assert_status(sc);

  if (int_status & BMA400_ASSERTED_ACT_CH_X) {
    sc = bma400_get_accel_data(BMA400_DATA_SENSOR_TIME, &accel, &bma);
    if (sc == BMA400_OK) {
      app_log("detect X changed sensor time: %lu\r\n", accel.sensortime);
    }
    operation_detected();
  }

  if (int_status & BMA400_ASSERTED_ACT_CH_Y) {
    sc = bma400_get_accel_data(BMA400_DATA_SENSOR_TIME, &accel, &bma);
    if (sc == BMA400_OK) {
      app_log("detect Y changed sensor time: %lu\r\n", accel.sensortime);
    }
    operation_detected();
  }

  if (int_status & BMA400_ASSERTED_ACT_CH_Z) {
    sc = bma400_get_accel_data(BMA400_DATA_SENSOR_TIME, &accel, &bma);
    if (sc == BMA400_OK) {
      app_log("detect Z changed sensor time: %lu\r\n", accel.sensortime);
    }
    operation_detected();
  }

  if (operation_time_updated) {
    operation_time_updated = false;
    // update operation time
    app_log("operation time update: %lu\r\n", operation_time);
    sensor_nvm3_set_operation_time(operation_time);

    // send notification for new operation time
    snprintf(data_send, 10, "%lu", operation_time);
    if (notification_status) {
      sc = sl_bt_gatt_server_notify_all(gattdb_operation_time,
                                        sl_strlen(data_send),
                                        (uint8_t *)data_send);
      if (sc != SL_STATUS_OK) {
        app_log("Failed to send notification\r\n");
      }
    }
    update_adv_data();
  }
}
