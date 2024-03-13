/***************************************************************************//**
 * @file indoor_positioning.c
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
// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------

#include "micro_oled_ssd1306.h"
#include "sl_i2cspm_instances.h"
#include "indoor_positioning.h"

// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------
static void oled_init(void);
static void oled_display_config(void);
static void oled_update_info(void);
static void oled_display_service_unavailable(void);
static void validate_new_configuration_value(uint8_t *request_data,
                                             IPAS_config_keys_enum_t nvm_key);

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------
// User defined advertising data
static custom_advert_t custom_advert;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t configMode_advertising_set_handle = 0xff;
static uint8_t normalMode_advertising_set_handle = 0xff;

// Handle for SleepTimers
static sl_sleeptimer_timer_handle_t IP_config_mode_timeout_timer;
static sl_sleeptimer_timer_handle_t IP_find_current_room_periodic_timer;
static sl_sleeptimer_timer_handle_t IP_gateway_finder_timeout_timer;
static glib_context_t glib_context;

// flags
static bool IP_start_positioning = false;
static bool IP_ready = false;
static bool IP_clear_stored_gateways = false;
static bool IP_config_mode = false;
static bool IP_BT_init_done = false;
static bool IP_mode_selected = false;
static bool IP_service_unavailable = false;
static bool IP_gateway_finding_finished = false;
static bool IP_service_enabled = true;

// Configuration data
static IPAS_config_data_t IPAS_config_data;

// Number of found and active gateways
static uint8_t gateway_counter = 0;

// Current room information
static char current_room_name[ROOM_NAME_LENGTH];
static uint8_t current_room_id = 0;

// Array to store gateway related data
static gateway_data_t gateway_data_storage[5];

// Asset position
static glib_context_t glib_context;

#define EXT_SIGNAL_CONFIG_MODE_TIMEOUT     (1 << 0)
#define EXT_SIGNAL_POSITIONING_TRIGGER     (1 << 2)
#define EXT_SIGNAL_GATEWAY_FINDER_TIMEOUT  (1 << 3)

// -----------------------------------------------------------------------------
//                                Callbacks
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * Callback for configuration mode timeout timer
 ******************************************************************************/
void config_mode_timeout_cb()
{
  sl_bt_external_signal(EXT_SIGNAL_CONFIG_MODE_TIMEOUT);
}

/***************************************************************************//**
 * Callback for normal mode timer
 ******************************************************************************/
void indoor_positioning_trigger_timer_cb()
{
  sl_bt_external_signal(EXT_SIGNAL_POSITIONING_TRIGGER);
}

/***************************************************************************//**
 * Callback for gateway finder timeout
 ******************************************************************************/
void gateway_finder_timeout_cb()
{
  sl_bt_external_signal(EXT_SIGNAL_GATEWAY_FINDER_TIMEOUT);
}

// -----------------------------------------------------------------------------
//                                  Advertising
// -----------------------------------------------------------------------------

/***************************************************************************//**
* Returns true if minimal number of RSSI measurements for all required gateways
*   are available
*******************************************************************************/
bool indoor_positioning_service_available(void)
{
  bool service_available = true;

  if (gateway_counter > 0) {
    // Loop through gateway data - Check if enough samples are gathered
    for (uint8_t i = 0; i < gateway_counter; i++) {
      if (false == gateway_data_storage[i].measurements_ready) {
        service_available = false;
      }
    }
  } else {
    service_available = false;
  }
  return service_available;
}

/***************************************************************************//**
 * Creates custom advertising package with current asset related data
 ******************************************************************************/
