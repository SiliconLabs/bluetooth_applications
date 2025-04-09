/***************************************************************************//**
 * @file app.c
 * @brief source for BLE - BG96 cellular gateway sample application
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

/*******************************************************************************
 *******************************   INCLUDES ************************************
 ******************************************************************************/
#include "sl_bluetooth.h"

#include "mikroe_bg96.h"
#include "at_parser_core.h"
#include "at_parser_events.h"
#include "bg96_at_commands.h"

#include "app_timer.h"
#include "app_log.h"
#include "app_assert.h"

#define MAIN_TIMER_MS                 300000 // AT+QIOPEN take upto 150s to response, this timer should longer than 150s
#define SENDER_TIMER_MS               100
#define BLE_SCANNING_TIMER_MS         5000
#define MAX_ADVANCE_TIMEOUT_COUNTER   300
#define QUERY_NET_TIMER_MS            30000
#define QUERY_SIM_TIMER_MS            20000

// connection parameters
#define CONN_INTERVAL_MIN             80   // 100ms
#define CONN_INTERVAL_MAX             80   // 100ms
#define CONN_RESPONDER_LATENCY        0    // no latency
#define CONN_TIMEOUT                  100  // 1000ms
#define CONN_MIN_CE_LENGTH            0
#define CONN_MAX_CE_LENGTH            0xffff

#define CONNECTION_HANDLE_INVALID     ((uint8_t)0xffu)
#define SERVICE_HANDLE_INVALID        ((uint32_t)0xffffffffu)
#define CHARACTERISTIC_HANDLE_INVALID ((uint16_t)0xffffu)

typedef struct {
  uint8_t connection_handle;
  uint32_t envir_service_handle;
  uint16_t temperature_characteristic_handle;
  uint16_t humidity_characteristic_handle;
} conn_properties_t;

typedef struct {
  int8_t highest_rssi;
  bd_addr address;
  uint8_t address_type;
} scan_properties_t;

typedef enum {
  scanning,
  opening,
  discover_services,
  discover_temperature_characteristic,
  discover_humidity_characteristic,
  read_temperature_characteristic,
  read_humidity_characteristic,
  data_received
} conn_state_t;

typedef enum {
  _waking_up,
  _woke_up,
  _sim_querying,
  _sim_not_ready,
  _sim_ready,
  _net_configurating,
  _net_configurated,
  _net_gprs_querying,
  _net_lte_querying,
  _net_gprs_not_ready,
  _net_lte_not_ready,
  _net_ready,
  _net_activating,
  _net_activated,
  _gps_starting,
  _gps_started,
  _gps_location_getting,
  _gps_location_not_ready,
  _gps_closing,
  _gps_closed,
  _sender_timer_stopped,
  _tcp_opening,
  _tcp_opened,
  _tcp_sending,
  _tcp_sent,
  _tcp_close,
  _bg96_error,
  _free_running
} bg96_state_t;

// State of the connection under establishment
static conn_state_t conn_state;
static scan_properties_t scan_properties;
static conn_properties_t conn_properties;

// Environmental sensing service UUID: 181a
const uint8_t envir_service[2] = { 0x1a, 0x18 };

// temperature data UUID: 2a6e
const uint8_t temperature_char[2] = { 0x6e, 0x2a };

// humidity data uuid: 2a6f
const uint8_t humidity_char[2] = { 0x6f, 0x2a };

char device_name[] = "Thunderboard";

// 50% RHT is interpreted 5000
static  uint16_t humidity_readout = 5000;
// 25.0 °C is interpreted 2500
static  int16_t temperature_readout = 2500;

/*
 * TODO: Replace DEVICE_KEY with the Hologram device key found here:
 * https://support.hologram.io/hc/en-us/articles/360035212714
 * Please note that this is different than the device ID
 * and must be manually generated.
 */
static uint8_t cloud_token[] = "DEVICE_KEY";
static uint8_t latitude_data[15] = "<n.a.>";
static uint8_t longitude_data[15] = "<n.a.>";
static uint8_t data_to_send[150] = "";

static uint16_t advance_timeout_counter;

// status flags
static bool temperature_received = false;
static bool humidity_received = false;
static bool gps_location_received = false;
static bool bt_scan_timeout_flag = false;
static bool bg96_sim_query_timer_expired = false;
static bool bg96_net_query_timer_expired = false;

bg96_state_t _bg96_state = _waking_up;
at_scheduler_status_t output_object = { SL_STATUS_OK, 0, "" };

static bool find_device_in_advertisement(uint8_t *data, uint8_t len);

static app_timer_t bg96_periodic_timer;
static app_timer_t main_periodic_timer;
static app_timer_t ble_scanning_timer;
static app_timer_t sender_periodic_timer;
static app_timer_t bg96_sim_query_timer;
static app_timer_t bg96_net_query_timer;

