/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
#include <string.h>
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app_log.h"
#include "sl_sleeptimer.h"
#include "sl_simple_button_instances.h"
#include "client_app.h"
#include "client_nvm.h"
#include "mikroe_cmt_8540s_smt.h"
#include "app.h"

#define client_BUTTON_EVENT                         1
#define TIMER_TIMEOUT                               10000

typedef enum {
  booting,
  scanning,
  opening,
  discover_services,
  discover_characteristics,
  enable_notification,
  running
} conn_state_t;

/**************************************************************************//**
 * Local variables.
 *****************************************************************************/
// The advertised name of the dosimeter unit
static char dev_name[] = "DM_SENSOR";

// Connection state handle.
static conn_state_t conn_state = booting;

// Dosimeter sensor service handle.
static uint32_t service_handle;

// Radiation characteristic handle.
static uint16_t characteristic_handle;

// Dosimeter sensor service UUID 128.
static const uint8_t dosimeter_sensor_service[16] = {
  0x4e, 0xc7, 0xa8, 0x22, 0xe5, 0x2b, 0x42, 0xb9,
  0xd4, 0x4b, 0xba, 0x7d, 0x54, 0xe0, 0xae, 0x9d
};

// Radiation characteristic UUID 128.
static const uint8_t radiation_characteristic[16] = {
  0x59, 0x51, 0xa2, 0xc7, 0xbc, 0xac, 0x0d, 0x8e,
  0x86, 0x4c, 0xff, 0x39, 0xeb, 0x90, 0xa0, 0x0d,
};

// Buzzer active status handle.
static bool buzzer_active;

// Struct that holds client configuration.
static client_nvm_config_t cfg;

// Timer handle.
static sl_sleeptimer_timer_handle_t my_timer;

// Connection handle.
static uint8_t sensor_connection_handle;
static uint8_t app_connection_handle;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

/**************************************************************************//**
 * Local function prototypes.
 *****************************************************************************/

// Find device by name.
static uint8_t find_name_in_advertisement(uint8_t *data, uint8_t len);

// System boot event handler.
static void system_boot_handle(sl_bt_msg_t *evt);

// Connection open event handler.
static void connection_open_handle(sl_bt_msg_t *evt);

// Connection close event handler.
static void connection_close_handle(sl_bt_msg_t *evt);

// Scanner legacy advertisement report handler.
static void scanner_legacy_advertisement_report_handle(sl_bt_msg_t *evt);

// Notification received event handler.
static void receive_notification(sl_bt_msg_t *evt);

// User write request event handler.
static void user_write_request_handle(sl_bt_msg_t *evt);

// User read request event handler.
static void user_read_request_handle(sl_bt_msg_t *evt);

static void comsumer_bt_evt_gatt_procedure_completed(sl_bt_msg_t *evt);

// Service found hanlder.
static void client_gatt_service(sl_bt_msg_t *evt);

// Characteristic found handler.
static void client_gatt_characteristic(sl_bt_msg_t *evt);

// Callback for configuration timeout.
static void timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data);

// External event handler.
static void client_external_event_handle();

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  app_log("BLE - Dosimeter (Sparkfun Type 5) - Client\r\n");
  client_app_init();
  client_nvm3_get_config(&cfg);

  // Load config from nvm.
  app_log("Client configuration\r\n");
  app_log("Alarm threshold: %.2f\r\n", cfg.alarm_threshold);
  app_log("Notification status: %d\r\n", cfg.notification_status);
  app_log("Click noise status: %d\r\n", cfg.click_noise_status);
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
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
      connection_close_handle(evt);
      break;

    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      scanner_legacy_advertisement_report_handle(evt);
      break;

    case sl_bt_evt_gatt_characteristic_value_id:
      receive_notification(evt);
      break;

    case sl_bt_evt_gatt_server_user_write_request_id:
      user_write_request_handle(evt);
      break;

    case sl_bt_evt_gatt_server_user_read_request_id:
      user_read_request_handle(evt);
      break;

    case sl_bt_evt_system_external_signal_id:
      client_external_event_handle();
      break;

    case sl_bt_evt_gatt_service_id:
      client_gatt_service(evt);
      break;

    case sl_bt_evt_gatt_procedure_completed_id:
      comsumer_bt_evt_gatt_procedure_completed(evt);
      break;

    case sl_bt_evt_gatt_characteristic_id:
      client_gatt_characteristic(evt);
      break;

    default:
      break;
  }
}

