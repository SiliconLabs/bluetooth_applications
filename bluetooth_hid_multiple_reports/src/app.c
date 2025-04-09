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
#include "app_assert.h"
#include "gatt_db.h"

#include "sl_simple_button_instances.h"
#include "sl_sleeptimer.h"
#include "sl_joystick.h"

#define EXT_SIG_MOUSE           (0x01)
#define EXT_SIG_KEEB            (0x02)

#define NOTIF_KEEB_FLAG         (0x01)
#define NOTIF_MOUSE_FLAG        (0x02)

#define KEY_ARRAY_SIZE          (25U)

#define MODIFIER_INDEX          (0)
#define DATA_INDEX              (2)

#define MOUSE_MIN               -127
#define MOUSE_MAX               127
#define MOUSE_INCR              25

#define BIT_MASK(b)             ((1U) << b)

typedef enum {
  MOUSE_BUTTON_IDX = 0,
  MOUSE_X_IDX,
  MOUSE_Y_IDX,
  MOUSE_TOTAL_IDX
} mouse_idx_t;

typedef enum {
  MOUSE_BUTTON_1 = 0,
  MOUSE_BUTTON_2,
  MOUSE_BUTTON_3
} mouse_button_t;

typedef enum {
  KEY_MOD_LCTRL = 0,
  KEY_MOD_LSHIFT,
  KEY_MOD_LALT,
  KEY_MOD_LGUI,
  KEY_MOD_RCTRL,
  KEY_MOD_RSHIFT,
  KEY_MOD_RALT,
  KEY_MOD_RGUI
} key_modifier_t;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static uint8_t notification_enabled = 0;