void create_custom_advert_package(custom_advert_t *pData,
                                  uint8_t flags,
                                  uint16_t companyID,
                                  char *name)
{
  int n;

  pData->len_flags = 0x02;
  pData->type_flags = 0x01;
  pData->val_flags = flags;

  pData->len_manuf = 16;  // 1+2+16 bytes for type, company ID and the payload
  pData->type_manuf = 0xFF;
  pData->company_LO = companyID & 0xFF;
  pData->company_HI = (companyID >> 8) & 0xFF;

  pData->network_UID = IPAS_config_data.network_UID;
  pData->room_id = current_room_id;
  strncpy(pData->room_name, current_room_name, ROOM_NAME_LENGTH - 1);
  pData->room_name[ROOM_NAME_LENGTH - 1] = '\0';

  // Name length, excluding null terminator
  n = strlen(name);
  if (n > DEVICENAME_LENGTH) {
    // Incomplete name
    pData->type_name = 0x08;
  } else {
    pData->type_name = 0x09;
  }

  strncpy(pData->name, name, DEVICENAME_LENGTH - 1);
  pData->name[DEVICENAME_LENGTH - 1] = '\0';

  if (n > DEVICENAME_LENGTH) {
    n = DEVICENAME_LENGTH;
  }

  pData->len_name = 1 + n; // length of name element is the name string length +
                           //   1 for the AD type

  // Calculate total length of advertising data
  pData->data_size = 3 + (1 + pData->len_manuf) + (1 + pData->len_name);
}

// -----------------------------------------------------------------------------
//                          Gateway finder & Sampling
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * Creates an entry in the stored gateways array if the gateway is not yet
 *   stored
 ******************************************************************************/
void create_gateway_storage_entry(uint8_t *data)
{
  bool gw_already_stored = false;
  uint8_t gw_index = 0;
  // Loop through gateway data array - check if found gateway is already stored
  //   or not. Gateway unique name is starting at the 27th index
  for (gw_index = 0; gw_index < gateway_counter; gw_index++) {
    if (0 == memcmp(&data[GW_DATA_INDEX_DEV_NAME],
                    &gateway_data_storage[gw_index].device_name,
                    DEVICENAME_LENGTH)) {
      gw_already_stored = true;
    }
  }

  if (!gw_already_stored) {
    memcpy(&gateway_data_storage[gateway_counter].network_UID,
           &data[GW_DATA_INDEX_NET_ID],
           sizeof(gateway_data_storage[gateway_counter].network_UID));
    memcpy(&gateway_data_storage[gateway_counter].room_id,
           &data[GW_DATA_INDEX_ROOM_ID],
           sizeof(gateway_data_storage[gateway_counter].room_id));
    memcpy(gateway_data_storage[gateway_counter].room_name,
           &data[GW_DATA_INDEX_ROOM_NAME],
           sizeof(gateway_data_storage[gateway_counter].room_name));
    memcpy(gateway_data_storage[gateway_counter].device_name,
           &data[GW_DATA_INDEX_DEV_NAME],
           sizeof(gateway_data_storage[gateway_counter].device_name));
    gateway_data_storage[gateway_counter].rssi_index = 0;
    gateway_counter += 1;
  }
}

/***************************************************************************//**
 * Stores RSSI measurements of a gateway
 ******************************************************************************/
void store_gateway_rssi(sl_bt_evt_scanner_scan_report_t *scan_report)
{
  // Loop through minimal number of gateways
  for (uint8_t i = 0; i < gateway_counter; i++) {
    // Find gateway based on it's advertised & stored name - store the RSSI
    //   value, increment index
    if ((scan_report->data.len <= 31)
        && (GW_DATA_INDEX_DEV_NAME < scan_report->data.len)) {
      uint8_t scan_data[scan_report->data.len];
      memcpy(scan_data, scan_report->data.data, scan_report->data.len);
      if (0 == memcmp(gateway_data_storage[i].device_name,
                      &scan_data[GW_DATA_INDEX_DEV_NAME],
                      DEVICENAME_LENGTH)) {
        gateway_data_storage[i].rssi[gateway_data_storage[i].rssi_index] =
          scan_report->rssi;
        gateway_data_storage[i].rssi_index += 1;

        // Reset rssi_index when reaching the end of the array - signal that
        //   required number of samples have been gathered
        if (gateway_data_storage[i].rssi_index
            >= REQUIRED_NUM_OF_RSSI_SAMPLES) {
          gateway_data_storage[i].rssi_index = 0;
          gateway_data_storage[i].measurements_ready = true;
        }
      }
    }
  }
}

/***************************************************************************//**
* Clears stored gateway data
* @note Usually called after an unsuccessful attempt at indoor positioning
*******************************************************************************/
void clear_gateways(void)
{
  // Reset data for every gateway
  for (uint8_t i = 0; i < gateway_counter; i++) {
    memset(&gateway_data_storage[i], 0, sizeof(gateway_data_t));
  }

  // Reset number of found gateways
  gateway_counter = 0;
}

