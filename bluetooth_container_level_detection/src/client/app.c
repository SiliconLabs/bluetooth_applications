/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
 ******************************************************************************/
#include <stdio.h>
#include "sl_bluetooth.h"
#include "sl_sleeptimer.h"
#include "gatt_db.h"
#include "em_common.h"
#include "app_assert.h"
#include "app.h"
#include "app_log.h"
#include "app_assert.h"
#include "sl_i2cspm_instances.h"
#include "glib.h"
#include "glib_font.h"
#include "container_nvm.h"
#include "container_rgb.h"
#include "sl_simple_button_instances.h"

// structure for container information
typedef struct container_info_t
{
  container_nvm_config_t config[4];
  uint32_t sample_counter[4];
  uint32_t container_value[4];
  uint8_t  is_updated[4];
}container_info_t;

/**************************************************************************//**
 * application definitions.
 *****************************************************************************/
// operational mode
#define NORMAL_MODE               0xff
#define CONFIG_MODE               0xfe

// external signal flag
#define C_LVL_CLIENT_TIMER_EVENT  1 << 0

// user request time out
#define TIMER_TIMEOUT             25000

// maximum container slot
#define MAX_CONTAINER_SLOT        4

/**************************************************************************//**
 * static variables.
 *****************************************************************************/

// timer for main periodic callback
static sl_sleeptimer_timer_handle_t app_sleep_timer_main;

// timer for connection and user request time out
static sl_sleeptimer_timer_handle_t app_advertising_timer;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// container object hold information of all container
static container_info_t container;

// glib object show status of application
static glib_context_t status_context;

// application selected mode
static uint8_t app_mode = NORMAL_MODE;

// part name of sensor devices
static char target_scanned_name[] = "CON_LEV_S_";

// connection handle
static uint8_t app_connection_handle;

// slot selected
static uint8_t slot_selected = 0;

/**************************************************************************//**
 * static functions.
 *****************************************************************************/

// main periodic callback to update sensor device's info
static void app_sleep_timer_main_callback(sl_sleeptimer_timer_handle_t *handle,
                                          void *data);

// connection timeout callback
static void app_advertising_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                           void *data);

// Scanner legacy advertisement report handler.
static void scanner_legacy_advertisement_report_handle(sl_bt_msg_t content);

// update sensor devices's level
static void container_level_device_check_event_handler(void);

// connect to phone
static void connection_open_event_handler(sl_bt_msg_t *evt);

// system boot
static void app_system_boot_event_handler(void);

// update application's status
static void container_display_status(char *status);

// find sensor device's name in advertisement packet
static uint16_t find_name_in_advertisement(uint8_t *data, uint8_t len);

// user write request handler
static void container_app_user_write_request_handler(sl_bt_msg_t *evt);

// user read request handler
static void container_app_user_read_request_handler(sl_bt_msg_t *evt);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
  sl_status_t sc;

  container_config_nvm3_init();
  container_nvm3_get_config(container.config);

  // show all container in nvm
  for (uint8_t slot = 0; slot < MAX_CONTAINER_SLOT; slot++)
  {
    app_log("container id: %x, low: %d, high: %d\n",
            container.config[slot].id_configuration,
            container.config[slot].lowest_level,
            container.config[slot].highest_level);
  }

  // initialize the led matrix
  sc = container_rgb_init(sl_i2cspm_qwiic);
  app_assert(sc == SL_STATUS_OK,
             "\rAdafruit RGB LED Matrix initialization fail\n");

  // show start up
  container_rgb_context_init(&status_context);
  container_display_status("<>");
  app_log("\rAdafruit RGB LED initialization done\n");
  app_log("\rStart displaying text\n");
  sl_sleeptimer_delay_millisecond(1000);

  // check user selected mode
  sl_simple_button_enable(&sl_button_btn0);
  if (sl_simple_button_get_state(&sl_button_btn0) == SL_SIMPLE_BUTTON_PRESSED) {
    app_mode = CONFIG_MODE;
    container_display_status("CFG");
    app_log("configuration mode.\n");
  } else {
    app_mode = NORMAL_MODE;
    container_display_status("Nom");
    app_log("Normal mode.\n");
    glib_clear(&status_context);
  }
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
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      app_system_boot_event_handler();
      break;

    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      scanner_legacy_advertisement_report_handle(*evt);
      break;
    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      connection_open_event_handler(evt);
      break;

    case sl_bt_evt_gatt_server_user_write_request_id:
      container_app_user_write_request_handler(evt);
      break;

    case sl_bt_evt_gatt_server_user_read_request_id:
      container_app_user_read_request_handler(evt);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    case sl_bt_evt_system_external_signal_id:
      container_level_device_check_event_handler();
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