// Application periodic timer callback.
static void bg96_periodic_timer_cb(app_timer_t *timer, void *data);
static void main_periodic_timer_cb(app_timer_t *timer, void *data);
static void ble_scanning_timer_cb(app_timer_t *timer, void *data);
static void sender_periodic_timer_cb(app_timer_t *timer, void *data);
static void bg96_sim_query_timer_cb(app_timer_t *timer, void *data);
static void bg96_net_query_timer_cb(app_timer_t *timer, void *data);

static void sim_status(void);
static void sim_status_handler(void *handler_data);
static void wakeup(void);
static void wakeup_handler(void *handler_data);
static void net_config(void);
static void net_config_handler(void *handler_data);
static void net_gprs_status(void);
static void net_gprs_status_handler(void *handler_data);
static void net_lte_status(void);
static void net_lte_status_handler(void *handler_data);
static void act(void);
static void act_handler(void *handler_data);
static void tcp_open(void);
static void tcp_open_handler(void *handler_data);
static void tcp_send(void);
static void tcp_send_handler(void *handler_data);
static void tcp_close(void);
static void tcp_close_handler(void *handler_data);
static void gps_start(void);
static void gps_start_handler(void *handler_data);
static void gps_get_location(void);
static void gps_get_location_handler(void *handler_data);
static void gps_stop(void);
static void gps_stop_handler(void *handler_data);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  sl_iostream_set_default(sl_iostream_vcom_handle);
  app_log_iostream_set(sl_iostream_vcom_handle);

  app_log("BLE Cellular Gateway example!\r\n");

  bg96_init(sl_iostream_uart_mikroe_handle);

  app_log("\r\nWaking-up device...\r\n");
  wakeup();
  _bg96_state = _waking_up;

  // Starting the scan timer, this period is for wake-up EFR32 device
  app_timer_start(&bg96_periodic_timer,
                  1,
                  bg96_periodic_timer_cb,
                  NULL,
                  true);
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
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Print boot message.
      app_log("Bluetooth stack booted: v%d.%d.%d-b%d" APP_LOG_NL,
              evt->data.evt_system_boot.major,
              evt->data.evt_system_boot.minor,
              evt->data.evt_system_boot.patch,
              evt->data.evt_system_boot.build);

      // Set the default connection parameters for subsequent connections
      sc = sl_bt_connection_set_default_parameters(CONN_INTERVAL_MIN,
                                                   CONN_INTERVAL_MAX,
                                                   CONN_RESPONDER_LATENCY,
                                                   CONN_TIMEOUT,
                                                   CONN_MIN_CE_LENGTH,
                                                   CONN_MAX_CE_LENGTH);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event is generated when an advertisement packet or a scan response
    // is received from a responder
    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      if (find_device_in_advertisement(
            &(evt->data.evt_scanner_legacy_advertisement_report.data.data[0]),
            evt->data.evt_scanner_legacy_advertisement_report.data.len)) {
        if (scan_properties.highest_rssi
            <= evt->data.evt_scanner_legacy_advertisement_report.rssi) {
          scan_properties.highest_rssi =
            evt->data.evt_scanner_legacy_advertisement_report.rssi;
          scan_properties.address =
            evt->data.evt_scanner_legacy_advertisement_report.address;
          scan_properties.address_type =
            evt->data.evt_scanner_legacy_advertisement_report.address_type;
        }

        if (bt_scan_timeout_flag == true) {
          app_log(
            "BLE: Found a Thunderboard, address: %02X:%02X:%02X:%02X:%02X:%02X, RSSI: %d\r\n",
            scan_properties.address.addr[5],
            scan_properties.address.addr[4],
            scan_properties.address.addr[3],
            scan_properties.address.addr[2],
            scan_properties.address.addr[1],
            scan_properties.address.addr[0],
            scan_properties.highest_rssi);
          app_log("BLE: Connecting...\r\n");
          sc = sl_bt_connection_open(scan_properties.address,
                                     scan_properties.address_type,
                                     sl_bt_gap_phy_1m,
                                     &conn_properties.connection_handle);
          app_assert_status(sc);
          sl_bt_scanner_stop();
          conn_state = opening;
          // Reset RSSI for next scan
          scan_properties.highest_rssi = -127;
          bt_scan_timeout_flag = false;
        }
      }
      break;

    // -------------------------------
    // This event is generated when a new connection is established
    case sl_bt_evt_connection_opened_id:
      app_log("BLE: Connection opened!\r\n");
      // Discover Environment Sensing service on the responder device
      sc = sl_bt_gatt_discover_primary_services_by_uuid(
        evt->data.evt_connection_opened.connection,
        sizeof(envir_service),
        (const uint8_t *)envir_service);
      app_assert_status(sc);
      conn_state = discover_services;
      break;

    // -------------------------------
    // This event is generated when a new service is discovered
    case sl_bt_evt_gatt_service_id:
      app_log("BLE: Environmental Sensing service discovered.\r\n");
      // Save service handle for future reference
      conn_properties.envir_service_handle = evt->data.evt_gatt_service.service;
      break;

    // -------------------------------
    // This event is generated when a new characteristic is discovered
    case sl_bt_evt_gatt_characteristic_id:
      // check for the characteristic identifier, and save it to handler variable
      if (evt->data.evt_gatt_characteristic.uuid.len
          == sizeof(temperature_char)) {
        if (memcmp(temperature_char,
                   evt->data.evt_gatt_characteristic.uuid.data,
                   sizeof(temperature_char)) == 0) {
          // Save characteristic handle for future reference
          conn_properties.temperature_characteristic_handle =
            evt->data.evt_gatt_characteristic.characteristic;
          app_log("BLE: Temperature characteristic discovered.\r\n");
        }
      }
      if (evt->data.evt_gatt_characteristic.uuid.len == sizeof(humidity_char)) {
        if (memcmp(humidity_char, evt->data.evt_gatt_characteristic.uuid.data,
                   sizeof(humidity_char)) == 0) {
          // Save characteristic handle for future reference
          conn_properties.humidity_characteristic_handle =
            evt->data.evt_gatt_characteristic.characteristic;
          app_log("BLE: Humidity characteristic discovered.\r\n");
        }
      }
      break;

    // -------------------------------
    // This event is generated for various procedure completions, e.g. when a
    // write procedure is completed, or service discovery is completed
    case sl_bt_evt_gatt_procedure_completed_id:
      // If service discovery finished
      if ((conn_state == discover_services)
          && (conn_properties.envir_service_handle != SERVICE_HANDLE_INVALID)) {
        // Discover temperature characteristic on the responder device
        sc = sl_bt_gatt_discover_characteristics_by_uuid(
          conn_properties.connection_handle,
          conn_properties.envir_service_handle,
          sizeof(temperature_char),
          (const uint8_t *)temperature_char);
        app_assert_status(sc);
        conn_state = discover_temperature_characteristic;
        break;
      }
      // If temperature characteristic discovery finished
      if (conn_state == discover_temperature_characteristic) {
        // Discover humidity characteristic on the responder device
        sc = sl_bt_gatt_discover_characteristics_by_uuid(
          evt->data.evt_gatt_procedure_completed.connection,
          conn_properties.envir_service_handle,
          sizeof(humidity_char),
          (const uint8_t *)humidity_char);
        app_assert_status(sc);
        conn_state = discover_humidity_characteristic;
        break;
      }
      // If humidity characteristic discovery finished
      if ((conn_state == discover_humidity_characteristic)
          && (conn_properties.temperature_characteristic_handle
              != CHARACTERISTIC_HANDLE_INVALID)) {
        // Read temperature characteristic value
        sc = sl_bt_gatt_read_characteristic_value_by_uuid(
          conn_properties.connection_handle,
          conn_properties.envir_service_handle,
          sizeof(temperature_char),
          (const uint8_t *)temperature_char);
        app_assert_status(sc);
        app_log("BLE: Temperature value reading...\r\n");
        conn_state = read_temperature_characteristic;
        break;
      }
      // If read temperature characteristic finished
      if ((conn_state == read_temperature_characteristic)
          && (conn_properties.humidity_characteristic_handle
              != CHARACTERISTIC_HANDLE_INVALID)) {
        // Read humidity characteristic value
        sc = sl_bt_gatt_read_characteristic_value_by_uuid(
          conn_properties.connection_handle,
          conn_properties.envir_service_handle,
          sizeof(humidity_char),
          (const uint8_t *)humidity_char);
        app_assert_status(sc);
        app_log("BLE: Humidity value reading...\r\n");
        conn_state = read_humidity_characteristic;
        break;
      }
      // If read humidity characteristic finished
      if (conn_state == read_humidity_characteristic) {
        conn_state = data_received;
        break;
      }
      break;

    // -------------------------------
    // This event is generated when a characteristic value was received e.g. an indication
    case sl_bt_evt_gatt_characteristic_value_id:
      // after request for read out, in this event is possible to read the actual measured data
      if (evt->data.evt_gatt_characteristic_value.characteristic
          == conn_properties.temperature_characteristic_handle) {
        memcpy(&temperature_readout,
               &evt->data.evt_gatt_server_attribute_value.value.data[0],
               evt->data.evt_gatt_server_attribute_value.value.len);
        app_log("BLE: Temperature: %d,%02d Celsius \r\n",
                temperature_readout / 100,
                temperature_readout % 100);
        temperature_received = true;
      }

      if (evt->data.evt_gatt_characteristic_value.characteristic
          == conn_properties.humidity_characteristic_handle) {
        memcpy(&humidity_readout,
               &evt->data.evt_gatt_server_attribute_value.value.data[0],
               evt->data.evt_gatt_server_attribute_value.value.len);
        app_log("BLE: Humidity: %d,%02d %s RHT \r\n",
                humidity_readout / 100,
                humidity_readout % 100, "%");
        humidity_received = true;
      }
      if (temperature_received && humidity_received) {
        sl_bt_connection_close(conn_properties.connection_handle);
      }
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("BLE: Connection closed!\r\n");
      if ((temperature_received == false) || (humidity_received == false)) {
        app_log("BLE: Device not found, starting another scan \r\n");
        main_periodic_timer_cb(NULL, NULL);
      }
      break;

    default:
      break;
  }
}

