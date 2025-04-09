/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 *
 * DEPRECATION NOTICE
 * This code has been deprecated. It has been provided for historical reference
 * only and should not be used. This code will not be maintained.
 * This code is subject to the quality disclaimer at the point in time prior
 * to deprecation and superseded by this deprecation notice.
 *
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "sl_common.h"
#include "sl_app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"

#include "sl_bt_api.h"
#include "sl_app_log.h"
#include "sl_simple_button_instances.h"
#include "sl_simple_led_instances.h"

#include "em_wdog.h"
#include "em_rmu.h"

/**************************************************************************//**
 * Local variable
 *****************************************************************************/
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static uint32_t reset_cause;
static const WDOG_Init_TypeDef init =
{
  .enable = true,            /* Start watchdog when init done */
  .debugRun = false,         /* WDOG not counting during debug halt */
  .em2Run = true,            /* WDOG counting when in EM2 */
  .em3Run = true,            /* WDOG counting when in EM3 */
  .em4Block = false,         /* EM4 can be entered */
  .swoscBlock = false,       /* Do not block disabling LFRCO/LFXO in CMU */
  .lock = false,             /* Do not lock WDOG configuration (if locked, reset needed to unlock) */
  .clkSel = wdogClkSelULFRCO, /* Select 1kHZ WDOG oscillator */
  .perSel = wdogPeriod_2k,   /* Set the watchdog period to 2049 clock periods (ie ~2 seconds)*/
};

/******************************************
 * Callback when button state change
 ******************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  sl_status_t sc;

  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
    if (&sl_button_btn0 == handle) {
      /* Stop software timer to imitate that the stack is frozen
       * This will cause a watchdog reset. */
      sc = sl_bt_system_set_soft_timer(0, 0, 0);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to stop soft timer\n",
                    (int)sc);
      sl_app_log("Stop feed the watchdog \n");
    }
  }
}

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
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

      sl_app_log("Start \n");

      // Get the reset cause
      reset_cause = RMU_ResetCauseGet();
      RMU_ResetCauseClear();
      if (reset_cause & RMU_RSTCAUSE_WDOGRST) {
        /* watchdog reset occured */
        sl_app_log("Watchdog Reset Occurred!\n");
      }

      // Init Watchdog driver
      WDOG_Init(&init);

      // Start the software timer to feed the watchdog every 1s
      sc = sl_bt_system_set_soft_timer(32768, 0, 0);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start soft timer\n",
                    (int)sc);

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

    // -------------------------------
    // This event is actived by the software timer
    case sl_bt_evt_system_soft_timer_id:
      /* Feed the watchdog every second while the stack is running and blink LED0 */
      sl_led_toggle(&sl_led_led0);
      WDOG_Feed();
      sl_app_log("Feed the watchdog \r\n");
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
