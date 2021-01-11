/***************************************************************************//**
 * @file app.c
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

#include "app_timer.h"
#include "heart_rate.h"
#include "pulse_oximeter.h"
#include "device_information.h"
#include "ble_att_handler.h"

#include "hrm_app.h"

static void services_init(void);

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static uint8_t connection = 0xFF;

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
  hrm_init_app();
  services_init();
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
  hrm_loop();
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

  switch (SL_BT_MSG_ID(evt->header))
  {
    /* ------------------------------- */
    /* This event indicates the device has started and the radio is ready. */
    /* Do not call any stack command before receiving this boot event! */
    case sl_bt_evt_system_boot_id:

      /* Extract unique ID from BT Address. */
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to get Bluetooth address\n",
                    (int)sc);

      /* Pad and reverse unique ID to get System ID. */
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

      /* Create an advertising set. */
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to create advertising set\n",
                    (int)sc);

      /* Set advertising interval to 100ms. */
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to set advertising timing\n",
                    (int)sc);
      /* Start general advertising and enable connections. */
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start advertising\n",
                    (int)sc);
      break;

    /* ------------------------------- */
    /* This event indicates that a new connection was opened. */
    case sl_bt_evt_connection_opened_id:
      connection = evt->data.evt_connection_opened.connection;
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      /* Service disconnect handlers */
      heart_rate_disconnect_event(evt);
      pulse_oximeter_disconnect_event(evt);
      device_information_disconnect_event(evt);

      /* Restart advertising after client has disconnected. */
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start advertising\n",
                    (int)sc);
      break;

    case sl_bt_evt_gatt_server_user_write_request_id:

      /* Service write handlers */
      /* Heart Rate characteristics written */
      if (evt->data.evt_gatt_server_user_write_request.characteristic == gattdb_heart_rate_control_point)
      {
        heart_rate_write_callback(evt);
      }
      /* Pulse Oximeter characteristics written */
      else if (evt->data.evt_gatt_server_user_write_request.characteristic == gattdb_record_access_control_point)
      {
        pulse_oximeter_write_callback(evt);
      }

      break;

    case sl_bt_evt_gatt_server_user_read_request_id:

      /* Handle previous read operation for long characteristics */
      if (ble_att_send_data_handler(evt->data.evt_gatt_server_user_read_request.characteristic, evt->data.evt_gatt_server_user_read_request.offset))
      {
        /* Event handled */
        break;
      }

      /* Service read handlers */
      /* Heart Rate characteristics read */
      if (evt->data.evt_gatt_server_user_read_request.characteristic == gattdb_body_sensor_location)
      {
        heart_rate_read_callback(evt);
      }
      /* Device Information characteristics read */
      else if ((evt->data.evt_gatt_server_user_read_request.characteristic >= gattdb_manufacturer_name_string) && (evt->data.evt_gatt_server_user_read_request.characteristic <= gattdb_firmware_revision_string))
      {
        device_information_read_callback(evt);
      }
      /* Pulse Oximeter characteristics read */
      else if ((evt->data.evt_gatt_server_user_read_request.characteristic >= gattdb_plx_spot_check_measurement) && (evt->data.evt_gatt_server_user_read_request.characteristic <= gattdb_record_access_control_point))
      {
        pulse_oximeter_read_callback(evt);
      }
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      /* Heart Rate characteristics read */
      if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_heart_rate_measurement)
      {
        heart_rate_characteristic_status(evt);
      }
      else if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_plx_continuous_measurement)
      {
        pulse_oximeter_characteristic_status(evt);
      }
      break;

      /* External signal indication (comes from the interrupt handler)
       ----------------------------------------------------------------------------- */
    /*  Handle GPIO IRQ and do something
     *  External signal command�s parameter can be accessed using
     * event->data.evt_system_external_signal.extsignals   */
    case sl_bt_evt_system_external_signal_id:
      hrm_process_event(evt->data.evt_system_external_signal.extsignals);
    break;

    /* Software Timer event */
    case sl_bt_evt_system_soft_timer_id:
      /* Check which software timer handle is in question */
      switch (evt->data.evt_system_soft_timer.handle)
      {
        case HEART_RATE_TIMER:
          heart_rate_send_new_data(connection);
        break;

        case PULSE_OXIMETER_TIMER:
          pulse_oximeter_send_new_data(connection);
        break;

        default:
          break;
      }
      break;

    /* ------------------------------- */
    /* Default event handler. */
    default:
      break;
  }
}

/*******************************************************************************
 * @brief
 *   BLE service initialization
 * @param
 *   None
 * @return
 *   None
 ******************************************************************************/
static void services_init(void)
{
  heart_rate_init();
  device_information_init();
  pulse_oximeter_init();
}

