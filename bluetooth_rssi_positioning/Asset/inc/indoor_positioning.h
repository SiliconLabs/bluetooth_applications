/*
 * indoor_positioning.h
 *
 *  Created on: 2022. máj. 9.
 *      Author: maorkeny
 */

#ifndef INDOOR_POSITIONING_H_
#define INDOOR_POSITIONING_H_

/***************************************************************************//**
 * @addtogroup Indoor Positioning - Asset
 * @{
 *
 * @brief
 * Indoor Positioning is made up by 2 parts - Gateways and Assets.
 * This project contains the Asset implementation with the following features:
 *   Configuration mode:
 *     Sets up device for configuration via bluetooth client using GATT entries
 *     Device is connectable and scannable
 *   Normal Mode:
 *     Device scans for gateways and collects RSSI measurements from all nodes
 *     Calculates own position based on the gateways' positions and their RSSI
 *     Sets up advertisment containing calculated position *
 *
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#include "em_common.h"
#include "stdio.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "nvm3.h"
#include "nvm3_hal_flash.h"
#include "sl_simple_button_instances.h"
#include "glib.h"
#include "math.h"

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------
#define CONFIG_MODE_TIMEOUT_MS				120000 	// 2 minutes
#define GATEWAY_FINDER_TIMEOUT_MS	        5000	// 5 seconds
#define ADVERTISER_TIMEOUT_MS		        3000	// 3 seconds
#define COMPANY_ID			                0x0047
#define DEVICENAME_LENGTH   		        9
#define ROOM_NAME_LENGTH                    10
#define REQUIRED_NUM_OF_RSSI_SAMPLES	    10

#define GW_DATA_INDEX_NET_ID                7
#define GW_DATA_INDEX_ROOM_ID               11
#define GW_DATA_INDEX_ROOM_NAME             13
#define GW_DATA_INDEX_DEV_NAME              23
/***************************************************************************//**
 * @brief
 *    struct typedef containing necessary gateway related info
 ******************************************************************************/
typedef struct
{
  char     device_name[DEVICENAME_LENGTH];
  char     room_name[ROOM_NAME_LENGTH];
  int8_t   rssi[REQUIRED_NUM_OF_RSSI_SAMPLES];
  float    rssi_filtered;
  uint32_t network_UID;
  uint16_t room_id;
  uint8_t  rssi_index;
  bool	   measurements_ready;
}gateway_data_t;


/***************************************************************************//**
 * @brief
 *    Custom(user) advertisement struct typedef for Asset data
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
  uint32_t network_UID;
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
}__attribute__((__packed__)) custom_advert_t;


/***************************************************************************//**
 * @brief
 *    Indoor Positioning - Asset related configuration struct typedef
 ******************************************************************************/
typedef struct
{
  char device_name[DEVICENAME_LENGTH];
  uint32_t network_UID;
  uint16_t reporting_interval;
}IPAS_config_data_t;

/***************************************************************************//**
 * @brief
 *    enumeration for keys used for the Non-volatile memory storage
 ******************************************************************************/
typedef enum IPAS_config_keys_enum
{
  IPAS_config_key_network_UID = 1,
  IPAS_config_key_reportingInterval,
  IPAS_config_num_of_keys
}IPAS_config_keys_enum_t;

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
 * 	 X position
 * 	 Y position
 * 	 Positioning method
 * 	 Position Type
 ******************************************************************************/
void create_custom_advert_package(custom_advert_t *pData, uint8_t flags, uint16_t companyID, char *name);

/***************************************************************************//**
 * Creates an entry in the stored gateways array if the gateway is not yet stored
 * @param[in] data - Pointer to the gateway data
 ******************************************************************************/
void create_gateway_storage_entry(uint8_t *data);

/***************************************************************************//**
 * Stores RSSI measurements of a gateway
 * @param[in] scan_report - Pointer to the scan report received as a bt event
 ******************************************************************************/
void store_gateway_rssi(sl_bt_evt_scanner_scan_report_t *scan_report);

/***************************************************************************//**
 * Clears stored gateway data
 * @note Usually called after an unsuccessful attempt at indoor positioning
*******************************************************************************/
void clear_gateways();

/***************************************************************************//**
 * Returns true if minimal number of RSSI measurements for all required gateways are available
*******************************************************************************/
bool indoor_positioning_service_available();

/***************************************************************************//**
 * Initialize Indoor Positioning service
*******************************************************************************/
void IPAS_init();

/***************************************************************************//**
 * Called periodically by the application
 * Handles periodic Indoor Positioning related tasks
*******************************************************************************/
void IPAS_step();

/***************************************************************************//**
 * Enables Indoor Positioning service
 * Starts a periodic timer to calculate position. Period set by configuration
 *
 * @note Must be called after IPAS_Init
*******************************************************************************/
void IPAS_enable_service();

/***************************************************************************//**
 * Disabled Indoor Positioning service
 * Stops the periodic timer which triggers the indoor positioning
*******************************************************************************/
void IPAS_disable_service();

/***************************************************************************//**
 * Enters configuration mode
 * prints out current configuration parameters
 * set up and start advertisements
 * starts timeout timer
*******************************************************************************/
void enter_config_mode();

/***************************************************************************//**
 * Enters Normal mode
 * Set up and start scanner
 * Set up and start advertisements
*******************************************************************************/
void enter_normal_mode();

/***************************************************************************//**
 * re-initializes internal indoor positioning related data
*******************************************************************************/
void init_position_calculator();

/***************************************************************************//**
 * Called periodically - calculates position of the asset
*******************************************************************************/
void calculate_position();

/***************************************************************************//**
 * Pre-filters RSSI measurements
*******************************************************************************/
void distance_pre_filtering();

/***************************************************************************//**
 * Based on the filtered RSSI of the gateways, the closest room is selected
 *******************************************************************************/
void find_closest_room();

/***************************************************************************//**
 * Resets Indoor Positioning flags
 *******************************************************************************/
void reset_flags();

/***************************************************************************//**
 * Sets up and starts advertisement about asset's position
 *******************************************************************************/
void start_position_advertisement();

/***************************************************************************//**
 * Resets RSSI measurement related counters, values and flags - called after calculations are done
*******************************************************************************/
void reset_gateway_data();

/***************************************************************************//**
 *Loads configuration from Non-volatile memory
*******************************************************************************/
void update_local_config();

/***************************************************************************//**
 * Updates GATT database with values stored in NVM
*******************************************************************************/
void update_gatt_entries();

/***************************************************************************//**
 * Indoor Positioning related BT event handler
 * @param[in] - evt - bluetooth event data structure
*******************************************************************************/
void IPAS_event_handler(sl_bt_msg_t *evt);

/***************************************************************************//**
 * Callback for configuration mode timeout timer
 ******************************************************************************/
void config_mode_timeout_cb();

/***************************************************************************//**
 * Callback for normal mode timer
 ******************************************************************************/
void indoor_position_calculation_timer_cb();

/***************************************************************************//**
* Callback for gateway finder timeout
 ******************************************************************************/
void gateway_finder_timeout_cb();

/***************************************************************************//**
 * Callback for advertiser timeout
 ******************************************************************************/
void advertiser_timeout_cb();

/** @} (end addtogroup Indoor Positioning - Asset) */
#ifdef __cplusplus
}
#endif

#endif /* INDOOR_POSITIONING_H_ */
