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
#include "app.h"
#include "app_log.h"
#include "gatt_db.h"
#include "sl_bluetooth.h"
#include "sl_simple_button_instances.h"

#define TICKS_PER_SECOND    (32768)
#define TICKS_STOP          (0)
#define TICKS_NOTIFY        (TICKS_PER_SECOND / 50) // 20ms
#define TICKS_KEY_SCAN      (TICKS_PER_SECOND / 100) // 10ms
#define TICKS_IR_REPEAT     (TICKS_PER_SECOND * 9 / 200) // 45ms
#define SIGNAL_NOTIFY       20
#define SIGNAL_KEY_SCAN     10
#define SIGNAL_IR_REPEAT    45

static sl_sleeptimer_timer_handle_t timer_test_notify;
static sl_sleeptimer_timer_handle_t timer_key_scan;
static sl_sleeptimer_timer_handle_t timer_ir_repeat;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static uint8_t connect_handle = 0xff;

/* Flag for indicating DFU Reset must be performed */
static uint8_t boot_to_dfu = 0;
static uint8_t battery_alert_nodif_data = 0;
static uint8_t key_test = 0xFF;
static uint8_t key_repeat = 0xFF;

static void timer_cb(sl_sleeptimer_timer_handle_t *timer, void *data)
{
  (void)&data;

  if (timer == &timer_test_notify) {
    sl_bt_external_signal(SIGNAL_NOTIFY);
  } else if (timer == &timer_key_scan) {
    sl_bt_external_signal(SIGNAL_KEY_SCAN);
  } else {
    sl_bt_external_signal(SIGNAL_IR_REPEAT);
  }
}

#if !D_KEYSCAN
#define BUTTON0   (uint32_t)(1 << 0)   // Bit flag to external signal command
#define BUTTON1   (uint32_t)(1 << 1)   // Bit flag to external signal command
void sl_button_on_change(const sl_button_t *handle)
{
  if (handle == &sl_button_btn0) {
    if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
      // PB0 pressed down
      sl_bt_external_signal(BUTTON0);
    }
  } else if (handle == &sl_button_btn1) {
    if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
      // PB1 pressed
      sl_bt_external_signal(BUTTON1);
    }
  }
}

#endif

/**
 * @brief key press wakeup callback
 *
 * @param interrupt pin
 *
 * @return none
 *
 */
void app_key_wakeup(uint8_t pin)
{
  // Test code, key jitter, avoid multiple set the timer.
  if (key_test == pin) {
    return;
  }
  key_test = pin;

  // app_log("key wakeup %d\r\n", pin);
  // Start the key timer
  sl_sleeptimer_start_periodic_timer(&timer_key_scan,
                                     TICKS_KEY_SCAN,
                                     timer_cb,
                                     NULL,
                                     0,
                                     0);
}

/**
 * @brief key detect callback
 *
 * @param key value, KEY_NONE means key release.
 *
 * @return none
 *
 */
void app_key_detect(key_code_t key)
{
  if (key == KEY_NONE) {
    app_log("key release\r\n");
    key_test = 0xFF;
    // Key have release, stop the key timer
    sl_sleeptimer_stop_timer(&timer_key_scan);
    sl_sleeptimer_stop_timer(&timer_ir_repeat);
  } else {
    key_repeat = key;
    app_log("key detect %d\r\n", key);
//    EMU_EnterEM2(true); // block the EM2, otherwise, timer1 may stop.
    ir_generate_stream(ir_table[key % 18][0], ir_table[key % 18][1], false);
    sl_sleeptimer_start_timer(&timer_ir_repeat,
                              TICKS_IR_REPEAT,
                              timer_cb,
                              NULL,
                              0,
                              0);
  }
}

/**
 * @brief ir sent callback
 *
 * @param none
 *
 * @return none
 *
 */
void app_ir_complete(void)
{
  app_log("ir complete\r\n");
}