static void scanner_legacy_advertisement_report_handle(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  // If a sensor advertisement is found...
  if (find_name_in_advertisement(evt->data.
                                 evt_scanner_legacy_advertisement_report.data.
                                 data,
                                 evt->data.
                                 evt_scanner_legacy_advertisement_report.data.
                                 len)) {
    // then stop scanning for a while
    sl_bt_scanner_stop();
    // and connect to that device
    sc = sl_bt_connection_open(
      evt->data.evt_scanner_legacy_advertisement_report.address,
      evt->data.evt_scanner_legacy_advertisement_report.address_type,
      sl_bt_gap_phy_1m,
      &sensor_connection_handle);
    app_assert_status(sc);
    conn_state = opening;
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
      if (memcmp(&data[i + 2], dev_name, (ad_field_length - 1)) == 0) {
        return 1;
      }
    }
    // advance to the next AD struct
    i = i + ad_field_length + 1;
  }
  return 0;
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
    app_log("Configuration mode\r\n");
    client_oled_app_config_mode_display();
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

    // Start scanning - looking for dosimeter devices
    sc = sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m,
                             sl_bt_scanner_discover_generic);
    app_assert_status(sc);
    app_log("Start scanning sensor\r\n");
    conn_state = scanning;
    client_oled_app_disconnected_display();
  }
}

/**************************************************************************//**
 * Handler function for connection open event.
 *****************************************************************************/
void connection_open_handle(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  if (conn_state == opening) {
    app_log("Connected to sensor\r\n");
    client_oled_app_radiation_display(0);

    // Stop scanning other devices.
    sc = sl_bt_scanner_stop();
    app_assert_status(sc);

    // Get sender connection handle.
    sensor_connection_handle = evt->data.evt_connection_opened.connection;

    // Find sender's service after connecting to sender.
    sc = sl_bt_gatt_discover_primary_services_by_uuid(sensor_connection_handle,
                                                      16,
                                                      dosimeter_sensor_service);
    app_assert_status(sc);
  } else {
    app_log("Connected to application\r\n");
    app_connection_handle = evt->data.evt_connection_opened.connection;
    sc = sl_sleeptimer_start_periodic_timer_ms(&my_timer,
                                               TIMER_TIMEOUT,
                                               timer_callback,
                                               NULL,
                                               1,
                                               0);
  }
}

/**************************************************************************//**
 * Handler function for connection close event.
 *****************************************************************************/
void connection_close_handle(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  if (evt->data.evt_connection_closed.connection == sensor_connection_handle) {
    client_oled_app_disconnected_display();
  }
  (void)evt;
  app_log("Connection close\r\n");
  sc = sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m,
                           sl_bt_scanner_discover_generic);

  app_assert_status_f(sc, "Failed to start discovery #1\n");
}

/**************************************************************************//**
 * Handler function for receive nitification event.
 *****************************************************************************/
void receive_notification(sl_bt_msg_t *evt)
{
  float recv_data =
    (float)(atof((char *)evt->data.evt_gatt_characteristic_value.value.data));
  client_oled_app_radiation_display((float)recv_data);
  if (cfg.click_noise_status) {
    mikroe_cmt_8540s_smt_set_duty_cycle(100);
    mikroe_cmt_8540s_smt_pwm_start();
    sl_sleeptimer_delay_millisecond(10);
    mikroe_cmt_8540s_smt_pwm_stop();
  }
  if (!cfg.notification_status) {
    return;
  }
  if ((float)recv_data > cfg.alarm_threshold) {
    buzzer_active = true;
    mikroe_cmt_8540s_smt_set_duty_cycle(70);
    mikroe_cmt_8540s_smt_pwm_start();
  } else {
    buzzer_active = false;
    mikroe_cmt_8540s_smt_pwm_stop();
  }
}

/**************************************************************************//**
 * Handler function for user write request event.
 *****************************************************************************/
