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
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "app_log.h"
#include "gatt_db.h"
#include "sl_sleeptimer.h"
#include "fingerprint_oled.h"
#include "fingerprint_app.h"
#include "fingerprint_nvm.h"
#include "sl_simple_led_instances.h"

#define APP_EVT_LOCKED                                     (1 << 0)
#define APP_EVT_SCAN_FP                                    (1 << 1)
#define APP_EVT_SHOW_FP                                    (1 << 2)
#define SHOW_FINGERPRINT_PERIOD_MS                         (4000)
#define UNLOCK_PERIOD_MS                                   10000
// -------------------------------
// Advertising flags (common)
#define ADVERTISE_FLAGS_LENGTH                             2
#define ADVERTISE_FLAGS_TYPE                               0x01

/** Bit mask for flags advertising data type. */
#define ADVERTISE_FLAGS_LE_LIMITED_DISCOVERABLE            0x01
#define ADVERTISE_FLAGS_LE_GENERAL_DISCOVERABLE            0x02
#define ADVERTISE_FLAGS_BR_EDR_NOT_SUPPORTED               0x04

// -------------------------------
// Scan Response
#define ADVERTISE_MANDATORY_DATA_LENGTH                    5
#define ADVERTISE_MANDATORY_DATA_TYPE_MANUFACTURER         0xFF

#define ADVERTISE_COMPANY_ID                               0x0047 /* Silicon
                                                                   *   Labs */
#define ADVERTISE_FIRMWARE_ID                              0x0000

/** Complete local name. */
#define ADVERTISE_TYPE_LOCAL_NAME                          0x09
#define ADVERTISE_DEVICE_NAME_LEN                          20
#define ADVERTISE_DEVICE_NAME \
  "FINGERPRINT DOORLOCK"

/** Helper macro */
#define UINT16_TO_BYTES(x) { (uint8_t)(x), (uint8_t)((x) >> 8) }

// Structure that holds Scan Response data

typedef struct {
  uint8_t flags_length;          /**< Length of the Flags field. */
  uint8_t flags_type;            /**< Type of the Flags field. */
  uint8_t flags;                 /**< Flags field. */
  uint8_t mandatory_data_length; /**< Length of the mandata field. */
  uint8_t mandatory_data_type;   /**< Type of the mandata field. */
  uint8_t company_id[2];         /**< Company ID. */
  uint8_t firmware_id[2];        /**< Firmware ID */
  uint8_t local_name_length;     /**< Length of the local name field. */
  uint8_t local_name_type;       /**< Type of the local name field. */
  uint8_t local_name[ADVERTISE_DEVICE_NAME_LEN]; /**< Local name field. */
} advertise_scan_response_t;

#define ADVERTISE_SCAN_RESPONSE_DEFAULT                                \
  {                                                                    \
    .flags_length = ADVERTISE_FLAGS_LENGTH,                            \
    .flags_type = ADVERTISE_FLAGS_TYPE,                                \
    .flags = ADVERTISE_FLAGS_LE_GENERAL_DISCOVERABLE                   \
             | ADVERTISE_FLAGS_BR_EDR_NOT_SUPPORTED,                   \
    .mandatory_data_length = ADVERTISE_MANDATORY_DATA_LENGTH,          \
    .mandatory_data_type = ADVERTISE_MANDATORY_DATA_TYPE_MANUFACTURER, \
    .company_id = UINT16_TO_BYTES(ADVERTISE_COMPANY_ID),               \
    .firmware_id = UINT16_TO_BYTES(ADVERTISE_FIRMWARE_ID),             \
    .local_name_length = ADVERTISE_DEVICE_NAME_LEN + 1,                \
    .local_name_type = ADVERTISE_TYPE_LOCAL_NAME,                      \
    .local_name = ADVERTISE_DEVICE_NAME                                \
  }

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static const advertise_scan_response_t adv_scan_response
  = ADVERTISE_SCAN_RESPONSE_DEFAULT;

typedef enum app_door_mode{
  NORMAL_MODE,
  MODE_1,
  MODE_2
}app_door_mode_t;

app_door_mode_t app_door_mode = NORMAL_MODE;

bool volatile app_door_is_locked = true;
bool volatile app_door_is_connected = false;
static uint8_t show_fp_slot = 0;
static uint8_t fp_selected_slot = 0;

static sl_sleeptimer_timer_handle_t unlock_door_timer;
static sl_sleeptimer_timer_handle_t show_fingerprint_timer;
static sl_sleeptimer_timer_handle_t scan_fingerprint_timer;

static void app_door_lock_set_bt_connection_status(bool is_connected);
static void app_show_next_fingerprint(void);
static void app_display_update_status(void);
static void app_door_lock_process_evt_gatt_server_user_write_request(
  sl_bt_evt_gatt_server_user_write_request_t *data);
