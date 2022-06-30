/*
 * indoor_positioning.h
 *
 *  Created on: 2022. máj. 9.
 *      Author: maorkeny
 */

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
 *    Custom(user) advertisement struct typedef for Gateway data
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
 *    Indoor Positioning - Gateway related configuration struct typedef
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
 *    enumeration for keys used for the Non-volatile memory storage
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
 * Initialize Indoor Positioning service
*******************************************************************************/
void IPGW_init();

/***************************************************************************//**
 * Called periodically by the application
 * Handles periodic Indoor Positioning related tasks
*******************************************************************************/
void IPGW_step();

/***************************************************************************//**
 * Enters Normal mode
 * Set up and start scanner
 * Set up and start advertisements
*******************************************************************************/
void enter_normal_mode();

/***************************************************************************//**
 * Enters configuration mode
 * prints out current configuration parameters
 * set up and start advertisements
 * starts timeout timer
*******************************************************************************/
void enter_config_mode();

/***************************************************************************//**
 * Updates GATT database with values stored in NVM
*******************************************************************************/
void update_gatt_entries();

/***************************************************************************//**
 *Loads configuration from Non-volatile memory
*******************************************************************************/
void update_local_config();

/***************************************************************************//**
 * Callback for configuration mode timeout timer
 ******************************************************************************/
void config_mode_timeout_cb();

/***************************************************************************//**
 * Indoor Positioning related BT event handler
 * @param[in] - evt - bluetooth event data structure
*******************************************************************************/
void IPGW_event_handler(sl_bt_msg_t *evt);


#endif /* INDOOR_POSITIONING_H_ */