// -----------------------------------------------------------------------------
//                          Indoor Positioning - Asset
// -----------------------------------------------------------------------------

/***************************************************************************//**
* Called periodically by the application
* Handles periodic Indoor Positioning related tasks
*******************************************************************************/
void IPAS_step(void)
{
  // --- Initialization ---
  // Check button status once bluetooth stack is initialized and neither mode is
  //   selected yet
  if (!IP_mode_selected && IP_BT_init_done) {
    // If button is pressed down, go to configuration mode
    if (sl_simple_button_get_state(&sl_button_btn0)
        == SL_SIMPLE_BUTTON_PRESSED) {
      IP_mode_selected = true;
      app_log("Button0 is pressed, entering Configuration mode\n\n");
      enter_config_mode();
    }
    // If button is released, go to normal mode
    else {
      IP_mode_selected = true;
      app_log("Button0 is released, entering Normal mode\n\n");
      enter_normal_mode();
    }
  }
  // --- End of Initialization ---

  // Periodic tasks
  if (IP_start_positioning) {
    if (!IP_ready) {
      init_position_calculator();
    } else {
      if (indoor_positioning_service_available()) {
        // Estimate device's location
        calculate_position();
      }
    }
  }

  if (IP_clear_stored_gateways) {
    clear_gateways();
    IP_clear_stored_gateways = false;
  }

  if (IP_service_unavailable) {
    app_log("No gateways were found\n");
    oled_display_service_unavailable();

    reset_IP_state();
  }
}

/***************************************************************************//**
* Resets Indoor Positioning flags
*******************************************************************************/
void reset_IP_state(void)
{
  IP_ready = false;
  IP_start_positioning = false;
  IP_gateway_finding_finished = false;
  IP_service_unavailable = false;
  IP_clear_stored_gateways = true;
}

/***************************************************************************//**
* Enables Indoor Positioning service
*******************************************************************************/
void IPAS_enable_service(void)
{
  sl_status_t sc;
  bool is_timer_running = false;

  IP_service_enabled = true;
  sc = sl_sleeptimer_is_timer_running(&IP_find_current_room_periodic_timer,
                                      &is_timer_running);
  if (!is_timer_running) {
    sc = sl_sleeptimer_start_periodic_timer_ms(
      &IP_find_current_room_periodic_timer,
      (IPAS_config_data.
       reporting_interval * 1000),
      indoor_positioning_trigger_timer_cb,
      (void *) NULL,
      0,
      0);
    app_assert_status(sc);
  }

  reset_IP_state();
}

/***************************************************************************//**
* Disabled Indoor Positioning service
*******************************************************************************/
void IPAS_disable_service(void)
{
  sl_status_t sc;
  bool is_timer_running = false;

  IP_service_enabled = false;

  sc = sl_sleeptimer_is_timer_running(&IP_find_current_room_periodic_timer,
                                      &is_timer_running);
  if (is_timer_running) {
    sc = sl_sleeptimer_stop_timer(&IP_find_current_room_periodic_timer);
    app_assert_status(sc);
  }

  reset_IP_state();
}

/***************************************************************************//**
* Initialize Indoor Positioning service
*******************************************************************************/
void IPAS_init(void)
{
  app_log("\nIndoor Positioning - Asset\n");
  app_log("Loading Configuration\n");
  update_local_config();
  // Print current configuration parameters
  app_log("NetworkU_ID: %lu | Reporting interval: %d\n",
          IPAS_config_data.network_UID,
          IPAS_config_data.reporting_interval);
  // Initialize the OLED display
  oled_init();
}