static void unlock_sleeptimer_callback(sl_sleeptimer_timer_handle_t *timer,
                                       void *data);
static void show_fingerprint_sleeptimer_callback(
  sl_sleeptimer_timer_handle_t *timer,
  void *data);
static void scan_fingerprint_sleeptimer_callback(
  sl_sleeptimer_timer_handle_t *timer,
  void *data);
static void connection_opened_handler(sl_bt_msg_t *evt);
static void app_bt_evt_system_external_signal(uint32_t extsignals);
static void app_door_lock_process_evt_gatt_server_user_read_request(
  sl_bt_evt_gatt_server_user_read_request_t *data);

/**************************************************************************//**
 * Application Init fucntion.
 *****************************************************************************/
void app_init(void)
{
  sl_iostream_set_default(sl_iostream_vcom_handle);
  app_log_iostream_set(sl_iostream_vcom_handle);
  fingerprint_oled_init();
  fingerprint_init();
  sl_sleeptimer_start_periodic_timer_ms(&scan_fingerprint_timer,
                                        500,
                                        scan_fingerprint_sleeptimer_callback,
                                        NULL,
                                        0,
                                        0);
  sl_sleeptimer_start_timer_ms(&show_fingerprint_timer,
                               SHOW_FINGERPRINT_PERIOD_MS,
                               show_fingerprint_sleeptimer_callback,
                               NULL,
                               0,
                               0);
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
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

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

      // Set custom advertising payload
      sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                            sl_bt_advertiser_scan_response_packet,
                                            sizeof(adv_scan_response),
                                            (uint8_t *)&adv_scan_response);
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      app_log("Start advertising ...\n");

      // Maximum allowed bonding count: 8
      // New bonding will overwrite the bonding that was used the longest time
      //   ago
      sc = sl_bt_sm_store_bonding_configuration(8, 0x2);
      app_assert_status(sc);

      // Capabilities: No Input and No Output
      sc =
        sl_bt_sm_configure(0b00000010, sl_bt_sm_io_capability_noinputnooutput);
      app_assert_status(sc);

      // Allow bondings
      sc = sl_bt_sm_set_bondable_mode(1);
      app_assert_status(sc);

      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      connection_opened_handler(evt);
      app_door_lock_set_bt_connection_status(true);
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
      app_log("Start advertising ...\n");

      break;

    case sl_bt_evt_gatt_server_user_write_request_id:
      app_door_lock_process_evt_gatt_server_user_write_request(
        &(evt->data.evt_gatt_server_user_write_request));
      break;

    case sl_bt_evt_gatt_server_user_read_request_id:
      app_door_lock_process_evt_gatt_server_user_read_request(
        &(evt->data.evt_gatt_server_user_read_request));
      break;

    case sl_bt_evt_system_external_signal_id:
      app_bt_evt_system_external_signal(
        evt->data.evt_system_external_signal.extsignals);
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

/***************************************************************************//**
 * Door Lock Application set BLE connection handle.
 ******************************************************************************/
static void app_door_lock_set_bt_connection_status(bool is_connected)
{
  app_door_is_connected = is_connected;

  if (app_door_mode == NORMAL_MODE) {
    app_display_update_status();
  }
}

/***************************************************************************//**
 * Door Lock Application connection open handle.
 ******************************************************************************/
static void connection_opened_handler(sl_bt_msg_t *evt)
{
  (void) evt;
  sl_status_t sc;

  app_log("Bluetooth Stack Event : CONNECTION OPENED\r\n");

  sc = sl_bt_advertiser_stop(advertising_set_handle);
  app_assert_status(sc);

#if ALWAYS_INCRASE_SECURITY
  active_connection_id = evt->data.evt_connection_opened.connection;
  if (ble_bonding_handle == 0xFF) {
    log_info("+ Increasing security\r\n");

    sc = sl_bt_sm_increase_security(active_connection_id);
    app_assert_status(sc);
  } else {
    log_info("+ Already Bonded (ID: %d)\r\n", ble_bonding_handle);
  }
#endif
}

/***************************************************************************//**
 * Door Lock Application update status on OLED screen function.
 ******************************************************************************/
static void app_display_update_status(void)
{
  char middle[15];
  char lower[15];

  if (app_door_is_locked) {
    snprintf(middle, sizeof(middle), " LOCKED");
  } else {
    snprintf(middle, sizeof(middle), "UNLOCKED");
  }

  if (app_door_is_connected) {
    snprintf(lower, sizeof(lower), "CONNECTED");
  } else {
    snprintf(lower, sizeof(lower), "DISCONNECTED");
  }

  fingerprint_oled_update(middle, lower);
}

