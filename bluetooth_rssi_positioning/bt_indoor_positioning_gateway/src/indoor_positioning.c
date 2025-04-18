/***************************************************************************//**
 * @file indoor_positioning.c
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
// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#include "indoor_positioning.h"

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------

// The advertising set handle allocated from Bluetooth stack.
static uint8_t configMode_advertising_set_handle = 0xff;
static uint8_t normalMode_advertising_set_handle = 0xff;

// Handle for SleepTimer
static sl_sleeptimer_timer_handle_t config_mode_timeout_timer;

// User defined advertising data
static custom_advert_t custom_advert;

// Configuration data
static IPGW_config_data_t IPGW_config_data;

// flags
static bool IP_config_mode = false;
static bool IP_BT_init_done = false;
static bool IP_mode_selected = false;

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                              Callbacks
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * Callback for configuration mode timeout timer
 * Resets the system
 *******************************************************************************/
void config_mode_timeout_cb()
{
  app_log("\nConfiguration mode timeout - Reset \n");
  sl_bt_system_reboot();
}

// -----------------------------------------------------------------------------
//                                 Advertising
// -----------------------------------------------------------------------------

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

  pData->len_manuf = 16;  // 1+2+13 bytes for type, company ID and the payload
  pData->type_manuf = 0xFF;
  pData->company_LO = companyID & 0xFF;
  pData->company_HI = (companyID >> 8) & 0xFF;

  pData->network_Uid = IPGW_config_data.network_Uid;
  pData->room_id = IPGW_config_data.room_id;

  strncpy(pData->room_name, IPGW_config_data.room_name, ROOM_NAME_LENGTH);

  // Name length, excluding null terminator
  n = strlen(name);
  if (n > DEVICENAME_LENGTH) {
    // Incomplete name
    pData->type_name = 0x08;
  } else {
    pData->type_name = 0x09;
  }

  strncpy(pData->name, name, DEVICENAME_LENGTH);

  if (n > DEVICENAME_LENGTH) {
    n = DEVICENAME_LENGTH;
  }

  // length of name element is the name string length + 1 for the AD type
  pData->len_name = 1 + n;

  // Calculate total length of advertising data
  pData->data_size = 3 + (1 + pData->len_manuf) + (1 + pData->len_name);
}

// -----------------------------------------------------------------------------
//                          Indoor Positioning - Gateway
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * Enters configuration mode
 * prints out current configuration parameters
 * sets up and starts advertisements
 * starts configuration mode timeout timer
 *******************************************************************************/
void enter_config_mode(void)
{
  sl_status_t sc;
  IP_config_mode = true;

  // Set up advertisement
  sc = sl_bt_advertiser_create_set(&configMode_advertising_set_handle);
  app_assert_status(sc);

  sc = sl_bt_gatt_server_write_attribute_value(gattdb_device_name,
                                               0,
                                               DEVICENAME_LENGTH,
                                               (const uint8_t *) IPGW_config_data.device_name);
  app_assert_status(sc);

  // Generate data for advertising
  sc = sl_bt_legacy_advertiser_generate_data(configMode_advertising_set_handle,
                                             sl_bt_advertiser_general_discoverable);
  app_assert_status(sc);

  sc = sl_bt_advertiser_set_timing(configMode_advertising_set_handle,
                                   100, // min. adv. interval (milliseconds * 1.6)
                                   160, // max. adv. interval (milliseconds * 1.6)
                                   0,   // adv. duration
                                   0);  // max. num. adv. events
  app_assert_status(sc);

  // Start general advertising and enable connections.
  sc = sl_bt_legacy_advertiser_start(configMode_advertising_set_handle,
                                     sl_bt_advertiser_connectable_scannable);
  app_assert_status(sc);

  // Start config mode timeout
  sl_sleeptimer_start_timer_ms(&config_mode_timeout_timer,
                               CONFIG_MODE_TIMEOUT_MS,
                               config_mode_timeout_cb,
                               (void *) NULL,
                               0,
                               0);
}

/***************************************************************************//**
 * Enters Normal mode
 * Sets up and starts custom advertisements
 *******************************************************************************/
