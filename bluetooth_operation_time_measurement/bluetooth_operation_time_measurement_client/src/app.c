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
#include <client_nvm.h>
#include <client_rgb.h>
#include "sl_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "sl_simple_button_instances.h"
#include "gatt_db.h"
#include "app.h"
#include "stdio.h"
#include "glib.h"
#include "sl_string.h"

/**************************************************************************//**
 * application definitions.
 *****************************************************************************/
// operational mode
#define NORMAL_MODE               0xff
#define CONFIG_MODE               0xfe

#define TIMER_RESTART_TIMEOUT     25000

typedef enum {
  idle,
  discover_services,
  discover_characteristics
} conn_state_t;

/**************************************************************************//**
 * static variables.
 *****************************************************************************/
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static uint8_t connection_handle;
static uint32_t service_handle;
static uint16_t characteristic_handle;

// application selected mode
static uint8_t operation_mode = NORMAL_MODE;

// application value measure
static uint32_t operation_time_threshold;
static uint32_t operation_time;

// timer for time out
static sl_sleeptimer_timer_handle_t client_time_out;
static sl_sleeptimer_timer_handle_t client_time_display_config;
static sl_sleeptimer_timer_handle_t client_time_display_normal;
static sl_sleeptimer_timer_handle_t client_time_display_warning;
// glib object and other stuff show status of application
static glib_context_t rgb_context;
static char content[25];
static int16_t cursor_x = 0;
// Connection state
static conn_state_t conn_state = idle;

// part name of sensor devices
static char target_scanned_name[] = "bma400_sensor";

// sensor chacteristic UUID 128. b5dfc72c-3bfe-47b2-a4a9-7a9b695ed846
static const uint8_t operation_time_chacteristic[16] = {
  0x46, 0xd8, 0x5e, 0x69, 0x9b, 0x7a, 0xa9, 0xa4,
  0xb2, 0x47, 0xfe, 0x3b, 0x2c, 0xc7, 0xdf, 0xb5
};

// sensor service UUID 128. 439d4a8f-cda9-4699-949a-731e27e841f4
static const uint8_t operation_time_service[16] = {
  0xf4, 0x41, 0xe8, 0x27, 0x1e, 0x73, 0x9a, 0x94,
  0x99, 0x46, 0xa9, 0xcd, 0x8f, 0x4a, 0x9d, 0x43
};

/**************************************************************************//**
 * Static functions declaration.
 *****************************************************************************/
// timer callback for time out
static void client_time_out_callback(sl_sleeptimer_timer_handle_t *handle,
                                     void *data);
static void client_time_display_config_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data);
static void client_time_display_normal_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data);
static void client_time_display_warning_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data);

// find sensor device's name in advertisement packet
static uint8_t scan_response(
  sl_bt_evt_scanner_legacy_advertisement_report_t *response);

// system boot handler
static void app_system_boot_handler(void);

// user write request handler
static void app_user_write_request_handler(
  sl_bt_evt_gatt_server_user_write_request_t *user_write_request);

// user read request handler
static void app_user_read_request_handler(
  sl_bt_evt_gatt_server_user_read_request_t *user_read_request);

// user read request handler
static void app_gatt_procedure_completed_handler(
  sl_bt_evt_gatt_procedure_completed_t *procedure_completed);

// user notification handler
static void app_gatt_characteristic_value_handler(
  sl_bt_evt_gatt_characteristic_value_t *characteristic_value);

