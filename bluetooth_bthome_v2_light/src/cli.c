/***************************************************************************//**
 * @file
 * @brief cli bare metal examples functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include <string.h>
#include "sl_bt_api.h"
#include "sl_cli.h"
#include "sl_cli_instances.h"
#include "sl_cli_arguments.h"
#include "sl_cli_handles.h"

#include "bthome_v2_server_config.h"
#include "bthome_v2_server_nvm3.h"
#include "bthome_v2_server.h"
#include "app_log.h"
#include "cli.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
#define MAX_INTERESTED_DEVICE   5

/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/
void scan(sl_cli_command_arg_t *arguments);
void key_register(sl_cli_command_arg_t *arguments);
void key_remove(sl_cli_command_arg_t *arguments);
void key_get(sl_cli_command_arg_t *arguments);
void device_list(sl_cli_command_arg_t *arguments);
void interested_add(sl_cli_command_arg_t *arguments);
void interested_remove(sl_cli_command_arg_t *arguments);
void light_system(sl_cli_command_arg_t *arguments);

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
static uint8_t interested_device[MAX_INTERESTED_DEVICE][6];
static uint8_t interested_count = 0;

static uint8_t registered_device[MAX_ENCRYPT_DEVICE][6];
static uint8_t registered_count = 0;

// timer to read data periodically
static sl_sleeptimer_timer_handle_t read_timer;
// timer callback
static void read_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                void *data);

static const sl_cli_command_info_t cmd__scan = \
  SL_CLI_COMMAND(scan,
                 "Scan BLE network and find BTHome devices",
                 "instruction: start or stop",
                 { SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__key_register = \
  SL_CLI_COMMAND(key_register,
                 "Register encryption key by MAC",
                 "parameter: MAC in string"SL_CLI_UNIT_SEPARATOR "parameter: Key in string",
                 { SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__key_remove = \
  SL_CLI_COMMAND(key_remove,
                 "Remove encryption key by MAC",
                 "parameter: MAC in string",
                 { SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__key_get = \
  SL_CLI_COMMAND(key_get,
                 "Get encryption key by MAC",
                 "parameter: MAC in string",
                 { SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__list = \
  SL_CLI_COMMAND(device_list,
                 "List all devices that already have encryption key",
                 "",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__interested_add = \
  SL_CLI_COMMAND(interested_add,
                 "Add device to interested list",
                 "parameter: MAC in string",
                 { SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__interested_remove = \
  SL_CLI_COMMAND(interested_remove,
                 "Remove device to interested list",
                 "parameter: MAC in string",
                 { SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cmd__light_system = \
  SL_CLI_COMMAND(light_system,
                 "Control light system",
                 "instruction: start or stop",
                 { SL_CLI_ARG_WILDCARD, SL_CLI_ARG_END, });

static sl_cli_command_entry_t a_table[] = {
  { "scan", &cmd__scan, false },
  { "key_register", &cmd__key_register, false },
  { "key_remove", &cmd__key_remove, false },
  { "key_get", &cmd__key_get, false },
  { "list", &cmd__list, false },
  { "interested_add", &cmd__interested_add, false },
  { "interested_remove", &cmd__interested_remove, false },
  { "light_system", &cmd__light_system, false },
  { NULL, NULL, false },
};

static sl_cli_command_group_t a_group = {
  { NULL },
  false,
  a_table
};

/*******************************************************************************
 *************************  EXPORTED VARIABLES   *******************************
 ******************************************************************************/

sl_cli_command_group_t *command_group = &a_group;

/*******************************************************************************
 ***************************   LOCAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Callback for scan network.
 *
 * This function is used as a callback when the scan command is called
 * in the cli.
 ******************************************************************************/
void scan(sl_cli_command_arg_t *arguments)
{
  char *instruction;

  app_log("<<scan network>>\r\n");

  // Get the instruction provided
  instruction = sl_cli_get_argument_string(arguments, 0);

  if (strcmp(instruction, "start") == 0) {
    // scan start instruction provided
    bthome_v2_server_start_scan_network();
  } else if (strcmp(instruction, "stop") == 0) {
    // scan stop instruction provided
    sl_bt_scanner_stop();
  } else {
    // valid instruction provided
    app_log("Incorrect instruction. Please use start or stop\r\n");
  }
}

/***************************************************************************//**
 * Callback register encryption key.
 *
 * This function is used as a callback when the key_register command is called
 * in the cli.
 ******************************************************************************/
