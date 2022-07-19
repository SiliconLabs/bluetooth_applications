/***************************************************************************//**
 * @file indoor_positioning.h
 * @brief Application Logic Source File
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
#ifndef INDOOR_POSITIONING_H_
#define INDOOR_POSITIONING_H_

#include "em_common.h"
#include "stdio.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "nvm3.h"
#include "nvm3_hal_flash.h"
#include "sl_simple_button_instances.h"

#define CONFIG_MODE_TIMEOUT_MS		    (120000) // 2 minutes
#define COMPANY_ID			            (0x0047)
#define ROOM_NAME_LENGTH                (10)
#define DEVICENAME_LENGTH 		        (9)

/***************************************************************************//**
 * @brief
 * Custom(user) advertisement struct typedef for Gateway data
 ******************************************************************************/
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

  // The next bytes are freely configurable - using one byte for counter value and one byte for last button press
  uint32_t network_Uid;
  uint16_t room_id;
  char     room_name[ROOM_NAME_LENGTH];

  // NAME_MAX_LENGTH must be sized so that total length of data does not exceed 31 bytes
  char name[DEVICENAME_LENGTH];

  // length of the name AD element is variable, adding it last to keep things simple
  uint8_t len_name;
  uint8_t type_name;

  // These values are NOT included in the actual advertising payload, just for bookkeeping
  char dummy;        // Space for null terminator
  uint8_t data_size; // Actual length of advertising data
}__attribute__((__packed__))  custom_advert_t;

/***************************************************************************//**
 * @brief
 * Indoor Positioning - Gateway related configuration struct typedef
 ******************************************************************************/
typedef struct
{
  char room_name[ROOM_NAME_LENGTH];
  uint32_t network_Uid;
  uint16_t room_id;
  char device_name[DEVICENAME_LENGTH];
}IPGW_config_data_t;

/***************************************************************************//**
 * @brief
 * enumeration for keys used for the Non-volatile memory storage
 ******************************************************************************/
typedef enum IPGW_config_keys_enum
{
  IPGW_config_key_network_UID = 1,
  IPGW_config_key_room_name,
  IPGW_config_key_room_id,
  IPGW_config_num_of_keys
}IPGW_config_keys_enum_t;

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                          Public Function Declarations
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * @brief
 * Creates custom advertising package
 *
 * @param[in] pData - Pointer to the struct holding the above mentioned asset data
 * @param[in] flags - advertisement flag
 * @param[in] companyID - Unique company ID
 * @param[in] name - Unique device name
 *
 * @note Creates custom advertising package with the following asset data:
 * 	 Network Unique ID
 * 	 Room ID
 * 	 Room Name
 *   Device name
 ******************************************************************************/
void create_custom_advert_package(custom_advert_t *pData, uint8_t flags, uint16_t companyID, char *name);

/***************************************************************************//**
 * @brief
 * Initialize Indoor Positioning service
 *
 * @param[in] None
*******************************************************************************/
void IPGW_init(void);

/***************************************************************************//**
 * @brief
 * Called periodically by the application
 * Handles periodic Indoor Positioning related tasks
*******************************************************************************/
void IPGW_step(void);

/***************************************************************************//**
 * @brief
 * Enters Normal mode
 * Set up and start scanner
 * Set up and start advertisements
 *
 * @param[in] None
*******************************************************************************/
void enter_normal_mode(void);

/***************************************************************************//**
 * @brief
 * Enters configuration mode
 * prints out current configuration parameters
 * set up and start advertisements
 * starts timeout timer
 *
 * @param[in] None
*******************************************************************************/
void enter_config_mode(void);

/***************************************************************************//**
 * @brief
 * Updates GATT database with values stored in NVM
 *
 * @param[in] None
*******************************************************************************/
void update_gatt_entries(void);

/***************************************************************************//**
 * @brief
 * Loads configuration from Non-volatile memory
 *
 * @param[in] None
*******************************************************************************/
void update_local_config(void);

/***************************************************************************//**
 * @brief
 * Indoor Positioning related BT event handler
 *
 * @param[in] - evt - bluetooth event data structure
*******************************************************************************/
void IPGW_event_handler(sl_bt_msg_t *evt);

/***************************************************************************//**
 * @brief
 * Callback for configuration mode timeout timer
 *
 * @param[in] None
 ******************************************************************************/
void config_mode_timeout_cb();


#endif /* INDOOR_POSITIONING_H_ */
