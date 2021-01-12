/***************************************************************************//**
 * @file app.c
 * @brief Silicon Labs Empty Example Project
 *
 * This example demonstrates the bare minimum needed for a Blue Gecko C application
 * that allows Over-the-Air Device Firmware Upgrading (OTA DFU). The application
 * starts advertising after boot and restarts advertising after a connection is closed.
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
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
#include <stdbool.h>
#include "em_common.h"
#include "sl_app_log.h"
#include "sl_app_assert.h"

#include "sl_bluetooth.h"
#include "gatt_db.h"
#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif // SL_COMPONENT_CATALOG_PRESENT
#ifdef SL_CATALOG_CLI_PRESENT
#include "sl_cli.h"
#endif // SL_CATALOG_CLI_PRESENT
#include "app.h"

#include "btl_interface.h"
#include "btl_interface_storage.h"

// The advertising set handle allocated from Bluetooth stack
static uint8_t advertising_set_handle = 0xff;

/* Flag for indicating DFU Reset must be performed */



static BootloaderInformation_t bldInfo;
static BootloaderStorageSlot_t slotInfo;

/* OTA variables */
static uint32_t ota_image_position = 0;
static uint8_t ota_in_progress = 0;
static uint8_t ota_image_finished = 0;
static uint16_t ota_time_elapsed = 0;


static int32_t get_slot_info()
{
    int32_t err;

    bootloader_getInfo(&bldInfo);
    sl_app_log("Gecko bootloader version: %u.%u\r\n", (bldInfo.version & 0xFF000000) >> 24, (bldInfo.version & 0x00FF0000) >> 16);

    err = bootloader_getStorageSlotInfo(0, &slotInfo);

    if(err == BOOTLOADER_OK)
    {
        sl_app_log("Slot 0 starts @ 0x%8.8x, size %u bytes\r\n", slotInfo.address, slotInfo.length);
    }
    else
    {
        sl_app_log("Unable to get storage slot info, error %x\r\n", err);
    }

    return(err);
}

static void erase_slot_if_needed()
{
    uint32_t offset = 0, num_blocks = 0, i = 0;
    uint8_t buffer[256];
    bool dirty = false;
    int32_t err = BOOTLOADER_OK;

    /* check the download area content by reading it in 256-byte blocks */
    num_blocks = slotInfo.length / 256;

    while((dirty == 0) && (offset < 256*num_blocks) && (err == BOOTLOADER_OK))
    {
        err = bootloader_readStorage(0, offset, buffer, 256);
        if(err == BOOTLOADER_OK)
        {
            i = 0;
            while(i < 256)
            {
                if(buffer[i++] != 0xFF)
                {
                    dirty = true;
                    break;
                }
            }
            offset += 256;
        }
        sl_app_log(".");
    }

    if(err != BOOTLOADER_OK)
    {
        sl_app_log("error reading flash! %x\r\n", err);
    }
    else if(dirty)
    {
        sl_app_log("download area is not empty, erasing...\r\n");
        bootloader_eraseStorageSlot(0);
        sl_app_log("done\r\n");
    }
    else
    {
        sl_app_log("download area is empty\r\n");
    }

    return;
}

