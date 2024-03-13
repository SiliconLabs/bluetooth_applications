/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "sl_bluetooth.h"
#include "gatt_db.h"

#include "app_log.h"
#include "app_assert.h"

#include "app.h"

#define ANC_NOTIF_SOURCE_LENGTH        8

#define INIT                           0x00
#define SERVICE_HANDLE_FOUND           0x01
#define CHAR_HANDLE_FOUND              0x02
#define DONE                           0x03
#define ERROR                          0xFF

/* category ID field */
#define ANC_CAT_OTHER                  0
#define ANC_CAT_INCOMINGCALL           1
#define ANC_CAT_MISSEDCALL             2
#define ANC_CAT_VOICEMAIL              3
#define ANC_CAT_SOCIAL                 4
#define ANC_CAT_SCHEDULE               5
#define ANC_CAT_EMAIL                  6
#define ANC_CAT_NEWS                   7
#define ANC_CAT_HEALTHANDFITNESS       8
#define ANC_CAT_BUSINESSANDFINANCE     9
#define ANC_CAT_LOCATION               10
#define ANC_CAT_ENTERTAINMENT          11

/* Text definitions for logging */
#define ANC_OTHER_TEXT                 "You have %d new notification(s) \r\n "
#define ANC_INCOMINGCALL_TEXT          "You have %d new incoming call(s) \r\n "
#define ANC_MISSEDCALL_TEXT            "You have %d new missed call(s)\r\n "
#define ANC_VOICEMAIL_TEXT             "You have %d new voice mail(s)\r\n "
#define ANC_SOCIAL_TEXT                "You have %d new text message(s)\r\n "
#define ANC_SCHEDULE_TEXT              "You have %d new appointment(s)\r\n "
#define ANC_EMAIL_TEXT                 "You have %d new email(s)\r\n "
#define ANC_NEWS_TEXT                  "You have %d news waiting\r\n "
#define ANC_HEALTHANDFITNESS_TEXT \
  "You have %d new health and fitness notification(s)\r\n "
#define ANC_BUSINESSANDFINANCE_TEXT \
  "You have %d new business and finance notification(s)\r\n "
#define ANC_LOCATION_TEXT \
  "You have %d new location notification(s)\r\n "
#define ANC_ENTERTAINMENT_TEXT \
  "You have %d new entertainment notification(s)\r\n "
#define ANC_TEXT_EXAMPLE \
  "You have 99999 new business and finance notification(s)\r\n "
#define ANC_TEXT_SIZE                  (sizeof(ANC_TEXT_EXAMPLE))

/* Fields of Apple Notification */
struct ancNotifChr_t {
  uint8_t ancEventID;
  uint8_t ancEventFlags;
  uint8_t ancCategoryId;
  uint8_t ancCategoryCount;
  uint32_t ancNotificationUID;
};

union ancNotifChrUnion_t {
  uint8_t ancNotifChrPayload[ANC_NOTIF_SOURCE_LENGTH];
  struct ancNotifChr_t ancNotifChr;
} ancNotifChrUnion;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
// Connection Handle
static uint8_t connection_handle = 0xff;
// Service and characteristic handles
static uint32_t service_handle = 0xffffffff;
static uint16_t characteristic_handle = 0xffff;

/* Discovery state */
static uint8_t   discovery_state = INIT;

/* Apple Notification Center Service 128-bit UUID
 * 7905F431-B5CE-4E99-A40F-4B1E122D00D0
 */
static const uint8_t service_uuid[] = { 0xD0,
                                        0x00,
                                        0x2D,
                                        0x12,
                                        0x1E,
                                        0x4B,
                                        0x0F,
                                        0xA4,
                                        0x99,
                                        0x4E,
                                        0xCE,
                                        0xB5,
                                        0x31,
                                        0xF4,
                                        0x05,
                                        0x79 };

/* Notification Source Characteristic 128-bit UUID
 * 9FBF120D-6301-42D9-8C58-25E699A21DBD
 */
static const uint8_t characteristic_uuid[] = { 0xBD,
                                               0x1D,
                                               0xA2,
                                               0x99,
                                               0xE6,
                                               0x25,
                                               0x58,
                                               0x8C,
                                               0xD9,
                                               0x42,
                                               0x01,
                                               0x63,
                                               0x0D,
                                               0x12,
                                               0xBF,
                                               0x9F };