/**************************************************************************//**
 *    @brief
 * Decoding advertising packets is done here. The list of AD types can be found
 * at: https://www.bluetooth.com/specifications/assigned-numbers/Generic-Access-Profile
 *****************************************************************************/
static bool find_device_in_advertisement(uint8_t *data, uint8_t len)
{
  uint8_t ad_field_length;
  uint8_t ad_field_type;
  uint8_t i = 0;

  while (i < len) {
    ad_field_length = data[i];
    ad_field_type = data[i + 1];

    if ((ad_field_type == 0x08) || (ad_field_type == 0x09)) {
      // Compare local name
      if (strncmp((char *)&data[i + 2], device_name,
                  sizeof(device_name) - 1) == 0) {
        return 1;
      }
    }

    if ((ad_field_type == 0x02) || (ad_field_type == 0x03)) {
      // Type 0x02 = Incomplete List of 16-bit Service Class UUIDs
      // Type 0x03 = Complete List of 16-bit Service Class UUIDs
      if (memcmp(envir_service, &data[i + 2], 2) == 0) {
        return 1;
      }
    }
    // advance to the next AD struct
    i = i + ad_field_length + 1;
  }

  return 0;
}

/**************************************************************************//**
 * @brief
 *   Callback of the BG96 periodic timer. This timer is used to wake-up
 *   EFR32 device from sleep mode
 *
 * @param[in] handle
 *   Pointer to the sleeptimer handler
 * @param[in] data
 *   Pointer to the data handler
 *****************************************************************************/
