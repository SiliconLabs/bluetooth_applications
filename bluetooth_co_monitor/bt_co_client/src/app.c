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
 * This software is provided \'as-is\', without any express or implied
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
#include <stdio.h>
#include "sl_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "sl_simple_button.h"
#include "sl_simple_button_instances.h"
#include "app.h"
#include "gatt_db.h"
#include "mikroe_cmt_8540s_smt.h"
#include "sl_sleeptimer.h"
#include "co_client_app.h"
#include "co_client_oled.h"
#include "co_client_nvm.h"

#define CLIENT_BUTTON_EVENT                        (1 << 0)
#define CO_MONITOR_EVENT                           (1 << 1)
#define TIMER_TIMEOUT                              25000
#define TIMER_CO_MONITOR                           10000
#define DATA_BUFFER_SIZE                           5

typedef enum {
  booting,
  scanning,
} conn_state_t;

/**************************************************************************//**
 * Local variables.
 *****************************************************************************/
// The advertised name of the CO sensor (prefix)
static char dev_name_to_cmp[] = "CO_S_";

// Connection state handle.
static conn_state_t conn_state = booting;

// Buffer contains the value of the CO level
static uint32_t value_ppm[DATA_BUFFER_SIZE] = { 0 };

// Status flags of scanning and stop scanning
static bool is_already_scanning_flag = false;

// Buzzer active status handle.
static bool buzzer_active;

// Struct that holds client configuration.
static client_nvm_config_t cfg;

// Timer handle.
static sl_sleeptimer_timer_handle_t my_timer;
static sl_sleeptimer_timer_handle_t co_monitor_timer;

// Connection handle.
static uint8_t app_connection_handle;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

/**************************************************************************//**
 * Local function prototypes.
 *****************************************************************************/

// Connect with phone
static void connection_open_handle(sl_bt_msg_t *evt);

// Find device by name.
static uint8_t find_name_in_advertisement(uint8_t *data, uint8_t len);

// System boot event handler.
static void system_boot_handle(sl_bt_msg_t *evt);

// Scanner legacy advertisement report handler.
static void scanner_legacy_advertisement_report_handle(sl_bt_msg_t *evt);

// User write request event handler.
static void user_write_request_handle(sl_bt_msg_t *evt);

// User read request event handler.
static void user_read_request_handle(sl_bt_msg_t *evt);

// Callback for configuration timeout.
static void timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data);

// Callback for CO monitor
static void co_monitor_callback(sl_sleeptimer_timer_handle_t *handle,
                                void *data);

// External event handler.
static void client_external_event_handle(uint32_t extsignals);

// Handler for monitor timer event
static void co_monitor_timer_event_handler(void);

// Data process
static void co_monitor_data_process(void);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  app_log("BLE Client\r\n");
  client_app_init();
  client_nvm3_get_config(&cfg);

  // Load config from nvm.
  app_log("Client configuration\r\n");
  app_log("Threshold CO PPM: %ld\r\n", cfg.threshold_co_ppm);
  app_log("Notification status: %d\r\n", cfg.is_notification_active);
  app_log("Buzzer volume: %d\r\n", cfg.buzzer_volume);
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_bt_evt_system_boot_id:
      system_boot_handle(evt);
      break;

    case sl_bt_evt_connection_opened_id:
      connection_open_handle(evt);
      break;

    case sl_bt_evt_connection_closed_id:
      break;

    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      scanner_legacy_advertisement_report_handle(evt);
      break;

    case sl_bt_evt_gatt_server_user_write_request_id:
      user_write_request_handle(evt);
      break;

    case sl_bt_evt_gatt_server_user_read_request_id:
      user_read_request_handle(evt);
      break;

    case sl_bt_evt_system_external_signal_id:
      client_external_event_handle(
        evt->data.evt_system_external_signal.extsignals);
      break;

    default:
      break;
  }
}

