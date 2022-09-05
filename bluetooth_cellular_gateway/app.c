/***************************************************************************//**
 * @file app.c
 * @brief source for BLE - BG96 cellular gateway sample application
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
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/

/*******************************************************************************
 *******************************   INCLUDES ************************************
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"

#include "bg96_driver.h"
#include "nb_iot.h"
#include "nb_gnss.h"
#include "at_parser_core.h"
#include "at_parser_core.h"
#include "at_parser_events.h"
#include <stdio.h>

/**************************************************************************//**
 * enums, handles, and variables                                              *
 *****************************************************************************/
typedef enum {
  FIND_SERVICE,
  FIND_FIRST_CHARACTERISTIC,
  FIND_SECOND_CHARACTERISTIC,
  ENABLE_FIRST_NOTIFICATION,
  ENABLE_SECOND_NOTIFICATION,
  DATA_RECIEVED
}ble_state;

// BLE handlers
ble_state _main_state = FIND_SERVICE;
static uint8_t _conn_handle = 0xFF;
static uint32_t _service_handle = 0;
static uint16_t _thermometer_char_handle = 0;
static uint16_t _humidity_char_handle = 0;

// maximum number of BLE devices
#define MAX_CONNECTIONS    3
#define ONE_SECOND_IN_HZ   32768

// Environmental sensing service UUID: 181a
const uint8_t serviceUUID[2] = { 0x1a, 0x18 };

// temperature data UUID: 2a6e
const uint8_t temperature_charUUID[2] = { 0x6e, 0x2a };

// humidity data uuid: 2a6f
const uint8_t humidity_charUUID[2] = { 0x6f, 0x2a };

char device_name[] = "Thunderboard";

// bg96  tokens and strings
static uint8_t cloud_token[] = "your_token_here"; // TODO change to your own cloud ID token
static uint8_t latitude_data[15] = "<n.a.>";
static uint8_t longitude_data[15] = "<n.a.>";
static uint8_t data_to_send[150] = "";

// 50% RHT  is interpreted 5000
static  uint16_t humidity_readout = 5000;

// 25.0 °C is interpreted 2500
static  int16_t temperature_readout = 2500;

// timer handles, variables and flags
sl_sleeptimer_timer_handle_t BLE_scanning_timeout_timer;
sl_sleeptimer_timer_handle_t bg96_state_timeout_timer;
sl_sleeptimer_timer_handle_t general_timer;

static uint8_t BLE_scanning_timeout_sec = 5;
static uint16_t general_timeout_sec = 120;

// variables to find highest rssi device at BLE scanning
static int8_t highest_rssi = -127;
static bd_addr saved_addr;
static uint8_t saved_address_type;

// status flags
static bool temperature_received = false;
static bool humidity_received = false;
static bool GNSS_position_received = false;
static bool BLE_scan_timeout_flag = false;
static bool general_timer_running = false;
static sl_status_t bg96_status = SL_STATUS_FAIL;

typedef enum{
  wake_up,
  network_registration,
  GPS_get_position,
  net_open,
  net_send,
  net_close,
  net_delay_open,
  net_delay_netreg,
  net_delay_gsm_start,
  net_delay,
  all_finished
}bg96_state;

bg96_state _bg96_state = wake_up;

at_scheduler_status_t output_object = { SL_STATUS_OK, 0, "" };

/**************************************************************************//**
 * Local functions and callbacks
 *****************************************************************************/
