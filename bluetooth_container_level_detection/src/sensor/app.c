/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
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
#include "sl_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"

#include "sl_sleeptimer.h"
#include "sl_i2cspm_instances.h"
#include "app_log.h"

#include "sparkfun_vl53l1x.h"
#include "sparkfun_vl53l1x_config.h"

#define C_LVL_SENSOR_TIMER_EVENT           (1 << 0)
#define NUMBER_SAMPLE                      10
// Advertising flags (common).
#define ADVERTISE_FLAGS_LENGTH             0x02
#define ADVERTISE_FLAGS_TYPE               0x01
#define ADVERTISE_FLAGS_DATA               0x06

// Complete local name.
#define DEVICE_NAME_LENGTH                 14
#define DEVICE_NAME_TYPE                   0x09

// Manufacturer ID (0x02FF - Silicon Labs' company ID)
#define MANUF_ID                           0x02FF
// 1+2+8 bytes for type, company ID and the payload
#define MANUF_LENGTH                       11
#define MANUF_TYPE                         0xFF

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
  uint32_t distance;

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
  .distance = 0,

  // Calculate total length of advertising data
  .data_size = 3 + (1 + MANUF_LENGTH) + (1 + DEVICE_NAME_LENGTH + 1),
};

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static sl_sleeptimer_timer_handle_t app_distance_timer;
static uint16_t arr_result[10];
static uint16_t sum = 0;
static uint8_t sample_counter = 0;

static void app_distance_init(void);

// VL53 Timer callback function
static void app_distance_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                        void *data);
static void container_level_detection_sensor_event_handler(void);
static void calculate_moving_average(void);
static void update_adv_data(uint16_t distance, uint16_t sample_counter);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_status_t sc;

  app_distance_init();
  calculate_moving_average();

  // Setup a periodic sleep timer with 1000 ms time period
  sc = sl_sleeptimer_start_periodic_timer_ms(&app_distance_timer,
                                             1000,
                                             app_distance_timer_callback,
                                             NULL,
                                             0,
                                             0);
  app_assert_status(sc);
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
               "CON_LEV_S_%x%x",
               system_id[6],
               system_id[7]);

      app_log("Container level sensor address: %d:%d:%d:%d:%d:%d\r\n",
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
        160,  // min. adv. interval (milliseconds * 1.6)
        160,  // max. adv. interval (milliseconds * 1.6)
        0,    // adv. duration
        0);   // max. num. adv. events
      app_assert_status(sc);

      // Set custom advertising payload
      sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                            sl_bt_advertiser_advertising_data_packet,
                                            advertising_data.data_size,
                                            (uint8_t *)&advertising_data);
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_scannable_non_connectable);
      app_assert_status(sc);
      app_log("Start advertising ...\n");
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_scannable_non_connectable);
      app_assert_status(sc);
      break;

    case sl_bt_evt_system_external_signal_id:
      container_level_detection_sensor_event_handler();
      break;
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

