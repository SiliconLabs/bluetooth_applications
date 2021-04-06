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
#include "sl_simple_led_instances.h"

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;



/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////

  sl_app_log("start \n");
  //turn on both 2 LEDs at the startup
  sl_led_turn_on(&sl_led_led0);
  sl_led_turn_on(&sl_led_led1);
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
  sl_led_state_t led_status;

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



      //print log
      sl_app_log("Stack version: %u.%u.%u\r\n", evt->data.evt_system_boot.major, \
             evt->data.evt_system_boot.minor, evt->data.evt_system_boot.patch);
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
    // write request      //
    ///////////////////////////////////////////////////////////////////////////
    case sl_bt_evt_gatt_server_user_write_request_id:
      // Automation IO digital control
      if (evt->data.evt_gatt_server_user_write_request.characteristic == gattdb_led0) {
          // Write user supplied value to LEDs.
          if(evt->data.evt_gatt_server_attribute_value.value.data[0] != 0){
              sl_app_log("turn on led 0 \n");
              sl_led_turn_on(&sl_led_led0);
          }
          else{
              sl_app_log("turn off led 0 \n");
              sl_led_turn_off(&sl_led_led0);
          }
          sc = sl_bt_gatt_server_send_user_write_response(evt->data.evt_gatt_server_user_write_request.connection,
                                                          gattdb_led0,
                                                          SL_STATUS_OK);
          sl_app_assert(sc == SL_STATUS_OK,
                        "[E: 0x%04x] Failed to send response \n",
                        (int)sc);
      }

      if (evt->data.evt_gatt_server_user_write_request.characteristic == gattdb_led1) {
          // Write user supplied value to LEDs.
          if(evt->data.evt_gatt_server_attribute_value.value.data[0] != 0){
              sl_app_log("turn on led 1 \n");
              sl_led_turn_on(&sl_led_led1);
          }
          else{
              sl_app_log("turn off led 1 \n");
              sl_led_turn_off(&sl_led_led1);
          }
          sc = sl_bt_gatt_server_send_user_write_response(evt->data.evt_gatt_server_user_write_request.connection,
                                                          gattdb_led1,
                                                          SL_STATUS_OK);
          sl_app_assert(sc == SL_STATUS_OK,
                        "[E: 0x%04x] Failed to send response \n",
                        (int)sc);
      }
      break;

    ///////////////////////////////////////////////////////////////////////////
    // read request      //
    ///////////////////////////////////////////////////////////////////////////
    case sl_bt_evt_gatt_server_user_read_request_id:
      // Automation IO digital control
      if (evt->data.evt_gatt_server_user_read_request.characteristic == gattdb_led0) {
          sl_app_log("read led 0 \n");
          led_status = sl_led_get_state(&sl_led_led0);
          sc = sl_bt_gatt_server_send_user_read_response(evt->data.evt_gatt_server_user_read_request.connection,
                                                         gattdb_led0,
                                                         SL_STATUS_OK,
                                                         1,
                                                         &led_status,
                                                         &sent_len);
          sl_app_assert(sc == SL_STATUS_OK,
                        "[E: 0x%04x] Failed to send response \n",
                        (int)sc);
      }

      if (evt->data.evt_gatt_server_user_read_request.characteristic == gattdb_led1) {
          sl_app_log("read led 1 \n");
          led_status = sl_led_get_state(&sl_led_led1);
          sc = sl_bt_gatt_server_send_user_read_response(evt->data.evt_gatt_server_user_read_request.connection,
                                                         gattdb_led1,
                                                         SL_STATUS_OK,
                                                         1,
                                                         &led_status,
                                                         &sent_len);
          sl_app_assert(sc == SL_STATUS_OK,
                        "[E: 0x%04x] Failed to send response \n",
                        (int)sc);
      }
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