static void print_progress()
{
    // estimate transfer speed in kbps
    int kbps = ota_image_position*8/(1024*ota_time_elapsed);

    sl_app_log("pos: %u, time: %u, kbps: %u\r\n", ota_image_position, ota_time_elapsed, kbps);
}

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  sl_app_log("soc_empty initialised.\r\n");
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
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t* evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;

  // Handle stack events
  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_bt_evt_system_boot_id:
      // Print boot message.
      sl_app_log("Bluetooth stack booted: v%d.%d.%d-b%d\r\n",
              evt->data.evt_system_boot.major,
              evt->data.evt_system_boot.minor,
              evt->data.evt_system_boot.patch,
              evt->data.evt_system_boot.build);
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      sl_app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to get Bluetooth address\n",
                 (int)sc);
      sl_app_log("Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
              address_type ? "static random" : "public device",
              address.addr[5],
              address.addr[4],
              address.addr[3],
              address.addr[2],
              address.addr[1],
              address.addr[0]);

      // 1 second soft timer, used for performance statistics during OTA file upload
      sl_bt_system_set_soft_timer(32768,  0, 0);



      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      sl_app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to create advertising set\n",
                 (int)sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle, // advertising set handle
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      sl_app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to set advertising timing\r\n",
                 (int)sc);
      // Start general advertising and enable connections.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,            // advertising set handle
        advertiser_general_discoverable,   // discoverable mode
        advertiser_connectable_scannable); // connectable mode
      sl_app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to start advertising\r\n",
                 (int)sc);
      sl_app_log("Boot event - started advertising\r\n");

      // bootloader init must be called before calling other bootloader_xxx API calls
      bootloader_init();

      // read slot information from bootloader
      if(get_slot_info() == BOOTLOADER_OK){

        // the download area is erased here (if needed), prior to any connections are opened
        erase_slot_if_needed();
      } else {
          sl_app_log("Check that you have installed correct type of Gecko bootloader!\r\n");
      }
      break;

    ///////////////////////////////////////////////////////////////////////////
    // This event indicates that a new connection was opened.                //
    ///////////////////////////////////////////////////////////////////////////
    case sl_bt_evt_connection_opened_id:
      sl_app_log("Connection opened\n");
      break;

    ///////////////////////////////////////////////////////////////////////////
    // This event indicates that a connection was closed.                    //
    ///////////////////////////////////////////////////////////////////////////
    case sl_bt_evt_connection_closed_id:
      sl_app_log("Connection closed, reason: 0x%2.2x\r\n", evt->data.evt_connection_closed.reason);

      if(ota_image_finished){
          sl_app_log("Installing new image\r\n");
        bootloader_setImageToBootload(0);
        bootloader_rebootAndInstall();

      } else{
        // Restart advertising after client has disconnected.
            sc = sl_bt_advertiser_start(
              advertising_set_handle,            // advertising set handle
              advertiser_general_discoverable,   // discoverable mode
              advertiser_connectable_scannable); // connectable mode
            sl_app_assert(sc == SL_STATUS_OK,
                       "[E: 0x%04x] Failed to start advertising\n",
                       (int)sc);
            sl_app_log("Started advertising\n");
      }

      break;

    case  sl_bt_evt_system_soft_timer_id:
      if(ota_in_progress)
         {
            ota_time_elapsed++;
            print_progress();
           }
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////



    case sl_bt_evt_gatt_server_user_write_request_id:
    {
                  uint32_t connection = evt->data.evt_gatt_server_user_write_request.connection;
                  uint32_t characteristic = evt->data.evt_gatt_server_user_write_request.characteristic;
                  if(characteristic == gattdb_ota_control)
                  {
                      switch(evt->data.evt_gatt_server_user_write_request.value.data[0])
                      {
                      case 0://Erase and use slot 0
                          // NOTE: download area is NOT erased here, because the long blocking delay would result in supervision timeout
                          //bootloader_eraseStorageSlot(0);
                          ota_image_position=0;
                          ota_in_progress=1;
                          break;
                      case 3://END OTA process
                          //wait for connection close and then reboot
                          ota_in_progress=0;
                          ota_image_finished=1;
                          sl_app_log("upload finished. received file size %u bytes\r\n", ota_image_position);

                          break;
                      default:
                          break;
                      }
                  } else if(characteristic == gattdb_ota_data)
                  {
                      if(ota_in_progress)
                      {
                          bootloader_writeStorage(0,//use slot 0
                                  ota_image_position,
                                  evt->data.evt_gatt_server_user_write_request.value.data,
                                  evt->data.evt_gatt_server_user_write_request.value.len);
                          ota_image_position+=evt->data.evt_gatt_server_user_write_request.value.len;
                      }
                  }
                  sl_bt_gatt_server_send_user_write_response(connection,characteristic,0);
              }
                  break;
    ///////////////////////////////////////////////////////////////////////////
    // Default event handler.                                                //
    ///////////////////////////////////////////////////////////////////////////
    default:
      break;
  }
}

#ifdef SL_CATALOG_CLI_PRESENT
void hello(sl_cli_command_arg_t *arguments)
{
  (void) arguments;
  bd_addr address;
  uint8_t address_type;
  sl_status_t sc = sl_bt_system_get_identity_address(&address, &address_type);
  sl_app_assert(sc == SL_STATUS_OK,
             "[E: 0x%04x] Failed to get Bluetooth address\n",
             (int)sc);
  sl_app_log("Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\n",
          address_type ? "static random" : "public device",
          address.addr[5],
          address.addr[4],
          address.addr[3],
          address.addr[2],
          address.addr[1],
          address.addr[0]);
}
#endif // SL_CATALOG_CLI_PRESENT

