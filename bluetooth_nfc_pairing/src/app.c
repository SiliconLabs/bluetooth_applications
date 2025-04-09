/***************************************************************************//**
 * @file   app.c
 * @brief  Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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
 *
 * # EXPERIMENTAL QUALITY
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 *
 ******************************************************************************/

#include <stdio.h>
#include "sl_common.h"
#include "app_assert.h"
#include "app_log.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "mikroe_nt3h2111.h"
#include "ndef_message.h"
#include "ndef_record.h"
#include "nfc_tlv.h"
#include "t2t.h"
#include "bluetooth_handover.h"
#include "sl_i2cspm_instances.h"

#define SECURE_CONNECTION           1

#define TLV_BUFFER_SIZE             200
#define PAYLOAD_BUFFER_SIZE         200
#define RECORD_TYPE_BUFFER_SIZE     50

uint8_t payload[PAYLOAD_BUFFER_SIZE];
uint8_t record_type[RECORD_TYPE_BUFFER_SIZE];
uint8_t btssp_config_addr[BLUETOOTH_LE_MAC_ADDR_FIELD_LEN];
uint8_t btssp_config_le_role[BLUETOOTH_LE_ROLE_FIELD_LEN];
uint8_t btssp_config_sm_tk_value[BLUETOOTH_SM_TK_VALUE_FIELD_LEN];
uint8_t btssp_config_sc_confirmation_value[
  BLUETOOTH_SC_CONFIRMATION_VALUE_FIELD_LEN];
uint8_t btssp_config_sc_random_value[BLUETOOTH_SC_RANDOM_VALUE_FIELD_LEN];
uint8_t btssp_config_appearance[BLUETOOTH_APPEARANCE_FIELD_LEN];
uint8_t btssp_config_local_name[] = "Silabs Bluetooth NFC Pairing";
uint8_t btssp_config_flags_value[BLUETOOTH_FLAGS_FIELD_LEN];
static uint8_t conn_handle = 0xFF;
static aes_key_128 key_random, key_confirm;

ndef_record_t record = {
  .payload = payload,
  .type = record_type
};

bt_carrier_config_t btssp_config = {
  .addr = {
    .value = btssp_config_addr
  },
  .le_role = {
    .value = btssp_config_le_role
  },
#if (SECURE_CONNECTION == 1)
  .secure_connections_confirm_value = {
    .value = btssp_config_sc_confirmation_value
  },
  .secure_connections_random_value = {
    .value = btssp_config_sc_random_value
  },
#endif
  .local_name = {
    .is_set = true,
    .type = BLUETOOTH_AD_TYPE_COMPLETE_LOCAL_NAME,

    /* Eliminate string termination character. */
    .length = sizeof(btssp_config_local_name) - 1,
    .value = btssp_config_local_name
  },
};

uint8_t ndef_message_buff[TLV_BUFFER_SIZE - 3];
uint8_t tlv_buff[TLV_BUFFER_SIZE];