static void ancCharValueReceivedCallback(uint8_t ancCategoryId,
                                         uint8_t ancCategoryCount);

/***************************************************************************//**
 * Application Init.
 ******************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

/***************************************************************************//**
 * Application Process Action.
 ******************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/***************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 ******************************************************************************/
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
      /* Print stack version */
      app_log("Bluetooth stack booted: v%d.%d.%d-b%d\r\n",
              evt->data.evt_system_boot.major,
              evt->data.evt_system_boot.minor,
              evt->data.evt_system_boot.patch,
              evt->data.evt_system_boot.build);

      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      app_assert_status(sc);

      /* Print Bluetooth address */
      app_log("Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
              address_type ? "static random" : "public device",
              address.addr[5],
              address.addr[4],
              address.addr[3],
              address.addr[2],
              address.addr[1],
              address.addr[0]);

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

      sl_bt_sm_configure(0x00, sl_bt_sm_io_capability_noinputnooutput);
      app_assert_status(sc);

      sc = sl_bt_sm_set_bondable_mode(1); // Bondings allowed
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

      // Start general advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);

      app_log("boot event - starting advertising\r\n");
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("Connection opened\r\n");
      // Save connection handle for future reference
      connection_handle = evt->data.evt_connection_opened.connection;

      /* Initiate pairing */
      if (evt->data.evt_connection_opened.bonding == 0xff) {
        sc = sl_bt_sm_increase_security(connection_handle);
        app_assert_status(sc);
      }
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("connection closed, reason: 0x%2.2x\r\n",
              evt->data.evt_connection_closed.reason);
      connection_handle = 0xff;
      characteristic_handle = 0xffff;
      service_handle = 0xffffffff;
      discovery_state = INIT;

      app_log("restarting advertising\r\n");
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);
      app_log("Started advertising\r\n");
      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    // -------------------------------
    //  Indicates that an ATT_MTU exchange procedure is completed
    case sl_bt_evt_gatt_mtu_exchanged_id:
      // Start discovery after the connection parameters negotiated
      if (discovery_state == INIT) {
        /* Search for the service handle first */
        app_log("Searching for ANCS service..\r\n");
        sc = sl_bt_gatt_discover_primary_services_by_uuid(connection_handle,
                                                          sizeof(service_uuid),
                                                          service_uuid);
        app_assert_status(sc);
      }
      break;

    // -------------------------------
    // This event is triggered after the bonding procedure
    // has been successfully completed.
    case sl_bt_evt_sm_bonded_id:
      app_log("Gecko just bonded\r\n");
      app_log("ANCS needs to be restarted \r\n");
      app_log("ANCS started...\r\n");
      sc = sl_bt_gatt_set_characteristic_notification(connection_handle,
                                                      characteristic_handle,
                                                      sl_bt_gatt_notification);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event is generated for various procedure completions, e.g. when a
    // write procedure is completed, or service discovery is completed
    case sl_bt_evt_gatt_procedure_completed_id:
      if (discovery_state == INIT) {
        app_log("ANCS service not found\r\n");
        discovery_state = ERROR;
        break;
      }

      if (discovery_state == SERVICE_HANDLE_FOUND) {
        app_log("Searching for ANCS characteristic..\r\n");
        sc = sl_bt_gatt_discover_characteristics_by_uuid(
          connection_handle,
          service_handle,
          sizeof(characteristic_uuid),
          characteristic_uuid);
        app_assert_status(sc);
        break;
      }

      if (discovery_state == CHAR_HANDLE_FOUND) {
        discovery_state = DONE;
        app_log("ANCS started...\r\n");
        sc = sl_bt_gatt_set_characteristic_notification(connection_handle,
                                                        characteristic_handle,
                                                        sl_bt_gatt_notification);
        app_assert_status(sc);
        break;
      }

      if (discovery_state == DONE) {
        /* Do nothing */
        break;
      }

      // default
      app_log("Discovery failed\r\n");
      discovery_state = ERROR;
      break;

    // -------------------------------
    // This event is generated when a new service is discovered
    case sl_bt_evt_gatt_service_id:
      app_log("ANCS service found\r\n");
      // Save service handle for future reference
      service_handle = evt->data.evt_gatt_service.service;
      discovery_state = SERVICE_HANDLE_FOUND;
      break;

    // -------------------------------
    // This event is generated when a new characteristic is discovered
    case sl_bt_evt_gatt_characteristic_id:
      app_log("ANCS characteristic found\r\n");
      // Save characteristic handle for future reference
      characteristic_handle = evt->data.evt_gatt_characteristic.characteristic;
      discovery_state = CHAR_HANDLE_FOUND;
      break;

    // -------------------------------
    // This event is generated when a characteristic value was received
    case sl_bt_evt_gatt_characteristic_value_id:
      app_log("\r\nANCS notification received \r\n");
      if (evt->data.evt_gatt_characteristic_value.att_opcode
          == sl_bt_gatt_handle_value_notification) {
        /* Check if Apple notification length is correct. */
        if (evt->data.evt_gatt_characteristic_value.value.len
            == ANC_NOTIF_SOURCE_LENGTH) {
          /* Determine notification type based on the Notification UID */
          memcpy(ancNotifChrUnion.ancNotifChrPayload,
                 evt->data.evt_gatt_characteristic_value.value.data,
                 ANC_NOTIF_SOURCE_LENGTH);

          /* Call callback */
          ancCharValueReceivedCallback(
            ancNotifChrUnion.ancNotifChr.ancCategoryId,
            ancNotifChrUnion.ancNotifChr.ancCategoryCount);
        }
      }
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
 *  \brief  prints the ANCS notification
 ******************************************************************************/