static void app_distance_init(void)
{
  uint8_t sensor_state = 0;
  sl_status_t vl53_status = SL_STATUS_OK;

  app_log("==== VL53L1X Test application ====\n");
  app_log("app_init function called...\n");

  // Initialize the sensor with the default settings
  vl53_status = vl53l1x_init(VL53L1X_ADDR, sl_i2cspm_qwiic);

  // Waiting for device to boot up...
  while (0 == sensor_state) {
    // Read sensor's state (0 = boot state, 1 = device is booted )
    vl53_status = vl53l1x_get_boot_state(VL53L1X_ADDR, &sensor_state);

    if (SL_STATUS_OK != vl53_status) {
      break;
    }

    // Wait for 2 ms
    sl_sleeptimer_delay_millisecond(2);
  }

  if (SL_STATUS_OK == vl53_status) {
    app_log("Platform I2C communication is OK.\n");
  } else {
    app_log("Platform I2C communication test has been failed.\n"
            "Please check the I2C bus connection and "
            "the I2C (I2CSPM) configuration.\n");
    return;
  }

  app_log("VL53L1X booted\n");

  // Optional sensor configuration example function calls, see API documentation
  //   for options
  {
    // Select distance mode
    vl53_status = vl53l1x_set_distance_mode(VL53L1X_ADDR,
                                            VL53L1X_DISTANCE_MODE_LONG);
    // in ms possible values [20,50,100,200,500]
    vl53_status = vl53l1x_set_timing_budget_in_ms(VL53L1X_ADDR, 100);

    // in ms, IM must be >= TB
    vl53_status = vl53l1x_set_inter_measurement_in_ms(VL53L1X_ADDR, 200);

    vl53_status = vl53l1x_set_roi_xy(VL53L1X_ADDR, 16, 16); // min. ROI is 4,4
  }

  // Check return codes of the optional configuration function calls
  if (SL_STATUS_OK == vl53_status) {
    app_log("Sensor initialization and configuration are done.\n");
  } else {
    app_log("Sensor initialization and configuration has been failed.\n");
    return;
  }

  // Start ranging
  vl53_status = vl53l1x_start_ranging(VL53L1X_ADDR);

  // Check ranging status
  if (SL_STATUS_OK == vl53_status) {
    app_log("VL53L1X ranging has been started ...\n");
  }
}

static void app_distance_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                        void *data)
{
  (void) handle;
  (void) data;
  sl_bt_external_signal(C_LVL_SENSOR_TIMER_EVENT);
}

static void container_level_detection_sensor_event_handler(void)
{
  sl_status_t sc = SL_STATUS_OK;
  uint8_t is_data_ready = 0;
  vl53l1x_result_t raw_result;
  uint16_t result = 0;

  // Check measurement data status
  sc = vl53l1x_check_for_data_ready(VL53L1X_ADDR, &is_data_ready);

  if (0 != is_data_ready) {
    // Measurement data is ready to read from the sensor
    sc = vl53l1x_get_result(VL53L1X_ADDR, &raw_result);
    sum = (sum - arr_result[sample_counter] + raw_result.distance);
    arr_result[sample_counter] = raw_result.distance;
    result = sum / NUMBER_SAMPLE;
    update_adv_data(result, sample_counter);
    sample_counter++;
    // Clear sensor's interrupt status
    sc = vl53l1x_clear_interrupt(VL53L1X_ADDR);
    // Print result
    app_log(" > Distance: %4u mm, sample counter: %d, sum: %d\n",
            raw_result.distance,
            sample_counter,
            sum);
  }

  if (sample_counter >= NUMBER_SAMPLE) {
    sample_counter = 0;
  }

  if (SL_STATUS_OK != sc) {
    app_log("VL53L1X sensor operation has been failed "
            "during the periodic distancing.\n");
  }
}

static void calculate_moving_average(void)
{
  sl_status_t sc = SL_STATUS_OK;
  uint8_t is_data_ready = 0;
  vl53l1x_result_t result;
  uint16_t value_distance_average = 0;

  sample_counter = 0;
  while (sample_counter < NUMBER_SAMPLE)
  {
    // Check measurement data status
    sc = vl53l1x_check_for_data_ready(VL53L1X_ADDR, &is_data_ready);
    if (is_data_ready != 0) {
      // Measurement data is ready to read from the sensor
      sc = vl53l1x_get_result(VL53L1X_ADDR, &result);
      app_assert_status(sc);
      arr_result[sample_counter] = result.distance;
      sum += result.distance;
      sample_counter++;
    }
  }

  value_distance_average = sum / sample_counter;
  sample_counter = 0;
  app_log("Distance average: %d\n", value_distance_average);
}

static void update_adv_data(uint16_t distance, uint16_t sample_counter)
{
  sl_status_t sc;
  advertising_data.distance = (uint32_t) distance;
  advertising_data.sample_counter = (uint32_t)sample_counter;
  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                        sl_bt_advertiser_advertising_data_packet,
                                        advertising_data.data_size,
                                        (uint8_t *)&advertising_data);
  app_assert_status(sc);
}
