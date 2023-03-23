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
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include <stdio.h>
#include "sparkfun_type5.h"

// Advertising flags (common).
#define ADVERTISE_FLAGS_LENGTH         0x02
#define ADVERTISE_FLAGS_TYPE           0x01
#define ADVERTISE_FLAGS_DATA           0x06

// Complete local name.
#define DEVICE_NAME_LENGTH             9
#define DEVICE_NAME_TYPE               0x09
#define DEVICE_NAME                    "DM_SENSOR"

// Manufacturer ID (0x02FF - Silicon Labs' company ID)
#define MANUF_ID                       0x02FF
// 1+2+8 bytes for type, company ID and the payload
#define MANUF_LENGTH                   11
#define MANUF_TYPE                     0xFF

SL_PACK_START(1)
typedef struct
{
  uint8_t len_flags;
  uint8_t type_flags;
  uint8_t val_flags;

  uint8_t len_manuf;
  uint8_t type_manuf;
  // First two bytes must contain the manufacturer ID (little-endian order)
  uint8_t company_LO;
  uint8_t company_HI;

  // The next bytes are freely configurable
  // using 4 bytes for counter value and 4 bytes for radiation level
  uint32_t data_counter;
  uint32_t radiation_level;

  // length of the name AD element is variable,
  // adding it last to keep things simple
  uint8_t len_name;
  uint8_t type_name;

  // NAME_MAX_LENGTH must be sized
  // so that total length of data does not exceed 31 bytes
  uint8_t name[DEVICE_NAME_LENGTH];

  // These values are NOT included in the actual advertising payload,
  // just for bookkeeping
  char dummy;        // Space for null terminator
  uint8_t data_size; // Actual length of advertising data
} SL_ATTRIBUTE_PACKED advertising_packet_t;
SL_PACK_END()

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static uint8_t notifications_enabled = 0;
static uint32_t radiation_value = 0;

// Advertising data.
static advertising_packet_t advertising_data = {
  .len_flags = ADVERTISE_FLAGS_LENGTH,
  .type_flags = ADVERTISE_FLAGS_TYPE,
  .val_flags = ADVERTISE_FLAGS_DATA,

  .len_manuf = MANUF_LENGTH,
  .type_manuf = MANUF_TYPE,
  .company_LO = MANUF_ID & 0xFF,
  .company_HI = (MANUF_ID >> 8) & 0xFF,

  // length of name element is the name string length + 1 for the AD type
  .len_name = DEVICE_NAME_LENGTH + 1,
  .type_name = DEVICE_NAME_TYPE,

  // Initialize for custom data
  .data_counter = 0,
  .radiation_level = 0,

  .name = DEVICE_NAME,

  // Calculate total length of advertising data
  .data_size = 3 + (1 + MANUF_LENGTH) + (1 + DEVICE_NAME_LENGTH + 1),
};

static void update_adv_data(void);
static void on_radiation_event_callback(void);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  app_log("BLE - Dosimeter (Sparkfun Type 5) - Sensor\r\n");

  sparkfun_type5_init();
  app_log("\nSparkfun-Type 5 Init done\r\n");

  sparkfun_type5_register_radiation_callback(on_radiation_event_callback);
  app_log("Sparkfun-Type 5 Register callback done\r\n");
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  sparkfun_type5_process();
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

      app_log("\nBLE stack booted\r\nStack version: %d.%d.%d\r\n",
              evt->data.evt_system_boot.major,
              evt->data.evt_system_boot.minor,
              evt->data.evt_system_boot.patch);

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

      // Set custom advertising payload
      sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                            sl_bt_advertiser_advertising_data_packet,
                                            advertising_data.data_size,
                                            (uint8_t *)&advertising_data);
      app_assert_status(sc);

      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      app_log("Start advertising ...\n");
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("Connected to client\r\n");
      app_log("Start advertising non connectable...\n");
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_scannable_non_connectable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      notifications_enabled = 0;
      app_log("Disconnected to client\r\n");
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set custom advertising payload
      sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                            sl_bt_advertiser_advertising_data_packet,
                                            advertising_data.data_size,
                                            (uint8_t *)&advertising_data);

      // Restart advertising after client has disconnected.
      app_log("Start advertising ...\n");
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    case sl_bt_evt_gatt_server_user_read_request_id:
    {
      uint16_t sent_len;
      char ascii_string[5];

      int len = snprintf(ascii_string, 5, "%ld", radiation_value);
      sc = sl_bt_gatt_server_send_user_read_response(
        evt->data.evt_gatt_server_user_read_request.connection,
        gattdb_radiation,
        SL_STATUS_OK,
        len,
        (uint8_t *)ascii_string,
        &sent_len);

      if (sc != SL_STATUS_OK) {
        app_log("Failed to send user read request response\r\n");
      }
    }
    break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (evt->data.evt_gatt_server_characteristic_status.characteristic
          == gattdb_radiation) {
        if (evt->data.evt_gatt_server_characteristic_status.client_config_flags)
        {
          app_log("Client turn on notification\r\n");
        } else {
          app_log("Client turn off notification\r\n");
        }
        notifications_enabled
          = evt->data.evt_gatt_server_characteristic_status.client_config_flags;
      }
      break;
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

/**************************************************************************//**
 * On radiation callback function.
 *****************************************************************************/
static void on_radiation_event_callback(void)
{
  sl_status_t sc;
  char send_data[20];
  double rad;

  rad = sparkfun_type5_get_usvh();
  radiation_value = (uint32_t)rad;

  if (notifications_enabled) {
    // Send notification to client.
    int len = snprintf(send_data, 10, "%.2f", rad);
    sc = sl_bt_gatt_server_notify_all(gattdb_radiation,
                                      len,
                                      (uint8_t *)send_data);
    if (sc != SL_STATUS_OK) {
      app_log("Failed to send notification\r\n");
    }
  }
  update_adv_data();
}

static void update_adv_data(void)
{
  sl_status_t sc;
  // Update the two variable fields in the custom advertising packet
  advertising_data.data_counter++;
  advertising_data.radiation_level = radiation_value;

  // Set custom advertising payload
  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                        sl_bt_advertiser_advertising_data_packet,
                                        advertising_data.data_size,
                                        (uint8_t *)&advertising_data);
  app_assert_status(sc);
}
