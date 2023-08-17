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
#include "sl_bt_api.h"
#include "sl_sleeptimer.h"

#include "app.h"
#include "app_log.h"
#include "app_assert.h"

#include "tempdrv.h"

#define SIGNAL_REFRESH_DATA   1
#define TICKS_PER_SECOND      32768

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
// Timer handle
static sl_sleeptimer_timer_handle_t sleep_timer_handle;
// Timer callback
static void app_timer_cb(sl_sleeptimer_timer_handle_t *handle, void *data);

// sample count + temperature
static int8_t adv_data[2] = { 0, 0 };

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_status_t sc;

  sc = TEMPDRV_Init();
  if (sc == SL_STATUS_OK) {
    app_log("\r\n------  BLE - PAwR Thermometer "
            "Demo: Sensor Role-----\r\n");
  }
  adv_data[1] = TEMPDRV_GetTemp();
  adv_data[0] = 0;
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

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Start extended advertising
      sc = sl_bt_extended_advertiser_generate_data(advertising_set_handle,
                                                   sl_bt_advertiser_general_discoverable);

      app_assert_status(sc);

      sc = sl_bt_extended_advertiser_start(advertising_set_handle,
                                           sl_bt_extended_advertiser_non_connectable,
                                           0);

      app_assert_status(sc);

      // Start periodic advertising with periodic interval 1541.25ms
      sc = sl_bt_pawr_advertiser_start(advertising_set_handle,
                                       1233,
                                       1233,
                                       0,
                                       28,  // sub event count = 28
                                       44,  // sub event interval = 55ms
                                       30,  // slot delay = 37.5ms
                                       6,   // slot spacing = 0.75ms
                                       23); // slot count = 23
      app_assert_status(sc);

      // timer for refresh data
      sc = sl_sleeptimer_start_periodic_timer(&sleep_timer_handle,
                                              (TICKS_PER_SECOND * 1541.25) / 1000,
                                              app_timer_cb,
                                              (void *)NULL,
                                              0,
                                              0);
      app_assert_status(sc);
      break;

    // -------------------------------
    // subevent_data_request event handler.
    case sl_bt_evt_pawr_advertiser_subevent_data_request_id:
      if (evt->data.evt_pawr_advertiser_subevent_data_request.subevent_start
          == 5) {
        sc = sl_bt_pawr_advertiser_set_subevent_data(
          advertising_set_handle,
          evt->data.evt_pawr_advertiser_subevent_data_request.subevent_start,
          0,
          23,
          sizeof(adv_data),
          (uint8_t *) adv_data);
        app_assert_status(sc);
      }

      break;

    // -------------------------------
    // subevent_tx_failed event handler.
    case sl_bt_evt_pawr_advertiser_subevent_tx_failed_id:
      app_log("sub event data failed \r\n");
      break;

    // -------------------------------
    // response_report event handler.
    case sl_bt_evt_pawr_advertiser_response_report_id:
      if (evt->data.evt_pawr_advertiser_response_report.data.len) {
        adv_data[0]++;
        app_log("<- data response from subevent %d response_slot %d\r\n",
                evt->data.evt_pawr_advertiser_response_report.subevent,
                evt->data.evt_pawr_advertiser_response_report.response_slot);
        app_log("got following response data: \r\n");
        for (int i = 0;
             i < evt->data.evt_pawr_advertiser_response_report.data.len ; i++) {
          app_log(" %d",
                  evt->data.evt_pawr_advertiser_response_report.data.data[i]);
        }
        app_log("\r\n");
      }
      break;

    // -------------------------------
    // external_signal event handler.
    case sl_bt_evt_system_external_signal_id:
      if (evt->data.evt_system_external_signal.extsignals
          == SIGNAL_REFRESH_DATA) {
        adv_data[1] = TEMPDRV_GetTemp();
      }
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

void app_timer_cb(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void) handle;
  (void) data;

  sl_bt_external_signal(SIGNAL_REFRESH_DATA);
}
