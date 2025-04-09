/***************************************************************************//**
 * @file   bluetooth_handover.c
 * @brief  Implementation for bluetooth connection handover.
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

#include <stdint.h>
#include <string.h>
#include "ndef_message.h"
#include "ndef_record.h"
#include "bluetooth_handover.h"

#define BLUETOOTH_LE_OOB_RECORD_TYPE_LEN        (32)
#define BLUETOOTH_EP_OOB_RECORD_TYPE_LEN        (32)

/// Bluetooth LE out-of-band record type.
const uint8_t bluetooth_le_oob_record_type[] =
  "application/vnd.bluetooth.le.oob";
/// Bluetooth BR/EDR out-of-band record type.
const uint8_t bluetooth_ep_oob_record_type[] =
  "application/vnd.bluetooth.ep.oob";

/**************************************************************************//**
 * @brief
 *  Set the address array as address plus 1 byte of address type, as
 *  specified in Bluetooth CSS v9 1.16.
 *
 * @param[out] buff
 *  Buffer to store the result
 *
 * @param[in] addr
 *  Address struct that contains the address and address type
 *****************************************************************************/
void ch_bluetooth_bd_addr_encode(uint8_t *buff, bt_le_mac_addr_t addr)
{
  /* Copy address. */
  memcpy(buff, addr.value, BLUETOOTH_MAC_ADDR_LEN);

  /* Set type. */
  buff[6] = addr.type;
}

/**************************************************************************//**
 * @brief
 *  Create a bluetooth carrier configuration NDEF record from a btssp
 *  configuration struct.
 *
 * @param[in] record
 *  NDEF record to store the bluetooth carrier configuration
 *
 * @param[in] config
 *  BTSSP configurations
 *****************************************************************************/