static void bg96_periodic_timer_cb(app_timer_t *timer, void *data)
{
  (void) data;
  (void) timer;

  at_parser_process();
  at_platform_process();
  at_event_process();

  switch (_bg96_state) {
    case _woke_up:
      // Start to query SIM card status
      app_log("\r\nSIM card status querying...\r\n");
      _bg96_state = _sim_querying;
      sim_status();
      // Start a oneshot timer 20s
      bg96_sim_query_timer_expired = false;
      app_timer_start(&bg96_sim_query_timer,
                      QUERY_SIM_TIMER_MS,
                      bg96_sim_query_timer_cb,
                      NULL,
                      false);
      break;
    case _sim_not_ready:
      if (bg96_sim_query_timer_expired == true) {
        _bg96_state = _bg96_error; // There are error, need to reboot BG96 module
        app_log("[E]: Failed to identify (U)SIM card in 20s,"
                " the BG96 module should be reboot!\r\n");
      } else {
        _bg96_state = _sim_querying;
        sim_status();
      }
      break;
    case _sim_ready:
      app_timer_stop(&bg96_sim_query_timer);
      // Starting configuration network
      app_log("\r\nNetwork configuring...\r\n");
      _bg96_state = _net_configurating;
      net_config();
      break;
    case _net_configurated:
      // Start query PS service status with GPRS service first
      app_log("\r\nPS service status querying...\r\n");
      _bg96_state = _net_gprs_querying;
      net_gprs_status();
      // Start a oneshot timer 60s
      bg96_net_query_timer_expired = false;
      app_timer_start(&bg96_net_query_timer,
                      QUERY_NET_TIMER_MS,
                      bg96_net_query_timer_cb,
                      NULL,
                      false);
      break;
    case _net_gprs_not_ready:
      // Try to query LTE service
      _bg96_state = _net_lte_querying;
      net_lte_status();
      break;
    case _net_lte_not_ready:
      if (bg96_net_query_timer_expired == true) {
        _bg96_state = _bg96_error; // There are error, need to reboot BG96 module
        app_log("[E]: Failed to register on PS domain service in 60s,"
                " the BG96 module should be reboot!\r\n");
      } else {
        // Retry to query GPRS service again
        _bg96_state = _net_gprs_querying;
        net_gprs_status();
      }
      break;
    case _net_ready:
      app_timer_stop(&bg96_net_query_timer);
      app_log("\r\nPDP Context activating...\r\n");
      // Start to Activate a PDP Context.
      _bg96_state = _net_activating;
      act();
      break;
    case _net_activated:
      _bg96_state = _free_running;
      app_log("\r\nStart a periodic timer used for gathering data!\r\n");
      // Start a periodic timer used for for gathering sensor data,
      // device location and send these data-set as a payload to the cloud service.
      app_timer_start(&main_periodic_timer,
                      MAIN_TIMER_MS,
                      main_periodic_timer_cb,
                      NULL,
                      true);
      // For first time
      main_periodic_timer_cb(NULL, NULL);
      break;
    case _gps_started:
      app_log("\r\nGPS: Location getting...\r\n\r\n");
      // Invoke GPS location process
      _bg96_state = _gps_location_getting;
      gps_get_location();
      break;
    case _sender_timer_stopped:
      app_log("\r\nSender timer stopped!\r\n");
      app_log("\r\nGPS: Session stopping...\r\n");
      // Stop GPS
      _bg96_state = _gps_closing;
      gps_stop();
      break;
    case _gps_closed:
      if (temperature_received || humidity_received || gps_location_received) {
        // Prepare payload data to send
        sprintf((char *) data_to_send,
                "{\"k\": \"%s\",\"d\":\"t: %d, h:%d, LON:%s LAT:%s\",\"t\":\"my_topic\"}",
                cloud_token,
                temperature_readout,
                humidity_readout,
                longitude_data,
                latitude_data);
        app_log("\r\nData to send:  %s \r\n", data_to_send);
        app_log("\r\nTCP: Connection opening...\r\n");
        // Open a TCP connection to server
        _bg96_state = _tcp_opening;
        tcp_open();
      } else {
        app_log("\r\nNo data found to send!\r\n");
        _bg96_state = _bg96_error;
      }
      break;
    case _tcp_opened:
      app_log("\r\nTCP: Data sending...\r\n");
      // Send a message via a TCP connection to server
      _bg96_state = _tcp_sending;
      tcp_send();
      break;
    case _tcp_sent:
      app_log("\r\nTCP: Connection closing...\r\n");
      // Close a TCP connection to server
      _bg96_state = _tcp_close;
      tcp_close();
      break;
    default:
      break;
  }
}