static void scanner_legacy_advertisement_report_handle(sl_bt_msg_t *evt)
{
  uint32_t sum = 0;
  uint32_t average_value_ppm = 0;
  uint8_t data_advertisement[100] = { 0 };
//  static uint32_t value_ppm[5] = {0};
  static uint32_t counter_ppm;
  static uint32_t counter_ppm_old = 0;

  static uint8_t sample_counter = 0;

  // If a sensor advertisement is found...
  if (find_name_in_advertisement(evt->data.
                                 evt_scanner_legacy_advertisement_report.data.
                                 data,
                                 evt->data.
                                 evt_scanner_legacy_advertisement_report.data.
                                 len)) {
    memcpy(data_advertisement,
           evt->data.evt_scanner_legacy_advertisement_report.data.data,
           evt->data.evt_scanner_legacy_advertisement_report.data.len);

    counter_ppm =
      (data_advertisement[7]
       | (data_advertisement[8] <<
          8) | (data_advertisement[9] << 16) | (data_advertisement[10] << 24));

    if (counter_ppm != counter_ppm_old) {
      value_ppm[sample_counter] = data_advertisement[11]
                                  | (data_advertisement[12] <<
                                     8)
                                  | (data_advertisement[13] <<
                                     16) | (data_advertisement[14] << 24);
      app_log("advertise receive: count: %ld, data in ppm: %ld\r\n",
              counter_ppm,
              value_ppm[sample_counter]);
      sample_counter++;
      counter_ppm_old = counter_ppm;
    }

    if (sample_counter >= 5) {
      sample_counter = 0;

      // calculate average of ppm value
      for (uint8_t i = 0; i < 5; i++) {
        sum += value_ppm[i];
      }
      average_value_ppm = sum / 5;
      app_log("average value (5 sample) %ld\r\n", average_value_ppm);
      sum = 0;

      // update value on oled
      client_oled_app_update_co_level_screen(average_value_ppm,
                                             cfg.threshold_co_ppm);

      // process data:
      co_monitor_data_process();

      if (is_already_scanning_flag == true) {
        // stop scanning
        app_log("stop scanning\r\n");
        sl_bt_scanner_stop();
        is_already_scanning_flag = false;
      }
    }
  }
}

/***************************************************************************//**
 * Parse advertisements looking for the name of the peripheral device
 * @param[in] data: Advertisement packet
 * @param[in] len:  Length of the advertisement packet
 ******************************************************************************/
static uint8_t find_name_in_advertisement(uint8_t *data, uint8_t len)
{
  uint8_t ad_field_length;
  uint8_t ad_field_type;
  uint8_t i = 0;

  // Parse advertisement packet
  while (i < len) {
    ad_field_length = data[i];
    ad_field_type = data[i + 1];
    // Shortened Local Name ($08) or Complete Local Name($09)
    if ((ad_field_type == 0x08) || (ad_field_type == 0x09)) {
      // compare name
      if (memcmp(&data[i + 2], dev_name_to_cmp, 5) == 0) {
        return 1;
      }
    }
    // advance to the next AD struct
    i = i + ad_field_length + 1;
  }
  return 0;
}

/***************************************************************************//**
 * @brief
 *  This function processes the measured values.
 *  Filters the lowest and greatest values, and calculates an average.
 *
 * @param[in] data
 *  Pointer to data storage structure
 *
 * @return
 *  Returns sample filtered.
 ******************************************************************************/
static uint32_t co_calculate_average_ppm(uint32_t *data)
{
  uint32_t sum = 0;
  uint16_t min, max;
  uint8_t i;

  // Calculate average value
  min = max = data[0];
  for (i = 0; i < DATA_BUFFER_SIZE; i++) {
    sum += data[i];
    if (data[i] > max) {
      max = data[i];
    }

    if (data[i] < min) {
      min = data[i];
    }
  }

  // Exclude  min. and max. values
  if (i > 2) {
    i -= 2;
    sum = sum - (min + max);
  }

  return (uint32_t) (sum / i);
}

/**************************************************************************//**
 * Handler function for boot event.
 *****************************************************************************/
