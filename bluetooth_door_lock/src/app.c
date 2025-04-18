/**************************************************************************//**
 * @file app.c
 * @brief Application interface provided to main().
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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
// Includes bluetooth core headers
#include "sl_common.h"
#include "em_cmu.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app_log.h"

// Includes components used headers
#include "sl_sleeptimer.h"
#include "sl_spidrv_instances.h"
#include "sl_i2cspm_instances.h"
#include "sl_pwm_instances.h"
#include "sl_simple_led_instances.h"

#include "app_log.h"
#include <glib.h>
#include <stdlib.h>

#include "mikroe_cap1166.h"
#include "mikroe_cmt_8540s_smt.h"
#include "buzz2.h"
#include "glib.h"
#include "glib_font.h"
#include "micro_oled_ssd1306.h"
#include "sl_udelay.h"
#include "mikroe_cap1166_config.h"
#include "gpiointerrupt.h"

/*******************************************************************************
 *****************************    DEFINE     ***********************************
 ******************************************************************************/
// Address to storage the door unlock password in Non-volatile memory
#define PASSWORD_NVM_KEY  0x4000

// The door unlock password length
#define PASSWORD_LEN      6

// The default passkey for the first time connect
// This passkey will be changed to be the same with door password every time
// update new door password
static uint32_t passkey = 123456;

#define DOOR_OPEN                             1 // Door open
#define DOOR_CLOSE                            0 // Door close

// External event
#define SLEEP_TIMER_EVENT                     0x02
#define CAP1166_IRQ_EVENT                     0x01

#define W                                     4 * Q // Whole 4/4 - 4 Beats
#define H                                     2 * Q // Half 2/4 - 2 Beats
#define Q                                     250 // Quarter 1/4 - 1 Beat
#define E                                     Q / 2 // Eighth 1/8 - 1/2 Beat
#define S                                     Q / 4 // Sixteenth 1/16 - 1/4 Beat

#define CAP1166_INT_MASK                      0x01

/*
 * Interrupt reason
 */
#define CAP1166_RESET_INT_MASK                (1 << 3)
#define CAP1166_TOUCH_DETECTED_INT_MASK       (1 << 0)
#define CAP1166_MULTI_TOUCH_PATTERN_INT_MASK  (1 << 1)

/*
 * Touch detection status
 */
#define CAP1166_BUTTON_PRESSED                0x01
#define CAP1166_BUTTON_RELEASED               0x02
#define CAP1166_BUTTON_NOT_DETECTED           0x00

// Handle structure for the door lock application
typedef struct {
  // Currently entered password cursor
  uint8_t password_cursor;

  // Currently entered password buffer
  uint8_t password_buffer[PASSWORD_LEN];

  uint8_t door_status;
} door_lock_handle_t;

/*******************************************************************************
 *****************************   VARIABLE    **********************************
 ******************************************************************************/
// The handle structure for the door lock application
static door_lock_handle_t door_lock_handle;

// The instance for OLED LCD
static glib_context_t glib_context;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xFF;

static uint8_t ble_bonding_handle = 0xFF;

// Sound volume
static int volume = 70;

// Sleep timer handle
static sl_sleeptimer_timer_handle_t auto_lock_the_door;
static volatile bool app_init_driver_done = false;

/*******************************************************************************
 *****************************   PROTOTYPE   ***********************************
 ******************************************************************************/

// Initialize the components used in the door lock application.
// Called in the Bluetooth system boot event after the radio is ready.
static void door_lock_init(void);

// Service the capacitive button pressed event.
// Called in the Bluetooth external event.
static void capacitive_button_pressed_handler(void);

// Check the password being entered is correct or incorrect
static uint8_t check_the_password(void);

// Sleep timer callback function
static void sleep_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                 void *data);

// Draw the application menu on the OLED display
static void oled_draw_static_menu(void);

// Print the message to OLED display
static void oled_update_message(char *message, uint8_t len, uint8_t x0);

// Print the lock's status to OLED display
static void oled_update_lock_status(char *status, uint8_t len, uint8_t x0);