static void bg96_sim_query_timer_cb(app_timer_t *timer, void *data)
{
  (void) data;
  (void) timer;

  bg96_sim_query_timer_expired = true;
}

static void bg96_net_query_timer_cb(app_timer_t *timer, void *data)
{
  (void) data;
  (void) timer;

  bg96_net_query_timer_expired = true;
}

/***************************************************************************//**
 * @brief
 *   callback of the main timer
 * @param[in] handle
 *   Pointer to the sleeptimer handler
 * @param[in] data
 *   Pointer to the data handler
 * ****************************************************************************/
static void main_periodic_timer_cb(app_timer_t *timer, void *data)
{
  (void) data;
  (void) timer;

  // Start scanning - looking for Thunderboard devices
  sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
  app_log("\r\nBLE: Scanning... \r\n");
  conn_state = scanning;
  // Starting the scan timer, this period is for find the strongest RSSI
  // source with the given device name, or service identifier.
  app_timer_start(&ble_scanning_timer,
                  BLE_SCANNING_TIMER_MS,
                  ble_scanning_timer_cb,
                  NULL,
                  false);
  temperature_received = false;
  humidity_received = false;
  gps_location_received = false;
  scan_properties.highest_rssi = -127;

  // Start retrieving device location
  app_log("\r\nGPS: Session starting... \r\n");
  _bg96_state = _gps_starting;
  gps_start();

  // Start Sender Timer
  advance_timeout_counter = 0;
  app_timer_start(&sender_periodic_timer,
                  SENDER_TIMER_MS,
                  sender_periodic_timer_cb,
                  NULL,
                  true);
}

/**************************************************************************//**
 * @brief
 *   callback of the BLE scan timer.
 *   will set a flag at the end of the period
 * @param[in] handle
 *   Pointer to the sleeptimer handler
 * @param[in] data
 *   Pointer to the data handler
 *****************************************************************************/
static void ble_scanning_timer_cb(app_timer_t *timer, void *data)
{
  (void) data;
  (void) timer;

  bt_scan_timeout_flag = true;
}

/**************************************************************************//**
 * @brief
 *   callback of the Sender periodic timer
 * @param[in] handle
 *   Pointer to the sleeptimer handler
 * @param[in] data
 *   Pointer to the data handler
 *****************************************************************************/
static void sender_periodic_timer_cb(app_timer_t *timer, void *data)
{
  (void) timer;
  (void) data;

  if (temperature_received && humidity_received && gps_location_received) {
    // All data received, Stop sender timer
    app_timer_stop(&sender_periodic_timer);
    _bg96_state = _sender_timer_stopped;
    return;
  }

  if (++advance_timeout_counter > MAX_ADVANCE_TIMEOUT_COUNTER) {
    // Timeout, Stop sender timer
    app_timer_stop(&sender_periodic_timer);
    _bg96_state = _sender_timer_stopped;
    return;
  }

  if ((!gps_location_received) && (_bg96_state == _gps_location_not_ready)) {
    // Invoke GPS location process
    _bg96_state = _gps_location_getting;
    gps_get_location();
  }
}

