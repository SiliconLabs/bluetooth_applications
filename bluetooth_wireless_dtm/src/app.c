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
 *
 * EVALUATION QUALITY
 * This code has been minimally tested to ensure that it builds with
 * the specified dependency versions and is suitable as a demonstration for
 * evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 *
 ******************************************************************************/

// -----------------------------------------------------------------------------
//                                 Includes
// -----------------------------------------------------------------------------
#include "sl_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"

// -----------------------------------------------------------------------------
//                             Macros and Typedefs
// -----------------------------------------------------------------------------
// External timeout event identifier.
#define DTM_TEST_TIMEOUT_EVENT  0x01

// Inner states of the device.
typedef enum {
  DTM_IDLE = 0,
  DTM_ADVERTISING,
  DTM_CONNECTED,
  DTM_TEST_STARTED,
  DTM_TEST_RUNNING
} dtm_state_t;

// The DTM test modes of the device.
typedef enum {
  DTM_MODE_NONE = 0,
  DTM_MODE_RX,
  DTM_MODE_TX,
  DTM_MODE_CW
} dtm_mode_t;

// Structure containing arguments of the testing commands.
typedef struct dtm_test_parameters {
  uint16_t duration;
  uint8_t channel;
  uint8_t phy;
  uint8_t packet_type;
  uint8_t length;
  int8_t power_level;
} dtm_test_parameters_t;

// Structure containing all settings, results and the inner state of the DTM
//   test.
typedef struct dtm_test {
  dtm_mode_t mode;
  dtm_state_t state;
  dtm_test_parameters_t parameters;
  uint16_t result;
  uint16_t number_of_packets;
} dtm_test_t;

// -----------------------------------------------------------------------------
//                             Local variables
// -----------------------------------------------------------------------------
// DTM test instance.
static dtm_test_t dtm_test;

// Timer used to run the test for a specific time.
static sl_sleeptimer_timer_handle_t dtm_sleeptimer_handle;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t app_ble_advertising_set_handle = 0xff;
// Bluetooth connection handle.
static uint8_t app_ble_connection_handle = 0xff;

// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------
static sl_status_t app_ble_set_system_id();
static sl_status_t app_ble_start_advertising();
static sl_status_t app_dtm_verify_settings();
static sl_status_t app_dtm_start_test();
static sl_status_t app_dtm_start_rx();
static sl_status_t app_dtm_start_tx();
static sl_status_t app_dtm_start_cw();
static sl_status_t app_dtm_test_completed_event_handler(uint16_t result,
                                                        uint16_t number_of_packets);
static sl_status_t app_dtm_connection_closed_event_handler(uint16_t reason);
static void app_dtm_connection_opened_event_handler(uint8_t connection);
static void app_dtm_stop_test();
static void app_dtm_publish_result();
static void app_dtm_load_settings();
static void app_dtm_reset_data();
static void app_dtm_error_handler();

static void on_dtm_sleeptimer_callback(sl_sleeptimer_timer_handle_t *handle,
                                       void *data)
{
  (void) data;
  (void) handle;

  sl_bt_external_signal(DTM_TEST_TIMEOUT_EVENT);
}

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
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
    case sl_bt_evt_system_boot_id:
      app_dtm_reset_data();
      app_ble_set_system_id();
      app_ble_start_advertising();
      break;

    case sl_bt_evt_connection_opened_id:

      app_dtm_connection_opened_event_handler(
        evt->data.evt_connection_opened.connection);
      break;

    case sl_bt_evt_connection_closed_id:

      sc = app_dtm_connection_closed_event_handler(
        evt->data.evt_connection_closed.reason);
      if (sc != SL_STATUS_OK) {
        app_dtm_error_handler();
      }
      break;

    case sl_bt_evt_test_dtm_completed_id:

      sc = app_dtm_test_completed_event_handler(
        evt->data.evt_test_dtm_completed.result,
        evt->data.evt_test_dtm_completed.number_of_packets);
      if (sc != SL_STATUS_OK) {
        app_dtm_error_handler();
      }
      break;

    case sl_bt_evt_system_external_signal_id:

      // Timeout occurred.
      if (evt->data.evt_system_external_signal.extsignals
          == DTM_TEST_TIMEOUT_EVENT) {
        app_dtm_stop_test();
      }
      break;

    default:
      break;
  }
}