/***************************************************************************//**
 * Door Lock Application user write request handle.
 ******************************************************************************/
static void app_door_lock_process_evt_gatt_server_user_write_request(
  sl_bt_evt_gatt_server_user_write_request_t *data)
{
  sl_status_t sc = SL_STATUS_NOT_SUPPORTED;
  switch (data->characteristic) {
    case gattdb_mode: {
      uint8_t value = (uint8_t)atof((char *)data->value.data);
      app_log("GATT: write mode: %d\r\n", value);
      if (value > 2) {
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
      } else {
        app_door_mode = value;

        if (app_door_mode == NORMAL_MODE) {
          app_display_update_status();
        } else if (app_door_mode == MODE_1) {
          static char temp[11];
          uint8_t num_authorized_fps = 0;
          num_authorized_fps = fingerprint_get_num_of_fps();
          app_log("Number of authorized fingerprints: %d\n",
                  num_authorized_fps);
          snprintf(temp, sizeof(temp), "%d", num_authorized_fps);
          fingerprint_oled_update(" CONFIG", temp);
        }
        sc = SL_STATUS_OK;
      }
      break;
    }
    case gattdb_remove_authorized_fingerprint: {
      uint8_t fp_slot = (uint8_t)atof((char *)data->value.data);
      if ((fp_slot < FINGERPRINT_MAX_SLOT)
          && (app_door_mode == MODE_1)) {
        app_log("GATT: write remove fingerprint: %d\r\n", fp_slot);
        if (!(sc = fingerprint_remove(fp_slot))) {
          static char temp[11];
          snprintf(temp, sizeof(temp), "%d", fp_slot);
          fingerprint_oled_update(" REMOVED", temp);
        }
      } else {
      }

      break;
    }
    case gattdb_open_lock: {
      uint8_t is_door_open = (uint8_t)atof((char *)data->value.data);
      if ((is_door_open) && (app_door_is_locked)
          && (app_door_mode == NORMAL_MODE)) {
        app_log("GATT: write open_lock: %d\r\n", is_door_open);
        sl_sleeptimer_start_timer_ms(&unlock_door_timer,
                                     UNLOCK_PERIOD_MS,
                                     unlock_sleeptimer_callback,
                                     NULL,
                                     0,
                                     0);
        app_door_is_locked = false;
        sl_led_turn_on(SL_SIMPLE_LED_INSTANCE(0));
        app_display_update_status();
        sc = SL_STATUS_OK;
      } else {
        app_log(
          "GATT: write open_lock: INVALID parameter or DOOR is UNLOCKED!\r\n");
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
      }
      break;
    }
    case gattdb_get_fingerprint_id: {
      fp_selected_slot = (uint8_t)atof((char *)data->value.data);
      break;
    }
    default:
      break;
  }

  sl_bt_gatt_server_send_user_write_response(data->connection,
                                             data->characteristic,
                                             sc);
}

/***************************************************************************//**
 * Door Lock Application external signal handle.
 ******************************************************************************/
static void app_bt_evt_system_external_signal(uint32_t extsignals)
{
  sl_status_t sc;
  if (extsignals & APP_EVT_LOCKED) {
    app_door_is_locked = true;
    app_display_update_status();
  } else if (extsignals & APP_EVT_SCAN_FP) {
    if (app_door_mode == MODE_2) {
      bool is_running = false;
      if (SL_STATUS_OK
          != sl_sleeptimer_is_timer_running(&show_fingerprint_timer,
                                            &is_running)) {
        is_running = false;
      }

      if (!is_running) {
        sl_sleeptimer_start_periodic_timer_ms(&show_fingerprint_timer,
                                              SHOW_FINGERPRINT_PERIOD_MS,
                                              show_fingerprint_sleeptimer_callback,
                                              NULL,
                                              0,
                                              0);
        show_fp_slot = 0;
        app_show_next_fingerprint();
      }
      return;
    }
    sl_sleeptimer_stop_timer(&show_fingerprint_timer);
    switch (app_door_mode) {
      case NORMAL_MODE: {
        if (app_door_is_locked
            && (SL_STATUS_ALREADY_EXISTS == fingerprint_compare())) {
          app_door_is_locked = false;
          uint8_t fp_authorized_index = 0;
          static char temp[11];
          fp_authorized_index = fingerprint_get_authorized_index();
          app_log("Fingerprint is authorized.\n");
          snprintf(temp, sizeof(temp), "%d", fp_authorized_index);
          fingerprint_oled_update("UNLOCKED", temp);
          sl_led_turn_on(SL_SIMPLE_LED_INSTANCE(0));
          sl_sleeptimer_start_timer_ms(&unlock_door_timer,
                                       UNLOCK_PERIOD_MS,
                                       unlock_sleeptimer_callback,
                                       NULL,
                                       0,
                                       0);
        }
        break;
      }

      case MODE_1: {
        if (fingerprint_compare() == SL_STATUS_INVALID_HANDLE) {
          uint8_t free_slot = 0;
          sc = fingerprint_get_free_slot(&free_slot);
          if (sc == SL_STATUS_OK) {
            if (SL_STATUS_OK == fingerprint_add(free_slot)) {
              static char temp[11];
              snprintf(temp, sizeof(temp), "%d", free_slot);
              app_log("Fingerprint is added to index: %d\n", free_slot);
              fingerprint_oled_update("  ADDED", temp);
              sl_sleeptimer_delay_millisecond(3000);
            } else {
              app_log("Registration is failed!\n");
            }
          } else {
            app_log("Storage space is full!\n");
          }
        }
        break;
      }
      default:
        break;
    }
  } else if (extsignals & APP_EVT_SHOW_FP) {
    if (app_door_mode == MODE_2) {
      app_show_next_fingerprint();
    } else {
      sl_sleeptimer_stop_timer(&show_fingerprint_timer);
    }
  }
}

