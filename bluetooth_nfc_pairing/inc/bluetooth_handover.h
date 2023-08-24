/***************************************************************************//**
 * @file   bluetooth_handover.h
 * @brief  Macros, Types and APIs for bluetooth connection handover.
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
 *
 * # EXPERIMENTAL QUALITY
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 *
 ******************************************************************************/

#ifndef __BLUETOOTH_HANDOVER_H__

/***************************************************************************//**
 * @addtogroup NFC Library
 * @brief NFC Bluetooth handover definitions, types and APIs.
 * @{
 ******************************************************************************/

#define __BLUETOOTH_HANDOVER_H__

#include <stdint.h>
#include <stdbool.h>
#include "ndef_record.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Check:
 * Supplement to Bluetooth Core Specification Part A Section 1 for All AD Types.
 *
 * https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile/
 * for all values.
 */

/// LE Bluetooth Device Address AD Type
#define BLUETOOTH_AD_TYPE_LE_DEVICE_ADDR                        (0x1B)
/// LE Role AD Type
#define BLUETOOTH_AD_TYPE_LE_ROLE                               (0x1C)
/// Security Manager TK Value AD Type
#define BLUETOOTH_AD_TYPE_SM_TK_VALUE                           (0x10)
/// Appearance AD Type
#define BLUETOOTH_AD_TYPE_APPEARANCE                            (0x19)
/// Flags AD Type
#define BLUETOOTH_AD_TYPE_FLAGS                                 (0x01)
/// Shortened Local Name AD Type
#define BLUETOOTH_AD_TYPE_SHORTENED_LOCAL_NAME                  (0x08)
/// Complete Local Name AD Type
#define BLUETOOTH_AD_TYPE_COMPLETE_LOCAL_NAME                   (0x09)
/// LE Secure Connections Confirmation Value AD Type
#define BLUETOOTH_AD_TYPE_LE_SC_CONFIRMATION_VALUE              (0x22)
/// LE Secure Connections Random Value AD Type
#define BLUETOOTH_AD_TYPE_LE_SC_RANDOM_VALUE                    (0x23)

/// Length for bluetooth AD type
#define BLUETOOTH_AD_TYPE_FIELD_LEN                             (1)
/// Length for bluetooth MAC address.
#define BLUETOOTH_MAC_ADDR_LEN                                  (6)
/// Length for BTSSP Bluetooth LE mac address field
#define BLUETOOTH_LE_MAC_ADDR_FIELD_LEN                         (7)
/// Length for BTSSP Bluetooth LE role field
#define BLUETOOTH_LE_ROLE_FIELD_LEN                             (1)
/// Length for BTSSP security manager TK value field
#define BLUETOOTH_SM_TK_VALUE_FIELD_LEN                         (16)
/// Length for BTSSP Secure Connection Confirmation value field
#define BLUETOOTH_SC_CONFIRMATION_VALUE_FIELD_LEN               (16)
/// Length for BTSSP Secure Connection Random value field
#define BLUETOOTH_SC_RANDOM_VALUE_FIELD_LEN                     (16)
/// Length for BTSSP appearance field
#define BLUETOOTH_APPEARANCE_FIELD_LEN                          (2)
/// Length for BTSSP flags field
#define BLUETOOTH_FLAGS_FIELD_LEN                               (1)

/// Bluetooth LE Role types.
typedef enum {
  only_peripheral             = 0x00,
  only_central                = 0x01,
  both_peripheral_preferred   = 0x02,
  both_central_preferred      = 0x03
} bt_le_role_t;

/// Bluetooth LE address types.
typedef enum {
  public_address              = 0x00,
  random_address              = 0x01
} bt_le_addr_t;

/// Bluetooth LE Address type.

typedef struct {
  bt_le_addr_t    type;
  uint8_t *value;
} bt_le_mac_addr_t;

/// Bluetooth configuration record required AD types.
typedef struct {
  uint8_t *value;
} bt_le_required_ad_t;

/// Bluetooth configuration record optional AD types.
typedef struct {
  bool            is_set;
  uint8_t         type;
  uint8_t         length;
  uint8_t *value;
} bt_le_optional_ad_t;

/// Bluetooth configuration record type.

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
typedef struct {
  bt_le_required_ad_t addr;
  bt_le_required_ad_t le_role;
  bt_le_optional_ad_t security_manager_tk_value;
  bt_le_optional_ad_t secure_connections_confirm_value;
  bt_le_optional_ad_t secure_connections_random_value;
  bt_le_optional_ad_t appearance;
  bt_le_optional_ad_t local_name;
  bt_le_optional_ad_t flags;
} bt_carrier_config_t;

void ch_bluetooth_bd_addr_encode (uint8_t *buff, bt_le_mac_addr_t addr);

void ch_bluetooth_le_carrier_configuration_record_encode (ndef_record_t *record,
                                                          bt_carrier_config_t *config);

void ch_bluetooth_le_carrier_configuration_record_decode (ndef_record_t *record,
                                                          bt_carrier_config_t *config);

#ifdef __cplusplus
}
#endif

/** @} (end addtogroup NFC Library) */
#endif