static uint8_t input_report_data[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

/* Report Formats:
 *
 * Keyboard
 *    Byte    Type
 *    0       Modifier
 *    1       Reserved
 *    2       Keycode 1
 *    3       Keycode 2
 *    4       Keycode 3
 *    5       Keycode 4
 *    6       Keycode 5
 *    7       Keycode 6
 *
 * Mouse
 *    Byte    Bits    Type
 *    0       0       Button 1
 *            1       Button 2
 *            2       Button 3
 *            4-7     device specific
 *    1       0-7     X displacement
 *    2       0-7     Y displacement
 *    3-n     0-7     Device specific (optional)
 *
 */

static volatile uint8_t actual_key, mod_key;
static volatile int8_t x_displacement = 0;
static volatile int8_t y_displacement = 0;
static volatile uint8_t mouse_button = 0;
static volatile uint8_t counter = 0;

static const uint8_t reduced_key_array[] = {
  0x04,   /* a */
  0x05,   /* b */
  0x06,   /* c */
  0x07,   /* d */
  0x08,   /* e */
  0x09,   /* f */
  0x0a,   /* g */
  0x0b,   /* h */
  0x0c,   /* i */
  0x0d,   /* j */
  0x0e,   /* k */
  0x0f,   /* l */
  0x10,   /* m */
  0x11,   /* n */
  0x12,   /* o */
  0x13,   /* p */
  0x14,   /* q */
  0x15,   /* r */
  0x16,   /* s */
  0x17,   /* t */
  0x18,   /* u */
  0x19,   /* v */
  0x1a,   /* w */
  0x1b,   /* x */
  0x1c,   /* y */
  0x1d,   /* z */
};

static sl_joystick_t sl_joystick_handle = JOYSTICK_HANDLE_DEFAULT;
static sl_sleeptimer_timer_handle_t joystick_timer;

void joystick_timer_cb(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  (void)data;

  sl_joystick_position_t pos;
  sl_joystick_get_position(&sl_joystick_handle, &pos);

  switch (pos) {
    case JOYSTICK_S:
      if (y_displacement < MOUSE_MAX) {
        y_displacement += MOUSE_INCR;
      }
      break;

    case JOYSTICK_N:
      if (y_displacement > MOUSE_MIN) {
        y_displacement -= MOUSE_INCR;
      }
      break;

    case JOYSTICK_E:
      if (x_displacement < MOUSE_MAX) {
        x_displacement += MOUSE_INCR;
      }
      break;

    case JOYSTICK_W:
      if (x_displacement > MOUSE_MIN) {
        x_displacement -= MOUSE_INCR;
      }
      break;

    case JOYSTICK_C:
      mouse_button ^= BIT_MASK(MOUSE_BUTTON_1);   // Mouse click
      break;

    default:
      return;
  }

  sl_bt_external_signal(EXT_SIG_MOUSE);
}

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_joystick_init(&sl_joystick_handle);
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
    case sl_bt_evt_system_boot_id:
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

      app_log("boot event - starting advertising\r\n");

      sc = sl_bt_sm_configure(0, sl_bt_sm_io_capability_noinputnooutput);
      app_assert_status(sc);
      sc = sl_bt_sm_set_bondable_mode(1);
      app_assert_status(sc);

      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    case sl_bt_evt_connection_opened_id:
      app_log("connection opened\r\n");
      sc =
        sl_bt_sm_increase_security(evt->data.evt_connection_opened.connection);
      app_assert_status(sc);
      break;

    case sl_bt_evt_connection_closed_id:

      sl_sleeptimer_stop_timer(&joystick_timer);

      sl_joystick_stop(&sl_joystick_handle);

      app_log("connection closed, reason: 0x%2.2x\r\n",
              evt->data.evt_connection_closed.reason);
      notification_enabled = 0;
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    case sl_bt_evt_sm_bonded_id:
      app_log("successful bonding\r\n");
      break;

    case sl_bt_evt_sm_bonding_failed_id:
      app_log("bonding failed, reason 0x%2X\r\n",
              evt->data.evt_sm_bonding_failed.reason);

      /* Previous bond is broken, delete it and close connection,
       *  host must retry at least once */
      sc = sl_bt_sm_delete_bondings();
      sc = sl_bt_connection_close(evt->data.evt_sm_bonding_failed.connection);
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (evt->data.evt_gatt_server_characteristic_status.characteristic
          == gattdb_report_keeb) {
        // client characteristic configuration changed by remote GATT client
        if (evt->data.evt_gatt_server_characteristic_status.status_flags
            == sl_bt_gatt_server_client_config) {
          if (evt->data.evt_gatt_server_characteristic_status.
              client_config_flags == sl_bt_gatt_notification) {
            notification_enabled |= NOTIF_KEEB_FLAG;
          } else {
            notification_enabled &= ~NOTIF_KEEB_FLAG;
          }
        }
      }

      if (evt->data.evt_gatt_server_characteristic_status.characteristic
          == gattdb_report_mouse) {
        // client characteristic configuration changed by remote GATT client
        if (evt->data.evt_gatt_server_characteristic_status.status_flags
            == sl_bt_gatt_server_client_config) {
          if (evt->data.evt_gatt_server_characteristic_status.
              client_config_flags == sl_bt_gatt_notification) {
            notification_enabled |= NOTIF_MOUSE_FLAG;
            sl_joystick_start(&sl_joystick_handle);
            sl_sleeptimer_start_periodic_timer_ms(&joystick_timer,
                                                  250,
                                                  joystick_timer_cb,
                                                  NULL,
                                                  32,
                                                  0);
          } else {
            notification_enabled &= ~NOTIF_MOUSE_FLAG;
            sl_sleeptimer_stop_timer(&joystick_timer);
            sl_joystick_stop(&sl_joystick_handle);
          }
        }
      }
      break;

    case  sl_bt_evt_system_external_signal_id:
      if (notification_enabled == 0) {
        break;
      }

      memset(input_report_data, 0, sizeof(input_report_data));

      if (evt->data.evt_system_external_signal.extsignals & EXT_SIG_MOUSE) {
        input_report_data[MOUSE_BUTTON_IDX] = mouse_button;
        input_report_data[MOUSE_X_IDX] = (uint8_t)x_displacement;
        input_report_data[MOUSE_Y_IDX] = (uint8_t)y_displacement;

        app_log("m:%x\r\n", mouse_button);

        x_displacement = 0;
        y_displacement = 0;

        sc = sl_bt_gatt_server_notify_all(gattdb_report_mouse,
                                          sizeof(input_report_data),
                                          input_report_data);
        app_assert_status(sc);
        app_log("Mouse report was sent\r\n");
      }

      if (evt->data.evt_system_external_signal.extsignals & EXT_SIG_KEEB) {
        input_report_data[MODIFIER_INDEX] = mod_key;
        input_report_data[DATA_INDEX] = actual_key;

        app_log("%x\r\n", mod_key);

        sc = sl_bt_gatt_server_notify_all(gattdb_report_keeb,
                                          sizeof(input_report_data),
                                          input_report_data);
        app_assert_status(sc);
        app_log("Key report was sent\r\n");
      }
      break;

    default:
      break;
  }
}

void sl_button_on_change(const sl_button_t *handle)
{
  if (&sl_button_btn0 == handle) {
    if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
      app_log("Button pushed - alpha\r\n");
      actual_key = reduced_key_array[counter];
    } else {
      app_log("Button released - alpha \r\n");

      if (KEY_ARRAY_SIZE == counter) {
        counter = 0;
      } else {
        counter++;
      }

      actual_key = 0;
    }
  }

  if (&sl_button_btn1 == handle) {
    if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
      app_log("Button pushed - LSHIFT\r\n");
      mod_key |= BIT_MASK(KEY_MOD_LSHIFT);
    } else {
      app_log("Button released - LSHIFT \r\n");
      mod_key &= ~BIT_MASK(KEY_MOD_LSHIFT);
    }
  }

  sl_bt_external_signal(EXT_SIG_KEEB);
}
