/***************************************************************************//**
 * @file indoor_positioning.h
 * @brief Application Logic Source File
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
#include "sl_common.h"
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
// 2 minutes
#define CONFIG_MODE_TIMEOUT_MS          (120000)

// 5 seconds
#define GATEWAY_FINDER_TIMEOUT_MS       (5000)

// 3seconds after the gateway finding finished
#define MINIMUM_REPORTING_INTERVAL      ((GATEWAY_FINDER_TIMEOUT_MS / 1000) + 3)

#define COMPANY_ID                      (0x0047)
#define DEVICENAME_LENGTH               (9)
#define ROOM_NAME_LENGTH                (7)
#define REQUIRED_NUM_OF_RSSI_SAMPLES    (10)

#define GW_DATA_INDEX_NET_ID            (7)
#define GW_DATA_INDEX_ROOM_ID           (11)
#define GW_DATA_INDEX_ROOM_NAME         (13)
#define GW_DATA_INDEX_DEV_NAME          (22)

/***************************************************************************//**
 * @brief
 *    struct typedef containing necessary gateway related info
 ******************************************************************************/
typedef struct {
  char device_name[DEVICENAME_LENGTH + 1];
  char room_name[ROOM_NAME_LENGTH + 1];
  int8_t rssi[REQUIRED_NUM_OF_RSSI_SAMPLES];
  float rssi_filtered;
  uint32_t network_UID;
  uint16_t room_id;
  uint8_t rssi_index;
  bool measurements_ready;
} gateway_data_t;

/***************************************************************************//**
 * @brief
 *    Custom(user) advertisement struct typedef for Asset data
 ******************************************************************************/
typedef struct {
  uint8_t len_flags;
  uint8_t type_flags;
  uint8_t val_flags;

  uint8_t len_manuf;
  uint8_t type_manuf;

  // First two bytes must contain the manufacturer ID (little-endian order)
  uint8_t company_LO;
  uint8_t company_HI;

  // The next bytes are freely configurable - using one byte for counter value
  //   and one byte for last button press
  uint32_t network_UID;
  uint16_t room_id;
  char room_name[ROOM_NAME_LENGTH];

  // length of the name AD element is variable
  uint8_t len_name;
  uint8_t type_name;

  // NAME_MAX_LENGTH must be sized so that total length of data does not exceed
  //   31 bytes
  char name[DEVICENAME_LENGTH];

  // These values are NOT included in the actual advertising payload, just for
  //   bookkeeping
  char dummy;        // Space for null terminator
  uint8_t data_size; // Actual length of advertising data
} __attribute__((__packed__)) custom_advert_t;

/***************************************************************************//**
 * @brief
 *    Indoor Positioning - Asset related configuration struct typedef
 ******************************************************************************/
typedef struct {
  char device_name[DEVICENAME_LENGTH + 1];
  uint32_t network_UID;
  uint16_t reporting_interval;
} IPAS_config_data_t;

/***************************************************************************//**
 * @brief
 *    enumeration for keys used for the Non-volatile memory storage
 ******************************************************************************/
typedef enum IPAS_config_keys_enum {
  IPAS_config_key_network_UID = 1, IPAS_config_key_reportingInterval,
  IPAS_config_num_of_keys
} IPAS_config_keys_enum_t;

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                          Public Function Declarations
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * @brief Checks if Indoor positioning is possible or not
 *
 * @param[in] None
 *
 * @return Returns true if minimal number of RSSI measurements for all required
 *   gateways are available
 *******************************************************************************/
bool indoor_positioning_service_available(void);

/***************************************************************************//**
 * @brief
 * Creates custom advertising package
 *
 * @param[in] pData - Pointer to the struct holding the above mentioned asset
 *   data
 * @param[in] flags - advertisement flag
 * @param[in] companyID - Unique company ID
 * @param[in] name - Unique device name
 *
 * @note Creates custom advertising package with the following asset data:
 *       Network Unique ID
 *       Room ID
 *       Room Name
 *       Device Name
 ******************************************************************************/
void create_custom_advert_package(custom_advert_t *pData,
                                  uint8_t flags,
                                  uint16_t companyID,
                                  char *name);

/***************************************************************************//**
 * @brief
 * Creates an entry in the stored gateways array if the gateway is not yet
 *   stored
 *
 * @param[in] data - Pointer to the gateway data
 ******************************************************************************/