/***************************************************************************//**
* Enters configuration mode
* prints out current configuration parameters
* set up and start advertisements
* starts timeout timer
*******************************************************************************/
void enter_config_mode(void)
{
  sl_status_t sc;
  IP_config_mode = true;

  // Set up advertisement
  sc = sl_bt_advertiser_create_set(&configMode_advertising_set_handle);
  app_assert_status(sc);

  // Update Device Name in GATT database
  sc = sl_bt_gatt_server_write_attribute_value(gattdb_device_name,
                                               0,
                                               DEVICENAME_LENGTH,
                                               (const uint8_t *) IPAS_config_data.device_name);
  app_assert_status(sc);

  // Generate data for advertising
  sc = sl_bt_legacy_advertiser_generate_data(configMode_advertising_set_handle,
                                             sl_bt_advertiser_general_discoverable);
  app_assert_status(sc);

  sc = sl_bt_advertiser_set_timing(
    configMode_advertising_set_handle,
    100, // min. adv. interval (milliseconds * 1.6)
    160, // max. adv. interval (milliseconds * 1.6)
    0,   // adv. duration
    0);  // max. num. adv. events
  app_assert_status(sc);

  // Start general advertising and enable connections.
  sc = sl_bt_legacy_advertiser_start(configMode_advertising_set_handle,
                                     sl_bt_advertiser_connectable_scannable);
  app_assert_status(sc);

  // Start configuration mode timeout timer
  sl_sleeptimer_start_timer_ms(&IP_config_mode_timeout_timer,
                               CONFIG_MODE_TIMEOUT_MS,
                               config_mode_timeout_cb,
                               (void *) NULL,
                               0,
                               0);

  // Display configuration values on the OLED display
  oled_display_config();
}

/***************************************************************************//**
* Enters Normal mode
* Set up and start scanner
* Set up and start advertisements
*******************************************************************************/
void enter_normal_mode(void)
{
  sl_status_t sc;
  IP_config_mode = false;

  // Set up and start scanning
  sc = sl_bt_scanner_set_parameters(sl_bt_scanner_scan_mode_active,
                                    100,
                                    100);
  app_assert_status(sc);

  sc = sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
  app_assert_status(sc);

  // Set up advertisement
  sc = sl_bt_advertiser_create_set(&normalMode_advertising_set_handle);
  app_assert_status(sc);

  sc = sl_bt_advertiser_set_timing(
    normalMode_advertising_set_handle, 100, // min. adv. interval
                                            // (milliseconds * 1.6)
    160, // max. adv. interval (milliseconds * 1.6)
    3,   // adv. duration
    5);  // max. num. adv. events
  app_assert_status(sc);

  if (IP_service_enabled) {
    sc = sl_sleeptimer_start_periodic_timer_ms(
      &IP_find_current_room_periodic_timer,
      (IPAS_config_data.
       reporting_interval * 1000),
      indoor_positioning_trigger_timer_cb,
      (void *) NULL,
      0,
      0);
    app_assert_status(sc);
  }
}

/***************************************************************************//**
* Sets up and starts advertisement about asset's position
*******************************************************************************/
void start_position_advertisement(void)
{
  sl_status_t sc;

  // Create user defined advertising packet
  create_custom_advert_package(&custom_advert,
                               0x06,
                               COMPANY_ID,
                               IPAS_config_data.device_name);

  // Set custom advertising payload
  sc = sl_bt_legacy_advertiser_set_data(normalMode_advertising_set_handle,
                                        sl_bt_advertiser_advertising_data_packet,
                                        custom_advert.data_size,
                                        (const uint8_t *) &custom_advert);
  app_assert_status(sc);

  // Start advertising using custom data
  sc = sl_bt_legacy_advertiser_start(normalMode_advertising_set_handle,
                                     sl_bt_advertiser_non_connectable);
  app_assert_status(sc);
}

/***************************************************************************//**
* re-initializes internal indoor positioning related data
*******************************************************************************/
void init_position_calculator(void)
{
  sl_status_t sc;
  bool timer_running = false;

  // Reset gateway related measurements and flags
  reset_gateway_data();

  sl_bt_scanner_stop();

  // active scanning
  // 50ms scan interval, 100ms scan window
  sc = sl_bt_scanner_set_parameters(sl_bt_scanner_scan_mode_active,
                                    100,
                                    100);
  app_assert_status(sc);

  sc = sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
  app_assert_status(sc);

  sl_sleeptimer_is_timer_running(&IP_gateway_finder_timeout_timer,
                                 &timer_running);

  if (!timer_running) {
    // Start gateway finder timeout
    sc = sl_sleeptimer_start_timer_ms(&IP_gateway_finder_timeout_timer,
                                      GATEWAY_FINDER_TIMEOUT_MS,
                                      gateway_finder_timeout_cb,
                                      (void *) NULL,
                                      0,
                                      0);
    app_assert_status(sc);
  }

  IP_ready = true;
}