/***************************************************************************//**
 * Door Lock Application user read request handle.
 ******************************************************************************/
static void app_door_lock_process_evt_gatt_server_user_read_request(
  sl_bt_evt_gatt_server_user_read_request_t *data)
{
  switch (data->characteristic) {
    case gattdb_mode: {
      uint8_t value = app_door_mode;
      sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        sizeof (value),
        (uint8_t *)&value,
        NULL);
      break;
    }
    case gattdb_get_fingerprint_id: {
      static char string_temp[20];
      bool is_free = false;
      uint8_t len = 0;
      sl_status_t sc;
      sc = fingerprint_check_slot(fp_selected_slot, &is_free);
      if (!sc) {
        if (is_free) {
          len = snprintf(string_temp, sizeof(string_temp), "EMPTY");
        } else {
          len = snprintf(string_temp, sizeof(string_temp), "EXISTS");
        }
        sl_bt_gatt_server_send_user_read_response(data->connection,
                                                  data->characteristic,
                                                  0,
                                                  len,
                                                  (uint8_t *)string_temp,
                                                  NULL);
      } else {
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
        sl_bt_gatt_server_send_user_read_response(data->connection,
                                                  data->characteristic,
                                                  0,
                                                  sizeof(sc),
                                                  (uint8_t *)sc,
                                                  NULL);
      }
      break;
    }
    default:
      break;
  }
}

/***************************************************************************//**
 * Door Lock Application show next fingerprint index function.
 ******************************************************************************/
static void app_show_next_fingerprint(void)
{
  uint8_t num_fps = 0;
  bool is_free = true;

  num_fps = fingerprint_get_num_of_fps();

  if (num_fps == 0) {
    fingerprint_oled_update("   -", "NO-FPs");
  } else {
    if (show_fp_slot >= FINGERPRINT_MAX_SLOT) {
      show_fp_slot = 0;
    }
    fingerprint_check_slot(show_fp_slot, &is_free);

    while (is_free)
    {
      show_fp_slot++;
      fingerprint_check_slot(show_fp_slot, &is_free);
    }

    static char temp[11];
    snprintf(temp, sizeof(temp), "   %d.", show_fp_slot);
    fingerprint_oled_update(temp, "");
    show_fp_slot++;
  }
}

/***************************************************************************//**
 * Door Lock Application unlock timer callback function.
 ******************************************************************************/
void unlock_sleeptimer_callback(sl_sleeptimer_timer_handle_t *timer, void *data)
{
  (void) timer;
  (void) data;
  sl_bt_external_signal(APP_EVT_LOCKED);
  sl_led_turn_off(SL_SIMPLE_LED_INSTANCE(0));
}

/***************************************************************************//**
 * Door Lock Application show fingerprint timer callback function.
 ******************************************************************************/
static void show_fingerprint_sleeptimer_callback(
  sl_sleeptimer_timer_handle_t *timer,
  void *data)
{
  (void) timer;
  (void) data;
  sl_bt_external_signal(APP_EVT_SHOW_FP);
}

/***************************************************************************//**
 * Door Lock Application scan fingerprint timer callback function.
 ******************************************************************************/
static void scan_fingerprint_sleeptimer_callback(
  sl_sleeptimer_timer_handle_t *timer,
  void *data)
{
  (void) timer;
  (void) data;
  sl_bt_external_signal(APP_EVT_SCAN_FP);
}
