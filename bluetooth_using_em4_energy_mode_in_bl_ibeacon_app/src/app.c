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

#include "sl_bluetooth.h"
#include "app_assert.h"
#include "gatt_db.h"
#include "app.h"
#include "sl_simple_button_btn1_config.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_burtc.h"
#include "em_gpio.h"
#include "gpiointerrupt.h"

// Macros.
#define UINT16_TO_BYTES(n)            ((uint8_t) (n)), ((uint8_t)((n) >> 8))
#define UINT16_TO_BYTE0(n)            ((uint8_t) (n))
#define UINT16_TO_BYTE1(n)            ((uint8_t) ((n) >> 8))

// Defines
#define ULFRCO_FREQUENCY                 1000
#define WAKEUP_INTERVAL_MS              15000 // 15 sec
#define BURTC_COUNT_BETWEEN_WAKEUP      (((ULFRCO_FREQUENCY * WAKEUP_INTERVAL_MS) / 1000)-1)
#define ADVERTISING_DURATION            500 // 5 seconds

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;


static void bcn_setup_adv_beaconing(void);
static void set_burtc_clk(void);
static void init_BURTC(void);
static void init_GPIO(void);
static void init_EM4(void);

void BURTC_IRQHandler(void){
   // Clear interrupt source
  BURTC_IntClear(BURTC_IEN_COMP);
}

static void set_burtc_clk(void){
 // Select reference clock/oscillator for the desired clock branch (BURTC Clk).
 // Reference selected for clocking: ULFRCO
  CMU_ClockSelectSet(cmuClock_BURTC, cmuSelect_ULFRCO);

  // Enable BURTC Clk
  CMU_ClockEnable(cmuClock_BURTC, true);
}

// Enable, config and init BURTC
static void init_BURTC(void){

  // Enable BURTC
  BURTC_Enable(true);

  BURTC_Init_TypeDef init_burtc = BURTC_INIT_DEFAULT;
  init_burtc.compare0Top = true;
  init_burtc.em4comp = true;
  init_burtc.em4overflow = true;

  BURTC_CompareSet(0, BURTC_COUNT_BETWEEN_WAKEUP);

  // Enable Interrupt from BURTC
  NVIC_EnableIRQ(BURTC_IRQn);
  // Enable compare interrupt flag
  BURTC_IntEnable(BURTC_IF_COMP);

  // Initialize BURTC. This also starts BURTC automatically.
  BURTC_Init(&init_burtc);
}

static void init_GPIO(void){
 // Configure Button PB1 as input and EM4 wake-on pin source
 GPIO_PinModeSet(SL_SIMPLE_BUTTON_BTN1_PORT, SL_SIMPLE_BUTTON_BTN1_PIN, gpioModeInput, 1);

 // Enable GPIO pin wake-up from EM4; PB1 is GPIO.EM4WU3
 GPIO_EM4EnablePinWakeup(GPIO_IEN_EM4WUIEN3, 0); // GPIO_IEN_EM4WUIEN3 = (0x1UL << 19)

}

// Init EM4
static void init_EM4(void){
  EMU_EM4Init_TypeDef em4Init = EMU_EM4INIT_DEFAULT;
  EMU_EM4Init(&em4Init);
}
/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  set_burtc_clk();
  init_BURTC();
  init_GPIO();
  init_EM4();
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
  int16_t min_pwr, max_pwr;
  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

     //Reset and stop BURTC counter until advertising timeout.
      BURTC_CounterReset();
      BURTC_Stop();
      BURTC_SyncWait(); // Wait for the stop to synchronize

      // Set 0 dBm Transmit Power.
      sc = sl_bt_system_set_tx_power(0, 0, &min_pwr, &max_pwr);
      app_assert_status(sc);
      // Initialize iBeacon ADV data.
      bcn_setup_adv_beaconing();
      break;

    case sl_bt_evt_advertiser_timeout_id:
      // Start BURTC
       BURTC_Start();
       BURTC_SyncWait();

       // Enter EM4
       EMU_EnterEM4();
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

