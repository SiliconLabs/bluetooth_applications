/***************************************************************************//**
 * @file
 * @brief Core application logic.
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
#include "em_common.h"
#include "sl_app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"

#include "sl_bt_api.h"
#include "sl_app_log.h"
#include "sl_sleeptimer.h"

#include "em_device.h"
#include "em_cmu.h"
#include "em_adc.h"

/**************************************************************************//**
 * Define
 *****************************************************************************/
#define VINATT(ATT_FACTOR) ATT_FACTOR << _ADC_SINGLECTRLX_VINATT_SHIFT
#define VREFATT(ATT_FACTOR)ATT_FACTOR << _ADC_SINGLECTRLX_VREFATT_SHIFT

#define MICROVOLTS_PER_STEP 1221
#define TICKS_PER_SECOND    32768
#define ADC_READ            2
#define BATTERY_READ_INTERVAL  1*TICKS_PER_SECOND
#define REPEATING 0

#define SLEEP_TIMER_TRIGGER  0x01

/**************************************************************************//**
 * Local function
 *****************************************************************************/
static void init_adc_for_supply_measurement(void);
static uint32_t read_adc(void);
static uint32_t read_supply_voltage(void);

static void sleep_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data);


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
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
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
      sl_app_assert(sc == SL_STATUS_OK,
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
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to write attribute\n",
                    (int)sc);

      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to create advertising set\n",
                    (int)sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to set advertising timing\n",
                    (int)sc);
      // Start general advertising and enable connections.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start advertising\n",
                    (int)sc);

      sl_app_log("Start \n");

      /* init adc for reading battery voltage */
      init_adc_for_supply_measurement();

      /* start sleep timer for periodic reading battery voltage */
      sc = sl_sleeptimer_start_periodic_timer_ms(&battery_voltage_sleep_timer,
                                                 1000,
                                                 sleep_timer_callback,
                                                 (void *)NULL,
                                                 0,
                                                 0);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start sleep timer \r\n",
                    (int)sc);

      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Restart advertising after client has disconnected.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start advertising\n",
                    (int)sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    case sl_bt_evt_system_external_signal_id:
      // Update battery voltage when the sleep timer trigger
      if((evt->data.evt_system_external_signal.extsignals & SLEEP_TIMER_TRIGGER) == SLEEP_TIMER_TRIGGER){
          battery_voltage = read_supply_voltage();
          battery_voltage_percentage = (uint8_t) (battery_voltage / 33);
          sl_app_log("battery_voltage: %d (mV)\n", battery_voltage);
          sl_app_log("battery_voltage_percentage: %d %%\n", battery_voltage_percentage);
      }
      break;

    case sl_bt_evt_gatt_server_user_read_request_id:
      if(evt->data.evt_gatt_server_user_read_request.characteristic == gattdb_battery_level) {
          sc = sl_bt_gatt_server_send_user_read_response(evt->data.evt_gatt_server_user_read_request.connection,
                                                         evt->data.evt_gatt_server_user_read_request.characteristic,
                                                         SL_STATUS_OK,
                                                         1, //data length (byte)
                                                         &battery_voltage_percentage,
                                                         &sent_len);
          sl_app_assert(sc == SL_STATUS_OK,
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
 * @brief Initialise ADC. Called after boot in this example.
 */
static void init_adc_for_supply_measurement(void)
{
  CMU_ClockEnable(cmuClock_ADC0, true);
  ADC_Init_TypeDef ADC_Defaults = ADC_INIT_DEFAULT;

  ADC_InitSingle_TypeDef init_single = ADC_INITSINGLE_DEFAULT;
  init_single.negSel = adcNegSelVSS;
  init_single.posSel = adcPosSelAVDD;
  init_single.reference = adcRef5V ;

  // Start with defaults
  ADC_Init(ADC0, &ADC_Defaults);

  ADC_InitSingle(ADC0, &init_single);
  ADC0->SINGLECTRLX = VINATT(12) | VREFATT(6);
}

/**
 * @brief Make one ACD conversion
 * @return Single ADC reading
 */
static uint32_t read_adc(void)
{
  ADC_Start(ADC0, adcStartSingle);
  while ( ( ADC0->STATUS & ADC_STATUS_SINGLEDV ) == 0 );
  return ADC_DataSingleGet(ADC0);
}

/**
  * @brief Read supply voltage raw reading from ADC and return reading in millivolts.
  * @return Supply Voltage in millivolts.
  * VFS = 2*VREF*VREFATTF/VINATTF, where
  * VREF is selected in the VREFSEL bitfield, and
  * VREFATTF (VREF attenuation factor) = (VREFATT+6)/24 when VREFATT is less than 13, and (VREFATT-3)/12 when VREFATT is
  * greater than or equal to 13, and
  * VINATTF (VIN attenuation factor) = VINATT/12, illegal settings: 0, 1, 2
  * VREFATTF = (6+6)/24 = 0.5
  * VINATTF = 12/12
  * VFS = 2*5.0*0.5 = 5.0
  * 1.221 mV/step
*/
static uint32_t read_supply_voltage(void)
{
  uint32_t raw_reading = read_adc();
  uint32_t supply_voltage_mV = raw_reading * MICROVOLTS_PER_STEP / 1000UL;
  return supply_voltage_mV;
}

/**
 *  function: sleep_timer_callback
 *  will be called when the sleep timer expire
 */
void sleep_timer_callback(sl_sleeptimer_timer_handle_t *handle, void __attribute__((unused)) *data)
{
  if(handle == &battery_voltage_sleep_timer){
      // Send the event to bluetooth stack and leave the processing for the event loop
      sl_bt_external_signal(SLEEP_TIMER_TRIGGER);
  }
}