ndef_record_t ndef_message[1];

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static void btssp_record_create(void)
{
  sl_status_t sc;

  bd_addr static_addr = { .addr = { 0x11, 0x11, 0x11, 0x11, 0x11, 0xC0 } };

  sc = sl_bt_advertiser_set_random_address(advertising_set_handle,
                                           1,
                                           static_addr,
                                           NULL);
  app_assert_status(sc);

#if (SECURE_CONNECTION == 1)

  /* Delete all bondings to force the pairing process */
  app_log_info("All bonding deleted\r\n");
  sc = sl_bt_sm_delete_bondings();
  app_assert_status(sc);

  sc = sl_bt_sm_configure(0x0A, sl_bt_sm_io_capability_noinputnooutput);
  app_assert_status(sc);

  sc = sl_bt_sm_set_bondable_mode(1);
  app_assert_status(sc);

  sc = sl_bt_sm_set_oob(1, &key_random, &key_confirm);
  app_assert_status(sc);

  app_log_info("Enable OOB with Secure Connection Pairing - Success.\r\n");
  app_log_info("16-byte random OOB data: ");
  for (uint8_t i = 0; i < 16; i++)
  {
    app_log("0x%02X ", key_random.data[i]);
  }
  app_log("\r\n");
  app_log_info("16-byte confirm value: ");
  for (uint8_t i = 0; i < 16; i++)
  {
    app_log("0x%02X ", key_confirm.data[i]);
  }
  app_log("\r\n");

#endif

  /** Address */

  uint8_t addr_buff[BLUETOOTH_MAC_ADDR_LEN];

  bt_le_mac_addr_t bt_le_addr = {
    .type = random_address,
    .value = addr_buff
  };

  /* Random static address, for android NFC pairing with secure connection, this
   * has to be used for some reason */
  memcpy(bt_le_addr.value, static_addr.addr, BLUETOOTH_MAC_ADDR_LEN);

  /* Encode address and its type. */
  ch_bluetooth_bd_addr_encode(btssp_config.addr.value, bt_le_addr);

  /** LE Role. */
  /* This device only acts as peripheral. */
  btssp_config.le_role.value[0] = only_peripheral;

#if (SECURE_CONNECTION == 1)

  /** LE Secure Connections Confirmation Value. */
  for (uint8_t i = 0; i < 16; i++) {
    btssp_config.secure_connections_confirm_value.value[i] =
      key_confirm.data[i];
  }

  btssp_config.secure_connections_confirm_value.is_set = true;

  /** LE Secure Connections Random Value. */
  for (uint8_t i = 0; i < 16; i++) {
    btssp_config.secure_connections_random_value.value[i] = key_random.data[i];
  }

  btssp_config.secure_connections_random_value.is_set = true;

#endif

  /* Encode bluetooth carrier configuration record. */
  ch_bluetooth_le_carrier_configuration_record_encode(&record, &btssp_config);

  ndef_message[0] = record;

  // encode ndef message
  ndef_message_encode_result_t ndef_message_encode_result;
  ndef_message_encode_result = ndef_message_encode(ndef_message_buff,
                                                   ndef_message);
  if (ndef_message_encode_result.err == ndefMessageEncodeFail) {
    app_log_info("NDEF message encode failed:(\r\n");
    while (1) {}
  }

  uint32_t write_size = ndef_message_encode_result.size;

  // encode ndef tlv
  if (tlv_encode(tlv_buff,
                 TLV_BUFFER_SIZE,
                 NFC_T2T_NDEF_MESSAGE_TLV,
                 ndef_message_encode_result.size,
                 ndef_message_buff
                 ) != tlvEncodeCompleted) {
    app_log_info("NDEF TLV encode failed:(\r\n");
    while (1) {}
  }

  write_size += 2;
  // encode terminator tlv
  if (tlv_encode(&tlv_buff[write_size],
                 (TLV_BUFFER_SIZE - write_size),
                 NFC_T2T_TERMINATOR_TLV,
                 0,
                 NULL
                 ) != tlvEncodeCompleted) {
    app_log_info("Terminator TLV encode failed:(\r\n");
    while (1) {}
  }

  write_size++;

  for (uint32_t i = 1; (i - 1) * 16 <= write_size; i++) {
    while (nt3h2111_write_block(i, &tlv_buff[(i - 1) * 16]) != SL_STATUS_OK) {}
  }
  app_log_info("Write to NT3H2111 successful\r\n");
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

  app_log_info("NT3H2x11 static handover example.\r\n");

  /* Initialize NT3H2x11 I2C communication. */
  nt3h2111_init(sl_i2cspm_mikroe);
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

      app_log_info("local BT device address: ");
      for (uint8_t i = 0; i < 5; i++)
      {
        app_log("%2.2x:", address.addr[5 - i]);
      }
      app_log("%2.2x\r\n", address.addr[0]);

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

      btssp_record_create();

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160,   // min. adv. interval (milliseconds * 1.6)
        160,   // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);   // max. num. adv. events
      app_assert_status(sc);

      sc = sl_bt_advertiser_set_channel_map(advertising_set_handle, 7);
      app_assert_status(sc);

      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      /* Start general advertising and enable connections. */
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log_info("Connected\r\n");
      conn_handle = evt->data.evt_connection_opened.connection;

      sc = sl_bt_sm_increase_security(conn_handle);
      app_assert_status(sc);
      app_log_info("Increasing security\r\n");
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    case sl_bt_evt_sm_passkey_display_id:
      app_log_info("Passkey display\r\n");
      break;

    case sl_bt_evt_sm_passkey_request_id:
      app_log_info("Passkey request\r\n");
      break;

    case sl_bt_evt_sm_confirm_passkey_id:
      app_log_info("Confirm passkey\r\n");
      break;

    case sl_bt_evt_sm_confirm_bonding_id:
      sc = sl_bt_sm_bonding_confirm(evt->data.evt_sm_confirm_bonding.connection,
                                    1);
      app_assert_status(sc);
      app_log_info("Bonding confirmed\r\n");
      break;

    case sl_bt_evt_sm_bonding_failed_id:
      /* If the attempt at bonding/pairing failed, clear the bonded flag and
       * display the reason */
      app_log_info("Bonding failed: ");
      switch (evt->data.evt_sm_bonding_failed.reason) {
        case SL_STATUS_BT_SMP_PASSKEY_ENTRY_FAILED:
          app_log_info("The user input of passkey failed\r\n");
          break;

        case SL_STATUS_BT_SMP_OOB_NOT_AVAILABLE:
          app_log_info(
            "Out of Band data is not available for authentication\r\n");
          break;

        case SL_STATUS_BT_SMP_AUTHENTICATION_REQUIREMENTS:
          app_log_info(
            "The pairing procedure cannot be performed as authentication requirements cannot be met due to IO capabilities of one or both devices\r\n");
          break;

        case SL_STATUS_BT_SMP_CONFIRM_VALUE_FAILED:
          app_log_info(
            "The confirm value does not match the calculated compare value\r\n");
          break;

        case SL_STATUS_BT_SMP_PAIRING_NOT_SUPPORTED:
          app_log_info("Pairing is not supported by the device\r\n");
          break;

        case SL_STATUS_BT_SMP_ENCRYPTION_KEY_SIZE:
          app_log_info(
            "The resultant encryption key size is insufficient for the security requirements of this device\r\n");
          break;

        case SL_STATUS_BT_SMP_COMMAND_NOT_SUPPORTED:
          app_log_info(
            "The SMP command received is not supported on this device\r\n");
          break;

        case SL_STATUS_BT_SMP_UNSPECIFIED_REASON:
          app_log_info("Pairing failed due to an unspecified reason\r\n");
          break;

        case SL_STATUS_BT_SMP_REPEATED_ATTEMPTS:
          app_log_info(
            "Pairing or authentication procedure is disallowed because too little time has elapsed since last pairing request or security request\r\n");
          break;

        case SL_STATUS_BT_SMP_INVALID_PARAMETERS:
          app_log_info("Invalid Parameters\r\n");
          break;

        case SL_STATUS_BT_SMP_DHKEY_CHECK_FAILED:
          app_log_info("The bonding does not exist\r\n");
          break;

        case SL_STATUS_BT_CTRL_PIN_OR_KEY_MISSING:
          app_log_info(
            "Pairing failed because of missing PIN, or authentication failed because of missing Key\r\n");
          break;

        case SL_STATUS_TIMEOUT:
          app_log_info("Operation timed out\r\n");
          break;

        default:
          app_log_info("Unknown error: 0x%X\r\n",
                       evt->data.evt_sm_bonding_failed.reason);
          break;
      }
      break;

    case sl_bt_evt_sm_bonded_id:
      /* The bonding/pairing was successful so set the flag to allow indications
       * to proceed */
      app_log_info("Bonding completed\r\n");
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}