/* Print stack version and local Bluetooth address as boot message */
static void bootMessage(sl_bt_evt_system_boot_t *bootevt)
{
#if DEBUG_LEVEL
  sl_status_t sc;
  bd_addr local_addr;
  uint8_t address_type;
  int i;

  app_log("stack version: %u.%u.%u\r\n",
          bootevt->major,
          bootevt->minor,
          bootevt->patch);
  sc = sl_bt_system_get_identity_address(&local_addr, &address_type);
  app_assert_status(sc);
  app_log("local BT device address: ");
  for (i = 0; i < 5; i++) {
    app_log("%2.2x:", local_addr.addr[5 - i]);
  }
  app_log("%2.2x\r\n", local_addr.addr[0]);
#endif
}

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
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
      bootMessage(&(evt->data.evt_system_boot));
      app_log("boot event - starting advertising\r\n");

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
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);

      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
#if D_KEYSCAN
      key_init(app_key_detect, app_key_wakeup);
#endif
#if D_IR
      ir_generate_init(CODE_SONY, app_ir_complete);
#endif
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("connection opened\r\n");
      connect_handle = evt->data.evt_connection_opened.connection;
      sl_sleeptimer_start_periodic_timer(&timer_test_notify,
                                         TICKS_NOTIFY,
                                         timer_cb,
                                         NULL,
                                         0,
                                         0);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("connection closed, reason: 0x%2.2x\r\n",
              evt->data.evt_connection_closed.reason);
      if (boot_to_dfu) {
        /* Enter to OTA DFU mode */
        sl_bt_system_reboot();
      } else {
        // Generate data for advertising
        sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                   sl_bt_advertiser_general_discoverable);
        app_assert_status(sc);

        // Restart advertising after client has disconnected.
        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                           sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);
      }
      break;

    case sl_bt_evt_system_external_signal_id:
      if (evt->data.evt_system_external_signal.extsignals == SIGNAL_NOTIFY) {
        uint8_t d = battery_alert_nodif_data++;
        sc = sl_bt_gatt_server_notify_all(gattdb_my_test,
                                          sizeof(d),
                                          &d);
        if (sc == SL_STATUS_OK) {
          app_log("\rsend notification ok\n");
        } else {
          app_log("\rsend notification fail: 0x%.4lx\n", sc);
        }
      }

      if (evt->data.evt_system_external_signal.extsignals == SIGNAL_KEY_SCAN) {
#if D_KEYSCAN
        key_scan();
#endif
      }

      if (evt->data.evt_system_external_signal.extsignals == SIGNAL_IR_REPEAT) {
//        EMU_EnterEM2(true);  // block the EM2, otherwise, timer1 may stop.
        ir_generate_stream(ir_table[key_repeat % 18][0],
                           ir_table[key_repeat % 18][1],
                           false);
        sl_sleeptimer_start_timer(&timer_ir_repeat,
                                  TICKS_IR_REPEAT,
                                  timer_cb,
                                  NULL,
                                  0,
                                  0);
      }
      // The capability status was changed
#if !D_KEYSCAN
      if (evt->data.evt_system_external_signal.extsignals == BUTTON0) {
        app_log("BUTTON0\r\n");
#if D_IR
        ir_generate_stream(ir_table[0][0], ir_table[0][1], false);
#endif
      } else if (evt->data.evt_system_external_signal.extsignals == BUTTON1) {
        app_log("BUTTON1\r\n");
#if D_IR
        ir_generate_stream(ir_table[0][0], ir_table[0][1], true);
#endif
      }
#endif
      break;

    /* Check if the user-type OTA Control Characteristic was written.
     * If ota_control was written, boot the device into Device Firmware Upgrade
     * (DFU) mode.
     */
    case sl_bt_evt_gatt_server_user_write_request_id:
      if (evt->data.evt_gatt_server_user_write_request.characteristic
          == gattdb_ota_control) {
        /* Set flag to enter to OTA mode */
        boot_to_dfu = 1;

        /* Send response to Write Request */
        sl_bt_gatt_server_send_user_write_response(
          evt->data.evt_gatt_server_user_write_request.connection,
          gattdb_ota_control,
          SL_STATUS_OK);

        /* Close connection to enter to DFU OTA mode */
        sl_bt_connection_close(
          evt->data.evt_gatt_server_user_write_request.connection);
      }
      break;

    default:
      break;
  }
}
