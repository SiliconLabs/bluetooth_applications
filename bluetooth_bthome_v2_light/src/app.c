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
#include "em_common.h"
#include "em_gpio.h"
#include "sl_bluetooth.h"
#include "sl_simple_led_led0_config.h"

#include "bthome_v2_server.h"
#include "bthome_v2.h"
#include "cli.h"

#include "app.h"
#include "app_log.h"
#include "app_assert.h"

static uint8_t name[] = "BTLight";
static uint8_t key[32] = "231d39c1d7cc1ab1aee224cd096db932";

static bool button_event_new = false;
static bool led_state;

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
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
  bthome_v2_bt_on_event(evt);

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      app_log("\r\nBluetooth stack boot event\r\n");
      cli_app_init();
      led_state = GPIO_PinOutGet(SL_SIMPLE_LED_LED0_PORT,
                                 SL_SIMPLE_LED_LED0_PIN);
      bthome_v2_init(name, true, key, false);

      bthome_v2_add_measurement_state(STATE_GENERIC_BOOLEAN,
                                      led_state, 0);
      bthome_v2_send_packet();
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      break;

    // -------------------------------
    case sl_bt_evt_system_external_signal_id:
      if (evt->data.evt_system_external_signal.extsignals
          == SIGNAL_READ_DATA) {
        uint8_t mac[6];
        uint8_t device_count;
        bthome_v2_server_sensor_data_t object;
        uint8_t object_count;
        bool encrypted;
        bool key_available;

        device_count = get_registered_device_count();

        for (uint8_t i = 0; i < device_count; i++) {
          get_registered_device_mac(i, mac);

          bthome_v2_server_check_device(mac, &encrypted, &key_available);
          if ((encrypted == true) && (key_available == false)) {
            break;
          }

          object_count = 0;
          // read only 1 button object
          bthome_v2_server_sensor_data_read(mac,
                                            &object, 1,
                                            &object_count,
                                            NULL);

          if (object.object_id != EVENT_BUTTON) {
            break;
          }

          if (object.data == EVENT_BUTTON_NONE) {
            button_event_new = true;
          } else if (object.data == EVENT_BUTTON_PRESS) {
            if (button_event_new) {
              button_event_new = false;
              GPIO_PinOutToggle(SL_SIMPLE_LED_LED0_PORT,
                                SL_SIMPLE_LED_LED0_PIN);

              led_state = GPIO_PinOutGet(SL_SIMPLE_LED_LED0_PORT,
                                         SL_SIMPLE_LED_LED0_PIN);
              bthome_v2_reset_measurement();
              bthome_v2_add_measurement_state(STATE_GENERIC_BOOLEAN,
                                              led_state, 0);
              bthome_v2_send_packet();
            }
          }
        }
      }
      if (evt->data.evt_system_external_signal.extsignals
          == SIMPLE_BUTTON_PRESSED) {
        GPIO_PinOutToggle(SL_SIMPLE_LED_LED0_PORT,
                          SL_SIMPLE_LED_LED0_PIN);

        led_state = GPIO_PinOutGet(SL_SIMPLE_LED_LED0_PORT,
                                   SL_SIMPLE_LED_LED0_PIN);
        bthome_v2_reset_measurement();
        bthome_v2_add_measurement_state(STATE_GENERIC_BOOLEAN,
                                        led_state, 0);
        bthome_v2_send_packet();
      }
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

void bthome_v2_server_found_device_callback(uint8_t *mac,
                                            uint8_t *payload,
                                            uint8_t payload_length)
{
  bool encrypted;
  bool key_available;

  app_log("\r\n->MAC: ");
  for (uint8_t i = 0; i < 6; i++) {
    app_log("%.2x", mac[i]);
  }
  app_log("\r\n");

  app_log("\r\n  raw data: ");
  for (uint8_t i = 0; i < payload_length; i++) {
    app_log("%.2x", payload[i]);
  }
  app_log("\r\n");

  bthome_v2_server_check_device(mac, &encrypted, &key_available);

  app_log("  Encryption: %s",
          (encrypted) ? "Yes\r\n" : "No\r\n");
  app_log("  Encryption Key Available: %s",
          (key_available) ? "Yes\r\n" : "No\r\n");
}