/***************************************************************************//**
 * @brief
 *    Wake up BG96 module.
 *    Result will be available in the global output_object.
 *
 ******************************************************************************/
static void wakeup(void)
{
  at_parser_init_output_object(&output_object);
  bg96_wake_up(&output_object);
  at_listen_event((uint8_t *) &output_object.status, SL_STATUS_OK,
                  wakeup_handler, (void *) &output_object);
}

/***************************************************************************//**
 * @brief
 *    Wake up handler.
 *
 * @param[in] handler_data
 *    Data sent by the event handler.
 *    Currently  handler_data is a pointer to an at_scheduler_status_t.
 *
 ******************************************************************************/
static void wakeup_handler(void *handler_data)
{
  at_scheduler_status_t *l_output = (at_scheduler_status_t *) handler_data;
  if (l_output->error_code) {
    app_log("[E]: Waking up BG96 module error: %d\r\n",
            l_output->error_code);
    app_log("The BG96 module should be reboot!\r\n");
    _bg96_state = _bg96_error;
  } else {
    app_log("BG96 module woke-up!\r\n");
    _bg96_state = _woke_up;
  }
}

/***************************************************************************//**
 * @brief
 *    Query SIM status.
 *    Result will be available in the global output_object.
 *
 ******************************************************************************/
static void sim_status(void)
{
  at_parser_init_output_object(&output_object);
  bg96_sim_status(&output_object);
  at_listen_event((uint8_t *) &output_object.status, SL_STATUS_OK,
                  sim_status_handler, (void *) &output_object);
}

/***************************************************************************//**
 * @brief
 *    Query SIM status handler.
 *
 * @param[in] handler_data
 *    Data sent by the event handler.
 *    Currently handler_data is a pointer to an at_scheduler_status_t.
 *
 ******************************************************************************/
static void sim_status_handler(void *handler_data)
{
  at_scheduler_status_t *l_output = (at_scheduler_status_t *) handler_data;

  if (l_output->error_code) {
    _bg96_state = _sim_not_ready;
  } else {
    app_log("SIM card ready to use!\r\n");
    _bg96_state = _sim_ready;
  }
}

/***************************************************************************//**
 * @brief
 *    Network configuration.
 *    Result will be available in the global output_object.
 *
 ******************************************************************************/
static void net_config(void)
{
  at_parser_init_output_object(&output_object);
  bg96_network_registration(&output_object);
  at_listen_event((uint8_t *) &output_object.status, SL_STATUS_OK,
                  net_config_handler, (void *) &output_object);
}

/***************************************************************************//**
 * @brief
 *    Network configuration handler.
 *
 * @param[in] handler_data
 *    Data sent by the event handler.
 *    Currently handler_data is a pointer to an at_scheduler_status_t.
 *
 ******************************************************************************/
static void net_config_handler(void *handler_data)
{
  at_scheduler_status_t *l_output = (at_scheduler_status_t *) handler_data;
  if (l_output->error_code) {
    app_log("[E]: Network configuration error: %d\r\n%s\r\n",
            l_output->error_code,
            l_output->response_data);
    app_log("The BG96 module should be reboot!\r\n");
    _bg96_state = _bg96_error;
  } else {
    app_log("Network configuration done!\r\n");
    _bg96_state = _net_configurated;
  }
}

/***************************************************************************//**
 * @brief
 *    Query the status of GPRS service
 *    Result will be available in the global output_object.
 *
 ******************************************************************************/
static void net_gprs_status(void)
{
  at_parser_init_output_object(&output_object);
  bg96_query_gprs_service(&output_object);
  at_listen_event((uint8_t *) &output_object.status, SL_STATUS_OK,
                  net_gprs_status_handler, (void *) &output_object);
}

/***************************************************************************//**
 * @brief
 *    Query the status of GPRS service handler
 *
 * @param[in] handler_data
 *    Data sent by the event handler.
 *    Currently handler_data is a pointer to an at_scheduler_status_t.
 *
 ******************************************************************************/
static void net_gprs_status_handler(void *handler_data)
{
  at_scheduler_status_t *l_output = (at_scheduler_status_t *) handler_data;
  if (l_output->error_code) {
    // Start Query the status of LTE service
    _bg96_state = _net_gprs_not_ready;
  } else {
    app_log("Registered to GPRS network!\r\n");
    _bg96_state = _net_ready;
  }
}

/***************************************************************************//**
 * @brief
 *    Query the status of LTE service
 *    Result will be available in the global output_object.
 *
 ******************************************************************************/
static void net_lte_status(void)
{
  at_parser_init_output_object(&output_object);
  bg96_query_lte_service(&output_object);
  at_listen_event((uint8_t *) &output_object.status, SL_STATUS_OK,
                  net_lte_status_handler, (void *) &output_object);
}

