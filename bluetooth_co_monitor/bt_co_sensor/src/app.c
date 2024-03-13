/***************************************************************************//**
 * @file
 * @brief Core application logic.
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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/
#include <stdio.h>
#include "em_common.h"
#include "app_assert.h"
#include "sl_sleeptimer.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "gatt_db.h"
#include "mikroe_mq7.h"
#include "math.h"

#define APP_EVT_CO_HANDLE                  (1 << 0)
#define NUMBER_SAMPLE                      10
// Advertising flags (common).
#define ADVERTISE_FLAGS_LENGTH             0x02
#define ADVERTISE_FLAGS_TYPE               0x01
#define ADVERTISE_FLAGS_DATA               0x06

// Complete local name.
#define DEVICE_NAME_LENGTH                 12
#define DEVICE_NAME_TYPE                   0x09

// Manufacturer ID (0x02FF - Silicon Labs' company ID)
#define MANUF_ID                           0x02FF
// 1+2+8 bytes for type, company ID and the payload
#define MANUF_LENGTH                       11
#define MANUF_TYPE                         0xFF

#define coefficient_A                      0.14
#define coefficient_B                      -2.26

// ratio: 0.1513
#define R0                                 179.18
#define v_in                               5

// load resister: 20k
#define _LOAD_RES                          20.0

SL_PACK_START(1)
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

  // The next bytes are freely configurable
  // using 4 bytes for counter value and 4 bytes for co level
  uint32_t sample_counter;
  uint32_t co_level;

  // length of the name AD element is variable,
  // adding it last to keep things simple
  uint8_t len_name;
  uint8_t type_name;

  // NAME_MAX_LENGTH must be sized
  // so that total length of data does not exceed 31 bytes
  uint8_t name[DEVICE_NAME_LENGTH + 1];

  // These values are NOT included in the actual advertising payload,
  // just for bookkeeping
  char dummy;        // Space for null terminator
  uint8_t data_size; // Actual length of advertising data
} SL_ATTRIBUTE_PACKED advertising_packet_t;
SL_PACK_END()

/**************************************************************************//**
 * Local variables.
 *****************************************************************************/
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// mac address:
uint8_t mac_address[2];

// Advertising data.
static advertising_packet_t advertising_data = {
  .len_flags = ADVERTISE_FLAGS_LENGTH,
  .type_flags = ADVERTISE_FLAGS_TYPE,
  .val_flags = ADVERTISE_FLAGS_DATA,

  .len_manuf = MANUF_LENGTH,
  .type_manuf = MANUF_TYPE,
  .company_LO = MANUF_ID & 0xFF,
  .company_HI = (MANUF_ID >> 8) & 0xFF,

  // length of name element is the name string length + 1 for the AD type
  .len_name = DEVICE_NAME_LENGTH + 1,
  .type_name = DEVICE_NAME_TYPE,

  // Initialize for custom data
  .sample_counter = 0,
  .co_level = 0,

  // Calculate total length of advertising data
  .data_size = 3 + (1 + MANUF_LENGTH) + (1 + DEVICE_NAME_LENGTH + 1),
};

// Timer handle
static sl_sleeptimer_timer_handle_t co_timer;

// Ppm value unit
static uint32_t value_co_ppm;

/**************************************************************************//**
 * Local function prototypes.
 *****************************************************************************/

// bluetooth event system external signal
static void app_bt_evt_system_external_signal(uint32_t extsignals);

// Handle for CO timer
static void app_co_timer_handle(void);

// CO timer callback
static void app_co_callback(sl_sleeptimer_timer_handle_t *timer, void *data);

// Function to update avertise data
static void update_adv_data(void);

// Function to read ppm value from mq7 sensor
static uint32_t mq7_read_ppm(float voltage);

// Function to read rs (resistance)
static float mq7_read_rs(float voltage);

// Function return Rs/Rl (Rl is R Load)
static float mq7_read_rs_rl(float voltage);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  app_log("BLE Sensor\r\n");

  if (mikroe_mq7_init(IADC0) == SL_STATUS_OK) {
    app_log("Initialize CO sensor successfully\n");
  }

  sl_sleeptimer_start_periodic_timer_ms(&co_timer,
                                        5000,
                                        app_co_callback,
                                        NULL,
                                        0,
                                        0);
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
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

static void app_co_callback(sl_sleeptimer_timer_handle_t *timer, void *data)
{
  (void) timer;
  (void) data;
  sl_bt_external_signal(APP_EVT_CO_HANDLE);
}

static void app_co_timer_handle(void)
{
  float value_voltage;
  float value_voltage_average;
  static float sum = 0;
  static uint8_t counter_sample = 0;

  if (mikroe_mq7_read_an_pin_voltage(&value_voltage)
      != SL_STATUS_OK) {
    app_log("Fail to read data from sensor\r\n");
    return;
  }
  counter_sample++;
  sum += value_voltage;

  if (counter_sample >= NUMBER_SAMPLE) {
    counter_sample = 0;
    value_voltage_average = sum / NUMBER_SAMPLE;
    sum = 0;

    // calculate to ppm
    value_co_ppm = mq7_read_ppm(value_voltage_average);
    app_log("Ppm: %ld\r\n", value_co_ppm);
    update_adv_data();
  }
}

static void update_adv_data(void)
{
  sl_status_t sc;

  // Update the two variable fields in the custom advertising packet
  advertising_data.sample_counter++;
  advertising_data.co_level = value_co_ppm;

  // Set custom advertising payload
  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                        sl_bt_advertiser_advertising_data_packet,
                                        advertising_data.data_size,
                                        (uint8_t *)&advertising_data);

  app_assert_status(sc);
  app_log("The custom advertising packet is updated\r\n");
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

      // Pad and reverse unique ID to get System ID.
      system_id[0] = address.addr[5];
      system_id[1] = address.addr[4];
      system_id[2] = address.addr[3];
      system_id[3] = 0xFF;
      system_id[4] = 0xFE;
      system_id[5] = address.addr[2];
      system_id[6] = address.addr[1];
      system_id[7] = address.addr[0];
      snprintf((char *)(advertising_data.name),
               sizeof(advertising_data.name),
               "CO_S_<%x:%x>",
               system_id[6],
               system_id[7]);

      app_log("CO sensor address: %d:%d:%d:%d:%d:%d\r\n",
              address.addr[0], address.addr[1], address.addr[2],
              address.addr[3], address.addr[4], address.addr[5]);

      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);

      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_scannable_non_connectable);
      app_assert_status(sc);
      break;
    case sl_bt_evt_system_external_signal_id:
      app_bt_evt_system_external_signal(
        evt->data.evt_system_external_signal.extsignals);
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

/***************************************************************************//**
 * CO Application external signal handle.
 ******************************************************************************/
static void app_bt_evt_system_external_signal(uint32_t extsignals)
{
  if (extsignals & APP_EVT_CO_HANDLE) {
    app_co_timer_handle();
  }
}

/**************************************************************************//**
 * Function to read ppm value from mq7 sensor
 *****************************************************************************/
static uint32_t mq7_read_ppm(float voltage)
{
  return (uint32_t) (coefficient_A
                     * pow(mq7_read_rs(voltage) / R0, coefficient_B));
}

/**************************************************************************//**
 * Function to read rs (resistance)
 *****************************************************************************/
static float mq7_read_rs(float voltage)
{
  return _LOAD_RES * mq7_read_rs_rl(voltage);
}

/**************************************************************************//**
 * Function return Rs/Rl (Rl is R Load)
 *****************************************************************************/
static float mq7_read_rs_rl(float voltage)
{
  return (v_in - voltage) / voltage;
}