// These following functions service the corresponding Bluetooth event.
// Called in the function sl_bt_on_event.
static void connection_parameters_handler(sl_bt_msg_t *evt);
static void connection_opened_handler(sl_bt_msg_t *evt);
static void connection_closed_handler(sl_bt_msg_t *evt);
static void sm_bonding_failed_handler(sl_bt_msg_t *evt);
static void sm_confirm_bonding_handler(sl_bt_msg_t *evt);
static void gatt_server_user_write_request_handler(sl_bt_msg_t *evt);
static void cap1166_alert_gpio_callback(uint8_t intNo, void *ctx);

/*******************************************************************************
 *******************************    CODE     ***********************************
 ******************************************************************************/

// Initialize the components used in the door lock application.
// Called in the Bluetooth system boot event after the radio is ready.
static void app_cap1166_init(void);
static void app_buzz2_cmt_8540s_init(void);
static void app_oled_init(void);

void cap1166_alert_gpio_init(void)
{
  unsigned int interrupt;

  CMU_ClockEnable(cmuClock_GPIO, true);

  GPIO_PinModeSet(CAP1166_ALERT_PORT, CAP1166_ALERT_PIN,
                  gpioModeInput,
                  0);

  GPIOINT_Init();
  interrupt = GPIOINT_CallbackRegisterExt(CAP1166_ALERT_PIN,
                                          cap1166_alert_gpio_callback,
                                          NULL);

  EFM_ASSERT(interrupt != INTERRUPT_UNAVAILABLE);
  GPIO_ExtIntConfig(CAP1166_ALERT_PORT,
                    CAP1166_ALERT_PIN,
                    interrupt,
                    false,
                    true,
                    true);

  GPIO_IntEnable(CAP1166_ALERT_PIN);
}

static void app_cap1166_init(void)
{
  sl_status_t ret_code;

  ret_code = mikroe_cap1166_init(sl_spidrv_mikroe_handle);
  app_assert_status(ret_code);

  mikroe_cap1166_reset();

  sl_sleeptimer_delay_millisecond(200);

  mikroe_cap1166_default_cfg();
  sl_sleeptimer_delay_millisecond(100);

  cap1166_alert_gpio_init();

  app_log(">> Cap Touch 2 is initialized\r\n");
}

static void app_buzz2_cmt_8540s_init(void)
{
  sl_status_t ret_code;
  ret_code = mikroe_cmt_8540s_smt_init(&sl_pwm_mikroe);
  app_assert_status(ret_code);

  mikroe_cmt_8540s_smt_set_duty_cycle(0.0);
  mikroe_cmt_8540s_smt_pwm_start();
  sl_sleeptimer_delay_millisecond(100);

  app_log(">> Buzzer 2 is initialized\r\n");
}

static void app_oled_init(void)
{
  /* Initialize the display */
  ssd1306_init(sl_i2cspm_qwiic);
  glib_init(&glib_context);

  /* Fill lcd with background color */
  glib_clear(&glib_context);

  app_log(">> Oled is initialized\r\n");
}

static void door_lock_init(void)
{
  app_cap1166_init();
  app_buzz2_cmt_8540s_init();

  /* Initialize the OLED display */
  app_oled_init();

  /* Draw the Door lock menu to the OLED display */
  oled_draw_static_menu();

  app_init_driver_done = true;
}

