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
#include "app_log.h"
#include "client_app.h"
#include "sl_sleeptimer.h"
#include "app.h"

#define SCANNING_SENSOR_TIMEOUT                 5500

/**************************************************************************//**
 * Local variables.
 *****************************************************************************/
static char dev_name[] = "BG22_SE";

static sl_sleeptimer_timer_handle_t sl_timer;

/**************************************************************************//**
 * Local function prototypes.
 *****************************************************************************/
// Sleeptimer callback.
static void sl_timer_cb(sl_sleeptimer_timer_handle_t *handle, void *data);

// Find device by name.
static uint8_t find_name_in_advertisement(uint8_t *data, uint8_t len);

// Scanner legacy advertisement report handler.
static void scanner_legacy_advertisement_report_handle(sl_bt_msg_t *evt);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  app_log("BLE - Optimized energy consuming client\r\n");
  client_app_init();
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
      // Set scanning parameters.
      sc = sl_bt_scanner_set_parameters(
        sl_bt_scanner_scan_mode_passive,
        16,
        16);
      app_assert_status_f(sc, "Failed to start discovery\n");

      // Start scanning - looking for dosimeter devices
      sc = sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m,
                               sl_bt_scanner_discover_generic);
      app_assert_status(sc);
      app_log("Start scanning sensor\r\n");
      break;

    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      scanner_legacy_advertisement_report_handle(evt);
      break;

    default:
      break;
  }
}

/**************************************************************************//**
 * Sleeptimer callback function.
 *****************************************************************************/
static void sl_timer_cb(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  (void)data;

  client_app_display_no_data();
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
 * Handler function for scanner legacy advertisement report event.
 *****************************************************************************/
static void scanner_legacy_advertisement_report_handle(sl_bt_msg_t *evt)
{
  bool is_timer_running;
  // If a sensor advertisement is found...
  if (find_name_in_advertisement(evt->data.
                                 evt_scanner_legacy_advertisement_report.data.
                                 data,
                                 evt->data.
                                 evt_scanner_legacy_advertisement_report.data.
                                 len)) {
    app_log("Found BG22_SE device\n");
    sl_sleeptimer_stop_timer(&sl_timer);
    uint8_t *ptr = evt->data.evt_scanner_legacy_advertisement_report.data.data;
    uint32_t temp = ptr[7] + (ptr[8] << 8) + (ptr[9] << 16) + (ptr[10] << 24);
    client_app_display_data((float)temp / 100);
  } else {
    sl_sleeptimer_is_timer_running(&sl_timer, &is_timer_running);
    if (is_timer_running == false) {
      sl_sleeptimer_start_periodic_timer_ms(&sl_timer,
                                            SCANNING_SENSOR_TIMEOUT,
                                            sl_timer_cb,
                                            NULL,
                                            0,
                                            0);
    }
  }
}