/***************************************************************************//**
* Called periodically - calculates position of the asset
*******************************************************************************/
void calculate_position(void)
{
  app_log("\nIndoor position calculation started... \n");

  // Stop scanner until position calculations are done
  sl_bt_scanner_stop();

  // Distance pre-filtering
  distance_pre_filtering();

  // Locate which room the asset is in
  find_closest_room();

  // Start advertisement about the calculated position
  start_position_advertisement();

  // Update the OLED display with the newly calculated position
  oled_update_info();

  // Reset IP flags
  reset_IP_state();

  app_log("Indoor position calculation finished\n");
  app_log("Asset is in room: %s\n", current_room_name);
}

// -----------------------------------------------------------------------------
//                          Indoor Positioning calculations
// -----------------------------------------------------------------------------

/***************************************************************************//**
* Pre-filters RSSI measurements
*******************************************************************************/
void distance_pre_filtering(void)
{
  int32_t temp_rssi_sum = 0;
  int32_t temp_rssi = 0;
  uint8_t filtered_rssi_counter = 0;
  float rssi_mean = 0.0;
  float rssi_std_dev = 0.0;
  float filtered_rssi[REQUIRED_NUM_OF_RSSI_SAMPLES];
  memset(filtered_rssi, 0, sizeof(filtered_rssi));

  for (uint8_t gw_index = 0; gw_index < gateway_counter; gw_index++) {
    temp_rssi_sum = 0;
    temp_rssi = 0;
    filtered_rssi_counter = 0;
    memset(filtered_rssi, 0, sizeof(filtered_rssi));

    // Calculate RSSI mean
    for (uint8_t rssi_index = 0; rssi_index < REQUIRED_NUM_OF_RSSI_SAMPLES;
         rssi_index++) {
      temp_rssi_sum += gateway_data_storage[gw_index].rssi[rssi_index];
    }
    rssi_mean = temp_rssi_sum / REQUIRED_NUM_OF_RSSI_SAMPLES;

    // Calculate RSSI standard deviation
    temp_rssi_sum = 0;
    for (uint8_t rssi_index = 0; rssi_index < REQUIRED_NUM_OF_RSSI_SAMPLES;
         rssi_index++) {
      temp_rssi = gateway_data_storage[gw_index].rssi[rssi_index];
      temp_rssi_sum += (temp_rssi - rssi_mean) * (temp_rssi - rssi_mean);
    }
    rssi_std_dev = sqrt(temp_rssi_sum / REQUIRED_NUM_OF_RSSI_SAMPLES);

    // Remove measurements which are below the mean - (2*std deviation) - single
    //   direction outlier removal to filter out invalid measurements due to
    //   indoor multipath fading
    for (uint8_t rssi_index = 0; rssi_index < REQUIRED_NUM_OF_RSSI_SAMPLES;
         rssi_index++) {
      if (gateway_data_storage[gw_index].rssi[rssi_index]
          >= (rssi_mean - (2 * rssi_std_dev))) {
        filtered_rssi[filtered_rssi_counter] =
          gateway_data_storage[gw_index].rssi[rssi_index];
        filtered_rssi_counter++;
      }
    }

    // Calculate filtered RSSI mean
    temp_rssi_sum = 0;
    for (uint8_t rssi_index = 0; rssi_index < filtered_rssi_counter;
         rssi_index++) {
      temp_rssi_sum += filtered_rssi[rssi_index];
    }
    gateway_data_storage[gw_index].rssi_filtered = temp_rssi_sum
                                                   / filtered_rssi_counter;
  }
}

/***************************************************************************//**
* Based on the filtered RSSI of the gateways, the closest room is selected
*******************************************************************************/
void find_closest_room(void)
{
  float highest_rssi = gateway_data_storage[0].rssi_filtered;
  uint8_t closest_gw_index = 0;

  for (uint8_t gw_index = 0; gw_index < gateway_counter; gw_index++) {
    if (gateway_data_storage[gw_index].rssi_filtered > highest_rssi) {
      highest_rssi = gateway_data_storage[gw_index].rssi_filtered;
      closest_gw_index = gw_index;
    }
  }

  current_room_id = gateway_data_storage[closest_gw_index].room_id;
  memcpy(current_room_name,
         gateway_data_storage[closest_gw_index].room_name,
         ROOM_NAME_LENGTH);
}