// Service the capacitive button pressed event.
// Called in the Bluetooth external event.
static void capacitive_button_pressed_handler(void)
{
  uint8_t sensor_input[6];
  uint8_t pressed_button = 0;

  mikroe_cap1166_detect_touch(sensor_input);

  for (pressed_button = 0; pressed_button < 6; pressed_button++) {
    // Find the pressed button
    if (sensor_input[pressed_button] == CAP1166_BUTTON_PRESSED) {
      // Play sound on the buzzer
      mikroe_cmt_8540s_smt_play_sound(BUZZ2_NOTE_C6, volume, S);

      if (door_lock_handle.password_cursor == 0) {
        glib_fill_rect(&glib_context, 0, 12, 64, 8, GLIB_BLACK);
        glib_update_display();
      }

      door_lock_handle.password_buffer[door_lock_handle.password_cursor]
        = pressed_button + '1';
      door_lock_handle.password_cursor++;

      oled_update_message("******",
                          door_lock_handle.password_cursor,
                          8);
    }
  }

  if (door_lock_handle.password_cursor >= PASSWORD_LEN) {
    // Clear the message box
    glib_fill_rect(&glib_context, 0, 12, 64, 8, GLIB_BLACK);
    glib_update_display();

    // Check the currently entered password
    if (check_the_password()) {
      app_log("Unlock the door !!\r\n");

      // Turn on the LED
      sl_led_turn_on(&sl_led_led0);

      // Clear the lock status box
      glib_fill_rect(&glib_context, 0, 34, 64, 8, GLIB_BLACK);
      glib_update_display();

      oled_update_message("correct", 7, 8);
      oled_update_lock_status("UNLOCK", 6, 10);

      // Play success sound on buzzer
      mikroe_cmt_8540s_smt_play_sound(BUZZ2_NOTE_G7, volume, S);
      mikroe_cmt_8540s_smt_play_sound(BUZZ2_NOTE_G7, volume, S);
      mikroe_cmt_8540s_smt_play_sound(BUZZ2_NOTE_G7, volume, S);
    } else {
      app_log("Lock the door !!\r\n");

      // Turn off the LED
      sl_led_turn_off(&sl_led_led0);

      oled_update_message("incorrect", 9, 1);
      oled_update_lock_status(" LOCK ", 6, 10);

      mikroe_cmt_8540s_smt_play_sound(BUZZ2_NOTE_G5, volume, H);
    }
    door_lock_handle.password_cursor = 0;

    /* Start sleep timer to auto close the door and clear
     * the display after 5 seconds*/
    sl_sleeptimer_start_timer_ms(&auto_lock_the_door,
                                 5000,
                                 sleep_timer_callback,
                                 (void *)NULL,
                                 0,
                                 0);
  }
}