void user_write_request_handle(sl_bt_msg_t *evt)
{
  sl_status_t sc = SL_STATUS_OK;
  uint8_t value;
  float threshold;

  switch (evt->data.evt_gatt_server_user_write_request.characteristic) {
    case gattdb_alarm_threshold:
      if (evt->data.evt_gatt_server_user_write_request.value.len > 10) {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      } else {
        threshold = (float)atof(
          (char *)evt->data.evt_gatt_server_user_write_request.value.data);
        app_log("GATT: write alarm threshold: %.2f\r\n", threshold);
        client_nvm3_set_alarm_threshold(threshold);
        sc = SL_STATUS_OK;
      }
      break;

    case gattdb_notification_status:
      if (evt->data.evt_gatt_server_user_write_request.value.len != 1) {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      } else {
        value = evt->data.evt_gatt_server_user_write_request.value.data[0] - 48;
        app_log("GATT: write notification status: %d\r\n", value);
        client_nvm3_set_notification_status((bool)value);
        sc = SL_STATUS_OK;
      }
      break;

    case gattdb_click_noise_status:
      if (evt->data.evt_gatt_server_user_write_request.value.len != 1) {
        sc = SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
      } else {
        value = evt->data.evt_gatt_server_user_write_request.value.data[0] - 48;
        app_log("GATT: write click noise status: %d\r\n", value);
        client_nvm3_set_click_noise_status((bool)value);
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
  float threshold;
  char res_str[10];
  int len;

  switch (evt->data.evt_gatt_server_user_read_request.characteristic) {
    case gattdb_alarm_threshold:
      threshold = client_nvm3_get_alarm_threshold();
      app_log("GATT: read alarm threshold: %.2f\r\n", threshold);
      len = snprintf(res_str, 10, "%.2f", threshold);
      sc = sl_bt_gatt_server_send_user_read_response(app_connection_handle,
                                                     gattdb_alarm_threshold,
                                                     0,
                                                     (size_t)len,
                                                     (uint8_t *)res_str,
                                                     NULL);
      app_assert_status(sc);
      break;

    case gattdb_notification_status:
      status = client_nvm3_get_notification_status();
      app_log("GATT: read notification status: %d\r\n", status);
      len = snprintf(res_str, 10, "%d", status);
      sc = sl_bt_gatt_server_send_user_read_response(app_connection_handle,
                                                     gattdb_notification_status,
                                                     0,
                                                     (size_t)len,
                                                     (uint8_t *)res_str,
                                                     NULL);
      app_assert_status(sc);
      break;

    case gattdb_click_noise_status:
      status = client_nvm3_get_click_noise_status();
      app_log("GATT: read click noise status: %d\r\n", status);
      len = snprintf(res_str, 10, "%d", status);
      sc = sl_bt_gatt_server_send_user_read_response(app_connection_handle,
                                                     gattdb_click_noise_status,
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
 * Handler function for gatt service found.
 *****************************************************************************/
void client_gatt_service(sl_bt_msg_t *evt)
{
  (void)&evt;
  service_handle = evt->data.evt_gatt_service.service;
  conn_state = discover_services;
}

/**************************************************************************//**
 * Handler function for gatt characteristic found.
 *****************************************************************************/
void client_gatt_characteristic(sl_bt_msg_t *evt)
{
  (void)&evt;
  characteristic_handle = evt->data.evt_gatt_characteristic.characteristic;
  conn_state = discover_characteristics;
}

/**************************************************************************//**
 * Handle function for complete GATT procedure.
 *****************************************************************************/
void comsumer_bt_evt_gatt_procedure_completed(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  (void)&evt;

  // If service discovery finished
  if (conn_state == discover_services) {
    // Discover sensor characteristic on the responder device
    sc = sl_bt_gatt_discover_characteristics_by_uuid(sensor_connection_handle,
                                                     service_handle,
                                                     16,
                                                     radiation_characteristic);
    app_assert_status(sc);
  }
  // If characteristic discovery finished, enable indications
  else if (conn_state == discover_characteristics) {
    sc = sl_bt_gatt_set_characteristic_notification(sensor_connection_handle,
                                                    characteristic_handle,
                                                    sl_bt_gatt_notification);
    app_assert_status(sc);
    conn_state = enable_notification;
  }
}

/**************************************************************************//**
 * Handler function for external event.
 *****************************************************************************/
void client_external_event_handle()
{
  if (!cfg.notification_status) {
    client_nvm3_set_notification_status(true);
    cfg.notification_status = true;
  } else {
    client_nvm3_set_notification_status(false);
    cfg.notification_status = false;
    if (buzzer_active) {
      buzzer_active = false;
      mikroe_cmt_8540s_smt_pwm_stop();
    }
  }
}

/**************************************************************************//**
 * System reset function.
 *****************************************************************************/
void timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)&handle;
  (void)&data;

  // Restart device
  sl_bt_system_reset(0);
}

void sl_button_on_change(const sl_button_t *handle)
{
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED) {
    if (&sl_button_btn0 == handle) {
      if (conn_state != booting) {
        sl_bt_external_signal(client_BUTTON_EVENT);
      }
    }
  }
}
