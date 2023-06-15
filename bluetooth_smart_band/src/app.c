/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided \'as-is\', without any express or implied
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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#include "sl_simple_button_instances.h"
#include "sl_simple_led_instances.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "sl_sleeptimer.h"
#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif // SL_COMPONENT_CATALOG_PRESENT
#ifdef SL_CATALOG_CLI_PRESENT
#include "sl_cli.h"
#endif // SL_CATALOG_CLI_PRESENT
#include "sl_sensor_rht.h"
#include "app.h"
#include "sl_smartwatch_ui.h"
#include <stdio.h>
#include <sl_string.h>

// -----------------------------------------------------------------------------
// Configuration

// To configure/customize display parameters, uncomment the following lines of
// codes and change the values
#define DISPLAY_SETTINGS
#define COL_OFFSET                  2
#define ROW_OFFSET                  4
#define FONT_TYPE                   ((GLIB_Font_t *)&GLIB_FontNormal8x8)
#define BUFFER_SIZE                 17

// other config parameters */
// number of notifications to be stored
#define NOTIF_COUNT                 10
// size of each notification
#define NOTIF_SIZE                  100
// Time threshold between two presses to be taken as double click (in ms)
#define DOUBLE_CLICK_THRESHOLD      250
// number of lines to scroll on display
#define LINES_TO_SCROLL             2

char notifData[NOTIF_COUNT][NOTIF_SIZE];
uint8_t notifPtr = NOTIF_COUNT;
// batteryLevel[6]: 00-not charging, 01-charging, 02-charged;
// batteryLevel[7]: battery %
uint8_t batteryLevel[] = { 0xAB, 0x00, 0x05, 0xFF, 0x91, 0x80, 0x00, 0x64 };

// {last press time of btn0, present press time of btn0,
// last press time of btn1, present press time of btn1};
uint32_t click_times[4] = { 0, 0, 0, 0 };

// flags for the currently open screen
bool timeScreen = false;
bool notifScreen = false;
bool canScrollUp = false;
bool canScrollDown = false;

// button press flags
bool btn_0_flag = false;
bool btn_1_flag = false;

sl_sleeptimer_date_t present;                   // time structure for system
                                                //   time
char formattedTime[100] = "";

uint8_t row_count = 0;
int16_t current_scroll = 0;          // to keep the track of row while scrolling

// screen configuration values, defined in sl_smartwatch_ui_init()
extern uint8_t max_rows_on_display;
extern uint8_t row_height;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// Button state.
static bool app_btn0_pressed = false;
static bool app_btn1_pressed = false;

// Periodic timer handle.
// static sl_simple_timer_t app_periodic_timer;

/**************************************************************************//**
 * Filters the received data. Stores information in the respective location
 * based on the information structure of DT78 app
 *
 * @param[in] data a pointer to uint8_t array upto 255 bytes in size.
 * @param[in] len the actual size of data[]
 *****************************************************************************/
void dataFilter(uint8_t len, uint8_t data[]);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  sl_smartwatch_ui_init();
  app_log_info("Smart band initialized\n");
}

#ifndef SL_CATALOG_KERNEL_PRESENT

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

#endif

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

  // Handle stack events
  switch (SL_BT_MSG_ID(evt->header)) {
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      app_log_info("Bluetooth stack booted: v%d.%d.%d-b%d\n",
                   evt->data.evt_system_boot.major,
                   evt->data.evt_system_boot.minor,
                   evt->data.evt_system_boot.patch,
                   evt->data.evt_system_boot.build);

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

      sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id, 0,
                                                   sizeof(system_id),
                                                   system_id);
      app_assert_status(sc);

      app_log_info("Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\n",
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
      sc = sl_bt_advertiser_set_timing(advertising_set_handle, // handle
                                       160, // min. adv. interval (milliseconds
                                            //   * 1.6)
                                       160, // max. adv. interval (milliseconds
                                            //   * 1.6)
                                       0, // adv. duration
                                       0); // max. num. adv. events
      app_assert_status(sc);

      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      app_log_info("Started advertising\n");
      break;

    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log_info("Connection opened\n");

      LOG("Turning screen off and clearing notification\n");
      sl_smartwatch_ui_clear_screen();
      sl_smartwatch_ui_update();
      timeScreen = false;
      notifScreen = false;
      canScrollUp = false;
      canScrollDown = false;

#ifdef SL_CATALOG_BLUETOOTH_FEATURE_POWER_CONTROL_PRESENT
      // Set remote connection power reporting - needed for Power Control
      sc = sl_bt_connection_set_remote_power_reporting(
        evt->data.evt_connection_opened.connection,
        sl_bt_connection_power_reporting_enable);
      app_assert_status(sc);
#endif // SL_CATALOG_BLUETOOTH_FEATURE_POWER_CONTROL_PRESENT

      break;

    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log_info("Connection closed\n");

      LOG("Turning screen off and clearing notification\n");
      sl_smartwatch_ui_clear_screen();
      sl_smartwatch_ui_update();
      timeScreen = false;
      notifScreen = false;
      canScrollUp = false;
      canScrollDown = false;
      notifPtr = NOTIF_COUNT;
      // clearing notification memory
      memset(notifData, 0, NOTIF_COUNT * NOTIF_SIZE * sizeof(notifData[0][0]));

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      app_log_info("Started advertising\n");
      break;

    case sl_bt_evt_gatt_server_attribute_value_id:
      dataFilter(evt->data.evt_gatt_server_attribute_value.value.len,
                 evt->data.evt_gatt_server_attribute_value.value.data);
      break;

    // Default event handler.
    default:
      break;
  }
}