void key_register(sl_cli_command_arg_t *arguments)
{
  char *mac_str;
  char *key_str;
  char octet[2];
  uint8_t mac[6];
  uint8_t key[16];

  app_log("<<key register>>\r\n");

  // Get the parameter provided
  mac_str = sl_cli_get_argument_string(arguments, 0);
  key_str = sl_cli_get_argument_string(arguments, 1);

  // Parse mac to 6 byte length format
  for (uint8_t i = 0; i < 6; i++) {
    memcpy(octet, (uint8_t *)&mac_str[2 * i], 2);
    mac[i] = (uint8_t)strtol(octet, NULL, 16);
  }

  // Check the registered list if device is available
  if (registered_count == (MAX_ENCRYPT_DEVICE - 1)) {
    app_log("List of registered device is full\r\n");
    return;
  }

  for (uint8_t i = 0; i < registered_count; i++) {
    if (memcmp(registered_device[i], mac, 6) == 0) {
      app_log("Device already exists\r\n");
      return;
    }
  }
  // Add to registered list
  if (registered_count < (MAX_ENCRYPT_DEVICE - 1)) {
    for (uint8_t i = 0; i < 6; i++) {
      registered_device[registered_count][i] = mac[i];
    }
    registered_count++;
  }

  // Parse key to 16 byte length format
  for (uint8_t i = 0; i < 16; i++) {
    memcpy(octet, (uint8_t *)&key_str[2 * i], 2);
    key[i] = (uint8_t)strtol(octet, NULL, 16);
  }

  bthome_v2_server_key_register(mac, key);
}

/***************************************************************************//**
 * Callback remove encryption key.
 *
 * This function is used as a callback when the key_remove command is called
 * in the cli.
 ******************************************************************************/
void key_remove(sl_cli_command_arg_t *arguments)
{
  char *mac_str;
  char octet[2];
  uint8_t mac[6];
  uint8_t index = MAX_ENCRYPT_DEVICE;

  app_log("<<key remove>>\r\n");

  // Get the mac provided
  mac_str = sl_cli_get_argument_string(arguments, 0);

  // Parse mac to 6 byte length format
  for (uint8_t i = 0; i < 6; i++) {
    memcpy(octet, (uint8_t *)&mac_str[2 * i], 2);
    mac[i] = (uint8_t)strtol(octet, NULL, 16);
  }

  // Find index of device in registered list
  for (uint8_t i = 0; i < registered_count; i++) {
    if (memcmp(registered_device[i], mac, 6) == 0) {
      index = i;
      break;
    }
  }

  if (index == MAX_ENCRYPT_DEVICE) {
    app_log("Device not exists\r\n");
    return;
  }

  // Remove and rearrange device
  for (uint8_t i = index; i < registered_count; i++) {
    memcpy(registered_device[i], registered_device[i + 1], 6);
  }
  for (uint8_t j = 0; j < 6; j++) {
    registered_device[registered_count][j] = 0;
  }

  registered_count--;
  bthome_v2_server_key_remove(mac);
}

/***************************************************************************//**
 * Callback get encryption key.
 *
 * This function is used as a callback when the key_get command is called
 * in the cli.
 ******************************************************************************/
void key_get(sl_cli_command_arg_t *arguments)
{
  char *mac_str;
  char octet[2];
  uint8_t mac[6];
  uint8_t key[16];

  app_log("<<key get>>\r\n");

  // Get the mac provided
  mac_str = sl_cli_get_argument_string(arguments, 0);

  // Parse mac to 6 byte length format
  for (uint8_t i = 0; i < 6; i++) {
    memcpy(octet, (uint8_t *)&mac_str[2 * i], 2);
    mac[i] = (uint8_t)strtol(octet, NULL, 16);
  }

  bthome_v2_server_key_get(mac, key);
  app_log("->bind key: ");
  for (uint8_t i = 0; i < 16; i++) {
    app_log("%.2x ", key[i]);
  }
  app_log("\r\n");
}

/***************************************************************************//**
 * Callback list all devices store with encryption key.
 *
 * This function is used as a callback when the list command is called
 * in the cli.
 ******************************************************************************/
void device_list(sl_cli_command_arg_t *arguments)
{
  sl_status_t sc;
  bthome_v2_server_data_t tmp;
  (void)arguments;

  app_log("<<device list>>\r\n");

  for (uint8_t i = 0; i < MAX_ENCRYPT_DEVICE; i++) {
    sc = bthome_v2_server_nvm3_read(i, tmp.data);
    if (sc == SL_STATUS_OK) {
      app_log("->MAC: ");
      for (uint8_t j = 0; j < 6; j++) {
        app_log("%.2x ", tmp.bthome_nvm3.mac[j]);
      }
      app_log("\r\n");

      app_log("  bind key: ");
      for (uint8_t j = 0; j < 16; j++) {
        app_log("%.2x ", tmp.bthome_nvm3.key[j]);
      }
      app_log("\r\n");
    }
  }
}