/***************************************************************************//**
* Resets RSSI measurement related counters, values and flags - called after
*   calculations are done
*******************************************************************************/
void reset_gateway_data(void)
{
  for (uint8_t gw_index = 0; gw_index < gateway_counter; gw_index++) {
    gateway_data_storage[gw_index].rssi_index = 0;
    gateway_data_storage[gw_index].rssi_filtered = 0;
    gateway_data_storage[gw_index].measurements_ready = false;
  }
}

// -----------------------------------------------------------------------------
//                          Non-volatile memory related functions
// -----------------------------------------------------------------------------

/***************************************************************************//**
* Loads configuration from Non-volatile memory
*******************************************************************************/
void update_local_config(void)
{
  uint32_t object_type;
  uintptr_t *entry_ptr;
  size_t temp_data_length = 0;
  size_t num_of_objects = nvm3_countObjects(nvm3_defaultHandle);
  bool valid_entry = false;

  memset(&IPAS_config_data, 0, sizeof(IPAS_config_data));
  num_of_objects = nvm3_countObjects(nvm3_defaultHandle);
  if (num_of_objects != 0) {
    // Read stored configuration from NVM
    for (uint16_t config_key = 0; config_key < IPAS_config_num_of_keys;
         config_key++) {
      nvm3_getObjectInfo(nvm3_defaultHandle,
                         config_key,
                         &object_type,
                         &temp_data_length);
      if (object_type == NVM3_OBJECTTYPE_DATA) {
        switch (config_key) {
          case IPAS_config_key_network_UID:
            entry_ptr = (uintptr_t *) &IPAS_config_data.network_UID;
            valid_entry = true;
            break;
          case IPAS_config_key_reportingInterval:
            entry_ptr = (uintptr_t *) &IPAS_config_data.reporting_interval;
            valid_entry = true;
            break;
          default:
            entry_ptr = NULL;
            valid_entry = false;
            break;
        }

        if (valid_entry) {
          // Update configuration entries in NVM
          nvm3_readData(nvm3_defaultHandle,
                        config_key,
                        entry_ptr,
                        temp_data_length);
        }
      }
    }
  }
  update_gatt_entries();
}

/***************************************************************************//**
* Updates GATT database with values stored in NVM
*******************************************************************************/
void update_gatt_entries(void)
{
  sl_status_t sc;
  uint8_t gatt_db_id;
  size_t value_size;
  uint8_t temp_value_array[8];
  bool valid_entry = false;

  for (uint16_t config_key = 0; config_key < IPAS_config_num_of_keys;
       config_key++) {
    switch (config_key) {
      case IPAS_config_key_network_UID:
        gatt_db_id = gattdb_NetworkUID;
        value_size = sizeof(IPAS_config_data.network_UID);
        memcpy(temp_value_array, &IPAS_config_data.network_UID, value_size);
        valid_entry = true;
        break;
      case IPAS_config_key_reportingInterval:
        gatt_db_id = gattdb_ReportingInterval;
        value_size = sizeof(IPAS_config_data.reporting_interval);
        memcpy(temp_value_array,
               &IPAS_config_data.reporting_interval,
               value_size);
        valid_entry = true;
        break;
      default:
        gatt_db_id = 0;
        value_size = 0;
        memset(temp_value_array, 0, 8);
        valid_entry = false;
        break;
    }
    if (valid_entry) {
      // Update GATT database
      sc = sl_bt_gatt_server_write_attribute_value(gatt_db_id,
                                                   0,
                                                   value_size,
                                                   temp_value_array);
      app_assert_status(sc);
    }
  }
}

// -----------------------------------------------------------------------------
//                               Helper functions
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * Validates and modifies configuration entries if necessary
 * to a valid or default value
 ******************************************************************************/