void create_gateway_storage_entry(uint8_t *data);

/***************************************************************************//**
 * @brief
 * Stores RSSI measurements of a gateway
 *
 * @param[in] scan_report - Pointer to the scan report received as a bt event
 ******************************************************************************/
void store_gateway_rssi(
  sl_bt_evt_scanner_legacy_advertisement_report_t *scan_report);

/***************************************************************************//**
 * @brief
 * Clears stored gateway data
 *
 * @param[in] None
 *
 * @note Usually called after an unsuccessful attempt at indoor positioning
 *******************************************************************************/
void clear_gateways(void);

/***************************************************************************//**
 * @brief
 * Initialize Indoor Positioning service
 *
 * @param[in] None
 *******************************************************************************/
void IPAS_init(void);

/***************************************************************************//**
 * @brief
 * Called periodically by the application
 * Handles periodic Indoor Positioning related tasks
 *
 * @param[in] None
 *******************************************************************************/
void IPAS_step(void);

/***************************************************************************//**
 * @brief
 * Enables Indoor Positioning service
 * Starts a periodic timer to calculate position. Period set by configuration
 *
 * @param[in] None
 *
 * @note Must be called after IPAS_Init
 *******************************************************************************/
void IPAS_enable_service(void);

/***************************************************************************//**
 * @brief
 * Disabled Indoor Positioning service
 * Stops the periodic timer which triggers the indoor positioning
 *
 * @param[in] None
 *******************************************************************************/
void IPAS_disable_service(void);

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
 * Enters Normal mode
 * Set up and start scanner
 * Set up and start advertisements
 *
 * @param[in] None
 *******************************************************************************/
void enter_normal_mode(void);

/***************************************************************************//**
 * @brief
 * re-initializes internal indoor positioning related data
 *
 * @param[in] None
 *******************************************************************************/
void init_position_calculator(void);

/***************************************************************************//**
 * @brief
 * Called periodically - calculates position of the asset
 *
 * @param[in] None
 *******************************************************************************/
void calculate_position(void);

/***************************************************************************//**
 * @brief
 * Pre-filters RSSI measurements
 *
 * @param[in] None
 *******************************************************************************/
void distance_pre_filtering(void);

/***************************************************************************//**
 * @brief
 * Based on the filtered RSSI of the gateways, the closest room is selected
 *
 * @param[in] None
 *******************************************************************************/
void find_closest_room(void);

/***************************************************************************//**
 * @brief
 * Resets Indoor Positioning flags
 *
 * @param[in] None
 *******************************************************************************/
void reset_IP_state(void);

/***************************************************************************//**
 * @brief
 * Sets up and starts advertisement about asset's position
 *
 * @param[in] None
 *******************************************************************************/
void start_position_advertisement(void);

/***************************************************************************//**
 * @brief
 * Resets RSSI measurement related counters, values and flags - called after
 *   calculations are done
 *
 * @param[in] None
 *******************************************************************************/
void reset_gateway_data(void);

/***************************************************************************//**
 * @brief
 * Loads configuration from Non-volatile memory
 *
 * @param[in] None
 *******************************************************************************/
void update_local_config(void);

/***************************************************************************//**
 * @brief
 * Updates GATT database with values stored in NVM
 *
 * @param[in] None
 *******************************************************************************/
void update_gatt_entries(void);

/***************************************************************************//**
 * @brief
 * Indoor Positioning related BT event handler
 *
 * @param[in] - evt - bluetooth event data structure
 *******************************************************************************/
void IPAS_event_handler(sl_bt_msg_t *evt);

/***************************************************************************//**
 * @brief
 * Callback for configuration mode timeout timer
 *
 * @param[in] None
 ******************************************************************************/
void config_mode_timeout_cb();

/***************************************************************************//**
 * @brief
 * Callback for normal mode timer
 *
 * @param[in] None
 ******************************************************************************/
void indoor_positioning_trigger_timer_cb();

/***************************************************************************//**
 * @brief
 * Callback for gateway finder timeout
 *
 * @param[in] None
 ******************************************************************************/
void gateway_finder_timeout_cb();

/** @} (end addtogroup Indoor Positioning - Asset) */
#ifdef __cplusplus
}
#endif

#endif /* INDOOR_POSITIONING_H_ */