// -----------------------------------------------------------------------------
//                          Static Function Definitions
// -----------------------------------------------------------------------------

/***************************************************************************//**
 *  Sets system ID in the local GATT database.
 ******************************************************************************/
static sl_status_t app_ble_set_system_id()
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];

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

  sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id,
                                               0,
                                               sizeof(system_id),
                                               system_id);
  app_assert_status(sc);
  return sc;
}

/***************************************************************************//**
 *  Starts advertising. The device is advertising the device name and the
 *  Silicon Labs DTM control service UUID.
 ******************************************************************************/
static sl_status_t app_ble_start_advertising()
{
  sl_status_t sc;

  if (app_ble_advertising_set_handle == 0xff) {
    // Create an advertising set.
    sc = sl_bt_advertiser_create_set(&app_ble_advertising_set_handle);
    app_assert_status(sc);
  }

  // Set advertising interval to 100ms.
  sc = sl_bt_advertiser_set_timing(app_ble_advertising_set_handle,
                                   160, // min. adv. interval (ms * 1.6)
                                   160, // max. adv. interval (ms * 1.6)
                                   0,   // adv. duration
                                   0);  // max. num. adv. events
  app_assert_status(sc);

  sl_bt_legacy_advertiser_generate_data(app_ble_advertising_set_handle,
                                        sl_bt_advertiser_general_discoverable);

  // Start general advertising and enable connections.
  sc = sl_bt_legacy_advertiser_start(app_ble_advertising_set_handle,
                                     sl_bt_advertiser_connectable_scannable);
  app_assert_status(sc);

  dtm_test.state = DTM_ADVERTISING;
  return sc;
}

/***************************************************************************//**
 *  Check if the parameters from the GATT database are valid.
 ******************************************************************************/
static sl_status_t app_dtm_verify_settings()
{
  app_dtm_load_settings();

  if ((dtm_test.mode == DTM_MODE_NONE)
      || (dtm_test.parameters.duration == 0)
      || (dtm_test.parameters.phy == 0)) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  return SL_STATUS_OK;
}

/***************************************************************************//**
 *  Start DTM test.
 ******************************************************************************/
static sl_status_t app_dtm_start_test()
{
  sl_status_t sc;

  sc = app_dtm_verify_settings();
  if (sc != SL_STATUS_OK) {
    return sc;
  }

  switch (dtm_test.mode) {
    case DTM_MODE_RX:
      sc = app_dtm_start_rx();
      break;

    case DTM_MODE_TX:
      sc = app_dtm_start_tx();
      break;

    case DTM_MODE_CW:
      sc = app_dtm_start_cw();
      break;

    default:
      return SL_STATUS_INVALID_PARAMETER;
  }

  dtm_test.state = DTM_TEST_STARTED;

  return sc;
}

/***************************************************************************//**
 *  Start DTM test in RX mode.
 ******************************************************************************/
static sl_status_t app_dtm_start_rx()
{
  return sl_bt_test_dtm_rx(dtm_test.parameters.channel,
                           dtm_test.parameters.phy);
}

/***************************************************************************//**
 *  Start DTM test in TX mode.
 ******************************************************************************/
static sl_status_t app_dtm_start_tx()
{
  return sl_bt_test_dtm_tx_v4(dtm_test.parameters.packet_type,
                              dtm_test.parameters.length,
                              dtm_test.parameters.channel,
                              dtm_test.parameters.phy,
                              dtm_test.parameters.power_level);
}

/***************************************************************************//**
 *  Start DTM test in CW mode.
 ******************************************************************************/
static sl_status_t app_dtm_start_cw()
{
  return sl_bt_test_dtm_tx_cw(dtm_test.parameters.packet_type,
                              dtm_test.parameters.channel,
                              dtm_test.parameters.phy,
                              dtm_test.parameters.power_level);
}

