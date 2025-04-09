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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/
#include "sl_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "app_log.h"
#include "sl_sleeptimer.h"
#include "bthome_v2_server.h"
#include "bthome_v2_server_nvm3.h"
#include "bthome_v2_server_config.h"
#include "adafruit_hxd8357d.h"
#include "lv_port_disp.h"
#include "nvm3_generic.h"
#include "lv_port_indev.h"
#if defined(SL_CATALOG_ADAFRUIT_TFT_LCD_HXD8357D_DMA_PRESENT)
#include "adafruit_hxd8357d_spi_config.h"
#endif
#include "app_ui/ui.h"

#define LVGL_TIMER_PERIOD              1
#define LVGL_TIMER_RUN_TIMER_HANDLER   5
#define APP_TIMER_UPDATE_SCREEN        500
#define APP_TIMER_TIMEOUT              1000

#define APP_READ_DATA_EXT_SIG          0x01
#define APP_UPDATE_SCREEN_EXT_SIG      0x02

#define APP_MAX_CLIENT_DEVICES         6
#define APP_MAC_SIZE                   6
#define APP_DEVICE_KEY_SIZE            16
#define APP_NVM3_DEVICE_NAME_OFF_SET   (MAX_DEVICE + 2)
#define APP_UPDATE_DATA_TIMEOUT_SECOND 10

MIPI_DBI_SPI_INTERFACE_DEFINE(hx8357d_config,
                              ADAFRUIT_HXD8357D_PERIPHERAL,
                              ADAFRUIT_HXD8357D_PERIPHERAL_NO,
                              ADAFRUIT_HXD8357D_BITRATE,
                              ADAFRUIT_HXD8357D_CLOCK_MODE,
                              ADAFRUIT_HXD8357D_CS_CONTROL,
                              ADAFRUIT_HXD8357D_CLK_PORT,
                              ADAFRUIT_HXD8357D_CLK_PIN,
                              ADAFRUIT_HXD8357D_TX_PORT,
                              ADAFRUIT_HXD8357D_TX_PIN,
                              ADAFRUIT_HXD8357D_RX_PORT,
                              ADAFRUIT_HXD8357D_RX_PIN,
                              ADAFRUIT_HXD8357D_CS_PORT,
                              ADAFRUIT_HXD8357D_CS_PIN,
                              ADAFRUIT_HXD8357D_DC_PORT,
                              ADAFRUIT_HXD8357D_DC_PIN);

typedef struct scanned_device {
  uint8_t device_mac[6];
  uint8_t device_mac_str[18];
  bool is_added;
} scanned_device_t;

typedef struct device {
  scanned_device_t device;
  uint8_t device_name[15];
  uint8_t device_key[16];
  uint32_t update_delay_ms;
} device_t;

static uint8_t scanned_device_count = 0;
static uint8_t added_device_count = 0;
static device_t added_device[6];
static scanned_device_t scanned_device[6];

static lv_obj_t **ui_devicename_label_scr1[6] =
{ &ui_devicenamelb0, &ui_devicenamelb1,
  &ui_devicenamelb2, &ui_devicenamelb3,
  &ui_devicenamelb4, &ui_devicenamelb5 };

static lv_obj_t **ui_mac_label_scr1[6] = { &ui_maclb0, &ui_maclb1, &ui_maclb2,
                                           &ui_maclb3, &ui_maclb4, &ui_maclb5 };

static lv_obj_t **ui_device_infor_scr1[6] =
{ &ui_deviceinforlb0, &ui_deviceinforlb1,
  &ui_deviceinforlb2, &ui_deviceinforlb3,
  &ui_deviceinforlb4, &ui_deviceinforlb5 };

static lv_obj_t **ui_device_name_label_scr3[6] = { &ui_macdevlb0, &ui_macdevlb1,
                                                   &ui_macdevlb2, &ui_macdevlb3,
                                                   &ui_macdevlb4,
                                                   &ui_macdevlb5 };