void enter_normal_mode(void)
{
  sl_status_t sc;
  IP_config_mode = false;

  // Set up advertisement
  sc = sl_bt_advertiser_create_set(&normalMode_advertising_set_handle);
  app_assert_status(sc);

  sc = sl_bt_advertiser_set_timing(
    normalMode_advertising_set_handle,
    100, // min. adv. interval (milliseconds * 1.6)
    160, // max. adv. interval (milliseconds * 1.6)
    0,   // adv. duration
    0);  // max. num. adv. events
  app_assert_status(sc);

  // Create user defined advertising packet
  create_custom_advert_package(&custom_advert,
                               0x06,
                               COMPANY_ID,
                               IPGW_config_data.device_name);

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
 * Called periodically by the application
 * Handles periodic Indoor Positioning related tasks
 *******************************************************************************/
void IPGW_step(void)
{
  // Check button status once bluetooth stack is initialized and neither mode is
  //   selected yet
  if (IP_BT_init_done && (false == IP_mode_selected)) {
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
}

/***************************************************************************//**
 * Initialize Indoor Positioning service
 *******************************************************************************/
void IPGW_init(void)
{
  app_log("\nIndoor Positioning - Gateway\n");
  app_log("Loading Configuration\n");
  update_local_config();

  // Print current configuration parameters
  app_log("NetworkU_ID: %ld | Room name: %s\n",
          IPGW_config_data.network_Uid,
          IPGW_config_data.room_name);
}

// -----------------------------------------------------------------------------
//                    Non-volatile memory related functions
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

  memset(&IPGW_config_data, 0, sizeof(IPGW_config_data));
  num_of_objects = nvm3_countObjects(nvm3_defaultHandle);
  if (num_of_objects != 0) {
    // Read stored configuration from NVM
    for (uint16_t config_key = 0; config_key < IPGW_config_num_of_keys;
         config_key++) {
      nvm3_getObjectInfo(nvm3_defaultHandle,
                         config_key,
                         &object_type,
                         &temp_data_length);
      if (object_type == NVM3_OBJECTTYPE_DATA) {
        switch (config_key) {
          case IPGW_config_key_network_UID:
            entry_ptr = (uintptr_t *) &IPGW_config_data.network_Uid;
            valid_entry = true;
            break;
          case IPGW_config_key_room_id:
            entry_ptr = (uintptr_t *) &IPGW_config_data.room_id;
            valid_entry = true;
            break;
          case IPGW_config_key_room_name:
            entry_ptr = (uintptr_t *) &IPGW_config_data.room_name;
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
  uint8_t temp_value_array[10];
  bool valid_entry = false;

  for (uint16_t config_key = 0; config_key < IPGW_config_num_of_keys;
       config_key++) {
    switch (config_key) {
      case IPGW_config_key_network_UID:
        gatt_db_id = gattdb_NetworkUID;
        value_size = sizeof(IPGW_config_data.network_Uid);
        memcpy(temp_value_array, &IPGW_config_data.network_Uid, value_size);
        valid_entry = true;
        break;
      case IPGW_config_key_room_id:
        gatt_db_id = gattdb_RoomId;
        value_size = sizeof(IPGW_config_data.room_id);
        memcpy(temp_value_array, &IPGW_config_data.room_id, value_size);
        valid_entry = true;
        break;
      case IPGW_config_key_room_name:
        gatt_db_id = gattdb_RoomName;
        value_size = ROOM_NAME_LENGTH;
        memcpy(temp_value_array, &IPGW_config_data.room_name, value_size);
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
//                              Event Handler
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * Indoor Positioning related BT event handler
 *******************************************************************************/
void IPGW_event_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];
  IPGW_config_keys_enum_t nvm_key = 0;

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
      sprintf(IPGW_config_data.device_name,
              "IPGW_%.2X%.2X",
              address.addr[0],
              address.addr[1]);
      app_log("Device unique name: %s\n", IPGW_config_data.device_name);

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

      /* Previous bond is broken, delete it and close connection, host must
       *   retry at least once */
      sl_bt_sm_delete_bondings();
      break;

    case sl_bt_evt_gatt_server_attribute_value_id:
      if (evt->data.evt_gatt_server_attribute_value.value.len <= 10) {
        uint8_t request_data[evt->data.evt_gatt_server_attribute_value.value.len
        ];

        app_log("Configuration updated\n");
        switch (evt->data.evt_gatt_server_attribute_value.attribute) {
          case gattdb_NetworkUID:
            nvm_key = IPGW_config_key_network_UID;
            break;
          case gattdb_RoomId:
            nvm_key = IPGW_config_key_room_id;
            break;
          case gattdb_RoomName:
            nvm_key = IPGW_config_key_room_name;
            break;
        }
        for (uint8_t i = 0;
             i < evt->data.evt_gatt_server_attribute_value.value.len; i++) {
          request_data[i] =
            evt->data.evt_gatt_server_attribute_value.value.data[i];
        }

        // Store new value in NVM
        nvm3_writeData(nvm3_defaultHandle,
                       nvm_key,
                       request_data,
                       evt->data.evt_gatt_server_attribute_value.value.len);

        // Update current configuration values
        update_local_config();

        // Reset configuration mode timeout as there was a user action
        sc = sl_sleeptimer_restart_timer_ms(&config_mode_timeout_timer,
                                            CONFIG_MODE_TIMEOUT_MS,
                                            config_mode_timeout_cb,
                                            (void *) NULL,
                                            0,
                                            0);
        app_assert_status(sc);
      }
      break;
  }
}
