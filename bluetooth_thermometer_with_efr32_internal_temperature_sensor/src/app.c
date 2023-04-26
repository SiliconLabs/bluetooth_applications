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
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "app_log.h"

#include "em_cmu.h"
#include "em_adc.h"

/***************************************************************************//**
 *  Macros for type conversion
 ******************************************************************************/
#define UINT8_TO_BITSTREAM(p, n)      { *(p)++ = (uint8_t)(n); }
#define FLT_TO_UINT32(m, e)           (((uint32_t)(m) & 0x00FFFFFFU) \
                                       | (uint32_t)((uint32_t)(e) << 24))
#define UINT32_TO_BITSTREAM(p, n)     { *(p)++ = (uint8_t)(n);         \
                                        *(p)++ = (uint8_t)((n) >> 8);  \
                                        *(p)++ = (uint8_t)((n) >> 16); \
                                        *(p)++ = (uint8_t)((n) >> 24); }

/* Factory calibration temperature from device information page. */
#define CAL_TEMP0  ((DEVINFO->CAL &_DEVINFO_CAL_TEMP_MASK) >> \
                    _DEVINFO_CAL_TEMP_SHIFT)

/* _DEVINFO_ADC0CAL3_TEMPREAD1V25_MASK is not correct in
 *    current CMSIS. This is a 12-bit value, not 16-bit. */
#define CAL_VALUE0 ((DEVINFO->ADC0CAL3 &_DEVINFO_ADC0CAL3_TEMPREAD1V25_MASK) >> \
                    _DEVINFO_ADC0CAL3_TEMPREAD1V25_SHIFT)
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
// Connection Handle
static uint8_t connection_handle = 0xff;

static void init_adc(void);
static uint32_t read_adc(void);
static int32_t convert_to_millicelsius(int32_t adc_sample);
static void measure_temperature(void);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////

  init_adc();
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

      sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id,
                                                   0,
                                                   sizeof(system_id),
                                                   system_id);
      app_assert_status(sc);

      app_log("\nBLE stack booted\r\nStack version: %d.%d.%d\r\n",
              evt->data.evt_system_boot.major,
              evt->data.evt_system_boot.minor,
              evt->data.evt_system_boot.patch);
      app_log("Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\n",
              address_type ? "static random" : "public device",
              address.addr[5],
              address.addr[4],
              address.addr[3],
              address.addr[2],
              address.addr[1],
              address.addr[0]);

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
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to set advertising timing\n",
                 (int)sc);
      // Start general advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(
        advertising_set_handle,
        sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);

      app_log("boot event - starting advertising\r\n");
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("Connection opened\r\n");
      // Save connection handle for future reference
      connection_handle = evt->data.evt_connection_opened.connection;
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("connection closed, reason: 0x%2.2x\r\n",
              evt->data.evt_connection_closed.reason);
      connection_handle = 0xff;

      /* Stop timer in case client disconnected before indications were turned
       *   off */
      sc = sl_bt_system_set_lazy_soft_timer(0, 0, 0, 0);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to stop a software timer\n",
                 (int)sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(
        advertising_set_handle,
        sl_bt_advertiser_connectable_scannable);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to start advertising\n",
                 (int)sc);
      break;

    /* This event is generated when a connected client has either
     * 1) changed a Characteristic Client Configuration, meaning that
     * they have enabled or disabled Notifications or Indications, or
     * 2) sent a confirmation upon a successful reception of the indication. */
    case sl_bt_evt_gatt_server_characteristic_status_id:
      /* Check that the characteristic in question is temperature -
       * its ID is defined in gatt_configuration.btconf as
       *   "temperature_measurement".
       * Also check that status_flags = 1, meaning that the characteristic
       * client configuration was changed (notifications or indications
       * enabled or disabled). */
      if (evt->data.evt_gatt_server_characteristic_status.status_flags
          != gatt_server_client_config) {
        break;
      }
      if (evt->data.evt_gatt_server_characteristic_status.characteristic
          != gattdb_temperature_measurement) {
        break;
      }

      if (evt->data.evt_gatt_server_characteristic_status.client_config_flags
          == gatt_indication) {
        /* Indications have been turned ON - start the repeating timer.
         * The 1st parameter '32768' tells the timer to run for
         * 1 second (32.768 kHz oscillator), the 2nd parameter is
         * the timer handle and the 3rd parameter '0' tells
         * the timer to repeat continuously until stopped manually.*/
        sc = sl_bt_system_set_lazy_soft_timer(32768, 0, 0, 0);
        app_assert(sc == SL_STATUS_OK,
                   "[E: 0x%04x] Failed to start a software timer\n",
                   (int)sc);
      } else if (evt->data.evt_gatt_server_characteristic_status.
                 client_config_flags
                 == gatt_disable) {
        /* Indications have been turned OFF - stop the timer. */
        sc = sl_bt_system_set_lazy_soft_timer(0, 0, 0, 0);
        app_assert(sc == SL_STATUS_OK,
                   "[E: 0x%04x] Failed to stop a software timer\n",
                   (int)sc);
      }
      break;

    /* This event is generated when the software timer has ticked.
     * In this example the temperature is read after every 1 second
     * and then the indication of that is sent to the listening client. */
    case sl_bt_evt_system_soft_timer_id:
      /* Measure the temperature as defined in the function
       *   measure_temperature() */
      measure_temperature();
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