void app_system_boot_event_handler()
{
  sl_status_t sc;

  switch (app_mode)
  {
    case NORMAL_MODE:
    {
      // start scanning devices
      sc = sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m,
                               sl_bt_scanner_discover_observation);
      app_assert_status(sc);
      app_log("Start scanning\r\n");

      // start main periodic callback to update status;
      sl_sleeptimer_start_periodic_timer_ms(&app_sleep_timer_main,
                                            1500,
                                            app_sleep_timer_main_callback,
                                            NULL,
                                            0,
                                            0);
      break;
    }
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
        160,   // min. adv. interval (milliseconds * 1.6)
        160,   // max. adv. interval (milliseconds * 1.6)
        0,     // adv. duration
        0);    // max. num. adv. events
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      app_log("Start advertising.\r\n");

      // start timer for checking timeout request
      sc = sl_sleeptimer_start_periodic_timer_ms(&app_advertising_timer,
                                                 TIMER_TIMEOUT,
                                                 app_advertising_timer_callback,
                                                 NULL,
                                                 1,
                                                 0);
      app_assert_status(sc);
      break;
    }
    default:
    {
      app_log("invalid mode. Please reset device.\r\n");
      container_display_status("ER");
      break;
    }
  }
}

static void scanner_legacy_advertisement_report_handle(sl_bt_msg_t content)
{
  sl_status_t sc;
  uint8_t data_advertisement[100] = { 0 };
  uint32_t sample_counter = 0;
  uint8_t slot = 0;
  uint8_t *raw_data = 0;
  uint8_t raw_data_len = 0;
  uint16_t scanned_name = 0;

  // get advertisement data
  raw_data = content.data.evt_scanner_legacy_advertisement_report.data.data;
  raw_data_len = content.data.evt_scanner_legacy_advertisement_report.data.len;
  scanned_name = find_name_in_advertisement(raw_data, raw_data_len);

  // If a sensor advertisement is found...
  if (scanned_name != 0) {
    for (slot = 0; slot < MAX_CONTAINER_SLOT; slot++)
    {
      // update information if container is already included
      if (scanned_name == container.config[slot].id_configuration) {
        // app_log("matched.\n");
        memcpy(data_advertisement, raw_data, raw_data_len);

        sample_counter = (data_advertisement[7]
                          | (data_advertisement[8] << 8)
                          | (data_advertisement[9] << 16)
                          | (data_advertisement[10] << 24));

        // update if sample counter changed
        if (sample_counter != container.sample_counter[slot]) {
          container.container_value[slot] = data_advertisement[11]
                                            | (data_advertisement[12] << 8)
                                            | (data_advertisement[13] << 16)
                                            | (data_advertisement[14] << 24);
          container.sample_counter[slot] = sample_counter;
          container.is_updated[slot] = true;
        }
        return;
      }
    }

    // found new device
    for (slot = 0; slot < MAX_CONTAINER_SLOT; slot++) {
      // save to slot have id 0x00.
      if (container.config[slot].id_configuration == 0) {
        app_log("Found device: %x, slot: %d\n",
                scanned_name,
                slot);

        // get information of new device
        container.config[slot].id_configuration = scanned_name;
        container.container_value[slot] = data_advertisement[11]
                                          | (data_advertisement[12] << 8)
                                          | (data_advertisement[13] << 16)
                                          | (data_advertisement[14] << 24);
        container.sample_counter[slot] = sample_counter;
        container.is_updated[slot] = true;

        // update in nvm
        sc = container_nvm3_write(NVM3_KEY_MIN + slot, container.config[slot]);
        app_assert_status(sc);
        return;
      }
    }
  }
}