static bool process_scan_response(sl_bt_evt_scanner_scan_report_t *pResp);
static void bg96_process_action(void);
static void start_bg_96_timer(at_scheduler_status_t *output_object, uint32_t ms);
static void start_BLE_scanning_timer(void);
static void start_general_timer(void);
static void start_BLE_scan_and_scan_timer(void);
static void BLE_scanning_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                 void *data);
static void bg96_state_machine_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                       void *data);
static void general_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                            void *data);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  printf("BLE to cellular started \r\n");

  bg96_nb_init();
  printf("bg96 init \r\n");

  start_general_timer();
  printf("general timer started \r\n");

  at_parser_init_output_object(&output_object);
  bg96_wake_up(&output_object);
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
  if( general_timer_running == false){
      start_general_timer();
      printf("general timer started \r\n");
  }

  // run BG96 driver
  bg96_process_action();
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t BLE_status;
  switch (SL_BT_MSG_ID(evt->header)) {
// -------------------------------
// This event indicates the device has started and the radio is ready.
// Do not call any stack command before receiving this boot event!
  case sl_bt_evt_system_boot_id:
    printf("BLE: client started \r\n");
    start_BLE_scan_and_scan_timer();
    temperature_received = false;
    humidity_received = false;
    break;

  case sl_bt_evt_scanner_scan_report_id:
    if (process_scan_response(&evt->data.evt_scanner_scan_report)) {
      if (highest_rssi <= evt->data.evt_scanner_scan_report.rssi) {

        highest_rssi = evt->data.evt_scanner_scan_report.rssi;
        saved_addr = evt->data.evt_scanner_scan_report.address;
        saved_address_type = evt->data.evt_scanner_scan_report.address_type;
      }

      if (BLE_scan_timeout_flag == true) {
        BLE_status = sl_bt_connection_open(saved_addr, saved_address_type,
            sl_bt_gap_1m_phy, &_conn_handle);
        if (SL_STATUS_OK == BLE_status) {
          sl_bt_scanner_stop();
          printf("BLE: device connected. \r\n");
        }
        highest_rssi = -127;
        BLE_scan_timeout_flag = false;
      }
    }
    break;

    // -------------------------------
    // This event indicates that a new connection was opened.
  case sl_bt_evt_connection_opened_id:

    sl_bt_gatt_discover_primary_services_by_uuid(
        evt->data.evt_connection_opened.connection, sizeof(serviceUUID),
        serviceUUID);
    printf("BLE: connection opened. \r\n");
    break;

  case sl_bt_evt_gatt_service_id:
    if (evt->data.evt_gatt_service.uuid.len == sizeof(serviceUUID)) {
      if (memcmp(serviceUUID, evt->data.evt_gatt_service.uuid.data,
          sizeof(serviceUUID) / serviceUUID[0]) == 0) {
        printf("BLE: environmental sensing characteristic discovered.\r\n");
        _service_handle = evt->data.evt_gatt_service.service;
        _main_state = FIND_SERVICE;
      }
    }
    break;

  case sl_bt_evt_gatt_procedure_completed_id:
    switch (_main_state) {
    case FIND_SERVICE:
      if (_service_handle > 0) {
        // Service found, next step: search for characteristics
        sl_bt_gatt_discover_characteristics(_conn_handle, _service_handle);
        _main_state = FIND_FIRST_CHARACTERISTIC;
        printf("BLE: environmental sensing characteristic found.\r\n");
      } else {
        //Service is not found: disconnect
        printf("BLE: Environmental service not found.\r\n");
        sl_bt_connection_close(_conn_handle);
      }
      break;

    case FIND_FIRST_CHARACTERISTIC:
      if (_humidity_char_handle > 0) {
        // Characteristic found, turn on indications
        sl_bt_gatt_set_characteristic_notification(_conn_handle,
            _humidity_char_handle, gatt_notification);
        _main_state = ENABLE_FIRST_NOTIFICATION;
        printf("BLE: Humidity Characteristic found. \r\n");
        _main_state = FIND_SECOND_CHARACTERISTIC;
      }

      else {
        //Characteristic is not found: disconnect
        printf("BLE: Humidity Characteristic not found. \r\n");
        sl_bt_connection_close(_conn_handle);
      }
      break;

    case FIND_SECOND_CHARACTERISTIC:
      if (_thermometer_char_handle > 0) {
        // Characteristic found, turn on indications
        sl_bt_gatt_set_characteristic_notification(_conn_handle,
            _thermometer_char_handle, gatt_notification);
        printf("BLE: Thermometer Characteristic found. \r\n");
        _main_state = ENABLE_FIRST_NOTIFICATION;
      } else {
        //Characteristic is not found: disconnect
        printf("BLE: Thermometer Characteristic not found. \r\n");
        sl_bt_connection_close(_conn_handle);
      }
      break;

    case ENABLE_FIRST_NOTIFICATION:
      // Enable read request
      sl_bt_gatt_read_characteristic_value_by_uuid(_conn_handle,
          _service_handle, sizeof(temperature_charUUID), temperature_charUUID);
      _main_state = ENABLE_SECOND_NOTIFICATION;
      break;

    case ENABLE_SECOND_NOTIFICATION:
      // Enable read request
      sl_bt_gatt_read_characteristic_value_by_uuid(_conn_handle,
          _service_handle, sizeof(humidity_charUUID), humidity_charUUID);
      _main_state = DATA_RECIEVED;
      break;

    case DATA_RECIEVED:
      break;

    default:
      break;
    }
    break;

    case sl_bt_evt_gatt_characteristic_id:
      //check for the characteristic identifier, and save it to handler variable
      if (evt->data.evt_gatt_characteristic.uuid.len
        == sizeof(temperature_charUUID)) {
        if (memcmp(temperature_charUUID,
            evt->data.evt_gatt_characteristic.uuid.data,
            sizeof(temperature_charUUID)) == 0) {
          _thermometer_char_handle =
              evt->data.evt_gatt_characteristic.characteristic;
        }
      }
      if (evt->data.evt_gatt_characteristic.uuid.len
          == sizeof(humidity_charUUID)) {
        if (memcmp(humidity_charUUID, evt->data.evt_gatt_characteristic.uuid.data,
            sizeof(humidity_charUUID)) == 0) {
          _humidity_char_handle =
              evt->data.evt_gatt_characteristic.characteristic;
        }
      }
    break;

    case sl_bt_evt_gatt_characteristic_value_id:
      // after request for read out, in this event is possible to read the actual measured data
      if (evt->data.evt_gatt_characteristic_value.characteristic
          == _thermometer_char_handle) {
        memcpy(&temperature_readout,
            &evt->data.evt_gatt_server_attribute_value.value.data[0],
            evt->data.evt_gatt_server_attribute_value.value.len);
        printf("BLE: Temperature: %d,%02d Celsius \r\n",
            temperature_readout / 100, temperature_readout % 100);
        temperature_received = true;
      }

      if (evt->data.evt_gatt_characteristic_value.characteristic
          == _humidity_char_handle) {
        memcpy(&humidity_readout,
            &evt->data.evt_gatt_server_attribute_value.value.data[0],
            evt->data.evt_gatt_server_attribute_value.value.len);
        printf("BLE: Humidity: %d,%02d %s RHT \r\n", humidity_readout / 100,
            humidity_readout % 100, "%");
        humidity_received = true;
      }
      if (temperature_received && humidity_received) {
        sl_bt_connection_close(_conn_handle);
      }
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:

      printf("BLE: connection closed \r\n");
      if (temperature_received == false || humidity_received == false) {
        printf("BLE: device not found, starting another scan \r\n");
        start_BLE_scan_and_scan_timer();
      }
      break;

    default:
    break;
  }
}

