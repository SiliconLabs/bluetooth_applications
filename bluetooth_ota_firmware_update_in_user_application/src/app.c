/***************************************************************************//**
 * @file app.c
 * @brief Silicon Labs Empty Example Project
 *
 * This example demonstrates the bare minimum needed for a Blue Gecko C
 * application that allows Over-the-Air Device Firmware Upgrading (OTA DFU).
 * The application starts advertising after boot and restarts advertising after
 * a connection is closed.
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.
 * This software is distributed to you in Source Code format and is governed by
 * the sections of the MSLA applicable to Source Code.
 ******************************************************************************/
#include <stdbool.h>
#include "em_common.h"
#include "app_log.h"
#include "app_assert.h"

#include "app_timer.h"
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

// OTA DFU block size
#define OTA_FIRMWARE_START        0x00
#define OTA_FIRMWARE_END          0x03

static app_timer_t app_timer_handle;

// The advertising set handle allocated from Bluetooth stack
static uint8_t advertising_set_handle = 0xff;
static uint32_t connection;
static uint32_t characteristic;

/* Flag for indicating DFU Reset must be performed */
static BootloaderInformation_t bldInfo;
static BootloaderStorageSlot_t slotInfo;

/* OTA variables */
static uint32_t ota_image_position = 0;
static uint8_t ota_in_progress = 0;
static uint8_t ota_image_finished = 0;
static uint16_t ota_time_elapsed = 0;

/**************************************************************************//**
 * Static function declaration
 *****************************************************************************/
static int32_t get_slot_info(void);
static void erase_slot_if_needed(void);
static void print_progress(void);
static int32_t verify_application(void);
static void app_timer_callback(app_timer_t *handle, void *data);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_status_t sc;

  app_log("...........................................\r\n");
  sc = app_timer_start(&app_timer_handle,
                       200,
                       app_timer_callback,
                       (void *)NULL,
                       true);
  app_assert_status(sc);
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
  int32_t err = BOOTLOADER_OK;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      app_assert_status(sc);
      app_log("Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
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
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      app_log("Boot event - started advertising\r\n");

      // bootloader init must be called before calling other bootloader_xxx API
      bootloader_init();

      // read slot information from bootloader
      if (get_slot_info() == BOOTLOADER_OK) {
        // the download area is erased here (if needed), prior to any
        //   connections are opened
        erase_slot_if_needed();
      } else {
        app_log(
          "Check that you have installed correct type of Gecko bootloader!\r\n");
      }
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("Connection opened\n");
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("Connection closed, reason: 0x%2.2x\r\n",
              evt->data.evt_connection_closed.reason);

      if (ota_image_finished) {
        app_log("Installing new image\r\n");

        bootloader_setImageToBootload(0);
        bootloader_rebootAndInstall();
      } else {
        // Generate data for advertising
        sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                   sl_bt_advertiser_general_discoverable);
        app_assert_status(sc);
        app_log("Restart advertising\n");
        // Restart advertising after client has disconnected.
        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                           sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);
      }
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    case sl_bt_evt_gatt_server_user_write_request_id:
      connection = evt->data.evt_gatt_server_user_write_request.connection;
      characteristic =
        evt->data.evt_gatt_server_user_write_request.characteristic;
      if (characteristic == gattdb_ota_control) {
        switch (evt->data.evt_gatt_server_user_write_request.value.data[0]) {
          case OTA_FIRMWARE_START:// Erase and use slot 0
            ota_image_position = 0;
            ota_in_progress = 1;
            break;

          case OTA_FIRMWARE_END:// END OTA process
            // wait for connection close and then reboot
            ota_in_progress = 0;
            ota_image_finished = 1;
            app_log("upload finished. received file size %lu bytes\r\n",
                    ota_image_position);
            err = verify_application();
            if (err == 0) {
              sl_bt_connection_close(connection);
            }
            break;

          default:
            break;
        }
      } else if (characteristic == gattdb_ota_data) {
        if (ota_in_progress) {
          bootloader_writeStorage(
            0, // use slot 0
            ota_image_position,
            evt->data.evt_gatt_server_user_write_request.value.data,
            evt->data.evt_gatt_server_user_write_request.value.len);
          ota_image_position +=
            evt->data.evt_gatt_server_user_write_request.value.len;
        }
      }
      sl_bt_gatt_server_send_user_write_response(connection, characteristic, 0);
      break;
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

/**************************************************************************//**
 * Static function definition
 *****************************************************************************/
static int32_t get_slot_info(void)
{
  int32_t err;

  bootloader_getInfo(&bldInfo);
  app_log("Gecko bootloader version: %lu.%lu\r\n",
          (bldInfo.version & 0xFF000000) >> 24,
          (bldInfo.version & 0x00FF0000) >> 16);

  err = bootloader_getStorageSlotInfo(0, &slotInfo);

  if (err == BOOTLOADER_OK) {
    app_log("Slot 0 starts @ 0x%8.8lx, size %lu bytes\r\n",
            slotInfo.address,
            slotInfo.length);
  } else {
    app_log("Unable to get storage slot info, error %lx\r\n", err);
  }

  return(err);
}

static void erase_slot_if_needed(void)
{
  uint32_t offset = 0, num_blocks = 0, i = 0;
  uint8_t buffer[256];
  bool dirty = false;
  int32_t err = BOOTLOADER_OK;

  /* check the download area content by reading it in 256-byte blocks */
  num_blocks = slotInfo.length / 256;

  while ((dirty == 0) && (offset < 256 * num_blocks) && (err == BOOTLOADER_OK))
  {
    err = bootloader_readStorage(0, offset, buffer, 256);
    if (err == BOOTLOADER_OK) {
      i = 0;
      while (i < 256)
      {
        if (buffer[i++] != 0xFF) {
          dirty = true;
          break;
        }
      }
      offset += 256;
    }
    app_log(".");
  }

  if (err != BOOTLOADER_OK) {
    app_log("error reading flash! %lx\r\n", err);
  } else if (dirty) {
    app_log("download area is not empty, erasing...\r\n");
    bootloader_eraseStorageSlot(0);
    app_log("done\r\n");
  } else {
    app_log("download area is empty\r\n");
  }

  return;
}

static void print_progress(void)
{
  // estimate transfer speed in kbps
  int kbps = ota_image_position * 8 / (1024 * ota_time_elapsed);

  app_log("pos: %lu, time: %u, kbps: %u\r\n",
          ota_image_position,
          ota_time_elapsed,
          kbps);
}

static int32_t verify_application(void)
{
  int32_t err;
  err = bootloader_verifyImage(0, NULL);
  if (err != BOOTLOADER_OK) {
    app_log("application verification failed. err: %lx \r\n", err);
  } else {
    app_log("application verified");
  }
  return err;
}

static void app_timer_callback(app_timer_t *handle, void *data)
{
  (void) handle;
  (void) data;
  if (ota_in_progress) {
    ota_time_elapsed++;
    print_progress();
  }
}