// display function
static void app_display_normal(void);
static void app_display_warning(void);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_status_t sc;

  client_config_nvm3_init();
  sc = client_nvm3_get_operation_time_threshold(&operation_time_threshold);
  app_assert_status(sc);

  // initialize the led matrix
  sc = client_rgb_init(&rgb_context, sl_i2cspm_qwiic);
  if (sc != SL_STATUS_OK) {
    cursor_x = rgb_context.width - 1;
    app_log("Adafruit RGB LED Matrix initialization fail\r\n");
  }
  // delete old bonding
  sc = sl_bt_sm_delete_bondings();
  app_assert_status(sc);
  // show start up
  client_rgb_display(&rgb_context, "BMA400");
  sl_sleeptimer_delay_millisecond(1000);

  if (sl_simple_button_get_state(&sl_button_btn0) == SL_SIMPLE_BUTTON_PRESSED) {
    operation_mode = CONFIG_MODE;
    app_log("go to configuration mode\r\n");
    client_rgb_display(&rgb_context, "CONFIG");
  } else {
    operation_mode = NORMAL_MODE;
    app_log("go to normal mode\r\n");
    client_rgb_display(&rgb_context, "NORMAL");
    sl_sleeptimer_delay_millisecond(1000);
  }
  app_log("\r\n------ Silicon Labs BLE Operation time Measurement "
          "Demo: Client Role-----\r\n");
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

      app_system_boot_handler();
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("connection open\r\n");
      sl_sleeptimer_stop_timer(&client_time_out);
      connection_handle = evt->data.evt_connection_opened.connection;
      if (operation_mode == NORMAL_MODE) {
        sc = sl_bt_gatt_discover_primary_services_by_uuid(
          connection_handle,
          16,
          operation_time_service);
        app_assert_status(sc);
        conn_state = discover_services;
      }
      break;

    // -------------------------------
    case sl_bt_evt_gatt_procedure_completed_id:
      app_gatt_procedure_completed_handler(
        &evt->data.evt_gatt_procedure_completed);
      break;

    // -------------------------------
    case sl_bt_evt_gatt_service_id:
      service_handle = evt->data.evt_gatt_service.service;
      app_log("discovered operation time service!\r\n");
      break;

    // -------------------------------
    case sl_bt_evt_gatt_characteristic_id:
      characteristic_handle = evt->data.evt_gatt_characteristic.characteristic;
      app_log("discovered operation time characteristic!\r\n");
      break;

    // -------------------------------
    case sl_bt_evt_gatt_characteristic_value_id:
      app_gatt_characteristic_value_handler(
        &evt->data.evt_gatt_characteristic_value);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("connection closed\r\n");
      if (operation_mode == CONFIG_MODE) {
        sl_bt_system_reboot(0);
      }
      break;

    // -------------------------------
    case sl_bt_evt_sm_bonded_id:
      app_log("sm_bonded event: bond handle %d and security mode %d\r\n",
              evt->data.evt_sm_bonded.bonding,
              evt->data.evt_sm_bonded.security_mode + 1);
      break;

    // -------------------------------
    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      if (scan_response(&evt->data.evt_scanner_legacy_advertisement_report)) {
        sc = sl_bt_scanner_stop();
        app_assert_status(sc);

        // and connect to that device
        sc = sl_bt_connection_open(
          evt->data.evt_scanner_legacy_advertisement_report.address,
          evt->data.evt_scanner_legacy_advertisement_report.address_type,
          sl_bt_gap_1m_phy,
          NULL);
      }
      break;

    // -------------------------------
    case sl_bt_evt_gatt_server_user_read_request_id:
      app_user_read_request_handler(
        &evt->data.evt_gatt_server_user_read_request);
      break;

    // -------------------------------
    case sl_bt_evt_gatt_server_user_write_request_id:
      app_user_write_request_handler(
        &evt->data.evt_gatt_server_user_write_request);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

/**************************************************************************//**
 * Static function definition.
 *****************************************************************************/
// timer callback function
static void client_time_out_callback(sl_sleeptimer_timer_handle_t *handle,
                                     void *data)
{
  (void) handle;
  (void) data;
  sl_bt_system_reboot(0);
}

static void client_time_display_config_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data)
{
  (void) handle;
  (void) data;
  static char conf_title[25];
  int length;
  uint8_t content_pos = 0;

  snprintf(conf_title, 25, "THRESHOLD:%lu", operation_time_threshold);
  length = (5 + 1) * sl_strlen(conf_title);
  glib_set_cursor(&rgb_context, cursor_x, 1);
  glib_fill(&rgb_context, 0);
  while (*(conf_title + content_pos) != '\0') {
    glib_write_char(&rgb_context, *(conf_title + content_pos));
    content_pos++;
  }
  glib_update_display();

  if (cursor_x > -length) {
    cursor_x--;
  } else {
    cursor_x = rgb_context.width;
  }
}

static void client_time_display_normal_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data)
{
  (void) handle;
  (void) data;
  int length = (5 + 1) * sl_strlen(content);
  uint8_t content_pos = 0;

  glib_set_cursor(&rgb_context, cursor_x, 1);
  glib_fill(&rgb_context, 0);
  while (*(content + content_pos) != '\0') {
    glib_write_char(&rgb_context, *(content + content_pos));
    content_pos++;
  }
  glib_update_display();

  if (cursor_x > -length) {
    cursor_x--;
  } else {
    cursor_x = rgb_context.width;
  }
}

static void client_time_display_warning_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data)
{
  (void) handle;
  (void) data;
  char num[15];

  if (rgb_context.bg_color == 0) {
    glib_set_bg_color(&rgb_context, 0xf800);
  } else if (rgb_context.bg_color == 0xf800) {
    glib_set_bg_color(&rgb_context, 0);
  }
  snprintf(num, 15, "WARN:%lu" "s", operation_time);
  client_rgb_display(&rgb_context, num);
  glib_update_display();
}

// display function
static void app_display_normal(void)
{
  sl_sleeptimer_stop_timer(&client_time_display_config);
  sl_sleeptimer_stop_timer(&client_time_display_warning);

  sl_sleeptimer_start_periodic_timer_ms(&client_time_display_normal,
                                        60,
                                        client_time_display_normal_callback,
                                        NULL, 2, 0);
}

static void app_display_warning(void)
{
  sl_sleeptimer_stop_timer(&client_time_display_config);
  sl_sleeptimer_stop_timer(&client_time_display_normal);
  glib_set_cursor(&rgb_context, 10, 1);
  sl_sleeptimer_start_periodic_timer_ms(&client_time_display_warning,
                                        500,
                                        client_time_display_warning_callback,
                                        NULL, 1, 0);
}