/**************************************************************************//**
@brief
 *   Starting the scan timer, resetting the flag timeout flag
 *****************************************************************************/
static void start_BLE_scanning_timer(void)
{
  sl_sleeptimer_start_timer(&BLE_scanning_timeout_timer,
      BLE_scanning_timeout_sec * ONE_SECOND_IN_HZ, BLE_scanning_timer_callback,
      NULL, 0, 0);
  BLE_scan_timeout_flag = false;
}

/**************************************************************************//**
@brief
 * Decoding advertising packets is done here. The list of AD types can be found
 * at: https://www.bluetooth.com/specifications/assigned-numbers/Generic-Access-Profile
 *****************************************************************************/
static bool process_scan_response(sl_bt_evt_scanner_scan_report_t *pResp)
{

  uint8_t i = 0, ad_len, ad_type;
  bool ad_match_found = false;

  char name[32];

  while (i < (pResp->data.len - 1)) {

    ad_len = pResp->data.data[i];
    ad_type = pResp->data.data[i + 1];

    if (ad_type == 0x08 || ad_type == 0x09) {
      // Type 0x08 = Shortened Local Name
      // Type 0x09 = Complete Local Name
      memcpy(name, &(pResp->data.data[i + 2]), ad_len - 1);
      name[ad_len - 1] = 0;

      if (memcmp(device_name, name,
          sizeof(device_name) / sizeof(device_name[0]) - 1) == 0) {
        ad_match_found = true;
      }
    }

    if (ad_type == 0x02 || ad_type == 0x03) {
      // Type 0x02 = Incomplete List of 16-bit Service Class UUIDs
      // Type 0x03 = Complete List of 16-bit Service Class UUIDs
      if (memcmp(serviceUUID, &(pResp->data.data[i + 2]), 2) == 0) {
        ad_match_found = true;
      }
    }
    // Jump to next AD record
    i = i + ad_len + 1;
  }

  return ad_match_found;
}