/***************************************************************************//**
 * @brief
 *    Query the status of LTE service handler
 *
 * @param[in] handler_data
 *    Data sent by the event handler.
 *    Currently handler_data is a pointer to an at_scheduler_status_t.
 *
 ******************************************************************************/
static void net_lte_status_handler(void *handler_data)
{
  at_scheduler_status_t *l_output = (at_scheduler_status_t *) handler_data;
  if (l_output->error_code) {
    _bg96_state = _net_lte_not_ready;
  } else {
    app_log("Registered to LTE network!\r\n");
    _bg96_state = _net_ready;
  }
}

/***************************************************************************//**
 * @brief
 *    Activate a PDP Context.
 *    Result will be available in the global output_object.
 *
 ******************************************************************************/
static void act(void)
{
  at_parser_init_output_object(&output_object);
  bg96_activate_pdp_context(&output_object);
  at_listen_event((uint8_t *) &output_object.status, SL_STATUS_OK,
                  act_handler, (void *) &output_object);
}

/***************************************************************************//**
 * @brief
 *    Activate a PDP context handler.
 *
 * @param[in] handler_data
 *    Data sent by the event handler.
 *    Currently handler_data is a pointer to an at_scheduler_status_t.
 *
 ******************************************************************************/
static void act_handler(void *handler_data)
{
  at_scheduler_status_t *l_output = (at_scheduler_status_t *) handler_data;
  if (l_output->error_code) {
    app_log("[E]: Activate a PDP Context error: %d\r\n%s\r\n",
            l_output->error_code,
            l_output->response_data);
    app_log("The BG96 module should be reboot!\r\n");
    _bg96_state = _bg96_error;
  } else {
    app_log("Activated a PDP Context successfully!\r\n");
    _bg96_state = _net_activated;
  }
}

/***************************************************************************//**
 * @brief
 *    TCP: Open connection.
 *    Result will be available in the global output_object.
 *
 ******************************************************************************/
static void tcp_open(void)
{
  bg96_tcp_connection_t connection = {
    0,
    9999,
    "TCP",
    (uint8_t *) "cloudsocket.hologram.io"
  };

  at_parser_init_output_object(&output_object);
  bg96_tcp_open_connection(&connection, &output_object);
  at_listen_event((uint8_t *) &output_object.status, SL_STATUS_OK,
                  tcp_open_handler, (void *) &output_object);
}

/***************************************************************************//**
 * @brief
 *    TCP: Open connection handler.
 *
 * @param[in] handler_data
 *    Data sent by the event handler.
 *    Currently handler_data is a pointer to an at_scheduler_status_t.
 *
 ******************************************************************************/
static void tcp_open_handler(void *handler_data)
{
  at_scheduler_status_t *l_output = (at_scheduler_status_t *) handler_data;

  if (l_output->error_code) {
    app_log("[E]: TCP open connection error: %d\r\n%s\r\n",
            l_output->error_code,
            l_output->response_data);
    // Need to close TCP connection
    _bg96_state = _tcp_sent;
  } else {
    app_log("TCP: Connection opened!\r\n");
    _bg96_state = _tcp_opened;
  }
}

/***************************************************************************//**
 * @brief
 *    TCP: Send data on an opened channel.
 *    Result will be available in the global output_object.
 *
 ******************************************************************************/
static void tcp_send(void)
{
  bg96_tcp_connection_t connection = {
    0,
    9999,
    "TCP",
    (uint8_t *) "cloudsocket.hologram.io"
  };

  at_parser_init_output_object(&output_object);
  bg96_tcp_send_data(&connection, data_to_send, &output_object);
  at_listen_event((uint8_t *) &output_object.status, SL_STATUS_OK,
                  tcp_send_handler,
                  (void *) &output_object);
}

/***************************************************************************//**
 * @brief
 *    TCP: Send data handler.
 *
 * @param[in] handler_data
 *    Data sent by the event handler.
 *    Currently handler_data is a pointer to an at_scheduler_status_t.
 *
 ******************************************************************************/
static void tcp_send_handler(void *handler_data)
{
  at_scheduler_status_t *l_output = (at_scheduler_status_t *) handler_data;
  if (l_output->error_code) {
    app_log("[E]: TCP send data error: %d\r\n%s\r\n",
            l_output->error_code,
            l_output->response_data);
  } else {
    app_log("TCP: Data sent!\r\n");
  }
  // Need to close TCP connection
  _bg96_state = _tcp_sent;
}

/***************************************************************************//**
 * @brief
 *    TCP: Close connection.
 *    Result will be available in the global output_object.
 *
 ******************************************************************************/