static lv_obj_t **ui_device_icon_scr1[6] =
{ &ui_deviceiconpn0, &ui_deviceiconpn1,
  &ui_deviceiconpn2, &ui_deviceiconpn3,
  &ui_deviceiconpn4, &ui_deviceiconpn5 };

static lv_obj_t **ui_light_off_icon_scr1[6] =
{ &ui_lightofficon0, &ui_lightofficon1,
  &ui_lightofficon2, &ui_lightofficon3,
  &ui_lightofficon4, &ui_lightofficon5 };

static lv_obj_t **ui_light_on_icon_scr1[6] =
{ &ui_lightonicon0, &ui_lightonicon1,
  &ui_lightonicon2, &ui_lightonicon3,
  &ui_lightonicon4, &ui_lightonicon5 };

static lv_obj_t **ui_device_panel_scr1[6] = { &ui_inforpanel0, &ui_inforpanel1,
                                              &ui_inforpanel2, &ui_inforpanel3,
                                              &ui_inforpanel4,
                                              &ui_inforpanel5 };

static lv_obj_t **ui_scanned_dev_list_scr3[6] = { &ui_devscannedpn0,
                                                  &ui_devscannedpn1,
                                                  &ui_devscannedpn2,
                                                  &ui_devscannedpn3,
                                                  &ui_devscannedpn4,
                                                  &ui_devscannedpn5 };

static lv_obj_t **ui_temperature_icon_scr1[6] = { &ui_tempicon0, &ui_tempicon1,
                                                  &ui_tempicon2, &ui_tempicon3,
                                                  &ui_tempicon4,
                                                  &ui_tempicon5 };

static lv_obj_t **ui_but_on_icon_scr1[6] = { &ui_butonicon0, &ui_butonicon1,
                                             &ui_butonicon2, &ui_butonicon3,
                                             &ui_butonicon4, &ui_butonicon5 };

static lv_obj_t **ui_but_off_icon_scr1[6] = { &ui_butofficon0,
                                              &ui_butofficon1, &ui_butofficon2,
                                              &ui_butofficon3, &ui_butofficon4,
                                              &ui_butofficon5 };

static lv_obj_t **ui_update_history_scr1[6] =
{ &ui_historylb0, &ui_historylb1, &ui_historylb2, &ui_historylb3,
  &ui_historylb4, &ui_historylb5 };

static sl_sleeptimer_timer_handle_t lvgl_tick_timer;
static sl_sleeptimer_timer_handle_t app_timer;
static sl_sleeptimer_timer_handle_t update_screen_timer;

static void app_get_client_information_handler(void);
static void app_convert_str_to_hex(uint8_t *in, uint8_t *str);
static void app_update_screen_handler(void);
static void app_registe_new_device(void);
static void app_remove_added_device(void);
static void app_hidden_icon_on_panel_index(uint8_t panel_idx);

static void lvgl_tick_timer_callback(sl_sleeptimer_timer_handle_t *timer,
                                     void *data);
static void app_timer_cb(sl_sleeptimer_timer_handle_t *handle,
                         void *data);
static void update_screen_timer_cb(sl_sleeptimer_timer_handle_t *handle,
                                   void *data);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_status_t sc;

  app_log("\nBTHome v2 - Server/Gateway Device.\n");

#if defined(SL_CATALOG_ADAFRUIT_TFT_LCD_HXD8357D_PRESENT)
  adafruit_hxd8357d_init();
#elif defined(SL_CATALOG_ADAFRUIT_TFT_LCD_HXD8357D_DMA_PRESENT)
  adafruit_hxd8357d_init(&hx8357d_config);
#endif
  lv_init();
  lv_port_disp_init();
  lv_port_indev_init();
  ui_init();
  sc = sl_sleeptimer_start_periodic_timer_ms(&lvgl_tick_timer,
                                             LVGL_TIMER_PERIOD,
                                             lvgl_tick_timer_callback,
                                             NULL,
                                             0,
                                             0);

  app_assert_status(sc);

  sc = sl_sleeptimer_start_periodic_timer_ms(&update_screen_timer,
                                             APP_TIMER_UPDATE_SCREEN,
                                             update_screen_timer_cb,
                                             NULL,
                                             0,
                                             0);
  app_assert_status(sc);
}

