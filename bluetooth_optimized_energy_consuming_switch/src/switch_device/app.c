/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"

#include "sl_sleeptimer.h"
#include "app_log.h"
#include "em_emu.h"
#include "em_cmu.h"
#include "sl_simple_button_instances.h"
#include "sl_simple_button_btn0_config.h"
#include "switch_nvm.h"

/**************************************************************************//**
 * application definition.
 *****************************************************************************/
#define APP_WAKEUP_ADVERTISE_EVT               0x01

// Advertising flags (common).
#define ADVERTISE_FLAGS_LENGTH                 0x02
#define ADVERTISE_FLAGS_TYPE                   0x01
#define ADVERTISE_FLAGS_DATA                   0x06
// Complete local name.
#define DEVICE_NAME_LENGTH                     7
#define DEVICE_NAME_TYPE                       0x09
#define ADVERTISE_DEVICE_NAME                  "BG22_SW"
// Manufacturer ID (0x02FF - Silicon Labs' company ID)
#define MANUF_ID                               0x02FF
// 1+2+8 bytes for type, company ID and the payload
#define MANUF_LENGTH                           4
#define MANUF_TYPE                             0xFF

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
  // using 1 bytes for counter value
  uint8_t SwitchStatus;

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
 * Static variable declaration.
 *****************************************************************************/
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
  .SwitchStatus = 0,

  .name = ADVERTISE_DEVICE_NAME,
  // Calculate total length of advertising data
  .data_size = 3 + (1 + MANUF_LENGTH) + (1 + DEVICE_NAME_LENGTH + 1),
};

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static sl_sleeptimer_timer_handle_t app_timer;
static uint8_t switch_state = 0;

/**************************************************************************//**
 * Static function declaration.
 *****************************************************************************/
static void GPIO_wakeup_pin_init(void);
static void app_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                               void *data);
static void app_system_boot_event_handle(sl_bt_msg_t *evt);
static void app_advertise_timeout_handle(sl_bt_msg_t *evt);
static void app_external_event_hanlde(uint32_t extsignals);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
  app_log("Bluetooth Optimized Energy Consuming - Switch Device Init.\n");
  switch_nvm3_init();
  GPIO_wakeup_pin_init();

  nvm3_get_switch_state(&switch_state);
  app_log("switch state: %d\n", switch_state);
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
  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      app_system_boot_event_handle(evt);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    // -------------------------------
    // This event indicates that an advertisement was finished.
    case sl_bt_evt_advertiser_timeout_id:
      app_advertise_timeout_handle(evt);
      break;

    // -------------------------------
    // This event indicates that an external signal was raised.
    case sl_bt_evt_system_external_signal_id:
      app_external_event_hanlde(
        evt->data.evt_system_external_signal.extsignals);
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

static void GPIO_wakeup_pin_init(void)
{
  // enable wake-up pin function for button 0
#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_2)
  GPIO_EM4EnablePinWakeup(GPIO_IEN_EM4WUIEN8, 0);
  BUS_RegBitWrite(&(GPIO->IEN), 8 + _GPIO_IEN_EM4WUIEN0_SHIFT, true);
#elif defined(_SILICON_LABS_32B_SERIES_2_CONFIG_4)
  GPIO_EM4EnablePinWakeup(GPIO_IEN_EM4WUIEN4, 0);
  BUS_RegBitWrite(&(GPIO->IEN), 4 + _GPIO_IEN_EM4WUIEN0_SHIFT, true);
#endif
}

static void app_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                               void *data)
{
  (void) handle;
  (void) data;

  sl_bt_external_signal(APP_WAKEUP_ADVERTISE_EVT);
}

void sl_button_on_change(const sl_button_t *handle)
{
  (void) handle;

  if (SL_SIMPLE_BUTTON_RELEASED == sl_simple_button_get_state(handle)) {
    switch_state = ~(switch_state | 0xFE);
    nvm3_save_switch_state(switch_state);
    sl_bt_external_signal(APP_WAKEUP_ADVERTISE_EVT);
  }
}

static void app_system_boot_event_handle(sl_bt_msg_t *evt)
{
  (void) evt;
  sl_status_t sc;
  int16_t tx_power_max;
  int16_t tx_power_min;

  // Create an advertising set.
  sc = sl_bt_advertiser_create_set(&advertising_set_handle);
  app_assert_status(sc);

  // Set advertising interval to 100ms.
  sc = sl_bt_advertiser_set_timing(
    advertising_set_handle,
    160, // min. adv. interval (milliseconds * 1.6)
    160, // max. adv. interval (milliseconds * 1.6)
    100,   // adv. duration
    5);  // max. num. adv. events
  app_assert_status(sc);

  sc = sl_bt_advertiser_set_channel_map(advertising_set_handle, 7);
  app_assert_status(sc);

  // Set custom advertising payload
  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                        sl_bt_advertiser_advertising_data_packet,
                                        advertising_data.data_size,
                                        (uint8_t *)&advertising_data);
  app_assert_status(sc);

  // set TX power
  sc = sl_bt_system_set_tx_power(0, 0, &tx_power_max, &tx_power_min);
  app_assert_status(sc);

  // start advertising after sleeping.
  sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                     sl_bt_advertiser_scannable_non_connectable);
  app_assert_status(sc);
  app_log("start advertising: %s...\n", advertising_data.name);
}

static void app_advertise_timeout_handle(sl_bt_msg_t *evt)
{
  (void) evt;
  sl_status_t sc;
  static int wakeUp_time = 0;

  sc = sl_sleeptimer_start_timer_ms(&app_timer,
                                    1000,
                                    app_timer_callback,
                                    NULL,
                                    0,
                                    0);

  if (sc == SL_STATUS_OK) {
    // advertisement is enabled 3 times when button is released.
    wakeUp_time++;
    if (wakeUp_time == 3) {
      wakeUp_time = 0;
      sl_sleeptimer_stop_timer(&app_timer);
    }
    // go to sleep after advertising.
    EMU_EnterEM2(false);
  }
}

static void app_external_event_hanlde(uint32_t extsignals)
{
  (void) extsignals;
  sl_status_t sc = SL_STATUS_OK;

  app_log("Switch state: %d\n", switch_state);
  app_log("Start advertising.\n");
  // update advertisement packet
  advertising_data.SwitchStatus = switch_state;

  // Set custom advertising payload
  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                        sl_bt_advertiser_advertising_data_packet,
                                        advertising_data.data_size,
                                        (uint8_t *)&advertising_data);
  app_assert_status(sc);

  // start advertising after sleeping.
  sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                     sl_bt_advertiser_scannable_non_connectable);
  app_assert_status(sc);
}