static void validate_new_configuration_value(uint8_t *request_data,
                                             IPAS_config_keys_enum_t nvm_key)
{
  if (nvm_key == IPAS_config_key_reportingInterval) {
    uint16_t temp_reporting_interval;
    memcpy(&temp_reporting_interval,
           request_data,
           sizeof(IPAS_config_data.reporting_interval));

    // Ensure that the reporting interval leaves enough time to calculate a
    //   position before triggered again
    // Invalid (less than required) values will be replaced with the minimum
    //   reporting interval in the application and in the GATT database also
    if (temp_reporting_interval < MINIMUM_REPORTING_INTERVAL) {
      temp_reporting_interval = MINIMUM_REPORTING_INTERVAL;
      memcpy(request_data,
             &temp_reporting_interval,
             sizeof(temp_reporting_interval));
    }
  }
}

/***************************************************************************//**
 * Initializes OLED display
 ******************************************************************************/
static void oled_init(void)
{
  ssd1306_init(sl_i2cspm_qwiic);
  glib_init(&glib_context);

  // Fill lcd with background color
  glib_clear(&glib_context);

  glib_draw_string(&glib_context,
                   "IP Asset",
                   0, 2);
  glib_draw_string(&glib_context,
                   "Waiting",
                   0, 12);
  glib_draw_string(&glib_context,
                   "for",
                   20, 22);
  glib_draw_string(&glib_context,
                   "gateways",
                   0, 32);
  glib_update_display();
}

/***************************************************************************//**
 * Displays current configuration values on the OLED display
 ******************************************************************************/
static void oled_display_config(void)
{
  char network_uid[12];
  char report_interval[12];

  snprintf(network_uid,
           sizeof(network_uid),
           "UID:%ld",
           IPAS_config_data.network_UID);
  snprintf(report_interval,
           sizeof(report_interval),
           "int:%d",
           IPAS_config_data.reporting_interval);

  /* Fill oled with background color */
  glib_clear(&glib_context);

  glib_draw_string(&glib_context, IPAS_config_data.device_name, 0, 2);
  glib_draw_string(&glib_context, network_uid, 0, 12);
  glib_draw_string(&glib_context, report_interval, 0, 22);

  glib_update_display();
}

/***************************************************************************//**
 * Updates asset info on the OLED display
 ******************************************************************************/
static void oled_update_info(void)
{
  /* Fill lcd with background color */
  glib_clear(&glib_context);

  glib_draw_string(&glib_context, IPAS_config_data.device_name, 0, 2);
  glib_draw_string(&glib_context, "Room:", 18, 12);
  glib_draw_string(&glib_context, current_room_name, 0, 22);

  glib_update_display();
}

/***************************************************************************//**
 * Displays "Waiting for gateways" message on the OLED display
 ******************************************************************************/
static void oled_display_service_unavailable(void)
{
  /* Fill lcd with background color */
  glib_clear(&glib_context);

  glib_draw_string(&glib_context, IPAS_config_data.device_name, 0, 2);
  glib_draw_string(&glib_context, "Waiting", 0, 12);
  glib_draw_string(&glib_context, "for", 20, 22);
  glib_draw_string(&glib_context, "gateways", 0, 32);

  glib_update_display();
}

// -----------------------------------------------------------------------------
//                                  Event Handler
// -----------------------------------------------------------------------------

