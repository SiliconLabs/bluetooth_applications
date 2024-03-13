/***************************************************************************//**
 * @file app.c
 * @brief Main application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 ********************************************************************************
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
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/
#include <stdio.h>
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "sl_sleeptimer.h"
#include "app.h"
#include "app_log.h"
#include "rfid_card.h"
#include "sparkfun_rfid_id12la.h"
#include "app_display.h"
#include "sl_i2cspm_instances.h"

/***************************************************************************//**
 * @addtogroup door_lock_app
 * @brief  Door Lock application.
 * @details
 * @{
 ******************************************************************************/

#define APP_EVT_LOCKED                              (1 << 0)
#define APP_EVT_SCAN_TAG                            (1 << 1)
#define APP_EVT_SHOW_CARD                           (1 << 2)
#define SHOW_CARD_PERIOD_MS                         (4000)

// -------------------------------
// Advertising flags (common)
#define ADVERTISE_FLAGS_LENGTH                      2
#define ADVERTISE_FLAGS_TYPE                        0x01

/** Bit mask for flags advertising data type. */
#define ADVERTISE_FLAGS_LE_LIMITED_DISCOVERABLE     0x01
#define ADVERTISE_FLAGS_LE_GENERAL_DISCOVERABLE     0x02
#define ADVERTISE_FLAGS_BR_EDR_NOT_SUPPORTED        0x04

// -------------------------------
// Scan Response
#define ADVERTISE_MANDATORY_DATA_LENGTH             5
#define ADVERTISE_MANDATORY_DATA_TYPE_MANUFACTURER  0xFF

#define ADVERTISE_COMPANY_ID                        0x0047 /* Silicon Labs */
#define ADVERTISE_FIRMWARE_ID                       0x0000

/** Complete local name. */
#define ADVERTISE_TYPE_LOCAL_NAME                   0x09
#define ADVERTISE_DEVICE_NAME_LEN                   13
#define ADVERTISE_DEVICE_NAME                       "RFID DOORLOCK"

/** Helper macro */
#define UINT16_TO_BYTES(x) { (uint8_t)(x), (uint8_t)((x) >> 8) }

/* maximum number of iterations when polling UART RX data before sending data
 *   over BLE connection
 * set value to 0 to disable optimization -> minimum latency but may decrease
 *   throughput */
#define UART_POLL_TIMEOUT                           5000

// -------------------------------
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

typedef struct rfid_reader_buffer {
  uint8_t id_tag[6];
}rfid_reader_buffer_t;

app_door_mode_t app_door_mode = NORMAL_MODE;
bool volatile app_door_is_locked = true;
bool volatile app_door_is_connected = false;
bool volatile app_door_selected_card_id = 0;
static uint8_t show_card_slot = 0;

static sl_sleeptimer_timer_handle_t unlock_timer;
static sl_sleeptimer_timer_handle_t scan_tag_timer;
static sl_sleeptimer_timer_handle_t show_card_timer;

static void app_bt_evt_system_external_signal(uint32_t extsignals);
static void app_show_next_card(void);
static void app_door_lock_init(void);

void app_door_lock_process_evt_gatt_server_user_write_request(
  sl_bt_evt_gatt_server_user_write_request_t *data);
void app_door_lock_process_evt_gatt_server_user_read_request(
  sl_bt_evt_gatt_server_user_read_request_t *data);
static void app_door_lock_set_bt_connection_status(bool is_connected);
static void app_display_update_status(void);

/***************************************************************************//**
 * Door Lock Application unlock timer callback function.
 ******************************************************************************/
void unlock_timer_callback(sl_sleeptimer_timer_handle_t *timer, void *data)
{
  (void) timer;
  (void) data;
  sl_bt_external_signal(APP_EVT_LOCKED);
}

/***************************************************************************//**
 * Door Lock Application scan tag timer callback function.
 ******************************************************************************/
void scan_tag_timer_callback(sl_sleeptimer_timer_handle_t *timer, void *data)
{
  (void) timer;
  (void) data;
  sl_bt_external_signal(APP_EVT_SCAN_TAG);
}

/***************************************************************************//**
 * Door Lock Application show card timer callback function.
 ******************************************************************************/