/**************************************************************************//**
 * Set up a custom advertisement package according to iBeacon specifications.
 * The advertisement package is 30 bytes long.
 * See the iBeacon specification for further details.
 *****************************************************************************/
static void bcn_setup_adv_beaconing(void)
{
  sl_status_t sc;

  struct {
    uint8_t flags_len;     // Length of the Flags field.
    uint8_t flags_type;    // Type of the Flags field.
    uint8_t flags;         // Flags field.
    uint8_t mandata_ten;   // Length of the Manufacturer Data field.
    uint8_t mandata_type;  // Type of the Manufacturer Data field.
    uint8_t comp_id[2];    // Company ID field.
    uint8_t beac_type[2];  // Beacon Type field.
    uint8_t uuid[16];      // 128-bit Universally Unique Identifier (UUID). The UUID is an identifier for the company using the beacon.
    uint8_t maj_num[2];    // Beacon major number. Used to group related beacons.
    uint8_t min_num[2];    // Beacon minor number. Used to specify individual beacons within a group.
    uint8_t tx_power;      // The Beacon's measured RSSI at 1 meter distance in dBm. See the iBeacon specification for measurement guidelines.
  }
  bcn_beacon_adv_data
    = {
    // Flag bits - See Bluetooth 4.0 Core Specification , Volume 3, Appendix C, 18.1 for more details on flags.
    2,            // Length of field.
    0x01,         // Type of field.
    0x04 | 0x02,  // Flags: LE General Discoverable Mode, BR/EDR is disabled.

    // Manufacturer specific data.
    26,   // Length of field.
    0xFF, // Type of field.

    // The first two data octets shall contain a company identifier code from
    // the Assigned Numbers - Company Identifiers document.
    // 0x004C = Apple
    { UINT16_TO_BYTES(0x004C) },

    // Beacon type.
    // 0x0215 is iBeacon.
    { UINT16_TO_BYTE1(0x0215), UINT16_TO_BYTE0(0x0215) },

    // 128 bit / 16 byte UUID
    { 0xE2, 0xC5, 0x6D, 0xB5, 0xDF, 0xFB, 0x48, 0xD2, \
      0xB0, 0x60, 0xD0, 0xF5, 0xA7, 0x10, 0x96, 0xE0 },

    // Beacon major number.
    // Set to 34987 and converted to correct format.
    { UINT16_TO_BYTE1(34987), UINT16_TO_BYTE0(34987) },

    // Beacon minor number.
    // Set as 1025 and converted to correct format.
    { UINT16_TO_BYTE1(1025), UINT16_TO_BYTE0(1025) },

    // The Beacon's measured RSSI at 1 meter distance in dBm.
    // 0xD7 is -41dBm
    0xD7
    };

  // Create an advertising set.
  sc = sl_bt_advertiser_create_set(&advertising_set_handle);
  app_assert_status(sc);

  // Set custom advertising data.
  sc = sl_bt_advertiser_set_data(advertising_set_handle,
                                 0,
                                 sizeof(bcn_beacon_adv_data),
                                 (uint8_t *)(&bcn_beacon_adv_data));
  app_assert_status(sc);

  // Set advertising parameters. 100ms advertisement interval.
  sc = sl_bt_advertiser_set_timing(
    advertising_set_handle,
    160,     // min. adv. interval (milliseconds * 1.6)
    160,     // max. adv. interval (milliseconds * 1.6)
    ADVERTISING_DURATION,       // adv. duration
    0);      // max. num. adv. events
  app_assert_status(sc);

  // Start advertising in user mode and disable connections.
  sc = sl_bt_advertiser_start(
    advertising_set_handle,
    advertiser_user_data,
    advertiser_non_connectable);
  app_assert_status(sc);
}