/***************************************************************************//**
 * @brief Initialize ADC
 ******************************************************************************/
static void init_adc(void)
{
  CMU_ClockEnable(cmuClock_ADC0, true);

  /* Base the ADC configuration on the default setup. */
  ADC_Init_TypeDef       init = ADC_INIT_DEFAULT;
  ADC_InitSingle_TypeDef init_single = ADC_INITSINGLE_DEFAULT;

  /* Initialize timebases */
  init.timebase = ADC_TimebaseCalc(0);
  init.prescale = ADC_PrescaleCalc(400000, 0);
  ADC_Init(ADC0, &init);

  /* Set input to temperature sensor. Reference must be 1.25V */
  init_single.reference = adcRef1V25;
  init_single.acqTime = adcAcqTime8;     /* Minimum time for temperature sensor
                                          */
  init_single.posSel = adcPosSelTEMP;
  ADC_InitSingle(ADC0, &init_single);
}

/***************************************************************************//**
 * @brief Make one ADC conversion
 ******************************************************************************/
static uint32_t read_adc(void)
{
  ADC_Start(ADC0, adcStartSingle);
  while ((ADC0->STATUS & ADC_STATUS_SINGLEDV) == 0) {}
  return ADC_DataSingleGet(ADC0);
}

/***************************************************************************//**
 * @brief Convert ADC temperature sensor readings into milli-celcius
 ******************************************************************************/
static int32_t convert_to_millicelsius(int32_t adc_sample)
{
  const float TGRAD_ADCTH = 1.835; // TGRAD_ADCTH = 1.835 mV/degC (from
                                   //   datasheet)
  const uint32_t VFS = 1250; // VFS = 1250 mV
  const uint32_t NUM_STEPS_12BIT = 4096;
  float T_Celsius;

  if ((CAL_TEMP0 == 0xFF) || (CAL_VALUE0 == 0xFFF)) {
    /* The temperature sensor is not calibrated */
    return -100.0;
  }

  /* e.g. EFRxG13 datasheet section 27.3.10.9 Temperature Measurement
   * for below formula. */
  int32_t readout_difference = CAL_VALUE0 - adc_sample;
  T_Celsius = ((float) readout_difference * VFS);
  T_Celsius /= (NUM_STEPS_12BIT * (-1 * TGRAD_ADCTH));

  /* Calculate offset from calibration temperature */
  T_Celsius = (float) CAL_TEMP0 - T_Celsius;
  return (int32_t) (T_Celsius * 1000);
}

/***************************************************************************//**
 *  @brief Function for taking a single temperature measurement
 *  with EFR32 internal temperature sensor.
 ******************************************************************************/
static void measure_temperature(void)
{
  sl_status_t sc;
  uint8_t htm_temperature_buffer[5]; /* Stores the temperature data in the
                                      *    Health Thermometer (HTM) format. */
  uint8_t flags = 0x00;   /* HTM flags set as 0 for Celsius,
                           *    no time stamp and no temperature type. */
  int32_t temperature_data; /* Stores the Temperature data read from the sensor.
                             */
  uint32_t temperature;   /* Stores the temperature data read from
                          *    the sensor in the correct format */
  uint8_t *p = htm_temperature_buffer; /* Pointer to HTM temperature buffer
                                        *    needed for converting values to
                                        *   bitstream. */

  /* Convert flags to bitstream and append them
   * in the HTM temperature data buffer (htm_temperature_buffer) */
  UINT8_TO_BITSTREAM(p, flags);

  temperature_data = convert_to_millicelsius(read_adc());

  /* Convert sensor data to correct temperature format */
  temperature = FLT_TO_UINT32(temperature_data, -3);

  /* Convert temperature to bitstream and
   * place it in the HTM temperature data buffer (htm_temperature_buffer) */
  UINT32_TO_BITSTREAM(p, temperature);

  /* Send indication of the temperature in htm_temperature_buffer to
   * all "listening" clients. This enables the Health Thermometer
   * in the EFR Connect app to display the temperature. */
  sc = sl_bt_gatt_server_send_indication(connection_handle,
                                         gattdb_temperature_measurement,
                                         5,
                                         htm_temperature_buffer);
  app_assert(sc == SL_STATUS_OK,
             "[E: 0x%04x] Failed to send notifications\n",
             (int)sc);
}