static void ancCharValueReceivedCallback(uint8_t ancCategoryId,
                                         uint8_t ancCategoryCount)
{
  char tempString[ANC_TEXT_SIZE];

  switch (ancCategoryId) {
    default:
    case ANC_CAT_OTHER:
      snprintf(tempString, ANC_TEXT_SIZE, ANC_OTHER_TEXT,
               ancCategoryCount);
      break;
    case ANC_CAT_INCOMINGCALL:
      snprintf(tempString, ANC_TEXT_SIZE, ANC_INCOMINGCALL_TEXT,
               ancCategoryCount);
      break;
    case ANC_CAT_MISSEDCALL:
      snprintf(tempString, ANC_TEXT_SIZE, ANC_MISSEDCALL_TEXT,
               ancCategoryCount);
      break;
    case ANC_CAT_VOICEMAIL:
      snprintf(tempString, ANC_TEXT_SIZE, ANC_VOICEMAIL_TEXT,
               ancCategoryCount);
      break;
    case ANC_CAT_SOCIAL:
      snprintf(tempString, ANC_TEXT_SIZE, ANC_SOCIAL_TEXT,
               ancCategoryCount);
      break;
    case ANC_CAT_SCHEDULE:
      snprintf(tempString, ANC_TEXT_SIZE, ANC_SCHEDULE_TEXT,
               ancCategoryCount);
      break;
    case ANC_CAT_EMAIL:
      snprintf(tempString, ANC_TEXT_SIZE, ANC_EMAIL_TEXT,
               ancCategoryCount);
      break;
    case ANC_CAT_NEWS:
      snprintf(tempString, ANC_TEXT_SIZE, ANC_NEWS_TEXT,
               ancCategoryCount);
      break;
    case ANC_CAT_HEALTHANDFITNESS:
      snprintf(tempString, ANC_TEXT_SIZE, ANC_HEALTHANDFITNESS_TEXT,
               ancCategoryCount);
      break;
    case ANC_CAT_BUSINESSANDFINANCE:
      snprintf(tempString, ANC_TEXT_SIZE, ANC_BUSINESSANDFINANCE_TEXT,
               ancCategoryCount);
      break;
    case ANC_CAT_LOCATION:
      snprintf(tempString, ANC_TEXT_SIZE, ANC_LOCATION_TEXT,
               ancCategoryCount);
      break;
    case ANC_CAT_ENTERTAINMENT:
      snprintf(tempString, ANC_TEXT_SIZE, ANC_ENTERTAINMENT_TEXT,
               ancCategoryCount);
      break;
  }

  /* Update string */
  app_log(tempString);
}