void system_boot_handle(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];

  // Extract unique ID from BT Address.
  sc = sl_bt_system_get_identity_address(&address, &address_type);
  app_assert_status(sc);

  // Pad and reverse unique ID to get System ID.
  system_id[0] = address.addr[5];
  system_id[1] = address.addr[4];
  system_id[2] = address.addr[3];
  system_id[3] = 0xFF;
  system_id[4] = 0xFE;
  system_id[5] = address.addr[2];
  system_id[6] = address.addr[1];
  system_id[7] = address.addr[0];

  sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id,
                                               0,
                                               sizeof(system_id),
                                               system_id);
  app_assert_status(sc);

  app_log("BLE stack booted\r\nStack version: %d.%d.%d\r\n",
          evt->data.evt_system_boot.major,
          evt->data.evt_system_boot.minor,
          evt->data.evt_system_boot.patch);

  // Set scanning parameters.
  sc = sl_bt_scanner_set_parameters(sl_bt_scanner_scan_mode_passive, 16, 16);
  app_assert_status(sc);

  app_assert_status_f(sc, "Failed to start discovery\n");

  // Create an advertising set.
  sc = sl_bt_advertiser_create_set(&advertising_set_handle);
  app_assert_status(sc);

  // Generate data for advertising
  sc = sl_bt_legacy_advertiser_generate_data(
    advertising_set_handle,
    sl_bt_advertiser_general_discoverable);
  app_assert_status(sc);

  // Set advertising interval to 100ms.
  sc = sl_bt_advertiser_set_timing(advertising_set_handle, 160, 160, 0, 0);
  app_assert_status(sc);

  if (sl_simple_button_get_state(&sl_button_btn0)) {
    client_oled_app_update_threshold_screen(client_nvm3_get_alarm_threshold(),
                                            client_nvm3_get_notification_status());

    app_log("Enter configuration mode\n");

    sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                       sl_bt_advertiser_connectable_scannable);
    app_assert_status(sc);

    sc = sl_sleeptimer_start_periodic_timer_ms(&my_timer,
                                               TIMER_TIMEOUT,
                                               timer_callback,
                                               NULL,
                                               1,
                                               0);
    app_assert_status(sc);
  } else {
    sc = sl_bt_advertiser_stop(advertising_set_handle);
    app_assert_status(sc);

    // start periodic timer event:
    sc = sl_sleeptimer_start_periodic_timer_ms(&co_monitor_timer,
                                               TIMER_CO_MONITOR,
                                               co_monitor_callback,
                                               NULL,
                                               1,
                                               0);
    app_assert_status(sc);
    conn_state = scanning;
  }
}

/**************************************************************************//**
 * Handler function for user write request event.
 *****************************************************************************/
void user_write_request_handle(sl_bt_msg_t *evt)
{
  sl_status_t sc = SL_STATUS_OK;
  uint8_t value;
  uint32_t threshold;

  switch (evt->data.evt_gatt_server_user_write_request.characteristic) {
    case gattdb_threshold_co_ppm:
      if (evt->data.evt_gatt_server_user_write_request.value.len > 10) {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      } else {
        threshold = (uint32_t)atoi(
          (char *)evt->data.evt_gatt_server_user_write_request.value.data);
        app_log("GATT: write alarm threshold: %ld\r\n", threshold);
        client_nvm3_set_alarm_threshold(threshold);
        cfg.threshold_co_ppm = threshold;
        client_oled_app_update_threshold_screen(cfg.threshold_co_ppm,
                                                cfg.is_notification_active);
        sc = SL_STATUS_OK;
      }
      break;

    case gattdb_notification:
      if (evt->data.evt_gatt_server_user_write_request.value.len != 1) {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      } else {
        value = evt->data.evt_gatt_server_user_write_request.value.data[0] - 48;
        app_log("GATT: write notification status: %d\r\n", value);
        client_nvm3_set_notification_status((bool)value);
        cfg.is_notification_active = (bool)value;
        client_oled_app_update_threshold_screen(cfg.threshold_co_ppm,
                                                cfg.is_notification_active);
        sc = SL_STATUS_OK;
      }
      break;

    case gattdb_buzzer_volume:
      if ((atoi((char *)evt->data.evt_gatt_server_user_write_request.value.data)
           > 10)
          || (atoi((char *)evt->data.evt_gatt_server_user_write_request.value.
                   data)
              < 0)) {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      } else {
        value = atoi(
          (char *)evt->data.evt_gatt_server_user_write_request.value.data);
        app_log("GATT: write buzzer volume: %d\r\n", value);
        client_nvm3_set_buzzer_volume(value);
        mikroe_cmt_8540s_smt_set_duty_cycle(value * 10);
        sc = SL_STATUS_OK;
      }
      break;

    default:
      break;
  }
  sc = sl_bt_gatt_server_send_user_write_response(
    app_connection_handle,
    evt->data.evt_gatt_server_user_write_request.characteristic,
    sc);
  app_assert_status(sc);

  sl_sleeptimer_restart_periodic_timer_ms(&my_timer,
                                          TIMER_TIMEOUT,
                                          timer_callback,
                                          NULL,
                                          1,
                                          0);
  app_assert_status(sc);
}

/**************************************************************************//**
 * Handler function for user read request event.
 *****************************************************************************/