/**************************************************************************//**
 * @brief
 *   This is the main logic of bg96 process, it is using the BG_96_driver.
 *   The state machine is handling the cellular control, like network
 *   registration, opening network, send data, and close network functions.
 *   It is also receiving GPS data, parsing GPS, humidity and temperature
 *   data to send them via cellular network to cloud provider.
 *
 *   this function need to be call infinitely *
 * ****************************************************************************/
static void bg96_process_action(void)
{
  bg96_nb_connection_t connection = {
                                      0,
                                      9999,
                                      "TCP",
                                      (uint8_t*) "cloudsocket.hologram.io" };
  bg96_process();
  at_parser_process();

  if (output_object.status == SL_STATUS_OK) {
    switch (_bg96_state) {
    case wake_up:
      printf("bg96 wake_up finished \r\n");
      _bg96_state = net_delay_netreg;
      at_parser_init_output_object(&output_object);
      start_bg_96_timer(&output_object, 500);
      break;

    case net_delay_netreg:
      printf("bg96 network registration started \r\n");
      _bg96_state = network_registration;
      at_parser_init_output_object(&output_object);
      bg96_status = bg96_network_registration(&output_object);
      break;

    case network_registration:
      printf("bg96 network registration finished \r\n");
      _bg96_state = net_delay_gsm_start;
      at_parser_init_output_object(&output_object);
      start_bg_96_timer(&output_object, 500);
      break;

    case net_delay_gsm_start:
      printf("bg96 GPS started\r\n");
      _bg96_state = GPS_get_position;
      at_parser_init_output_object(&output_object);
      bg96_status = gnss_start(&output_object);
      break;

    case GPS_get_position:
      printf("bg96 asking for GPS position started \r\n");
      _bg96_state = net_delay_open;
      at_parser_init_output_object(&output_object);
      bg96_status = gnss_get_position(&output_object);
      break;

    case net_delay_open:
      if (output_object.error_code == 0) {
        printf("Found valid GPS position \r\n");
        GNSS_position_received = true;
        memset(latitude_data, '\0', sizeof(latitude_data));
        memset(longitude_data, '\0', sizeof(longitude_data));

        memcpy(latitude_data, &output_object.response_data[19], 10);
        memcpy(longitude_data, &output_object.response_data[30], 11);

        printf("latitude_data:%s\r\n", latitude_data);
        printf("longitude_data:%s\r\n", longitude_data);
      } else {
        printf("GPS position is not available\r\n");
        strcpy((char*)latitude_data, "<n.a.>");
        strcpy((char*)longitude_data, "<n.a.>");
      }
      printf("bg96 step to net_open \r\n");
      _bg96_state = net_open;
      at_parser_init_output_object(&output_object);
      bg96_status = bg96_nb_open_connection(&connection, &output_object);
      break;

    case net_open:
      printf("bg96 step to net_open finished \r\n");
      printf("bg96 starting net_send \r\n");
      _bg96_state = net_send;
      if (temperature_received || humidity_received || GNSS_position_received ){
        sprintf((char*) data_to_send,
        "{\"k\": \"%s\",\"d\":\"t: %d, h:%d, LON:%s LAT:%s\",\"t\":\"my_topic\"}",
        cloud_token, temperature_readout, humidity_readout, longitude_data,
        latitude_data);
        printf("message out:  %s \r\n", data_to_send);
        at_parser_init_output_object(&output_object);
        bg96_status = bg96_nb_send_data(&connection, data_to_send,
        &output_object);
      } else {
        printf("bg96 neither BLE temperature, BLE humidity nor bg96 position data is present \r\n");
      }
      break;

    case net_send:
      printf("bg96 exit net_send \r\n");

      printf("bg96 starting net_close \r\n");
      _bg96_state = net_close;
      at_parser_init_output_object(&output_object);
      bg96_status = bg96_nb_close_connection(&connection, &output_object);

      if (temperature_received && humidity_received && GNSS_position_received){
        printf("All data sent, wait for new termine \r\n");
        _bg96_state =  all_finished;
      }
      break;

    case net_close:
      printf("bg96 net_close finished \r\n");
      printf("bg96 starting delay \r\n");
      _bg96_state = net_delay;
      at_parser_init_output_object(&output_object);
      start_bg_96_timer(&output_object, 4000);
      break;

    case net_delay:
      printf("bg96 asking for GPS position started \r\n");
      _bg96_state = net_delay_open;
      at_parser_init_output_object(&output_object);
      bg96_status = gnss_get_position(&output_object);
      break;

    case all_finished:
      break;

    default:
      break;

    }
  }
}