/***************************************************************************//**
 *  Handles the test_dtm_completed Bluetooth event.
 ******************************************************************************/
static sl_status_t app_dtm_test_completed_event_handler(uint16_t result,
                                                        uint16_t number_of_packets)
{
  if (result != 0) {
    return SL_STATUS_FAIL;
  }
  switch (dtm_test.state) {
    case DTM_TEST_STARTED:
      sl_sleeptimer_start_timer_ms(&dtm_sleeptimer_handle,
                                   dtm_test.parameters.duration * 1000,
                                   on_dtm_sleeptimer_callback,
                                   NULL,
                                   0,
                                   0);
      dtm_test.state = DTM_TEST_RUNNING;
      break;

    case DTM_TEST_RUNNING:
      dtm_test.result = result;
      dtm_test.number_of_packets = number_of_packets;
      app_dtm_publish_result();
      // Test has finished, start advertising.
      app_ble_start_advertising();
      break;

    default:
      return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

/***************************************************************************//**
 *  Handles the connection_closed Bluetooth event.
 ******************************************************************************/
static sl_status_t app_dtm_connection_closed_event_handler(uint16_t reason)
{
  sl_status_t sc;
  // Test is only started if the client terminates the connection.
  if (reason == SL_STATUS_BT_CTRL_REMOTE_USER_TERMINATED) {
    sc = app_dtm_start_test();
    if (sc == SL_STATUS_OK) {
      return SL_STATUS_OK;
    }
  }
  // An error occurred.
  dtm_test.state = DTM_IDLE;
  return SL_STATUS_FAIL;
}

/***************************************************************************//**
 *  Handles the connection_opened Bluetooth event.
 ******************************************************************************/
static void app_dtm_connection_opened_event_handler(uint8_t connection)
{
  app_ble_connection_handle = connection;
  dtm_test.state = DTM_CONNECTED;
}

/***************************************************************************//**
 *  Stop DTM test.
 ******************************************************************************/
static void app_dtm_stop_test()
{
  sl_bt_test_dtm_end();
}

/***************************************************************************//**
 *  Write result to the local GATT database so the client can read it.
 ******************************************************************************/
static void app_dtm_publish_result()
{
  sl_status_t sc;
  uint8_t result_data[2];
  result_data[0] = (uint8_t) dtm_test.number_of_packets & 0x00ff;
  result_data[1] = (uint8_t) (dtm_test.number_of_packets >> 8) & 0x00ff;

  sc = sl_bt_gatt_server_write_attribute_value(gattdb_dtm_result,
                                               0,
                                               sizeof(result_data),
                                               result_data);
  app_assert_status(sc);

  // Clear mode parameter in GATT database. It will indicate that
  // there is no valid test configuration for the next test.
  dtm_test.mode = DTM_MODE_NONE;
  sc = sl_bt_gatt_server_write_attribute_value(gattdb_dtm_mode,
                                               0,
                                               1,
                                               &dtm_test.mode);
  app_assert_status(sc);
}

/***************************************************************************//**
 *  Load test parameters from the GATT database.
 ******************************************************************************/
static void app_dtm_load_settings()
{
  sl_status_t sc;
  uint8_t dtm_parameter_uint8;
  uint8_t dtm_parameter_uint16[2];
  size_t size;

  sc = sl_bt_gatt_server_read_attribute_value(gattdb_dtm_mode,
                                              0,
                                              sizeof(dtm_parameter_uint8),
                                              &size,
                                              &dtm_test.mode);
  app_assert_status(sc);

  sc = sl_bt_gatt_server_read_attribute_value(gattdb_dtm_duration,
                                              0,
                                              sizeof(dtm_parameter_uint16),
                                              &size,
                                              dtm_parameter_uint16);
  app_assert_status(sc);

  memcpy(&dtm_test.parameters.duration, dtm_parameter_uint16, size);

  if (dtm_test.mode == DTM_MODE_RX) {
    sc = sl_bt_gatt_server_read_attribute_value(gattdb_dtm_rx_channel,
                                                0,
                                                sizeof(dtm_parameter_uint8),
                                                &size,
                                                &dtm_parameter_uint8);
    app_assert_status(sc);
    dtm_test.parameters.channel = dtm_parameter_uint8;

    sc = sl_bt_gatt_server_read_attribute_value(gattdb_dtm_rx_phy,
                                                0,
                                                sizeof(dtm_parameter_uint8),
                                                &size,
                                                &dtm_parameter_uint8);
    app_assert_status(sc);
    dtm_test.parameters.phy = dtm_parameter_uint8;
  } else if ((dtm_test.mode == DTM_MODE_TX) || (dtm_test.mode == DTM_MODE_CW)) {
    sc = sl_bt_gatt_server_read_attribute_value(gattdb_dtm_tx_packet_type,
                                                0,
                                                sizeof(dtm_parameter_uint8),
                                                &size,
                                                &dtm_parameter_uint8);
    app_assert_status(sc);
    dtm_test.parameters.packet_type = dtm_parameter_uint8;

    sc = sl_bt_gatt_server_read_attribute_value(gattdb_dtm_tx_length,
                                                0,
                                                sizeof(dtm_parameter_uint8),
                                                &size,
                                                &dtm_parameter_uint8);
    app_assert_status(sc);
    dtm_test.parameters.length = dtm_parameter_uint8;

    sc = sl_bt_gatt_server_read_attribute_value(gattdb_dtm_tx_channel,
                                                0,
                                                sizeof(dtm_parameter_uint8),
                                                &size,
                                                &dtm_parameter_uint8);
    app_assert_status(sc);
    dtm_test.parameters.channel = dtm_parameter_uint8;

    sc = sl_bt_gatt_server_read_attribute_value(gattdb_dtm_tx_phy,
                                                0,
                                                sizeof(dtm_parameter_uint8),
                                                &size,
                                                &dtm_parameter_uint8);
    app_assert_status(sc);
    dtm_test.parameters.phy = dtm_parameter_uint8;

    sc = sl_bt_gatt_server_read_attribute_value(gattdb_dtm_tx_power,
                                                0,
                                                sizeof(dtm_parameter_uint8),
                                                &size,
                                                (uint8_t *)&dtm_test.parameters.power_level);
    app_assert_status(sc);
  }
}

/***************************************************************************//**
 *  Sets all characteristics to zero in the DTM control service.
 ******************************************************************************/
static void app_dtm_reset_data()
{
  sl_status_t sc;
  uint8_t dtm_duration_data[2] = { 0 };
  uint8_t dtm_mode_data = DTM_MODE_NONE;
  uint8_t dtm_result_data[2] = { 0 };

  memset(&dtm_test, 0x00, sizeof(dtm_test));

  // If an error occur, all characteristic values are zero in the
  // Silicon Labs DTM control service.
  sc = sl_bt_gatt_server_write_attribute_value(gattdb_dtm_mode,
                                               0,
                                               sizeof(dtm_mode_data),
                                               &dtm_mode_data);
  app_assert_status(sc);

  sc = sl_bt_gatt_server_write_attribute_value(gattdb_dtm_duration,
                                               0,
                                               sizeof(dtm_duration_data),
                                               dtm_duration_data);
  app_assert_status(sc);

  sc = sl_bt_gatt_server_write_attribute_value(gattdb_dtm_result,
                                               0,
                                               sizeof(dtm_result_data),
                                               dtm_result_data);
  app_assert_status(sc);

  dtm_test.state = DTM_IDLE;
}

/***************************************************************************//**
 *  Error handler. If an error occurs, the device resets its settings and
 *  starts advertising.
 ******************************************************************************/
static void app_dtm_error_handler()
{
  sl_status_t sc;

  // If device is connected, disconnect first, then clear data and start
  //   advertising
  if (dtm_test.state == DTM_CONNECTED) {
    sc = sl_bt_connection_close(app_ble_connection_handle);
    app_assert_status(sc);
  } else {
    app_dtm_reset_data();
    app_ble_start_advertising();
  }
}