void user_read_request_handle(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bool status;
  uint32_t threshold;
  uint8_t buzzer_volume;
  char res_str[10];
  int len;

  switch (evt->data.evt_gatt_server_user_read_request.characteristic) {
    case gattdb_threshold_co_ppm:
      threshold = client_nvm3_get_alarm_threshold();
      app_log("GATT: read alarm threshold: %ld\r\n", threshold);
      len = snprintf(res_str, 10, "%ld", threshold);
      sc = sl_bt_gatt_server_send_user_read_response(app_connection_handle,
                                                     gattdb_threshold_co_ppm,
                                                     0,
                                                     (size_t)len,
                                                     (uint8_t *)res_str,
                                                     NULL);
      app_assert_status(sc);
      break;

    case gattdb_notification:
      status = client_nvm3_get_notification_status();
      app_log("GATT: read notification status: %d\r\n", status);
      len = snprintf(res_str, 10, "%d", status);
      sc = sl_bt_gatt_server_send_user_read_response(app_connection_handle,
                                                     gattdb_notification,
                                                     0,
                                                     (size_t)len,
                                                     (uint8_t *)res_str,
                                                     NULL);
      app_assert_status(sc);
      break;

    case gattdb_buzzer_volume:
      buzzer_volume = client_nvm3_get_buzzer_volume();
      app_log("GATT: read buzzer volume: %d\r\n", buzzer_volume);
      len = snprintf(res_str, 10, "%d", buzzer_volume);
      sc = sl_bt_gatt_server_send_user_read_response(app_connection_handle,
                                                     gattdb_buzzer_volume,
                                                     0,
                                                     (size_t)len,
                                                     (uint8_t *)res_str,
                                                     NULL);
      app_assert_status(sc);
      break;

    default:
      break;
  }
  sc = sl_sleeptimer_restart_periodic_timer_ms(&my_timer,
                                               TIMER_TIMEOUT,
                                               timer_callback,
                                               NULL,
                                               1,
                                               0);
  app_assert_status(sc);
}

/**************************************************************************//**
 * Handler function for connection open event.
 *****************************************************************************/
void connection_open_handle(sl_bt_msg_t *evt)
{
  app_log("Connected to application\r\n");
  app_connection_handle = evt->data.evt_connection_opened.connection;
}

/**************************************************************************//**
 * Handler function for external event.
 *****************************************************************************/
void client_external_event_handle(uint32_t extsignals)
{
  if (extsignals & CLIENT_BUTTON_EVENT) {
    if (!cfg.is_notification_active) {
      client_nvm3_set_notification_status(true);
      cfg.is_notification_active = true;
    } else {
      client_nvm3_set_notification_status(false);
      cfg.is_notification_active = false;
      if (buzzer_active) {
        buzzer_active = false;
        mikroe_cmt_8540s_smt_pwm_stop();
      }
    }
  }

  if (extsignals & CO_MONITOR_EVENT) {
    co_monitor_timer_event_handler();
  }
}

/**************************************************************************//**
 * System reset function.
 *****************************************************************************/
void timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void) handle;
  (void) data;

  // Restart device
  sl_bt_system_reboot();
}

void co_monitor_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void) handle;
  (void) data;
  sl_bt_external_signal(CO_MONITOR_EVENT);
}

void co_monitor_timer_event_handler(void)
{
  sl_status_t sc;

  if (is_already_scanning_flag == false) {
    // start scanning devices
    sc = sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m,
                             sl_bt_scanner_discover_observation);

    is_already_scanning_flag = true;
    app_assert_status(sc);
    app_log("Start scanning\r\n");
  }
}

void co_monitor_data_process(void)
{
  uint32_t average_ppm_value = co_calculate_average_ppm(value_ppm);
  app_log("average ppm value for control buzzer is: %ld\r\n",
          average_ppm_value);

  if (!cfg.is_notification_active) {
    return;
  }
  if ((uint32_t)average_ppm_value > cfg.threshold_co_ppm) {
    buzzer_active = true;
    mikroe_cmt_8540s_smt_pwm_start();
  } else {
    buzzer_active = false;
    mikroe_cmt_8540s_smt_pwm_stop();
  }
}

void sl_button_on_change(const sl_button_t *handle)
{
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED) {
    if (&sl_button_btn0 == handle) {
      if (conn_state != booting) {
        sl_bt_external_signal(CLIENT_BUTTON_EVENT);
      }
    }
  }
}