// main timer tick
static void app_sleep_timer_main_callback(sl_sleeptimer_timer_handle_t *handle,
                                          void *data)
{
  (void)data;
  (void)handle;
  sl_bt_external_signal(C_LVL_CLIENT_TIMER_EVENT); // trigger main timer tick
}

// connection and request timeout callback
static void app_advertising_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                           void *data)
{
  (void) handle;
  (void) data;
  app_log("time out. reseting.\r\n");
  sl_bt_system_reset(0);
}

void container_level_device_check_event_handler()
{
  int distance = 0;

  for (int slot = 0; slot < MAX_CONTAINER_SLOT; slot++)
  {
    if (container.config[slot].id_configuration != 0x00) {
      // if sensor device is updated
      if ((container.is_updated[slot] == true)) {
        // get sensor value
        if (container.container_value[slot]
            < container.config[slot].lowest_level) {
          distance = 9;
        } else if (container.container_value[slot]
                   >= container.config[slot].highest_level) {
          distance = 1;
        } else {
          distance = (container.config[slot].highest_level
                      - container.container_value[slot]) * 9
                     / (container.config[slot].highest_level
                        - container.config[slot].lowest_level);
          if (distance == 0) {
            distance = 1;
          }
        }
      } else {
        distance = 0;
      }

      // update in the led matrix
      container_rgb_update_level(slot, distance);
      container.is_updated[slot] = false;
      app_log("updating device: %x,   distance: %d, distan: %d\n\n",
              (int)container.config[slot].id_configuration,
              (int)container.container_value[slot],
              distance);
    }
  }
}

void container_display_status(char *status)
{
  glib_clear(&status_context);
  glib_draw_string(&status_context, status, 0, 5);
  glib_update_display();
}

static void connection_open_event_handler(sl_bt_msg_t *evt)
{
  app_log("connection opened. \r\n");
  container_display_status("->");
  app_connection_handle = evt->data.evt_connection_opened.connection;
}

static uint16_t find_name_in_advertisement(uint8_t *data, uint8_t len)
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
      if (memcmp(&data[i + 2], target_scanned_name, 5) == 0) {
        return (data[i + 12] << 8 | data[i + 13]);
      }
    }
    // advance to the next AD struct
    i = i + ad_field_length + 1;
  }

  return 0;
}