/**************************************************************************//**
 * Show time on the display
 * Don't call this before initializing display
 *****************************************************************************/
void show_time()
{
  sl_status_t sc;
  int32_t temperature = 0;
  uint32_t humidity = 0;
  float tmp_c = 0.0;

  sc = sl_sleeptimer_get_datetime(&present);
  app_assert(sc == SL_STATUS_OK, "Failed to get date and time");
  snprintf(formattedTime,
           sizeof(formattedTime),
           "%d:%d \n%d/%d/%d\n",
           present.hour,
           present.min,
           present.month_day,
           ((uint8_t)present.month + 1),
           present.year + 1900);
  sl_smartwatch_ui_print_time(formattedTime);

  // Measure temperature; units are % and milli-Celsius.
  sc = sl_sensor_rht_get(&humidity, &temperature);
  if (sc != SL_STATUS_OK) {
    app_log_warning("Invalid RHT reading: %lu %ld\n", humidity, temperature);
  }

  char tempString[14] = "";
  tmp_c = (float) temperature / 1000;
  snprintf(tempString, sizeof(tempString), "Temp: %5.2f C", tmp_c);

  sl_smartwatch_ui_print_text_wrapped(tempString);

  if (notifPtr == NOTIF_COUNT) { // if there are no notifications
    sl_smartwatch_ui_print_text_wrapped("No notifications");
  }
  sl_smartwatch_ui_update();
  LOG("PRINTED TIME AND TEMPERATURE ON SCREEN\n");
}

/**************************************************************************//**
 * Show notification data stored in the memory on the display
 * Don't call this before initializing display
 *
 * @param[in] xOffset value to be offset on x axis while printing. positive
 *   values shift the data towards right and negative values shift them to left
 * @param[in] yOffset value to be offset on y axis while printing. positive
 *   values shift the data downwards and negative values shifts it upwards
 *****************************************************************************/
void show_notif(uint16_t xOffset, uint16_t yOffset)
{
  char tempString[NOTIF_COUNT * NOTIF_SIZE] = "";
  uint16_t j = 0;
  for (uint8_t i = notifPtr; i < NOTIF_COUNT; i++) {
    sl_strcat_s(tempString, sizeof(tempString), notifData[i]);
    sl_strcat_s(tempString, sizeof(tempString), "\n");
  }
  for (uint16_t i = 0; i < sl_strlen(tempString); i++) {
    if ((!((tempString[i] < ' ') || (tempString[i] > '~')))
        || (tempString[i] == '\n')) {
      tempString[j] = tempString[i];
      j++;
    }
  }
  char notifString[j];
  for (uint16_t i = 0; i < j; i++) {
    notifString[i] = tempString[i];
  }
  sl_smartwatch_ui_print_text_with_offset(notifString, xOffset, yOffset);
  sl_smartwatch_ui_update();
  LOG("NOTIF PRINTED\n");
}

/**************************************************************************//**
 * Callback function of gatt server attribute value.
 *
 * @param[in] uint8array that has two elements data[] and len wherein data is a
 *   pointer to uint8_t array upto 255 bytes in size.
 *****************************************************************************/