// Check the password being entered is correct or incorrect
static uint8_t check_the_password(void)
{
  uint8_t unnlock_pwd[6];
  size_t len;

  // read password unlock door from nvm
  sl_bt_nvm_load(PASSWORD_NVM_KEY, sizeof(unnlock_pwd), &len, unnlock_pwd);
  app_log("password: %s\r\n", unnlock_pwd);

  if (door_lock_handle.password_cursor == PASSWORD_LEN) {
    if (!memcmp(door_lock_handle.password_buffer, unnlock_pwd, PASSWORD_LEN)) {
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

// Draw the application menu on the OLED display
static void oled_draw_static_menu(void)
{
  /* Fill OLED with background color */
  glib_clear(&glib_context);

  glib_draw_string(&glib_context, "Enter pw:", 0, 0);

  glib_draw_string(&glib_context, "The door:", 0, 24);
  oled_update_lock_status(" LOCK ", 6, 10);

  glib_update_display();
}

// Print the message to OLED display
static void oled_update_message(char *message, uint8_t len, uint8_t x0)
{
  static char buffer[10];

  // Maximum 9 character per line, size 6x8
  for (int j = 0; j < len; j++) { buffer[j] = message[j]; }
  buffer[len] = 0;

  glib_draw_string(&glib_context, buffer, x0, 12);
  glib_update_display();
}

// Print the lock's status to OLED display
static void oled_update_lock_status(char *status, uint8_t len, uint8_t x0)
{
  static char buffer[7];

  // Maximum 6 character per line, size 7x10
  for (int j = 0; j < len; j++) { buffer[j] = status[j]; }
  buffer[len] = 0;

  glib_draw_string(&glib_context, buffer, x0, 34);
  glib_update_display();
}

// Service the parameters event
static void connection_parameters_handler(sl_bt_msg_t *evt)
{
  uint8_t security_level = evt->data.evt_connection_parameters.security_mode
                           + 1;
  uint16_t timeout = evt->data.evt_connection_parameters.timeout;

  app_log("Bluetooth Stack Event : CONNECTION Parameters ID mode = %d\r\n",
          evt->data.evt_connection_parameters.security_mode);

  // If security is less than 2 increase so devices can bond
  if (security_level <= 2) {
    app_log("Bluetooth Stack Event : CONNECTION PARAMETERS : \
              SecLvl : %d, timeout : %d\r\n",
            security_level,
            timeout);
    app_log("+ Bonding Handle is: 0x%04X\r\n", ble_bonding_handle);
  } else {
    app_log("[OK]      Bluetooth Stack Event : CONNECTION PARAMETERS : \
             SecLvl : %d, Timeout : %d\r\n",
            security_level,
            timeout);
  }
}

// Service the connection opened event
static void connection_opened_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  uint8_t active_connection_id = 0;

  app_log("Bluetooth Stack Event : CONNECTION OPENED\r\n");

  sc = sl_bt_advertiser_stop(advertising_set_handle);
  app_assert(sc == SL_STATUS_OK,
             "[E: 0x%04x] Failed to stop the advertising\r\n",
             (int)sc);

  active_connection_id = evt->data.evt_connection_opened.connection;
  ble_bonding_handle = evt->data.evt_connection_opened.bonding;

  if (ble_bonding_handle == 0xFF) {
    app_log("+ Increasing security\r\n");

    sc = sl_bt_sm_increase_security(active_connection_id);
    app_assert(sc == SL_STATUS_OK,
               "[E: 0x%04x] Failed to enhance the security\r\n",
               (int)sc);
  } else {
    app_log("+ Already Bonded (ID: %d)\r\n", ble_bonding_handle);
  }
}

// Service the connection closed event
static void connection_closed_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  // reset bonding handle variable to avoid deleting wrong bonding info
  ble_bonding_handle = 0xFF;

  app_log("Bluetooth Stack Event : CONNECTION CLOSED (reason: 0x%04X)\r\n",
          evt->data.evt_connection_closed.reason);

  // Restart advertising after client has disconnected.
  sc = sl_bt_legacy_advertiser_start(
    advertising_set_handle,
    sl_bt_legacy_advertiser_connectable);
  app_assert(sc == SL_STATUS_OK,
             "[E: 0x%04x] Failed to start advertising\r\n",
             (int)sc);
}

// Service the security management bonding failed event
static void sm_bonding_failed_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  uint8_t connection_handle = evt->data.evt_sm_bonding_failed.connection;
  uint16_t reason = evt->data.evt_sm_bonding_failed.reason;

  app_log("Bluetooth Stack Event : BONDING FAILED (connection: %d, \
          reason: 0x%04X, bondingHandle: 0x%04X)\r\n",
          connection_handle,
          reason,
          ble_bonding_handle);

  if ((reason == SL_STATUS_BT_SMP_PASSKEY_ENTRY_FAILED)
      || (reason == SL_STATUS_TIMEOUT)) {
    app_log("+ Increasing security... because reason is 0x%04x\r\n", reason);

    sc = sl_bt_sm_increase_security(connection_handle);
    app_assert(sc == SL_STATUS_OK,
               "[E: 0x%04x] Failed to enhance the security\r\n",
               (int)sc);
  } else if ((reason == SL_STATUS_BT_SMP_PAIRING_NOT_SUPPORTED)
             || (reason == SL_STATUS_BT_CTRL_PIN_OR_KEY_MISSING)) {
    if (ble_bonding_handle != 0xFF) {
      app_log("+ Broken bond, deleting ID:%d...\r\n", ble_bonding_handle);

      sc = sl_bt_sm_delete_bonding(ble_bonding_handle);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to delete specified bonding \
                     information or whitelist\r\n",
                 (int)sc);

      sc = sl_bt_sm_increase_security(
        evt->data.evt_connection_opened.connection);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to enhance the security\r\n",
                 (int)sc);

      ble_bonding_handle = 0xFF;
    } else {
      app_log("+ Increasing security in one second...\r\n");

      sc = sl_bt_sm_increase_security(connection_handle);
      app_log("Result... = 0x%04lx\r\n", sc);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to enhance the security\r\n",
                 (int)sc);

      if (sc == SL_STATUS_INVALID_STATE) {
        app_log("+ Trying to increase security again");

        sc = sl_bt_sm_increase_security(connection_handle);
        app_assert(sc == SL_STATUS_OK,
                   "[E: 0x%04x] Failed to enhance the security\r\n",
                   (int)sc);
      }
    }
  } else if (reason == SL_STATUS_BT_SMP_UNSPECIFIED_REASON) {
    app_log("+ Increasing security... because reason is 0x0308\r\n");

    sc = sl_bt_sm_increase_security(connection_handle);
    app_assert(sc == SL_STATUS_OK,
               "[E: 0x%04x] Failed to enhance the security\r\n",
               (int)sc);
  } else {
    app_log("+ Close connection : %d",
            evt->data.evt_sm_bonding_failed.connection);

    sc = sl_bt_connection_close(evt->data.evt_sm_bonding_failed.connection);
    app_assert(sc == SL_STATUS_OK,
               "[E: 0x%04x] Failed to close connection\r\n",
               (int)sc);
  }
}