static void container_app_user_write_request_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc = 0;
  uint16_t data = 1;
  uint8_t *raw_data = 0;

  raw_data = evt->data.evt_gatt_server_user_write_request.value.data;
  app_log("writing\n");

  data = (uint16_t)atoi((char *)raw_data);
  switch (evt->data.evt_gatt_server_user_write_request.characteristic)
  {
    case gattdb_select_container_to_configure:
    {
      if (data >= MAX_CONTAINER_SLOT) {
        app_log("invalid selected slot");
        container_display_status("ER");
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
      } else {
        app_log("Writing selected slot");
        container_display_status("WT");
        slot_selected = data;
      }
      break;
    }
    case gattdb_container_lowest_level:
    {
      if ((data > 4000) || (data < 40)
          || (data > container.config[slot_selected].highest_level)) {
        app_log("invalid lowest value");
        container_display_status("ER");
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
      } else {
        app_log("Writing lowest value");
        container_display_status("LO");
        container.config[slot_selected].lowest_level = data;
        sc = container_nvm3_write(NVM3_KEY_MIN + slot_selected,
                                  container.config[slot_selected]);
      }
      break;
    }
    case gattdb_container_highest_level:
    {
      if ((data > 4000) || (data < 40)
          || (data < container.config[slot_selected].lowest_level)) {
        app_log("invalid highest value");
        container_display_status("ER");
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
      } else {
        app_log("Writing highest value");
        container_display_status("HI");
        container.config[slot_selected].highest_level = data;
        sc = container_nvm3_write(NVM3_KEY_MIN + slot_selected,
                                  container.config[slot_selected]);
      }
      break;
    }
    default: break;
  }
  sc = sl_bt_gatt_server_send_user_write_response(
    app_connection_handle,
    evt->data.evt_gatt_server_user_write_request.characteristic,
    sc);
  app_assert_status(sc);

  // time out last request.
  sc = sl_sleeptimer_restart_periodic_timer_ms(&app_advertising_timer,
                                               TIMER_TIMEOUT,
                                               app_advertising_timer_callback,
                                               NULL,
                                               1,
                                               0);
  app_assert_status(sc);
}

static void container_app_user_read_request_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc = 0;
  char respond_data[10];
  uint8_t respond_data_length = 0;

  memset(respond_data, 0, sizeof(respond_data));
  switch (evt->data.evt_gatt_server_user_write_request.characteristic)
  {
    case gattdb_select_container_to_configure:
    {
      app_log("reading selected slot");
      container_display_status("RD");

      respond_data_length = snprintf(respond_data, 10, "%d", slot_selected);
      sc = sl_bt_gatt_server_send_user_read_response(app_connection_handle,
                                                     gattdb_select_container_to_configure,
                                                     0,
                                                     (size_t)respond_data_length,
                                                     (uint8_t *)respond_data,
                                                     NULL);
      app_assert_status(sc);
      break;
    }
    case gattdb_container_slot_configuration:
    {
      app_log("reading id slot");
      container_display_status("id");

      respond_data_length = snprintf(respond_data,
                                     10,
                                     "%d",
                                     container.config[slot_selected].id_configuration);
      sc = sl_bt_gatt_server_send_user_read_response(app_connection_handle,
                                                     gattdb_select_container_to_configure,
                                                     0,
                                                     (size_t)respond_data_length,
                                                     (uint8_t *)respond_data,
                                                     NULL);
      app_assert_status(sc);
      break;
    }
    case gattdb_container_lowest_level:
    {
      app_log("reading lowest value");
      container_display_status("LO");

      container_nvm3_get_config(container.config);
      respond_data_length = snprintf(respond_data,
                                     10,
                                     "%d",
                                     container.config[slot_selected].lowest_level);

      sc = sl_bt_gatt_server_send_user_read_response(app_connection_handle,
                                                     gattdb_container_lowest_level,
                                                     0,
                                                     (size_t)respond_data_length,
                                                     (uint8_t *)respond_data,
                                                     NULL);
      app_assert_status(sc);
      break;
    }
    case gattdb_container_highest_level:
    {
      app_log("reading highest value");
      container_display_status("HI");

      container_nvm3_get_config(container.config);
      respond_data_length = snprintf(respond_data,
                                     10,
                                     "%d",
                                     container.config[slot_selected].highest_level);

      sc = sl_bt_gatt_server_send_user_read_response(app_connection_handle,
                                                     gattdb_container_highest_level,
                                                     0,
                                                     (size_t)respond_data_length,
                                                     (uint8_t *)respond_data,
                                                     NULL);
      app_assert_status(sc);
      break;
    }
    default:
      break;
  }

  // time out last request.
  sc = sl_sleeptimer_restart_periodic_timer_ms(&app_advertising_timer,
                                               TIMER_TIMEOUT,
                                               app_advertising_timer_callback,
                                               NULL,
                                               1,
                                               0);
  app_assert_status(sc);
}