/**************************************************************************//**
 * Device found handle.
 *****************************************************************************/
void bthome_v2_server_found_device_callback(uint8_t *mac,
                                            uint8_t *payload,
                                            uint8_t payload_length)
{
  (void)payload;
  (void)payload_length;
  sl_status_t sc;
  char temp_mac_str[18];
  bool encrypted;
  bool key_available;
  bool is_running;
  int nvm3_device_index;

  if (added_device_count == APP_MAX_CLIENT_DEVICES) {
    app_log(
      "\n[WRN]: Found a new device, but the example only supports 6 client devices!\n"
      "Please remove the added client device to add the new device.\n");
  }

  app_log("\r\n");
  app_log("Found a new client device.\n");

  for (uint8_t i = 0; i < APP_MAC_SIZE; i++) {
    if (i == 5) {
      snprintf(temp_mac_str + 3 * i, 3, "%.2X",
               (uint8_t)mac[i]);
      break;
    }
    snprintf(temp_mac_str + 3 * i,
             4,
             "%.2X:",
             (uint8_t)mac[i]);
  }
  app_log("MAC: %s\n", temp_mac_str);
  bthome_v2_server_check_device(mac, &encrypted, &key_available);
  app_log("Encryption: %s",
          (encrypted) ? "Yes\r\n" : "No\r\n");
  app_log("Encryption Key Available: %s",
          (key_available) ? "Yes\r\n" : "No\r\n");

  if (key_available && (added_device_count < APP_MAX_CLIENT_DEVICES)) {
    memcpy(&added_device[added_device_count].device.device_mac,
           mac,
           APP_MAC_SIZE);
    memcpy(&added_device[added_device_count].device.device_mac_str,
           temp_mac_str,
           sizeof(temp_mac_str));
    added_device[added_device_count].device.is_added = true;
    nvm3_device_index =
      bthome_v2_server_nvm3_find_index(
        added_device[added_device_count].device.device_mac);
    if (nvm3_device_index == -1) {
      app_log("[WRN]: Cannot found the device name from NVM3.\n");
    } else {
      bthome_v2_server_nvm3_read(
        nvm3_device_index + APP_NVM3_DEVICE_NAME_OFF_SET,
        added_device[added_device_count].device_name);
      app_log("Device name: %s\n",
              added_device[added_device_count].device_name);
    }
    added_device_count++;
  } else if (scanned_device_count < APP_MAX_CLIENT_DEVICES) {
    memcpy(scanned_device[scanned_device_count].device_mac, mac,
           sizeof(scanned_device[scanned_device_count].device_mac));
    scanned_device[scanned_device_count].is_added = false;

    memcpy(scanned_device[scanned_device_count].device_mac_str, temp_mac_str,
           sizeof(temp_mac_str));
    scanned_device_count++;
  } else {
    app_log("[WRN]: Full of the scanned devices!\n");
  }

  sc = sl_sleeptimer_is_timer_running(&app_timer, &is_running);
  app_assert_status(sc);
  if (!is_running) {
    sc = sl_sleeptimer_start_periodic_timer_ms(&app_timer,
                                               APP_TIMER_TIMEOUT,
                                               app_timer_cb,
                                               NULL,
                                               0,
                                               0);
    app_assert_status(sc);
  }
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
  lv_timer_handler_run_in_period(5);
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
      app_log("Start scanning ....\n");

      sc = bthome_v2_server_start_scan_network();
      app_assert_status(sc);
      break;
    case sl_bt_evt_system_external_signal_id:

      if (evt->data.evt_system_external_signal.extsignals
          == APP_UPDATE_SCREEN_EXT_SIG) {
        app_update_screen_handler();
      } else if (evt->data.evt_system_external_signal.extsignals
                 == APP_READ_DATA_EXT_SIG) {
        app_get_client_information_handler();
      }
      break;
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

/**************************************************************************//**
 * Get client information handler.
 *****************************************************************************/
static void app_get_client_information_handler(void)
{
  sl_status_t sc;
  float tempertature;
  bthome_v2_server_sensor_data_t object[2];
  uint8_t object_count;
  char temp_str[9];
  uint32_t update_time;

  if (remove_completed) {
    for (int i = 0; i < added_device_count; i++) {
      object_count = 0;
      sc = bthome_v2_server_sensor_data_read(added_device[i].device.device_mac,
                                             object, 2,
                                             &object_count,
                                             &update_time);
      app_assert_status(sc);
      lv_obj_clear_flag(*(ui_device_icon_scr1[i]), LV_OBJ_FLAG_HIDDEN);
      added_device[i].update_delay_ms = sl_sleeptimer_tick_to_ms(
        sl_sleeptimer_get_tick_count() - update_time) / 1000;
      for (uint8_t j = 0; j < object_count; j++) {
        switch (object[j].object_id) {
          case ID_TEMPERATURE:
            tempertature = (float) object[j].data / object[j].factor;
            snprintf(temp_str, sizeof(temp_str), "%.1f °C", tempertature);
            lv_label_set_text(*(ui_device_infor_scr1[i]), temp_str);
            if (lv_obj_has_flag(*(ui_temperature_icon_scr1[i]),
                                LV_OBJ_FLAG_HIDDEN)) {
              lv_obj_clear_flag(*(ui_temperature_icon_scr1[i]),
                                LV_OBJ_FLAG_HIDDEN);
            }
            break;
          case STATE_GENERIC_BOOLEAN:
            if (object[j].data) {
              lv_label_set_text(*(ui_device_infor_scr1[i]), "ON");
              if (lv_obj_has_flag(*(ui_light_on_icon_scr1[i]),
                                  LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_add_flag(*(ui_light_off_icon_scr1[i]),
                                LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(*(ui_light_on_icon_scr1[i]),
                                  LV_OBJ_FLAG_HIDDEN);
              }
            } else {
              lv_label_set_text(*(ui_device_infor_scr1[i]), "OFF");
              if (lv_obj_has_flag(*(ui_light_off_icon_scr1[i]),
                                  LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_add_flag(*(ui_light_on_icon_scr1[i]),
                                LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(*(ui_light_off_icon_scr1[i]),
                                  LV_OBJ_FLAG_HIDDEN);
              }
            }
            break;
          case EVENT_BUTTON:
            if ((object[j].data == EVENT_BUTTON_DOUBLE_PRESS)
                || (object[j].data == EVENT_BUTTON_PRESS)
                || (object[j].data == EVENT_BUTTON_LONG_PRESS)
                || (object[j].data == EVENT_BUTTON_TRIPLE_PRESS)) {
              lv_label_set_text(*(ui_device_infor_scr1[i]), "PRESS");
              if (lv_obj_has_flag(*(ui_but_on_icon_scr1[i]),
                                  LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_clear_flag(*(ui_but_on_icon_scr1[i]),
                                  LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(*(ui_but_off_icon_scr1[i]), LV_OBJ_FLAG_HIDDEN);
              }
            } else if (object[j].data == EVENT_BUTTON_NONE) {
              lv_label_set_text(*(ui_device_infor_scr1[i]), "NONE");
              if (lv_obj_has_flag(*(ui_but_off_icon_scr1[i]),
                                  LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_clear_flag(*(ui_but_off_icon_scr1[i]),
                                  LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(*(ui_but_on_icon_scr1[i]), LV_OBJ_FLAG_HIDDEN);
              }
            } else {
            }
            break;
          default:
            break;
        }
      }
    }
  }
}

/**************************************************************************//**
 * Update screen function.
 *****************************************************************************/
static void app_update_screen_handler(void)
{
  char history_str[18];
  lv_obj_t *current_screen;

  current_screen = lv_scr_act();

  if (ui_Screen4 == current_screen) {
    app_registe_new_device();
  } else if (ui_Screen1 == current_screen) {
    for (int i = 0; i < APP_MAX_CLIENT_DEVICES; i++) {
      if (added_device[i].device.is_added) {
        if (added_device[i].update_delay_ms > APP_UPDATE_DATA_TIMEOUT_SECOND) {
          if (added_device[i].update_delay_ms > 86400) {
            snprintf(history_str,
                     sizeof(history_str),
                     "%ld days ago",
                     added_device[i].update_delay_ms / 86400);
          } else if (added_device[i].update_delay_ms > 3600) {
            snprintf(history_str,
                     sizeof(history_str),
                     "%ld hours ago",
                     added_device[i].update_delay_ms / 3600);
          } else if (added_device[i].update_delay_ms > 60) {
            snprintf(history_str,
                     sizeof(history_str),
                     "%ld minutes ago",
                     added_device[i].update_delay_ms / 60);
          } else {
            snprintf(history_str,
                     sizeof(history_str),
                     "%ld seconds ago",
                     added_device[i].update_delay_ms);
          }
          lv_label_set_text(*(ui_update_history_scr1[i]), history_str);
        } else {
          lv_label_set_text(*(ui_update_history_scr1[i]), " ");
        }
        lv_obj_clear_flag(*(ui_device_panel_scr1[i]), LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(*(ui_devicename_label_scr1[i]),
                          (char *)added_device[i].device_name);
        lv_label_set_text(*(ui_mac_label_scr1[i]),
                          (char *)added_device[i].device.device_mac_str);
      } else {
        lv_obj_add_flag(*(ui_device_panel_scr1[i]), LV_OBJ_FLAG_HIDDEN);
      }
    }
    if (!remove_completed && (remove_slot < added_device_count)
        && added_device[remove_slot].device.is_added) {
      app_remove_added_device();
    }
  } else if (ui_Screen3 == current_screen) {
    for (int i = 0; i < scanned_device_count; i++) {
      lv_obj_clear_flag(*(ui_scanned_dev_list_scr3[i]), LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(*(ui_device_name_label_scr3[i]),
                        (char *)scanned_device[i].device_mac_str);
    }
  } else {
    /*Nothing*/
  }
}

/**************************************************************************//**
 * Register new device function.
 *****************************************************************************/
static void app_registe_new_device(void)
{
  sl_status_t sc;
  const char *data;
  int nvm3_device_index;

  if (!register_completed && (added_device_count < APP_MAX_CLIENT_DEVICES)) {
    for (int i = 0; i < scanned_device_count; i++) {
      if (!strcmp((char *)scanned_device[i].device_mac_str,
                  lv_label_get_text(ui_deviceconfigmac))) {
        scanned_device[i].is_added = true;
        memcpy(&added_device[added_device_count].device,
               &scanned_device[i],
               sizeof(scanned_device_t));
        data = lv_textarea_get_text(ui_setdevnametxtarea);
        memcpy(added_device[added_device_count].device_name, data,
               sizeof(added_device[added_device_count].device_name));
        data = lv_textarea_get_text(ui_setenckeytxtarea);
        app_convert_str_to_hex(added_device[added_device_count].device_key,
                               (uint8_t *)data);

        sc = bthome_v2_server_key_register(
          added_device[added_device_count].device.device_mac,
          added_device[added_device_count].device_key);
        app_assert_status(sc);
        nvm3_device_index =
          bthome_v2_server_nvm3_find_index(
            added_device[added_device_count].device.device_mac);

        sc = bthome_v2_server_nvm3_write(
          nvm3_device_index + APP_NVM3_DEVICE_NAME_OFF_SET,
          added_device[added_device_count].device_name);
        app_assert_status(sc);
        app_log("Device is added: [NAME]: %s       [MAC]: %s\n",
                added_device[added_device_count].device_name,
                added_device[added_device_count].device.device_mac_str);
        added_device_count++;
        for (int j = i; j < scanned_device_count - 1; j++) {
          memcpy(&scanned_device[j], &scanned_device[j + 1],
                 sizeof(scanned_device_t));
        }
        scanned_device_count--;
        memset(&scanned_device[scanned_device_count],
               0,
               sizeof(scanned_device_t));
        register_completed = true;
        lv_obj_add_flag(*(ui_scanned_dev_list_scr3[scanned_device_count]),
                        LV_OBJ_FLAG_HIDDEN);
        lv_disp_load_scr(ui_Screen3);
      }
    }
  }
}

/**************************************************************************//**
 * Remove added device function.
 *****************************************************************************/
static void app_remove_added_device(void)
{
  sl_status_t sc;

  app_log("Device is removed: [NAME]: %s       [MAC]: %s\n",
          added_device[remove_slot].device_name,
          added_device[remove_slot].device.device_mac_str);
  for (int j = remove_slot; j < added_device_count; j++) {
    app_hidden_icon_on_panel_index(j);
  }
  lv_obj_add_flag(*(ui_device_icon_scr1[remove_slot]), LV_OBJ_FLAG_HIDDEN);
  if (scanned_device_count < APP_MAX_CLIENT_DEVICES) {
    memcpy(&scanned_device[scanned_device_count],
           &added_device[remove_slot].device,
           sizeof(scanned_device_t));
    scanned_device[scanned_device_count].is_added = false;
    scanned_device_count++;
  }
  sc = bthome_v2_server_key_remove(
    added_device[remove_slot].device.device_mac);
  added_device_count--;
  for (int k = remove_slot; k < added_device_count; k++) {
    memcpy(&added_device[k], &added_device[k + 1], sizeof(device_t));
  }

  app_assert_status(sc);

  memset(&added_device[added_device_count], 0, sizeof(device_t));

  lv_label_set_text(*(ui_device_infor_scr1[added_device_count]), "");
  lv_label_set_text(*(ui_devicename_label_scr1[added_device_count]), "");
  lv_label_set_text(*(ui_mac_label_scr1[added_device_count]), "");
  lv_label_set_text(*(ui_update_history_scr1[added_device_count]), "");
  lv_obj_add_flag(*(ui_device_icon_scr1[added_device_count]),
                  LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(*(ui_device_panel_scr1[added_device_count]),
                  LV_OBJ_FLAG_HIDDEN);
  remove_completed = true;
}

/**************************************************************************//**
 * Convert string to hex funtion.
 *****************************************************************************/
static void app_convert_str_to_hex(uint8_t *in, uint8_t *str)
{
  char octet[2];

  for (int i = 0; i < APP_DEVICE_KEY_SIZE; i++) {
    memcpy(octet, (uint8_t *)&str[2 * i], 2);
    in[i] = (uint8_t)strtol(octet, NULL, APP_DEVICE_KEY_SIZE);
  }
}

/**************************************************************************//**
 * Hidden all information icons in screen 1.
 *****************************************************************************/
static void app_hidden_icon_on_panel_index(uint8_t panel_idx)
{
  lv_obj_add_flag(*(ui_temperature_icon_scr1[panel_idx]), LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(*(ui_light_on_icon_scr1[panel_idx]), LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(*(ui_light_off_icon_scr1[panel_idx]), LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(*(ui_but_off_icon_scr1[panel_idx]), LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(*(ui_but_on_icon_scr1[panel_idx]), LV_OBJ_FLAG_HIDDEN);
}

/**************************************************************************//**
 * Update screen timer callback.
 *****************************************************************************/
static void update_screen_timer_cb(sl_sleeptimer_timer_handle_t *handle,
                                   void *data)
{
  (void)handle;
  (void)data;

  sl_bt_external_signal(APP_UPDATE_SCREEN_EXT_SIG);
}

/**************************************************************************//**
 * LGVL tick timer callback.
 *****************************************************************************/
static void lvgl_tick_timer_callback(sl_sleeptimer_timer_handle_t *timer,
                                     void *data)
{
  (void)timer;
  (void)data;

  lv_tick_inc(LVGL_TIMER_PERIOD);
}

/**************************************************************************//**
 * App timer callback function.
 *****************************************************************************/
static void app_timer_cb(sl_sleeptimer_timer_handle_t *handle,
                         void *data)
{
  (void)handle;
  (void)data;

  sl_bt_external_signal(APP_READ_DATA_EXT_SIG);
}