// Service the security management confirm bonding event
static void sm_confirm_bonding_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint8_t connection_handle = evt->data.evt_connection_parameters.connection;

  app_log("Bluetooth Stack Event : CONFIRM BONDING\r\n");

  sc = sl_bt_sm_bonding_confirm(connection_handle, 1);
  app_assert(sc == SL_STATUS_OK,
             "[E: 0x%04x] Failed to confirm bonding requests\r\n",
             (int)sc);
}

// Service the gatt server user write request event
static void gatt_server_user_write_request_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  // Allocate buffer to store characteristic's value
  uint8_t user_buf[6] = { 0x00 };
  uint16_t len = 0;

  if (evt->data.evt_gatt_server_user_write_request.characteristic
      == gattdb_new_password) {
    len = evt->data.evt_gatt_server_user_write_request.offset
          + evt->data.evt_gatt_server_user_write_request.value.len;

    if (len == sizeof(user_buf)) {
      memcpy(user_buf + evt->data.evt_gatt_server_user_write_request.offset,
             evt->data.evt_gatt_server_user_write_request.value.data,
             evt->data.evt_gatt_server_user_write_request.value.len);

      sc = sl_bt_gatt_server_send_user_write_response(
        evt->data.evt_gatt_server_user_write_request.connection,
        evt->data.evt_gatt_server_user_write_request.characteristic,
        (uint8_t)SL_STATUS_OK);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to send a write response\n",
                 (int)sc);

      sc = sl_bt_nvm_save(PASSWORD_NVM_KEY, sizeof(user_buf), user_buf);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to write NVM\n",
                 (int)sc);

      // Change passkey
      sl_bt_sm_set_passkey(atoi((char *) user_buf));

      app_log("Change password successful: %s\r\nClose connection\r\n",
              user_buf);
      // Delete all the bonding handle
      sc = sl_bt_sm_delete_bondings();
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to delete specified bonding \
                    information or whitelist\r\n",
                 (int)sc);
    } else {
      sc = sl_bt_gatt_server_send_user_write_response(
        evt->data.evt_gatt_server_user_write_request.connection,
        evt->data.evt_gatt_server_user_write_request.characteristic,
        (uint8_t)SL_STATUS_BT_ATT_INVALID_ATT_LENGTH);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to send a write response\n",
                 (int)sc);
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
  uint8_t passkey_nvm[PASSWORD_LEN];
  uint32_t passkey_nvm_to_int;
  size_t passkey_len_nvm;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

      // Print stack version
      app_log("Bluetooth stack booted: v%d.%d.%d-b%d\r\n",
              evt->data.evt_system_boot.major,
              evt->data.evt_system_boot.minor,
              evt->data.evt_system_boot.patch,
              evt->data.evt_system_boot.build);

      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to get Bluetooth address\r\n",
                 (int)sc);

      // Print Bluetooth address
      app_log("Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
              address_type ? "static random" : "public device",
              address.addr[5],
              address.addr[4],
              address.addr[3],
              address.addr[2],
              address.addr[1],
              address.addr[0]);

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
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to write attribute value\r\n",
                 (int)sc);

      // Bonding configuration
      sc = sl_bt_sm_configure(0x0B, sl_bt_sm_io_capability_displayonly);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to configure security\r\n",
                 (int)sc);

      sc = sl_bt_sm_store_bonding_configuration(1, 2);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to set the maximum allowed bonding count \
                 and bonding policy\r\n",
                 (int)sc);

      // Read passkey from NVM
      sl_bt_nvm_load(PASSWORD_NVM_KEY,
                     sizeof(passkey_nvm),
                     &passkey_len_nvm,
                     passkey_nvm);

      // Convert string to int
      passkey_nvm_to_int = atoi((char *) passkey_nvm);

      if (passkey_nvm_to_int != 0) {
        passkey = passkey_nvm_to_int;
      }
      app_log("passkey = %ld\r\n", passkey);

      sc = sl_bt_sm_set_passkey(passkey);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to set passkey\r\n",
                 (int)sc);

      sc = sl_bt_sm_set_bondable_mode(1);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to set bondalbe mode\r\n",
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
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to write attribute\r\n",
                 (int)sc);

      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to create advertising set\r\n",
                 (int)sc);

      sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                            sl_bt_advertiser_general_discoverable);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);

      // Start general advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(
        advertising_set_handle,
        sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);

      // Initialize the door lock application
      door_lock_init();
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      connection_opened_handler(evt);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      connection_closed_handler(evt);
      break;

    // -------------------------------
    // The parameters event
    case sl_bt_evt_connection_parameters_id:
      connection_parameters_handler(evt);
      break;

    // -------------------------------
    // The confirm_bonding event
    case sl_bt_evt_sm_confirm_bonding_id:
      sm_confirm_bonding_handler(evt);
      break;

    // -------------------------------
    // This event triggered after the pairing or bonding procedure is
    // successfully completed.
    case sl_bt_evt_sm_bonded_id:
      app_log("Bluetooth Stack Event : BONDED\r\n");
      break;

    // -------------------------------
    // This event is triggered if the pairing or bonding procedure fails.
    case sl_bt_evt_sm_bonding_failed_id:
      sm_bonding_failed_handler(evt);
      break;

    // -------------------------------
    // This event is triggered if client is attempting to write a value of an
    // attribute into the local GATT database.
    case sl_bt_evt_gatt_server_user_write_request_id:
      gatt_server_user_write_request_handler(evt);
      break;

    // -------------------------------
    // This event is triggered if the value of an attribute in the local GATT
    // database was changed by a remote GATT client
    case sl_bt_evt_gatt_server_attribute_value_id:
      break;

    // -------------------------------
    // External signal indication (comes from the interrupt handler)
    case sl_bt_evt_system_external_signal_id:

      if ((evt->data.evt_system_external_signal.extsignals & CAP1166_IRQ_EVENT) \
          == CAP1166_IRQ_EVENT) {
        capacitive_button_pressed_handler();
      }

      if ((evt->data.evt_system_external_signal.extsignals & SLEEP_TIMER_EVENT) \
          == SLEEP_TIMER_EVENT) {
        app_log("Timeout auto lock the door\r\n");
        mikroe_cmt_8540s_smt_play_sound(BUZZ2_NOTE_G7, volume, S);

        // Turn off the LED
        sl_led_turn_off(&sl_led_led0);

        // Clear the message box
        glib_fill_rect(&glib_context, 0, 12, 64, 8, GLIB_BLACK);
        glib_update_display();

        // Clear the lock status box
        glib_fill_rect(&glib_context, 0, 34, 64, 8, GLIB_BLACK);
        glib_update_display();

        // Update LOCK status field in OLED
        oled_update_lock_status(" LOCK ", 6, 10);
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

/**************************************************************************//**
 * Handler for the capacitive touch sensor interrupt event
 *****************************************************************************/
static void cap1166_alert_gpio_callback(uint8_t intNo, void *ctx)
{
  (void)intNo;
  (void)ctx;

  if (!GPIO_PinInGet(CAP1166_ALERT_PORT, CAP1166_ALERT_PIN)) {
    if (app_init_driver_done) {
      // Send the external event to the Bluetooth stack
      sl_bt_external_signal(CAP1166_IRQ_EVENT);
    }
  }
}

/**************************************************************************//**
 * Callback when the sleep timer expire
 *****************************************************************************/
void sleep_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void) data;
  if (handle == &auto_lock_the_door) {
    // Send the external event to the Bluetooth stack
    sl_bt_external_signal(SLEEP_TIMER_EVENT);
  }
}