void dataFilter(uint8_t len, uint8_t data[])
{
  // data format: AB 00 xx FF yy
  // xx = length;  yy = command type;
  // for command types, refer GitHub of mobile app (DT78)

  sl_status_t sc;

  uint8_t pData[len];

  for (int i = 0; i < (len); i++) {
    pData[i] = *((data) + i);
  }

  if (pData[0] != 0xAB) {
    // extension message of long notifs
    uint8_t packetNo = pData[0];
    for (uint8_t i = 1; i < len; i++) {
      notifData[notifPtr][(packetNo * 19) + 12 + i - 1] = (char) pData[i];
    }
    row_count = 0;
    for (uint8_t i = notifPtr; i < NOTIF_COUNT; i++) {
      row_count += sl_smartwatch_ui_char_to_rows((uint8_t) (sl_strlen(
                                                              notifData[i])));
    }
    if (row_count > max_rows_on_display) {
      current_scroll = 0;
      canScrollUp = false;
      canScrollDown = true;
    }
    sl_smartwatch_ui_clear_screen();
    timeScreen = false;
    notifScreen = true;
    show_notif(0, 0);
  } else {
    switch (pData[4]) {
      case 0x93:
        // for reference:
        // packet structure: AB 00 xx FF 93 80 00 YY YY MM DD h m s
        // build_datetime() input parameters : ptr, YY, MM, DD, h, m, s, offset
        // offset is set to zero because mobile app (DT78) sends local time
        // month structure iterates from 0 to 11 rather than 1 to 12 as normal
        //   date format, hence - 1.
        sc = sl_sleeptimer_build_datetime(&present,
                                          (pData[8] | pData[7] << 8),
                                          (pData[9] - 1),
                                          pData[10],
                                          pData[11],
                                          pData[12],
                                          pData[13],
                                          0);
        app_assert(SL_STATUS_OK == sc,
                   "Error while building date and time. ERR CODE: %ld\n",
                   sc);
        sc = sl_sleeptimer_set_datetime(&present);
        app_assert(SL_STATUS_OK == sc,
                   "Error while setting date and time. ERR CODE: %ld\n",
                   sc);
        LOG("Time set successfully\n");
        break;

      // battery level request
      case 0x91:

        // No actual battery connected, hence sending a mock value
        // UUID: 6e 40 00 03-b5 a3-f3 93-e0 a9-e5 0e 24 dc ca 9e
        // in little endian format
        batteryLevel[7]--; // reducing battery level by 1% on every request

        sl_status_t sc;

        uint8_t char_uuid[] = { 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
                                0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40,
                                0x6E };
        uint16_t char_handle = 0;

        sc = sl_bt_gatt_server_find_attribute(0x01, 0x10, char_uuid,
                                              &char_handle);
        app_assert_status(sc);

        sc = sl_bt_gatt_server_send_notification(0x01, char_handle, 8,
                                                 batteryLevel);
        app_assert_status(sc);
        LOG("BATTERY LEVEL SENT\n");
        break;

      case 0x72:

        if (notifPtr == 0) {
          // overwrite notifs when allocated memory is full
          notifPtr = NOTIF_COUNT;
        }

        switch (pData[6]) {
          case 0x01:

            notifPtr--;
            char prefix_str[] = "Call from: ";
            for (uint8_t i = 0; i < sl_strlen(prefix_str); i++) {
              notifData[notifPtr][i] = (char) prefix_str[i];
            }
            for (uint8_t i = sl_strlen(prefix_str);
                 i < (len - 8 + sl_strlen(prefix_str)); i++) {
              notifData[notifPtr][i] = (char) pData[i + 8 - sl_strlen(
                                                      prefix_str)];
            }
            LOG("saved CALL INFO to memory\n");
            break;

          default:
            notifPtr--;
            for (uint8_t i = 0; i < (len - 8); i++) {
              notifData[notifPtr][i] = (char) pData[i + 8];
            }
            break;
        }

        row_count = 0;

        for (uint8_t i = notifPtr; i < NOTIF_COUNT; i++) {
          row_count += sl_smartwatch_ui_char_to_rows(
            (uint8_t) (sl_strlen(notifData[i])));
        }

        if (row_count > max_rows_on_display) {
          current_scroll = 0;
          canScrollUp = false;
          canScrollDown = true;
        }

        sl_smartwatch_ui_clear_screen();
        timeScreen = false;
        notifScreen = true;
        show_notif(0, 0);
        break;

      case 0x71:

        app_log_info("SOMEONE IS LOOKING FOR ME \n");
        break;

      default:
        app_log_info("Unserved app command type = %d\n", pData[4]);
        break;
    }
  }
}