/***************************************************************************//**
 * Callback add device to to the interested devices.
 *
 * This function is used as a callback when the interested_add command is called
 * in the cli.
 ******************************************************************************/
void interested_add(sl_cli_command_arg_t *arguments)
{
  char *mac_str;
  char octet[2];
  uint8_t mac[6];

  app_log("<<interested list add>>\r\n");

  // Get the MAC provided
  mac_str = sl_cli_get_argument_string(arguments, 0);

  // Parse MAC to 6 byte length format
  for (uint8_t i = 0; i < 6; i++) {
    memcpy(octet, (uint8_t *)&mac_str[2 * i], 2);
    mac[i] = (uint8_t)strtol(octet, NULL, 16);
  }

  if (interested_count == (MAX_INTERESTED_DEVICE - 1)) {
    app_log("List of interested device is full\r\n");
    return;
  }

  for (uint8_t i = 0; i < interested_count; i++) {
    if (memcmp(interested_device[i], mac, 6) == 0) {
      app_log("Device already exists\r\n");
      return;
    }
  }
  // Add to interested list
  if (interested_count < (MAX_INTERESTED_DEVICE - 1)) {
    for (uint8_t i = 0; i < 6; i++) {
      interested_device[interested_count][i] = mac[i];
    }
    interested_count++;
  }
}

/***************************************************************************//**
 * Callback remove device from the interested devices.
 *
 * This function is used as a callback when the interested_remove command is
 * called in the cli.
 ******************************************************************************/
void interested_remove(sl_cli_command_arg_t *arguments)
{
  char *mac_str;
  char octet[2];
  uint8_t mac[6];
  uint8_t index = MAX_INTERESTED_DEVICE;

  app_log("<<interested list remove>>\r\n");

  // Get the mac provided
  mac_str = sl_cli_get_argument_string(arguments, 0);

  // Parse mac to 6 byte length format
  for (uint8_t i = 0; i < 6; i++) {
    memcpy(octet, (uint8_t *)&mac_str[2 * i], 2);
    mac[i] = (uint8_t)strtol(octet, NULL, 16);
  }

  // Find index of device in interested list
  for (uint8_t i = 0; i < interested_count; i++) {
    if (memcmp(interested_device[i], mac, 6) == 0) {
      index = i;
      break;
    }
  }

  if (index == MAX_INTERESTED_DEVICE) {
    app_log("Device not exists\r\n");
    return;
  }

  // Remove and rearrange device
  for (uint8_t i = index; i < interested_count; i++) {
    memcpy(interested_device[i], interested_device[i + 1], 6);
  }
  for (uint8_t j = 0; j < 6; j++) {
    interested_device[interested_count][j] = 0;
  }

  interested_count--;
}

/***************************************************************************//**
 * Callback for light system.
 *
 * This function is used as a callback when the scan command is called
 * in the cli.
 ******************************************************************************/
void light_system(sl_cli_command_arg_t *arguments)
{
  char *instruction;

  app_log("<<light system>>\r\n");

  // Get the instruction provided
  instruction = sl_cli_get_argument_string(arguments, 0);

  if (strcmp(instruction, "start") == 0) {
    // start read timer
    sl_sleeptimer_start_periodic_timer_ms(&read_timer,
                                          300,
                                          read_timer_callback,
                                          NULL, 0, 0);
  } else if (strcmp(instruction, "stop") == 0) {
    // scan stop instruction provided
    sl_sleeptimer_stop_timer(&read_timer);
  } else {
    // valid instruction provided
    app_log("Incorrect instruction. Please use start or stop\r\n");
  }
}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/
uint8_t get_interested_device_count(void)
{
  return interested_count;
}

void get_interested_device_mac(uint8_t index, uint8_t *mac)
{
  memcpy(mac, interested_device[index], 6);
}

uint8_t get_registered_device_count(void)
{
  return registered_count;
}

void get_registered_device_mac(uint8_t index, uint8_t *mac)
{
  memcpy(mac, registered_device[index], 6);
}

/*******************************************************************************
 * Initialize cli example.
 ******************************************************************************/
void cli_app_init(void)
{
  bool status;

  status = sl_cli_command_add_command_group(sl_cli_inst_handle, command_group);
  EFM_ASSERT(status);

  app_log("\r\n------ Silicon Labs BThome v2 Light Example-----\r\n");
}

static void read_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                void *data)
{
  (void)handle;
  (void)data;

  sl_bt_external_signal(SIGNAL_READ_DATA);
}