/**************************************************************************//**
@brief
 *   Starting the global timer, the timeout will restart the BLE and cellular
 *   activity
 *****************************************************************************/
static void start_general_timer(void){
  sl_sleeptimer_start_timer(&general_timer,
                            sl_sleeptimer_ms_to_tick(general_timeout_sec * 1000),
                            general_timer_callback,
                            NULL, 0, 0);
  general_timer_running = true;
}

/**************************************************************************//**
@brief
 *   Starting the scan timer, this period is for find the strongest RSSI
 *   source with the given device name, or service identifier.
 *****************************************************************************/
static void start_BLE_scan_and_scan_timer(void){
  sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
  start_BLE_scanning_timer();
}

/**************************************************************************//**
 * @brief
 *   Start a one-shot timer in bg96 state, to wait some for the proper answer
 * @param[in] ms
 *   timer in millisecond
 * @param[in] *output
 *   pointer to the output object of the scheduler
 *****************************************************************************/
static void start_bg_96_timer(at_scheduler_status_t *output, uint32_t ms){
  sl_sleeptimer_start_timer(&bg96_state_timeout_timer,
      sl_sleeptimer_ms_to_tick(ms), bg96_state_machine_timer_callback, output,
      0, 0);
  }

/**************************************************************************//**
 * @brief
 *   callback of the BLE scan timer.
 *   will set a flag at the end of the period
 *   @param[in] *handle
 *   pointer to the sleeptimer handler
 *****************************************************************************/
static void BLE_scanning_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                 void *data){
  (void) data;
  (void) handle;
  BLE_scan_timeout_flag = true;
}

/***************************************************************************//**
 * @brief
 *   callback of the one-shot timer in bg96 state
 *   will set a flag at the end of the period
 * @param[in] *handle
 *   pointer to the sleeptimer handler
 *****************************************************************************/
static void bg96_state_machine_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                       void *data){
  (void) handle;
  at_scheduler_status_t *t_output = (at_scheduler_status_t*) data;
  printf("bg96 state machine timer expired \r\n");
  t_output->status = SL_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *   callback of the general timer
 *   set flags to will set to a basic state and
 *   start BLE scan process, and also bg96 process will re-start
 * @param[in] *handle
 *   pointer to the sleeptimer handler
 * ****************************************************************************/
static void general_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                            void *data){
  (void) data;
  (void) handle;
  printf("General timer expired \r\n");

///start BLE scanning
  start_BLE_scan_and_scan_timer();
///start BG 96 state
  _bg96_state = wake_up;
  temperature_received = false;
  humidity_received = false;
  GNSS_position_received = false;
  general_timer_running = false;
 }