static void tcp_close(void)
{
  bg96_tcp_connection_t connection = {
    0,
    9999,
    "TCP",
    (uint8_t *) "cloudsocket.hologram.io"
  };

  at_parser_init_output_object(&output_object);
  bg96_tcp_close_connection(&connection, &output_object);
  at_listen_event((uint8_t *) &output_object.status,
                  SL_STATUS_OK,
                  tcp_close_handler,
                  (void *) &output_object);
}

/***************************************************************************//**
 * @brief
 *    TCP: Close connection handler.
 *
 * @param[in] handler_data
 *    Data sent by the event handler.
 *    Currently handler_data is a pointer to an at_scheduler_status_t.
 *
 ******************************************************************************/
static void tcp_close_handler(void *handler_data)
{
  at_scheduler_status_t *l_output = (at_scheduler_status_t *) handler_data;

  if (l_output->error_code) {
    app_log("[E]: TCP close connection error: %d\r\n%s\r\n",
            l_output->error_code,
            l_output->response_data);
    _bg96_state = _bg96_error;
  } else {
    app_log("TCP: Connection closed!\r\n");
  }
}

/***************************************************************************//**
 * @brief
 *    GPS: Start session.
 *    Result will be available in the global output_object.
 *
 ******************************************************************************/
static void gps_start(void)
{
  at_parser_init_output_object(&output_object);
  bg96_gnss_start(&output_object);
  at_listen_event((uint8_t *) &output_object.status, SL_STATUS_OK,
                  gps_start_handler, (void *) &output_object);
}

/***************************************************************************//**
 * @brief
 *    GPS: Start session handler.
 *
 * @param[in] handler_data
 *    Data sent by the event handler.
 *    Currently handler_data is a pointer to an at_scheduler_status_t.
 *
 ******************************************************************************/
static void gps_start_handler(void *handler_data)
{
  at_scheduler_status_t *l_output = (at_scheduler_status_t *) handler_data;

  if (l_output->error_code) {
    app_log("[E]: GPS start session error: %d\r\n%s\r\n",
            l_output->error_code,
            l_output->response_data);
  } else {
    app_log("GPS: Session started!\r\n");
    _bg96_state = _gps_started;
  }
}

/***************************************************************************//**
 * @brief
 *    GPS: Get location.
 *    Result will be available in the global output_object.
 *
 ******************************************************************************/
static void gps_get_location(void)
{
  at_parser_init_output_object(&output_object);
  bg96_gnss_get_position(&output_object);
  at_listen_event((uint8_t *) &output_object.status, SL_STATUS_OK,
                  gps_get_location_handler, (void *) &output_object);
}

/***************************************************************************//**
 * @brief
 *    GPS: Get location handler.
 *
 * @param[in] handler_data
 *    Data sent by the event handler.
 *    Currently handler_data is a pointer to an at_scheduler_status_t.
 *    +QGPSLOC: <UTC>,<latitude>,<longitude>,<hdop>,<altitude>,<fix>,<cog>,
 *    <spkm>,<spkn>,<date>,<nsat>
 ******************************************************************************/
static void gps_get_location_handler(void *handler_data)
{
  at_scheduler_status_t *l_output = (at_scheduler_status_t *) handler_data;

  if (l_output->error_code) {
    _bg96_state = _gps_location_not_ready;
  } else {
    app_log("GPS: Raw data: %s\r\n", l_output->response_data);
    gps_location_received = true;
    memset(latitude_data, '\0', sizeof(latitude_data));
    memset(longitude_data, '\0', sizeof(longitude_data));

    memcpy(latitude_data, &output_object.response_data[19], 10);
    memcpy(longitude_data, &output_object.response_data[30], 11);

    app_log("GPS: Latitude data: %s\r\n", latitude_data);
    app_log("GPS: Longitude data: %s\r\n", longitude_data);
  }
}

/***************************************************************************//**
 * @brief
 *    GPS: Stop session.
 *    Result will be available in the global output_object.
 *
 ******************************************************************************/
static void gps_stop(void)
{
  at_parser_init_output_object(&output_object);
  bg96_gnss_stop(&output_object);
  at_listen_event((uint8_t *) &output_object.status, SL_STATUS_OK,
                  gps_stop_handler, (void *) &output_object);
}

/***************************************************************************//**
 * @brief
 *    GPS: Stop session handler.
 *
 * @param[in] handler_data
 *    Data sent by the event handler.
 *    Currently handler_data is a pointer to an at_scheduler_status_t.
 *
 ******************************************************************************/
static void gps_stop_handler(void *handler_data)
{
  at_scheduler_status_t *l_output = (at_scheduler_status_t *) handler_data;

  if (l_output->error_code) {
    app_log("[E]: GPS stop session error: %d\r\n%s\r\n",
            l_output->error_code,
            l_output->response_data);
    _bg96_state = _bg96_error;
  } else {
    app_log("GPS: Session stopped!\r\n");
  }
  _bg96_state = _gps_closed;
}
