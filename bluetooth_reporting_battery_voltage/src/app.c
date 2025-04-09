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
#include "sl_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"

#include "sl_bt_api.h"
#include "app_log.h"
#include "sl_sleeptimer.h"

#include "em_device.h"
#include "em_cmu.h"
#include "em_iadc.h"

/**************************************************************************//**
 * Define
 *****************************************************************************/
#define MICROVOLTS_PER_STEP    1183
#define TICKS_PER_SECOND       32768
#define ADC_READ               2
#define BATTERY_READ_INTERVAL  1 * TICKS_PER_SECOND
#define REPEATING              0

#define SLEEP_TIMER_TRIGGER    0x01

/**************************************************************************//**
 * Local function
 *****************************************************************************/
static void init_adc_for_supply_measurement(void);
static uint32_t read_adc(void);
static uint32_t read_supply_voltage(void);
static void read_supply_status(void);
static void reporting_battery_app_init(void);
static void sleep_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                 void *data);

/**************************************************************************//**
 * Local variable
 *****************************************************************************/
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// Sleep timer handle
sl_sleeptimer_timer_handle_t battery_voltage_sleep_timer;

static uint32_t battery_voltage = 0;
static uint8_t battery_voltage_percentage = 0;

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  reporting_battery_app_init();
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!
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
  uint16_t sent_len;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to get Bluetooth address\n",
                 (int)sc);

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
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to write attribute\n",
                 (int)sc);

      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to create advertising set\n",
                 (int)sc);

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
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to set advertising timing\n",
                 (int)sc);
      // Start general advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(
        advertising_set_handle,
        sl_bt_advertiser_connectable_scannable);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to start advertising\n",
                 (int)sc);
      app_log("Start advertising...\n");
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("Connected opened.\n");
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("Connected closed.\n");
      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(
        advertising_set_handle,
        sl_bt_advertiser_connectable_scannable);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to start advertising\n",
                 (int)sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    case sl_bt_evt_system_external_signal_id:
      // Update battery voltage when the sleep timer trigger
      if ((evt->data.evt_system_external_signal.extsignals
           & SLEEP_TIMER_TRIGGER) == SLEEP_TIMER_TRIGGER) {
        read_supply_status();
      }
      break;

    case sl_bt_evt_gatt_server_user_read_request_id:
      if (evt->data.evt_gatt_server_user_read_request.characteristic
          == gattdb_battery_level) {
        sc = sl_bt_gatt_server_send_user_read_response(
          evt->data.evt_gatt_server_user_read_request.connection,
          evt->data.evt_gatt_server_user_read_request.characteristic,
          SL_STATUS_OK,
          1,
          // data length (byte)
          &battery_voltage_percentage,
          &sent_len);
        app_assert(sc == SL_STATUS_OK,
                   "[E: 0x%04x] Failed to send user read response\n",
                   (int)sc);
      }
      break;
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

/**
 * @brief Initialise IADC. Called after boot in this example.
 */
static void init_adc_for_supply_measurement(void)
{
  CMU_ClockEnable(cmuClock_IADC0, true);

  IADC_Init_t IADC_init_defaults = IADC_INIT_DEFAULT;
  IADC_InitSingle_t IADC_init_single = IADC_INITSINGLE_DEFAULT;

  IADC_AllConfigs_t IADC_allConfigs = IADC_ALLCONFIGS_DEFAULT;
  IADC_SingleInput_t IADC_singleInput = IADC_SINGLEINPUT_DEFAULT;

  IADC_reset(IADC0);

  IADC_singleInput.negInput = iadcNegInputGnd;
  IADC_singleInput.posInput = iadcPosInputAvdd;
  IADC_allConfigs.configs[0].reference = iadcCfgReferenceInt1V2;

  // Start with defaults
  IADC_init(IADC0, &IADC_init_defaults, &IADC_allConfigs);

  IADC_initSingle(IADC0, &IADC_init_single, &IADC_singleInput);
}

/**
 * @brief Make one ADC conversion
 * @return Single ADC reading
 */
static uint32_t read_adc(void)
{
  IADC_command(IADC0, iadcCmdStartSingle);
  while ((IADC0->STATUS & IADC_STATUS_SINGLEFIFODV) == 0) {}
  return IADC_readSingleData(IADC0);
}

/**
 * @brief Read supply voltage raw reading from ADC and return reading in
 *   millivolts.
 * @return Supply Voltage in millivolts.
 * The IADC has an internal 1.21V bandgap reference voltage that is
 * independent of the chip's voltage supplies, and it's capable of
 * attenuating the supply voltage input by a factor of 4, meaning we must
 * multiply the raw data by (4 * 1210) to accommodate. We further divide
 * the value by 4095 (by default, series 2 devices' IADC are 12-bits) to
 * get the desired MICROVOLTS_PER_STEP
 * 1183 mV/step
 */
static uint32_t read_supply_voltage(void)
{
  uint32_t raw_reading = read_adc();
  uint32_t supply_voltage_mV = raw_reading * MICROVOLTS_PER_STEP / 1000UL;
  return supply_voltage_mV;
}

/**
 *  function: sleep_timer_callback
 *  will be called when the sleep timer expires
 */
void sleep_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                          void __attribute__((unused)) *data)
{
  if (handle == &battery_voltage_sleep_timer) {
    // Send the event to bluetooth stack and leave the processing for the event
    //   loop
    sl_bt_external_signal(SLEEP_TIMER_TRIGGER);
  }
}

/**
 * @brief Get the battery voltage status,
 *  including battery capacity and battery percent
 */
static void read_supply_status(void)
{
  battery_voltage = read_supply_voltage();
  battery_voltage_percentage = (uint8_t) (battery_voltage / 33);
  app_log("Battery voltage: %d (mV)\n", (int)battery_voltage);
  app_log("Battery voltage percentage: %d %%\n",
          (int)battery_voltage_percentage);
}

/**
 * @brief Initialize for application.
 *
 */
static void reporting_battery_app_init(void)
{
  sl_status_t sc;

  /* init adc for reading battery voltage */
  init_adc_for_supply_measurement();

  /* start sleep timer for periodic reading battery voltage */
  sc = sl_sleeptimer_start_periodic_timer_ms(&battery_voltage_sleep_timer,
                                             1000,
                                             sleep_timer_callback,
                                             (void *)NULL,
                                             0,
                                             0);
  app_assert(sc == SL_STATUS_OK,
             "[E: 0x%04x] Failed to start sleep timer \r\n",
             (int)sc);
}