/***************************************************************************//**
* Indoor Positioning related BT event handler
*******************************************************************************/
void IPAS_event_handler(sl_bt_msg_t *evt)
{
  static const char gateway_name_prefix[5] = { 'I', 'P', 'G', 'W', '_' };
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];
  IPAS_config_keys_enum_t nvm_key = 0;

  switch (SL_BT_MSG_ID(evt->header)) {
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

      // Create device name string
      snprintf(IPAS_config_data.device_name,
               sizeof(IPAS_config_data.device_name),
               "IPAS_%.2X%.2X",
               address.addr[5],
               address.addr[4]);
      app_log("Device unique name: %s\n", IPAS_config_data.device_name);

      // Enable pairing
      sc = sl_bt_sm_set_bondable_mode(1);
      app_assert_status(sc);

      IP_BT_init_done = true;
      break;

    case sl_bt_evt_connection_opened_id:
      sc =
        sl_bt_sm_increase_security(evt->data.evt_connection_opened.connection);
      app_assert_status(sc);
      break;

    case sl_bt_evt_sm_bonding_failed_id:
      app_log("Bonding failed, reason: 0x%2X\r\n",
              evt->data.evt_sm_bonding_failed.reason);
      // Previous bond is broken, delete it and close connection, host must
      //   retry at least once
      sl_bt_sm_delete_bondings();
      break;

    case sl_bt_evt_gatt_server_attribute_value_id:
      if (evt->data.evt_gatt_server_attribute_value.value.len <= 4) {
        uint8_t request_data[evt->data.evt_gatt_server_attribute_value.value.len
        ];

        app_log("Configuration updated\n");

        // Get GATT database ID of the updated value
        switch (evt->data.evt_gatt_server_attribute_value.attribute) {
          case gattdb_NetworkUID:
            nvm_key = IPAS_config_key_network_UID;
            break;
          case gattdb_ReportingInterval:
            nvm_key = IPAS_config_key_reportingInterval;
            break;
        }

        // Copy received raw data into a byte array
        memcpy(request_data,
               evt->data.evt_gatt_server_attribute_value.value.data,
               evt->data.evt_gatt_server_attribute_value.value.len);

        // Check if the new configuration value is valid if necessary
        validate_new_configuration_value(request_data, nvm_key);

        // Store new GATT entry value
        nvm3_writeData(nvm3_defaultHandle,
                       nvm_key,
                       request_data,
                       evt->data.evt_gatt_server_attribute_value.value.len);

        // Refresh locally stored configuration so GATT database and local
        //   configuration is synchronized
        update_local_config();

        // Reset configuration mode timeout
        sc = sl_sleeptimer_restart_timer_ms(&IP_config_mode_timeout_timer,
                                            CONFIG_MODE_TIMEOUT_MS,
                                            config_mode_timeout_cb,
                                            (void *) NULL,
                                            0,
                                            0);
        app_assert_status(sc);
      }
      break;
    case sl_bt_evt_scanner_scan_report_id:
      if ((evt->data.evt_scanner_scan_report.data.len <= 31)
          && ((sizeof(IPAS_config_data.network_UID) + GW_DATA_INDEX_NET_ID)
              < evt->data.evt_scanner_scan_report.data.len)
          && ((sizeof(gateway_name_prefix) + GW_DATA_INDEX_DEV_NAME)
              < evt->data.evt_scanner_scan_report.data.len)) {
        uint8_t scan_data[evt->data.evt_scanner_scan_report.data.len];

        memcpy(scan_data,
               evt->data.evt_scanner_scan_report.data.data,
               evt->data.evt_scanner_scan_report.data.len);
        // Check for network UID in the beginning of data field. First 6 bytes
        //   are
        //   flags and company ID. Data field starts at index 7.
        // Check for device name also which starts at the index of 24. Gateways'
        //   device name start with "IPGW_" prefix
        if ((0 == memcmp(&IPAS_config_data.network_UID,
                         &scan_data[GW_DATA_INDEX_NET_ID],
                         sizeof(IPAS_config_data.network_UID)))
            && (0 == memcmp(&gateway_name_prefix,
                            &scan_data[GW_DATA_INDEX_DEV_NAME],
                            sizeof(gateway_name_prefix)))) {
          if (!IP_gateway_finding_finished) {
            create_gateway_storage_entry(
              evt->data.evt_scanner_scan_report.data.data);
          } else {
            store_gateway_rssi(&evt->data.evt_scanner_scan_report);
          }
        }
      }
      break;

    case sl_bt_evt_system_external_signal_id:
      if (evt->data.evt_system_external_signal.extsignals
          & EXT_SIGNAL_CONFIG_MODE_TIMEOUT) {
        app_log("\nConfiguration mode timeout - Reset \n");
        sl_bt_system_reset(sl_bt_system_boot_mode_normal);
      }
      if (evt->data.evt_system_external_signal.extsignals
          & EXT_SIGNAL_POSITIONING_TRIGGER) {
        reset_IP_state();
        IP_start_positioning = true;
        app_log("\nIndoor Positioning triggered\n");
      }
      if (evt->data.evt_system_external_signal.extsignals
          & EXT_SIGNAL_GATEWAY_FINDER_TIMEOUT) {
        if (gateway_counter == 0) {
          IP_service_unavailable = true;
        }
        IP_gateway_finding_finished = true;
        app_log("\nGateway finding finished\n");
      }
      break;
  }
}
