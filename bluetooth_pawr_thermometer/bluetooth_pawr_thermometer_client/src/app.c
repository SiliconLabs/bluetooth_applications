/***************************************************************************//**
 * @file scanner
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

#include "app.h"
#include "app_oled.h"
#include "app_log.h"
#include "app_assert.h"

#define SIGNAL_UPDATE_DISPLAY 5

/**************************************************************************//**
 * static variables.
 *****************************************************************************/
// This constant is UUID of PAwR Sync Service
// 74 e2 e8 78 e8 2c 4e 07 b2 76 5d 2a ff e4 23 9f
static const uint8_t pawr_sync[16] = {
  0x9f, 0x23, 0xe4, 0xff, 0x2a, 0x5d, 0x76, 0xb2,
  0x07, 0x4e, 0x2c, 0xe8, 0x78, 0xe8, 0xe2, 0x74
};
// variable to use sync
static uint16_t sync;
static uint32_t sync_timeout;
// sub event to send response
static uint8_t subevents = 5;
// data get in sub event report
static uint8_t sample_cnt;
static uint8_t temp;

/**************************************************************************//**
 * Static functions declaration.
 *****************************************************************************/
// Parse advertisements looking for advertised PAwR Sync Service.
static uint8_t find_service_in_adv(uint8_t *data, uint8_t len);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  app_log("\r\n------  BLE - PAwR Thermometer "
          "Demo: Client Role-----\r\n");
  app_oled_init();
  app_oled_display_no_data();
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
      /* pawr scanner setting */
      sc = sl_bt_scanner_set_parameters(sl_bt_scanner_scan_mode_passive,
                                        200,
                                        200);
      app_assert_status(sc);
      sc = sl_bt_scanner_start(gap_1m_phy,
                               scanner_discover_observation);
      app_assert_status(sc);

      break;

    // -------------------------------
    case sl_bt_evt_scanner_extended_advertisement_report_id:
      /* scan response */
      if (find_service_in_adv(
            evt->data.evt_scanner_extended_advertisement_report.data.data,
            evt->data.evt_scanner_extended_advertisement_report.data.len)) {
        sl_bt_scanner_stop();

        app_log("Found pawr sync service, attempting to open sync\r\n");

        sc = sl_bt_sync_scanner_set_sync_parameters(
          0,
          evt->data.evt_scanner_extended_advertisement_report.periodic_interval,
          sl_bt_sync_report_all);
        app_assert_status(sc);

        sc = sl_bt_sync_scanner_open(
          evt->data.evt_scanner_extended_advertisement_report.address,
          evt->data.evt_scanner_extended_advertisement_report.address_type,
          evt->data.evt_scanner_extended_advertisement_report.adv_sid,
          &sync);
        app_assert_status(sc);
        // sync must = 0
        if (sync != 0) {
          app_log("open sync fail sync = 0x%x\r\n", sync);
        }
      }
      break;

    // -------------------------------
    case sl_bt_evt_pawr_sync_opened_id:
      /* now that sync is open */
      sync_timeout = (10 * evt->data.evt_pawr_sync_opened.adv_interval) / 8;
      sc = sl_bt_sync_update_sync_parameters(
        evt->data.evt_pawr_sync_opened.sync,
        0,
        sync_timeout);
      app_assert_status(sc);
      // synchronization to sub event 5
      sc = sl_bt_pawr_sync_set_sync_subevents(
        evt->data.evt_pawr_sync_opened.sync,
        1,
        &subevents);
      app_assert_status(sc);
      break;

    // -------------------------------
    case sl_bt_evt_sync_closed_id:
      app_log("sync closed. reason 0x%2X, sync handle %d\r\n",
              evt->data.evt_sync_closed.reason,
              evt->data.evt_sync_closed.sync);
      // update display
      app_oled_display_no_data();

      /* restart discovery */
      sl_bt_scanner_start(gap_1m_phy,
                          scanner_discover_observation);
      break;

    // -------------------------------
    case sl_bt_evt_pawr_sync_subevent_report_id:
      /* receive response in sub event */
      if (evt->data.evt_pawr_sync_subevent_report.data.len == 2) {
        sample_cnt = evt->data.evt_pawr_sync_subevent_report.data.data[0];
        temp = evt->data.evt_pawr_sync_subevent_report.data.data[1];
        sample_cnt++;

        /* send response */
        sc = sl_bt_pawr_sync_set_response_data(
          evt->data.evt_pawr_sync_subevent_report.sync,
          evt->data.evt_pawr_sync_subevent_report.event_counter,
          evt->data.evt_pawr_sync_subevent_report.subevent,
          evt->data.evt_pawr_sync_subevent_report.subevent,
          10,
          sizeof(sample_cnt),
          &sample_cnt);
        if (sc != SL_STATUS_OK) {
          app_log("response failed with sc = 0x%lx\r\n", sc);
        }
        sl_bt_external_signal(SIGNAL_UPDATE_DISPLAY);
      }
      break;

    // -------------------------------
    // external_signal event handler.
    case sl_bt_evt_system_external_signal_id:
      if (evt->data.evt_system_external_signal.extsignals
          == SIGNAL_UPDATE_DISPLAY) {
        app_oled_display_data(sample_cnt - 1, temp, sample_cnt - 1);
      }
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

/**************************************************************************//**
 * Static function definition.
 *****************************************************************************/
static uint8_t find_service_in_adv(uint8_t *data, uint8_t len)
{
  uint8_t ad_length;
  uint8_t ad_type;
  uint8_t i = 0;

  // Parse advertisement packet
  while (i < len) {
    ad_length = data[i];
    ad_type = data[i + 1];
    // Partial ($02) or complete ($03) list of 128-bit UUIDs
    if ((ad_type == 0x06) || (ad_type == 0x07)) {
      // compare UUID to service UUID
      if (memcmp(&data[i + 2], pawr_sync, 16) == 0) {
        return 1;
      }
    }
    // advance to the next AD element
    i = i + ad_length + 1;
  }
  return 0;
}