// BLE handle function
static uint8_t scan_response(
  sl_bt_evt_scanner_legacy_advertisement_report_t *response)
{
  uint8_t ad_length;
  uint8_t ad_type;
  char name[32];
  bool device_found = false;
  uint8_t i = 0;

  while (i < response->data.len - 1) {
    ad_length = response->data.data[i];
    ad_type = response->data.data[i + 1];

    // Type 0x08 = Shortened Local Name
    // Type 0x09 = Complete Local Name
    if ((ad_type == 0x08) || (ad_type == 0x09)) {
      memcpy(name, &(response->data.data[i + 2]), ad_length - 1);
      name[ad_length - 1] = 0;
      app_log("%s\r\n", name);
      if (memcmp(name, target_scanned_name, ad_length - 1) == 0) {
        device_found = true;
      }
    }
    // advance to the next AD struct
    i = i + ad_length + 1;
  }
  return device_found;
}

static void app_system_boot_handler(void)
{
  sl_status_t sc;

  switch (operation_mode)
  {
    case NORMAL_MODE:
      cursor_x = rgb_context.width - 1;
      app_display_normal();
      // configure security before start advertise
      sc = sl_bt_sm_configure(0, sl_bt_sm_io_capability_noinputnooutput);
      app_assert_status(sc);
      sc = sl_bt_sm_set_bondable_mode(1);
      app_assert_status(sc);
      sc = sl_bt_sm_store_bonding_configuration(4, 0x2);
      app_assert_status(sc);

      // start scanning devices
      sc = sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m,
                               sl_bt_scanner_discover_observation);
      app_assert_status(sc);
      app_log("start scanning...\r\n");
      break;

    case CONFIG_MODE:
    {
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160,     // min. adv. interval (milliseconds * 1.6)
        160,     // max. adv. interval (milliseconds * 1.6)
        0,       // adv. duration
        0);      // max. num. adv. events
      app_assert_status(sc);

      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      app_log("start advertising...\r\n");

      sc = sl_sleeptimer_start_timer_ms(&client_time_out,
                                        TIMER_RESTART_TIMEOUT,
                                        client_time_out_callback,
                                        NULL, 0, 0);
      sl_sleeptimer_start_periodic_timer_ms(&client_time_display_config,
                                            60,
                                            client_time_display_config_callback,
                                            NULL, 3, 0);
      app_assert_status(sc);
    }
    break;
  }
}

// for config mode
static void app_user_write_request_handler(
  sl_bt_evt_gatt_server_user_write_request_t *user_write_request)
{
  sl_status_t sc;

  if (user_write_request->characteristic == gattdb_operation_time_threshold) {
    operation_time_threshold = (uint32_t)atoi(
      (char *)user_write_request->value.data);
    app_log("operation_time_threshold %lu\r\n", operation_time_threshold);
    client_nvm3_set_operation_time_threshold(operation_time_threshold);
  }
  sc = sl_bt_gatt_server_send_user_write_response(
    user_write_request->connection,
    user_write_request->characteristic,
    SL_STATUS_OK);
  app_assert_status(sc);
}

static void app_user_read_request_handler(
  sl_bt_evt_gatt_server_user_read_request_t *user_read_request)
{
  sl_status_t sc;
  uint16_t sent_len;
  char data_send[10];

  if (user_read_request->characteristic == gattdb_operation_time_threshold) {
    client_nvm3_get_operation_time_threshold(&operation_time_threshold);
    snprintf(data_send, 10, "%lu", operation_time_threshold);
    sc = sl_bt_gatt_server_send_user_read_response(
      user_read_request->connection,
      user_read_request->characteristic,
      SL_STATUS_OK,
      sizeof(data_send),
      (uint8_t *)data_send,
      &sent_len);
    app_assert_status(sc);
  }
}

static void app_gatt_characteristic_value_handler(
  sl_bt_evt_gatt_characteristic_value_t *characteristic_value)
{
  operation_time = atoi((char *)characteristic_value->value.data);
  if (operation_time > operation_time_threshold) {
    app_log("WARNING! reach threshold value\r\n");
    snprintf(content, 25, "TIME:%ld(s)", operation_time);
    app_display_warning();
  }
  snprintf(content, 25, "OPERATION TIME:%lu" "s", operation_time);
  app_log("get notification! operation time = %lu\r\n", operation_time);
}

static void app_gatt_procedure_completed_handler(
  sl_bt_evt_gatt_procedure_completed_t *procedure_completed)
{
  sl_status_t sc;

  switch (conn_state)
  {
    case discover_services:
      if (procedure_completed->result == SL_STATUS_OK) {
        sc = sl_bt_gatt_discover_characteristics_by_uuid(
          connection_handle,
          service_handle,
          16,
          operation_time_chacteristic);
        app_assert_status(sc);
        conn_state = discover_characteristics;
      }
      break;

    case discover_characteristics:
      if (procedure_completed->result == SL_STATUS_OK) {
        sc = sl_bt_gatt_set_characteristic_notification(
          connection_handle,
          characteristic_handle,
          sl_bt_gatt_notification);
        app_assert_status(sc);
        app_log("enable characteristic notification\r\n");
        conn_state = idle;
      }
      break;

    default:
      break;
  }
}