void sl_button_on_change(const sl_button_t *handle)
{
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED) {
    if (&sl_button_btn0 == handle) {
      sl_led_turn_off(&sl_led_led0);
      app_btn0_pressed = true;

      click_times[1] = (uint32_t) sl_sleeptimer_tick_to_ms(
        sl_sleeptimer_get_tick_count());

      if (!(timeScreen || notifScreen)) {
        // Open time screen
        show_time();
        timeScreen = true;
        notifScreen = false;
        canScrollUp = false;
        canScrollDown = false;
      } else if (timeScreen) {
        // Open notif screen
        timeScreen = false;
        notifScreen = true;
        canScrollUp = false;
        canScrollDown = false;

        if (notifPtr == NOTIF_COUNT) {
          sl_smartwatch_ui_clear_screen();
          sl_smartwatch_ui_print_text_wrapped("No notifications");
          sl_smartwatch_ui_update();
        } else {
          sl_smartwatch_ui_clear_screen();
          row_count = 0;
          for (uint8_t i = notifPtr; i < NOTIF_COUNT; i++) {
            row_count += sl_smartwatch_ui_char_to_rows(
              (uint8_t) (sl_strlen(notifData[i])));
          }
          if (row_count > max_rows_on_display) {
            current_scroll = 0;
            canScrollUp = false;
            canScrollDown = true;
          }
          show_notif(0, 0);
        }
      } else if (notifScreen) {
        btn_0_flag = true;

        if (click_times[1] - click_times[0] < DOUBLE_CLICK_THRESHOLD) {
          LOG("DOUBLE CLICK 0\n");
          btn_0_flag = false;

          if (canScrollDown) {
            current_scroll -= (LINES_TO_SCROLL * row_height);
            sl_smartwatch_ui_clear_screen();
            show_notif(0, current_scroll);
            LOG("SCROLLING DOWN\n");
            canScrollUp = true;
            canScrollDown = true;
            if ((abs(current_scroll) + (max_rows_on_display * row_height))
                > (row_count * row_height)) {
              current_scroll = 0;
              canScrollUp = true;
              canScrollDown = false;
            }
          }
        } else if (btn_1_flag) {
          LOG("SWIPE 1 TO 0\n");
          btn_0_flag = false;
          btn_1_flag = false;
          // reserved (no functionality)
        }
      }

      click_times[0] = (uint32_t) click_times[1];
    } else if (&sl_button_btn1 == handle) {
      sl_led_turn_off(&sl_led_led1);
      app_btn1_pressed = true;

      click_times[3] = (uint32_t) sl_sleeptimer_tick_to_ms(
        sl_sleeptimer_get_tick_count());

      if (!(timeScreen || notifScreen)) {
        // Open time screen
        show_time();
        timeScreen = true;
        notifScreen = false;
      } else if (timeScreen) {
        // Turn off screen
        LOG("TURNING SCREEN OFF\n");
        sl_smartwatch_ui_clear_screen();
        sl_smartwatch_ui_update();
        timeScreen = false;
        notifScreen = false;
        canScrollUp = false;
        canScrollDown = false;
      } else if (notifScreen) {
        // Open time screen
        btn_1_flag = true;

        if (click_times[3] - click_times[2] < DOUBLE_CLICK_THRESHOLD) {
          LOG("DOUBLE CLICK 1\n");
          btn_1_flag = false;

          if (canScrollUp) {
            current_scroll += (LINES_TO_SCROLL * row_height);
            sl_smartwatch_ui_clear_screen();
            show_notif(0, current_scroll);
            LOG("SCROLLING UP\n");
            canScrollUp = true;
            canScrollDown = true;
            if (current_scroll >= 0) {
              current_scroll = 0;
              canScrollUp = false;
              canScrollDown = true;
            }
          }
        } else if (btn_0_flag) {
          LOG("SWIPE 0 TO 1\n");
          btn_0_flag = false;
          btn_1_flag = false;
          sl_smartwatch_ui_clear_screen();
          show_time();
          timeScreen = true;
          notifScreen = false;
        }
      }
      click_times[2] = (uint32_t) click_times[3];
    }
  } else if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
    if (&sl_button_btn0 == handle) {
      sl_led_turn_on(&sl_led_led0);
      app_btn0_pressed = false;
    } else if (&sl_button_btn1 == handle) {
      sl_led_turn_on(&sl_led_led1);
      app_btn1_pressed = false;
    }
  }
}

#ifdef SL_CATALOG_CLI_PRESENT
void hello(sl_cli_command_arg_t *arguments)
{
  (void) arguments;
  bd_addr address;
  uint8_t address_type;
  sl_status_t sc = sl_bt_system_get_identity_address(&address, &address_type);
  app_assert_status(sc);
  app_log_info("Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\n",
               address_type ? "static random" : "public device",
               address.addr[5],
               address.addr[4],
               address.addr[3],
               address.addr[2],
               address.addr[1],
               address.addr[0]);
}

#endif // SL_CATALOG_CLI_PRESENT
