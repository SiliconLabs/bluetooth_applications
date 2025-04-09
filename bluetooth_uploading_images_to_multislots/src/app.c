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
#include "gatt_db.h"
#include "sl_bluetooth.h"
#include "app_assert.h"
#include "btl_interface.h"
#include "btl_interface_storage.h"
#include "app_log.h"

typedef enum {
  OTA_NoError    = 0x00,
  OTA_SlotError  = 0x81,
  OTA_SizeError  = 0x82,
  OTA_WriteError = 0x83,
  OTA_EraseError = 0x84
} OTAErrorCodes_t;

// OTA DFU block size
#define MULTISLOTS_OTA_DFU_START        0x00
#define MULTISLOTS_OTA_DFU_COMMIT       0x03
#define MAX_SLOT_ID                     0xFFFFFFFF

static uint32_t ota_dfu_offset;
static uint32_t ota_dfu_slot_id = MAX_SLOT_ID;
static uint8_t connection = 0;
static sl_status_t sc;
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

OTAErrorCodes_t multislot_ota_dfu_transaction_begin();
OTAErrorCodes_t multislot_ota_dfu_transaction_finish();
OTAErrorCodes_t multislot_ota_dfu_data_received(uint8_t len, uint8_t *data);

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
      // bootloader initialization.
      sc = bootloader_init();
      app_assert_status(sc);
      app_log("bootloader init.\n");
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(advertising_set_handle,
                                       160, // min interval (ms * 1.6)
                                       160, // max interval (ms * 1.6)
                                       0,   // adv. duration
                                       0);  // max. num. adv. events
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      app_log("hola.\n");
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      connection = evt->data.evt_connection_opened.connection;
      app_log("connection Opened.\n");
      // set timeout to 10 sec, because erasing the slot may take several
      //   seconds!!!
      sl_bt_connection_set_parameters(connection, 6, 6, 0, 1000, 0, 0xffff);
      break;

    case  sl_bt_evt_gatt_server_user_write_request_id:
    {
      uint32_t  connection =
        evt->data.evt_gatt_server_user_write_request.connection;
      uint32_t  characteristic =
        evt->data.evt_gatt_server_user_write_request.characteristic;
      uint8_t   *charValueData =
        evt->data.evt_gatt_server_user_write_request.value.data;

      uint8_t AttOpcode =
        evt->data.evt_gatt_server_user_write_request.att_opcode;
      uint8_t charLen = evt->data.evt_gatt_server_user_write_request.value.len;

      /*****  OTA Control  *****/
      if (characteristic == gattdb_ota_control) {
        OTAErrorCodes_t responseVal = OTA_NoError;
        uint8_t ota_dfu_command = charValueData[0];

        switch (ota_dfu_command) {
          case MULTISLOTS_OTA_DFU_START:
            app_log("start uploading.\n");
            responseVal = multislot_ota_dfu_transaction_begin();
            break;

          case MULTISLOTS_OTA_DFU_COMMIT:
            app_log("finish.\n");
            responseVal = multislot_ota_dfu_transaction_finish();
            break;

          default:
            // responseVal = OTA_WrongValueError;
            sc = sl_bt_gatt_server_send_user_write_response(connection,
                                                            characteristic,
                                                            OTA_NoError);

            app_assert_status(sc);

            sc = sl_bt_connection_close(connection);

            app_assert_status(sc);

            break;
        }

        sc = sl_bt_gatt_server_send_user_write_response(connection,
                                                        characteristic,
                                                        responseVal);

        app_assert_status(sc);
      } else

      /*****  OTA Data  *****/
      if (characteristic == gattdb_ota_data) {
        OTAErrorCodes_t responseVal = OTA_NoError;

        app_log("flashing..\n");
        responseVal = multislot_ota_dfu_data_received(charLen, charValueData);
        // Send back response in case of data was sent as a write request
        if (sl_bt_gatt_write_request == AttOpcode) {
          sc = sl_bt_gatt_server_send_user_write_response(connection,
                                                          characteristic,
                                                          responseVal);

          app_assert_status(sc);
        }
      }
      break;
    }

    // Value of attribute changed from the local database by remote GATT client
    case sl_bt_evt_gatt_server_attribute_value_id:
    {
      uint8_t *charValueData =
        evt->data.evt_gatt_server_attribute_value.value.data;
      uint8_t len = evt->data.evt_gatt_server_user_write_request.value.len;

      /*****  OTA Data  *****/
      if (evt->data.evt_gatt_server_attribute_value.attribute
          == gattdb_ota_data) {
        sc = multislot_ota_dfu_data_received(len, charValueData);
        app_assert_status(sc);
      } else

      /***** Boot slot *****/
      if (evt->data.evt_gatt_server_attribute_value.attribute
          == gattdb_boot_slot) {
        app_log("rebooting...\n");
        if (BOOTLOADER_OK == bootloader_setImageToBootload(charValueData[0])) {
          bootloader_rebootAndInstall();
        }
      }
    }

    break;
    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("connection closed.\n");
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

OTAErrorCodes_t multislot_ota_dfu_transaction_begin()
{
  int32_t rv;

  ota_dfu_slot_id = 0x00000000;

  sc = sl_bt_gatt_server_read_attribute_value(gattdb_upload_slot,
                                              0,
                                              1,
                                              NULL,
                                              (uint8_t *)&ota_dfu_slot_id);

  app_assert_status(sc);

  ota_dfu_offset = 0;

  // erasing slot. This may take several seconds!!!
  rv = bootloader_eraseStorageSlot(ota_dfu_slot_id);

  return (BOOTLOADER_OK == rv) ? OTA_NoError : OTA_EraseError;
}

OTAErrorCodes_t multislot_ota_dfu_transaction_finish()
{
  int8_t ret = OTA_NoError;

  ota_dfu_slot_id = MAX_SLOT_ID;

  return ret;
}

OTAErrorCodes_t multislot_ota_dfu_data_received(uint8_t len, uint8_t *data)
{
  int32_t rv;
  BootloaderStorageSlot_t storageSlot;

  rv = bootloader_getStorageSlotInfo(ota_dfu_slot_id, &storageSlot);
  if (BOOTLOADER_OK != rv) {
    return OTA_SlotError;
  }

  if ((ota_dfu_offset + len) > storageSlot.length) {
    return OTA_SizeError;
  }

  rv = bootloader_writeStorage(ota_dfu_slot_id, ota_dfu_offset, data, len);

  if (BOOTLOADER_OK == rv) {
    ota_dfu_offset += len;
    return OTA_NoError;
  }
  return OTA_WriteError;
}