void ch_bluetooth_le_carrier_configuration_record_encode(ndef_record_t *record,
                                                         bt_carrier_config_t *config)
{
  /*
   * ---------------------------------------------------------
   *         Bluetooth Carrier Configuration Record           |
   *       mime-type "application/vnd.bluetooth.le.oob"       |
   *                        Payload ID                        |
   * ---------------------------------------------------------|
   * - LE Bluetooth Device Address                (required)  |
   * - LE Role                                    (required)  |
   * - Security Manager TK Value                  (optional)  |
   * - LE Secure Connections Confirmation Value   (optional)  |
   * - LE Secure Connections Random Value         (optional)  |
   * - Appearance                                 (optional)  |
   * - Local Name                                 (optional)  |
   * - Flags                                      (optional)  |
   * ---------------------------------------------------------
   */

  /* Set record header. */
  record->header.mb = 1;
  record->header.me = 1;
  record->header.cf = 0;
  record->header.sr = 1;
  record->header.il = 0;
  record->header.tnf = ndefTnfMIMEMedia;

  /* Set record type length. */
  record->type_length = BLUETOOTH_LE_OOB_RECORD_TYPE_LEN;

  /* Set record type. */
  memcpy(record->type, bluetooth_le_oob_record_type, record->type_length);

  /* Set payload. */
  uint16_t index = 0;

  /* Set LE Bluetooth Device Address field length and increment index by 1. */
  record->payload[index++] = BLUETOOTH_AD_TYPE_FIELD_LEN
                             + BLUETOOTH_LE_MAC_ADDR_FIELD_LEN;

  /* Set LE Bluetooth Devicess Address AD type and increment index by 1.*/
  record->payload[index++] = BLUETOOTH_AD_TYPE_LE_DEVICE_ADDR;

  /* Set MAC address. */
  memcpy(&record->payload[index],
         config->addr.value,
         BLUETOOTH_LE_MAC_ADDR_FIELD_LEN);

  /* Increment index by length of value.  */
  index += BLUETOOTH_LE_MAC_ADDR_FIELD_LEN;

  /* Set LE Role length and increment index by 1. */
  record->payload[index++] = BLUETOOTH_AD_TYPE_FIELD_LEN
                             + BLUETOOTH_LE_ROLE_FIELD_LEN;

  /* Set LE Role AD type and increment index by 1. */
  record->payload[index++] = BLUETOOTH_AD_TYPE_LE_ROLE;

  /* Set LE Role and increment index by 1. */
  record->payload[index++] = config->le_role.value[0];

  /* Check if Security Manager TK value is included. If so, set its values. */
  if (config->security_manager_tk_value.is_set) {
    /* Set Security Manager TK value field length and increment index by 1. */
    record->payload[index++] = BLUETOOTH_AD_TYPE_FIELD_LEN
                               + BLUETOOTH_SM_TK_VALUE_FIELD_LEN;

    /* Set Security Manager TK value AD type. */
    record->payload[index++] = BLUETOOTH_AD_TYPE_SM_TK_VALUE;

    /* Set Security Manager TK value. */
    memcpy(&record->payload[index],
           config->security_manager_tk_value.value,
           BLUETOOTH_SM_TK_VALUE_FIELD_LEN);

    /* Increment index by BLUETOOTH_SM_TK_VALUE_field_LEN. */
    index += BLUETOOTH_SM_TK_VALUE_FIELD_LEN;
  }

  /* Check if Secure Connections Confirmation value is included.
   * If so, set its values. */
  if (config->secure_connections_confirm_value.is_set) {
    /* Set Secure Connections Confirmation value field length and increment
     *   index by 1. */
    record->payload[index++] = BLUETOOTH_AD_TYPE_FIELD_LEN
                               + BLUETOOTH_SC_CONFIRMATION_VALUE_FIELD_LEN;

    /* Set Secure Connections Confirmation value AD type. */
    record->payload[index++] = BLUETOOTH_AD_TYPE_LE_SC_CONFIRMATION_VALUE;

    /* Set Secure Connections Confirmation value. */
    memcpy(&record->payload[index],
           config->secure_connections_confirm_value.value,
           BLUETOOTH_SC_CONFIRMATION_VALUE_FIELD_LEN);

    /* Increment index by BLUETOOTH_SC_CONFIRMATION_VALUE_FIELD_LEN.*/
    index += BLUETOOTH_SC_CONFIRMATION_VALUE_FIELD_LEN;
  }

  /* Check if Secure Connections Random value is included.
   * If so, set its values. */
  if (config->secure_connections_random_value.is_set) {
    /* Set Secure Connections Random value field length
     *  and increment index by 1. */
    record->payload[index++] = BLUETOOTH_AD_TYPE_FIELD_LEN
                               + BLUETOOTH_SC_RANDOM_VALUE_FIELD_LEN;

    /* Set Secure Connections Random value AD type. */
    record->payload[index++] = BLUETOOTH_AD_TYPE_LE_SC_RANDOM_VALUE;

    /* Set Secure Connections Random value. */
    memcpy(&record->payload[index],
           config->secure_connections_random_value.value,
           BLUETOOTH_SC_RANDOM_VALUE_FIELD_LEN);

    /* Increment index by BLUETOOTH_SC_RANDOM_VALUE_FIELD_LEN.*/
    index += BLUETOOTH_SC_RANDOM_VALUE_FIELD_LEN;
  }

  /* Check if Appearance is included. If so, set its values. */
  if (config->appearance.is_set) {
    /* Set Appearance field length and increment index by 1. */
    record->payload[index++] = BLUETOOTH_AD_TYPE_FIELD_LEN
                               + BLUETOOTH_APPEARANCE_FIELD_LEN;

    /* Set Appearance AD type. */
    record->payload[index++] = BLUETOOTH_AD_TYPE_APPEARANCE;

    /* Set Appearance. */
    memcpy(&record->payload[index],
           config->appearance.value,
           BLUETOOTH_APPEARANCE_FIELD_LEN);

    /* Increment index by BLUETOOTH_APPEARANCE_FIELD_LEN.*/
    index += BLUETOOTH_APPEARANCE_FIELD_LEN;
  }

  /* Check if Local Name is included. If so, set its values. */
  if (config->local_name.is_set) {
    /* Set Local Name field length and increment index by 1. */
    record->payload[index++] = BLUETOOTH_AD_TYPE_FIELD_LEN
                               + config->local_name.length;

    /* Set Local Name AD type. */
    record->payload[index++] = config->local_name.type;

    /* Set Local Name. */
    memcpy(&record->payload[index],
           config->local_name.value,
           config->local_name.length);

    /* Increment index by length of local name.*/
    index += config->local_name.length;
  }

  /* Check if Flags are included, If so, set their values. */
  if (config->flags.is_set) {
    /* Set Flags field length and increment index by 1. */
    record->payload[index++] = BLUETOOTH_AD_TYPE_FIELD_LEN
                               + BLUETOOTH_FLAGS_FIELD_LEN;

    /* Set Flags AD type. */
    record->payload[index++] = BLUETOOTH_AD_TYPE_FLAGS;

    /* Set Flags. */
    record->payload[index++] = config->flags.value[0];
  }

  record->payload_length = index;
}

void ch_bluetooth_le_carrier_configuration_record_decode(ndef_record_t *record,
                                                         bt_carrier_config_t *config)
{
  (void) record;
  (void) config;
  // Todo
}