void show_card_timer_callback(sl_sleeptimer_timer_handle_t *timer, void *data)
{
  (void) timer;
  (void) data;
  sl_bt_external_signal(APP_EVT_SHOW_CARD);
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

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  app_door_lock_init();
  app_display_init();
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
//  app_door_lock_handle_normal_mode();
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
      app_door_lock_set_bt_connection_status(false);
      app_assert_status(sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
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
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

/***************************************************************************//**
 * Door Lock Application external signal handle.
 ******************************************************************************/
static void app_bt_evt_system_external_signal(uint32_t extsignals)
{
  if (extsignals & APP_EVT_LOCKED) {
    app_door_is_locked = true;
    app_display_update_status();
  } else if (extsignals & APP_EVT_SCAN_TAG) {
    static id12la_tag_list_t rfid_reader_buffer;
    static uint8_t rfid_reader_count_tag = 0;

    if (app_door_mode == MODE_2) {
      bool running = false;

      if (SL_STATUS_OK
          != sl_sleeptimer_is_timer_running(&show_card_timer, &running)) {
        running = false;
      }
      if (!running) {
        sl_sleeptimer_start_periodic_timer_ms(&show_card_timer,
                                              SHOW_CARD_PERIOD_MS,
                                              show_card_timer_callback,
                                              NULL,
                                              0,
                                              0);
        show_card_slot = 0;
        app_show_next_card();
      }
      // No need to continue scan tag
      return;
    }

    // Stop showing card
    sl_sleeptimer_stop_timer(&show_card_timer);

    if (sparkfun_id12la_get_all_tag(&rfid_reader_buffer,
                                    &rfid_reader_count_tag) != SL_STATUS_OK) {
      app_log("Error while scanning tags, check connection!!!\r\n");
    }

    if (rfid_reader_count_tag > 0) {
      app_log("Count tag: %d\r\n", rfid_reader_count_tag);
    }

    for (uint8_t i = 0; i < rfid_reader_count_tag; i++) {
      if (rfid_reader_buffer.id12la_data[i].checksum_valid == true) {
        switch (app_door_mode) {
          case NORMAL_MODE:
            if (rfid_card_find(rfid_reader_buffer.id12la_data[i].id_tag)
                && app_door_is_locked) {
              static char temp[13];

              app_door_is_locked = false;
              snprintf(temp, sizeof(temp), "%02X%02X%02X%02X%02X%02X",
                       rfid_reader_buffer.id12la_data[i].id_tag[0],
                       rfid_reader_buffer.id12la_data[i].id_tag[1],
                       rfid_reader_buffer.id12la_data[i].id_tag[2],
                       rfid_reader_buffer.id12la_data[i].id_tag[3],
                       rfid_reader_buffer.id12la_data[i].id_tag[3],
                       rfid_reader_buffer.id12la_data[i].id_tag[5]);
              app_display_update("UNLOCKED", temp);
              app_log("Card ID: %s\n", temp);
              app_log("Card Authorized\r\n");
              sl_sleeptimer_start_timer_ms(&unlock_timer,
                                           10000,
                                           unlock_timer_callback,
                                           NULL,
                                           0,
                                           0);
            }
            break;

          case MODE_1:
            if (SL_STATUS_OK
                == rfid_card_add(rfid_reader_buffer.id12la_data[i].id_tag)) {
              static char temp[13];

              snprintf(temp,
                       sizeof(temp),
                       "%02X%02X%02X%02X%02X%02X",
                       rfid_reader_buffer.id12la_data[i].id_tag[0],
                       rfid_reader_buffer.id12la_data[i].id_tag[1],
                       rfid_reader_buffer.id12la_data[i].id_tag[2],
                       rfid_reader_buffer.id12la_data[i].id_tag[3],
                       rfid_reader_buffer.id12la_data[i].id_tag[4],
                       rfid_reader_buffer.id12la_data[i].id_tag[5]);
              app_display_update("  ADDED", temp);
              app_log("This is a new card\r\n");
            }
            break;

          default:
            break;
        }
      } else {
        app_log("Tag %d : checksum error, please scan the tag again\n", i);
      }
    }
  } else if (extsignals & APP_EVT_SHOW_CARD) {
    if (app_door_mode == MODE_2) {
      app_show_next_card();
    } else {
      // Mode has changed, stop showing card
      sl_sleeptimer_stop_timer(&show_card_timer);
    }
  }
}

/***************************************************************************//**
 * Door Lock Application show next card.
 ******************************************************************************/
static void app_show_next_card(void)
{
  uint8_t number_of_cards = 0;
  rfid_card_get_number_of_cards(&number_of_cards);
  if (number_of_cards == 0) {
    app_display_update("    -", "  NO CARDS");
  } else {
    uint8_t id_tag[6];
    bool is_free = true;
    rfid_card_check_free_slot(show_card_slot, &is_free);

    if (show_card_slot >= RFID_CARD_TAG_COUNT) {
      show_card_slot = 0;
    }

    while (is_free) {
      show_card_slot++;
      rfid_card_check_free_slot(show_card_slot, &is_free);
    }
    rfid_card_get(show_card_slot, id_tag);
    app_display_show_card(show_card_slot, id_tag);
    show_card_slot++;
  }
}

/***************************************************************************//**
 * Door Lock Application init.
 ******************************************************************************/
static void app_door_lock_init(void)
{
  rfid_card_init();

  sl_status_t ret;

  ret = sparkfun_id12la_init(sl_i2cspm_qwiic);

  if (ret != SL_STATUS_OK) {
    app_log("i2c address has been changed before\n");
    ret = sparkfun_id12la_scan_address();
  }

  sl_sleeptimer_start_periodic_timer_ms(&scan_tag_timer,
                                        100,
                                        scan_tag_timer_callback,
                                        NULL,
                                        0,
                                        0);

  // Just initialize timer
  sl_sleeptimer_start_timer_ms(&show_card_timer,
                               SHOW_CARD_PERIOD_MS,
                               show_card_timer_callback,
                               NULL,
                               0,
                               0);
}

/***************************************************************************//**
 * Door Lock Application BLE write request handle.
 ******************************************************************************/
void app_door_lock_process_evt_gatt_server_user_write_request(
  sl_bt_evt_gatt_server_user_write_request_t *data)
{
  sl_status_t sc = SL_STATUS_NOT_SUPPORTED;

  switch (data->characteristic) {
    case gattdb_mode: {
      uint8_t value = (uint8_t)atof((char *)data->value.data);
      app_log("GATT: write: mode: %d\r\n", value);
      if (value > 2) {
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
      } else {
        app_door_mode = value;

        if (value == NORMAL_MODE) {
          app_display_update_status();
        } else if (value == MODE_1) {
          static char temp[11];
          uint8_t app_door_number_registed_card = 0;
          rfid_card_get_number_of_cards(&app_door_number_registed_card);
          snprintf(temp, 11, "    %d", app_door_number_registed_card);
          app_display_update(" CONFIG", temp);
        }
        sc = SL_STATUS_OK;
      }

      break;
    }
    case gattdb_remove_card: {
      uint8_t location = (uint8_t)atof((char *)data->value.data);
      if ((location < 10) && (app_door_mode == MODE_1)) {
        static char temp[11];

        app_log("GATT: write: remove_card: %d\r\n", location);
        if (!(sc = rfid_card_remove_slot(location))) {
          snprintf(temp, 11, "     %d", location);
          app_display_update(" REMOVED", temp);
        }
      } else {
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
      }

      break;
    }
    case gattdb_get_card_id: {
      uint8_t value = (uint8_t)atof((char *)data->value.data);
      if (value < 10) {
        app_door_selected_card_id = value;
        app_log("GATT: write: get_card_id: %d\r\n", value);
        sc = SL_STATUS_OK;
      } else {
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
      }
      break;
    }
    case gattdb_open_lock: {
      uint8_t door_is_open = (uint8_t)atof((char *)data->value.data);
      if ((door_is_open == 1) && app_door_is_locked
          && (app_door_mode == NORMAL_MODE)) {
        app_log("GATT: write: open_lock: %d\r\n", door_is_open);
        app_door_is_locked = false;
        sl_sleeptimer_start_timer_ms(&unlock_timer,
                                     10000,
                                     unlock_timer_callback,
                                     NULL,
                                     0,
                                     0);
        if (app_door_mode == NORMAL_MODE) {
          app_display_update_status();
        }
        sc = SL_STATUS_OK;
      } else {
        app_log("GATT: write: open_lock: INVALID PARA or DOOR is UNLOCKED\r\n");
        sc = SL_STATUS_BT_ATT_OUT_OF_RANGE;
      }
      break;
    }
  }

  sl_bt_gatt_server_send_user_write_response(
    data->connection,
    data->characteristic,
    sc);
}

/***************************************************************************//**
 * Door Lock Application BLE read request handle.
 ******************************************************************************/
void app_door_lock_process_evt_gatt_server_user_read_request(
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
    case gattdb_get_card_id: {
      static char string_temp[20];
      static uint8_t id_tag_temp[6];
      uint8_t len = 0;

      rfid_card_get(app_door_selected_card_id, id_tag_temp);
      len = snprintf(string_temp,
                     20,
                     "%02X %02X %02X %02X %02X %02X",
                     id_tag_temp[0],
                     id_tag_temp[1],
                     id_tag_temp[2],
                     id_tag_temp[3],
                     id_tag_temp[4],
                     id_tag_temp[5]);
      sl_bt_gatt_server_send_user_read_response(
        data->connection,
        data->characteristic,
        0,
        len,
        (uint8_t *)&string_temp,
        NULL);
      break;
    }
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

  app_display_update(middle, lower);
}

/** @} (end group door_lock_app) */
