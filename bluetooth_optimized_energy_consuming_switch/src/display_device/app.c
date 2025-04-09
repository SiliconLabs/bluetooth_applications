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
#include "sl_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"

#include "app_log.h"
#include "switch_display_oled.h"
#include "sl_simple_led_instances.h"

/**************************************************************************//**
 * static variable declaration.
 *****************************************************************************/
// target device's name
static uint8_t dev_name[] = "BG22_SW";

/**************************************************************************//**
 * static function declaration.
 *****************************************************************************/
static uint8_t find_name_in_advertisement(uint8_t *data, uint8_t len);
static void scanner_legacy_advertisement_report_handle(sl_bt_msg_t *evt);

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

  app_log("Switch status display device.\n");
  sc = switch_status_oled_init();

  if (sc == SL_STATUS_OK) {
    app_log("Sparkfun Micro Oled Initializes Done.\n");
  } else {
    app_log("Sparkfun Micro Oled Initializes Fail.\n");
  }
  sl_led_turn_on(&sl_led_led0);
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
      // start scanning devices
      sc = sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m,
                               sl_bt_scanner_discover_observation);
      app_assert_status(sc);
      app_log("Start scanning\r\n");
      break;

    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      scanner_legacy_advertisement_report_handle(evt);
      break;
    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
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

static void scanner_legacy_advertisement_report_handle(sl_bt_msg_t *evt)
{
  uint8_t *adv_data = 0;
  uint8_t adv_len = 0;
  uint8_t switch_status = 0;
  // get advertisement data
  adv_data = evt->data.evt_scanner_legacy_advertisement_report.data.data;
  adv_len = evt->data.evt_scanner_legacy_advertisement_report.data.len;

  if (find_name_in_advertisement(adv_data, adv_len)) {
    app_log(": %s\n", dev_name);
    switch_status = adv_data[7];
    app_log("switch: %d\n", switch_status);
    if (switch_status == 1) {
      switch_status_oled_update(CIRCLE_WHITE, "ON");
      sl_led_turn_on(&sl_led_led0);
    } else {
      switch_status_oled_update(CIRCLE_BLACK, "OFF");
      sl_led_turn_off(&sl_led_led0);
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
      if (memcmp(&data[i + 2], dev_name, (ad_field_length - 1)) == 0) {
        return 1;
      }
    }
    // advance to the next AD struct
    i = i + ad_field_length + 1;
  }
  return 0;
}
