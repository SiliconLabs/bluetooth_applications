/***************************************************************************//**
 * @file app.c
 * @brief Main application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 ********************************************************************************
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
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/
#include <math.h>

#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "dweet_http_client.h"
#include "app.h"

// -----------------------------------------------------------------------------
// Local Macros and Definitions

#if defined(SL_CATALOG_SIMPLE_LED_PRESENT) && defined(SL_CATALOG_LED0_PRESENT)
#include "sl_simple_led_instances.h"
#define led0_on()   sl_led_turn_on(&sl_led_led0);
#define led0_off()  sl_led_turn_off(&sl_led_led0);
#else
#define led0_on()
#define led0_off()
#endif

#define THUNDERBOARD_NAME_CHECK         "Thunderboard"
#define THUNDERBOARD_NAME_CHECK_LENGTH  12

#define DATA_COLLECTER_INTERVAL_MS      10000
#define CONNECTION_TIMEOUT_MS           3000
#define SCANNER_TIMEOUT_MS              3000

// connection parameters
#define CONN_INTERVAL_MIN               80 // 100ms
#define CONN_INTERVAL_MAX               80 // 100ms
#define CONN_RESPONDER_LATENCY          0  // no latency
#define CONN_TIMEOUT                    100 // 1000ms
#define CONN_MIN_CE_LENGTH              0
#define CONN_MAX_CE_LENGTH              0xffff

#define SCAN_INTERVAL                   16 // 10ms
#define SCAN_WINDOW                     16 // 10ms
#define SCAN_PASSIVE                    0

#define TEMP_INVALID                    NAN
#define HUMID_INVALID                   NAN
#define CONNECTION_HANDLE_INVALID       ((uint8_t)0xFFu)
#define RSSI_INVALID                    ((int8_t)0x7F)
#define SERVICE_HANDLE_INVALID          ((uint32_t)0xFFFFFFFFu)
#define CHARACTERISTIC_HANDLE_INVALID   ((uint16_t)0xFFFFu)
#define TX_POWER_INVALID                ((uint8_t)0x7C)
#define TX_POWER_CONTROL_ACTIVE         ((uint8_t)0x00)
#define TX_POWER_CONTROL_INACTIVE       ((uint8_t)0x01)
#define TABLE_INDEX_INVALID             ((uint8_t)0xFFu)

#define CHARACTERISTIC_COUNT            2

#define SCANNER_TIMEOUT_EVENT           (1<<0)
#define CONNECTION_CLOSE_REQUEST_EVENT  (1<<1)
#define DATA_COLLECT_EVENT              (1<<2)

// -----------------------------------------------------------------------------
// Application state
typedef enum {
  scanning,
  opening,
  discover_services,
  discover_characteristics,
  enable_indication,
  stopping,
  running
} conn_state_t;

// -----------------------------------------------------------------------------
// Characteristic index
typedef enum {
  characteristic_index_temperature,
  characteristic_index_humidity,
  characteristic_index_max
} characteristic_index_t;

// -----------------------------------------------------------------------------
// Structure that holds Thunderboard sense connection data
typedef struct {
  uint8_t  connection_handle;
  int8_t   rssi;
  uint16_t server_address;
  uint32_t es_service_handle;
  uint16_t characteristic_handle[CHARACTERISTIC_COUNT];
  float temperature;
  float humidity;
} conn_properties_t;

// -----------------------------------------------------------------------------
// Structure that holds Thunderboard sense address info
typedef struct {
  bd_addr address;
  uint8_t address_type;
  int8_t rssi;
} remote_sensor_t;

// -----------------------------------------------------------------------------
// Characteristic uuid
typedef struct {
  const uint8_t *uuid;
  const uint8_t size;
} characteristic_uuid_t;

// -----------------------------------------------------------------------------
// Characteristic reading state
typedef struct {
  characteristic_index_t index;
  uint8_t count;
}characteristic_read_t;

// -----------------------------------------------------------------------------
// Local variables
static remote_sensor_t remote_sensor;

// Array for holding properties of multiple (parallel) connections
static conn_properties_t conn_properties;
// State of the connection under establishment
static conn_state_t conn_state;
// Environment sensing service UUID defined by Bluetooth SIG
static const uint8_t environment_sensing_service[2] = { 0x1a, 0x18 };
// Temperature  & Humidity Measurement characteristic UUID defined by Bluetooth
//   SIG
static const uint8_t es_temperature_char[2] = { 0x6e, 0x2a };
static const uint8_t es_humidity_char[2] = { 0x6f, 0x2a };

static characteristic_read_t characteristic_read;

static sl_sleeptimer_timer_handle_t scanner_timeout_timer;
static sl_sleeptimer_timer_handle_t connection_timeout_timer;
static sl_sleeptimer_timer_handle_t data_collector_timer;

// -----------------------------------------------------------------------------
// Common local function declarations

static void init_properties(void);
static uint8_t find_device_name_pattern_in_advertisement(const uint8_t *data,
                                                         uint8_t data_length,
                                                         const char *name,
                                                         uint8_t name_length);
static void add_connection(uint8_t connection, uint16_t address);
static void remove_connection(void);
static uint8_t bt_gatt_read_characteristic_value_from_offset(uint16_t offset,
                                                             uint16_t maxlen);
static void read_first_characteristic_value(void);
static void read_next_characteristic_value(void);
static void start_scanner_timeout_timer(void);
static void stop_scanner_timeout_timer(void);
static void start_connection_timeout_timer(void);
static void stop_connection_timeout_timer(void);
static void start_periodic_timer(void);
static void stop_periodic_timer(void);
static void start_discovery(void);
static void start_connect(void);


static void app_bt_system_boot(const sl_bt_evt_system_boot_t *data);
static void app_bt_evt_scanner_scan_report(const sl_bt_evt_scanner_scan_report_t *scan_report);
static void app_bt_evt_connection_opened(const sl_bt_evt_connection_opened_t *connection_opened);
static void app_bt_evt_gatt_service(const sl_bt_evt_gatt_service_t *evt_data);
static void app_bt_evt_gatt_procedure_completed(const sl_bt_evt_gatt_procedure_completed_t *evt_data);
static void app_bt_evt_gatt_characteristic(const sl_bt_evt_gatt_characteristic_t *evt_data);
static void app_bt_evt_gatt_characteristic_value(const sl_bt_evt_gatt_characteristic_value_t *evt_data);
static void app_bt_evt_connection_closed(const sl_bt_evt_connection_closed_t *evt_data);
static void app_bt_evt_system_external_signal(const sl_bt_evt_system_external_signal_t *evt_data);
static void scanner_timeout_callback(sl_sleeptimer_timer_handle_t *timer,
                                    void *data);
static void connection_timeout_callback(sl_sleeptimer_timer_handle_t *timer,
                                    void *data);
static void data_collecter_callback(sl_sleeptimer_timer_handle_t *timer,
                                    void *data);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  init_properties();
  dweet_http_client_init();
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
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
  switch (SL_BT_MSG_ID(evt->header)) {
  // -------------------------------
  // This event indicates the device has started and the radio is ready.
  // Do not call any stack command before receiving this boot event!
  case sl_bt_evt_system_boot_id:
    app_bt_system_boot(&(evt->data.evt_system_boot));
    break;

  // -------------------------------
  // This event is generated when an advertisement packet or a scan response
  // is received from a responder
  case sl_bt_evt_scanner_scan_report_id:
    app_bt_evt_scanner_scan_report(&(evt->data.evt_scanner_scan_report));
    break;

  // -------------------------------
  // This event indicates that a new connection was opened.
  case sl_bt_evt_connection_opened_id:
    app_bt_evt_connection_opened(&(evt->data.evt_connection_opened));
    break;

  // -------------------------------
  // This event is generated when a new service is discovered
  case sl_bt_evt_gatt_service_id:
    app_bt_evt_gatt_service(&(evt->data.evt_gatt_service));
    break;

  // -------------------------------
  // This event is generated when a new characteristic is discovered
  case sl_bt_evt_gatt_characteristic_id:
    app_bt_evt_gatt_characteristic(&(evt->data.evt_gatt_characteristic));
    break;

  // -------------------------------
  // This event is generated for various procedure completions, e.g. when a
  // write procedure is completed, or service discovery is completed
  case sl_bt_evt_gatt_procedure_completed_id:
    app_bt_evt_gatt_procedure_completed(&(evt->data.evt_gatt_procedure_completed));
    break;

  case sl_bt_evt_gatt_characteristic_value_id:
    app_bt_evt_gatt_characteristic_value(&(evt->data.evt_gatt_characteristic_value));
    break;

  // -------------------------------
  // This event indicates that a connection was closed.
  case sl_bt_evt_connection_closed_id:
    app_bt_evt_connection_closed(&(evt->data.evt_connection_closed));
    break;

  case sl_bt_evt_system_external_signal_id:
    app_bt_evt_system_external_signal(&(evt->data.evt_system_external_signal));
    break;

  // -------------------------------
  // Default event handler.
  default:
    break;
  }
}

// -----------------------------------------------------------------------------
// Common local function definitions

// Init connection properties
static void init_properties(void)
{
  uint8_t i;

  conn_properties.connection_handle = CONNECTION_HANDLE_INVALID;
  conn_properties.es_service_handle = SERVICE_HANDLE_INVALID;
  for (i = 0; i < CHARACTERISTIC_COUNT; i++) {
    conn_properties.characteristic_handle[i] = CHARACTERISTIC_HANDLE_INVALID;
  }
  conn_properties.temperature = TEMP_INVALID;
  conn_properties.humidity = HUMID_INVALID;
  conn_properties.rssi = RSSI_INVALID;
}

static uint8_t find_device_name_pattern_in_advertisement(const uint8_t *data,
                                                         uint8_t data_length,
                                                         const char *name,
                                                         uint8_t name_length)
{
  uint8_t ad_field_length;
  uint8_t ad_field_type;
  uint8_t i = 0;
  // Parse advertisement packet
  while (i < data_length) {
    ad_field_length = data[i];
    ad_field_type = data[i + 1];
    // Complete ($02) local name.
    if (ad_field_type == 0x09) {
      // check local name is match
      if ((ad_field_length >= name_length)
          && (strncmp((char *)&data[i + 2], name, name_length) == 0)) {
        return 1;
      }
    }
    // advance to the next AD struct
    i = i + ad_field_length + 1;
  }
  return 0;
}

// Add a new connection to the connection_properties array
static void add_connection(uint8_t connection, uint16_t address)
{
  conn_properties.connection_handle = connection;
  conn_properties.server_address = address;
}

// Remove a connection from the connection_properties array
static void remove_connection(void)
{
  uint8_t i;

  conn_properties.connection_handle = CONNECTION_HANDLE_INVALID;
  conn_properties.es_service_handle = SERVICE_HANDLE_INVALID;
  for (i = 0; i < CHARACTERISTIC_COUNT; i++) {
    conn_properties.characteristic_handle[i] = CHARACTERISTIC_HANDLE_INVALID;
  }
  conn_properties.temperature = TEMP_INVALID;
  conn_properties.humidity = HUMID_INVALID;
  conn_properties.rssi = RSSI_INVALID;
}

static uint8_t bt_gatt_read_characteristic_value_from_offset(uint16_t offset,
                                                             uint16_t maxlen)
{
  sl_status_t sc = SL_STATUS_OK;
  uint8_t read_count = 0;

  if (conn_properties.connection_handle != CONNECTION_HANDLE_INVALID) {
    if (characteristic_read.index < characteristic_index_max) {
      uint16_t characteristic_handle;

      characteristic_handle = conn_properties.characteristic_handle[characteristic_read.index];
      if (characteristic_handle != CHARACTERISTIC_HANDLE_INVALID) {
        sc = sl_bt_gatt_read_characteristic_value_from_offset(
            conn_properties.connection_handle,
            characteristic_handle,
            offset,
            maxlen);
        app_assert_status(sc);
      }
    }
    read_count++;
  }
  return read_count;
}

static void read_first_characteristic_value(void)
{
  characteristic_read.index = characteristic_index_temperature;
  characteristic_read.count = 0;
  bt_gatt_read_characteristic_value_from_offset(0, 2);
}

static void read_next_characteristic_value(void)
{
  characteristic_read.index++;
  characteristic_read.count++;
  if (characteristic_read.index < characteristic_index_max) {
    bt_gatt_read_characteristic_value_from_offset(0, 2);
  } else {
    // Finish reading all characteristic
    characteristic_read.index = characteristic_index_temperature;
  }
}

static void start_scanner_timeout_timer(void)
{
  sl_status_t sc;
  bool running;

  sc = sl_sleeptimer_is_timer_running(&scanner_timeout_timer, &running);
  app_assert_status(sc);

  if (!running) {
    sc = sl_sleeptimer_start_timer_ms(&scanner_timeout_timer,
                                      SCANNER_TIMEOUT_MS,
                                      scanner_timeout_callback,
                                      NULL,
                                      0,
                                      0);
    app_assert_status(sc);
  }
  app_assert_status(sc);
}

static void stop_scanner_timeout_timer(void)
{
  sl_status_t sc;
  bool running;

  sc = sl_sleeptimer_is_timer_running(&scanner_timeout_timer, &running);
  app_assert_status(sc);

  if (running) {
    sc = sl_sleeptimer_stop_timer(&scanner_timeout_timer);
    app_assert_status(sc);
  }
}

static void start_connection_timeout_timer(void)
{
  sl_status_t sc;
  bool running;

  sc = sl_sleeptimer_is_timer_running(&connection_timeout_timer, &running);
  app_assert_status(sc);

  if (!running) {
    sc = sl_sleeptimer_start_timer_ms(&connection_timeout_timer,
                                      CONNECTION_TIMEOUT_MS,
                                      connection_timeout_callback,
                                      NULL,
                                      0,
                                      0);
    app_assert_status(sc);
  }
  app_assert_status(sc);
}

static void stop_connection_timeout_timer(void)
{
  sl_status_t sc;
  bool running;

  sc = sl_sleeptimer_is_timer_running(&connection_timeout_timer, &running);
  app_assert_status(sc);

  if (running) {
    sc = sl_sleeptimer_stop_timer(&connection_timeout_timer);
    app_assert_status(sc);
  }
}

static void start_periodic_timer(void)
{
  sl_status_t sc;
  bool running;

  sc = sl_sleeptimer_is_timer_running(&data_collector_timer, &running);
  app_assert_status(sc);

  if (!running) {
    sc = sl_sleeptimer_start_periodic_timer_ms(&data_collector_timer,
                                               DATA_COLLECTER_INTERVAL_MS,
                                               data_collecter_callback,
                                               NULL,
                                               0,
                                               0);
    app_assert_status(sc);
  }
}

static void stop_periodic_timer(void)
{
  sl_status_t sc;
  bool running;

  sc = sl_sleeptimer_is_timer_running(&data_collector_timer, &running);
  app_assert_status(sc);

  if (running) {
    sc = sl_sleeptimer_stop_timer(&data_collector_timer);
    app_assert_status(sc);
  }
}

static void start_discovery(void)
{
  sl_status_t sc;

  sc = sl_bt_scanner_stop();
  app_assert_status(sc);

  // Start scanning - looking for Thunderboard sense 2 devices
  sc = sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
  app_assert_status_f(sc,
                      "Failed to start discovery #1\n");
  memset(remote_sensor.address.addr, 0, 6);
  remote_sensor.rssi = -127;
  start_scanner_timeout_timer();
  conn_state = scanning;
  app_log_info("Start scan for new device\r\n");
}

static void start_connect(void)
{
  sl_status_t sc;

  // Stop scanning for a while
  sc = sl_bt_scanner_stop();
  app_assert_status(sc);
  // and connect to that device
  sc = sl_bt_connection_open(remote_sensor.address,
                             remote_sensor.address_type,
                             sl_bt_gap_1m_phy,
                             NULL);
  app_assert_status(sc);
  conn_state = opening;
}

static void app_bt_system_boot(const sl_bt_evt_system_boot_t *data)
{
  sl_status_t sc;

  // Print boot message.
  app_log_info("Bluetooth stack booted: v%d.%d.%d-b%d\r\n",
               data->major,
               data->minor,
               data->patch,
               data->build);
  // Print bluetooth address.
//  print_bluetooth_address();
  // Set passive scanning on 1Mb PHY
  sc = sl_bt_scanner_set_mode(sl_bt_gap_1m_phy, SCAN_PASSIVE);
  app_assert_status(sc);
  // Set scan interval and scan window
  sc = sl_bt_scanner_set_timing(sl_bt_gap_1m_phy, SCAN_INTERVAL, SCAN_WINDOW);
  app_assert_status(sc);
  // Set the default connection parameters for subsequent connections
  sc = sl_bt_connection_set_default_parameters(CONN_INTERVAL_MIN,
                                               CONN_INTERVAL_MAX,
                                               CONN_RESPONDER_LATENCY,
                                               CONN_TIMEOUT,
                                               CONN_MIN_CE_LENGTH,
                                               CONN_MAX_CE_LENGTH);
  app_assert_status(sc);
  start_discovery();
}

static void app_bt_evt_scanner_scan_report(const sl_bt_evt_scanner_scan_report_t *scan_report)
{
  // Parse advertisement packets
  if (scan_report->packet_type == 0) {
    // If a thunderboard advertisement is found...
    if (find_device_name_pattern_in_advertisement(scan_report->data.data,
                                                  scan_report->data.len,
                                                  THUNDERBOARD_NAME_CHECK,
                                                  THUNDERBOARD_NAME_CHECK_LENGTH)) {
      app_log_info("New device found: %02x:%02x:%02x:%02x:%02x:%02x, rssi: %d\r\n",
                   scan_report->address.addr[0],
                   scan_report->address.addr[1],
                   scan_report->address.addr[2],
                   scan_report->address.addr[3],
                   scan_report->address.addr[4],
                   scan_report->address.addr[5],
                   (int)scan_report->rssi);
      if (remote_sensor.rssi < scan_report->rssi) {
        remote_sensor.rssi = scan_report->rssi;
        remote_sensor.address = scan_report->address;
        remote_sensor.address_type = scan_report->address_type;
      }
    }
  }
}

static void app_bt_evt_connection_opened(const sl_bt_evt_connection_opened_t *connection_opened)
{
  sl_status_t sc;
  uint16_t addr_value;

  // Start connection timeout timer
  start_connection_timeout_timer();

  // Get last two bytes of sender address
  addr_value = (uint16_t)(connection_opened->address.addr[1] << 8) + connection_opened->address.addr[0];
  // Add connection to the connection_properties array
  add_connection(connection_opened->connection, addr_value);
  // Discover Health Thermometer service on the responder device
  sc = sl_bt_gatt_discover_primary_services_by_uuid(connection_opened->connection,
                                                    sizeof(environment_sensing_service),
                                                    (const uint8_t *)environment_sensing_service);
  app_assert_status(sc);

  conn_state = discover_services;
  app_log_info("Connection opened: 0x%x\r\n", connection_opened->connection);
}

static void app_bt_evt_gatt_service(const sl_bt_evt_gatt_service_t *evt_data)
{
  // Save service handle for future reference
  conn_properties.es_service_handle = evt_data->service;
}

static void app_bt_evt_gatt_procedure_completed(const sl_bt_evt_gatt_procedure_completed_t *evt_data)
{
  sl_status_t sc;

  // If service discovery finished
  if ((conn_state == discover_services)
      && (conn_properties.es_service_handle != SERVICE_HANDLE_INVALID)) {
    // Discover Thunderboard sense 2 characteristic on the responder device
    sc = sl_bt_gatt_discover_characteristics(evt_data->connection,
                                             conn_properties.es_service_handle);
    app_assert_status(sc);
    conn_state = discover_characteristics;
    return;
  }
}

static void app_bt_evt_gatt_characteristic(const sl_bt_evt_gatt_characteristic_t *evt_data)
{
  bool characteristic_found = false;

  if ((evt_data->uuid.len == sizeof(es_temperature_char))
      && (0 == memcmp(es_temperature_char, evt_data->uuid.data, evt_data->uuid.len))) {
    // Save characteristic handle for future reference
    conn_properties.characteristic_handle[characteristic_index_temperature] = evt_data->characteristic;
    characteristic_found = true;
    app_log_info("Found humidity characteristic: 0x%x\r\n", evt_data->characteristic);
  }

  if ((evt_data->uuid.len == sizeof(es_humidity_char))
      && (0 == memcmp(es_humidity_char, evt_data->uuid.data, evt_data->uuid.len))) {
    // Save characteristic handle for future reference
    conn_properties.characteristic_handle[characteristic_index_humidity] = evt_data->characteristic;
    characteristic_found = true;
    app_log_info("Found temperature characteristic: 0x%x\r\n", evt_data->characteristic);
  }

  if (characteristic_found) {
    // Stop connection timeout timer
    stop_connection_timeout_timer();

    // Start data collector timer
    start_periodic_timer();

    characteristic_read.index = characteristic_index_temperature;
    characteristic_read.count = 0;
    conn_state = running;
    led0_on();
  }
}

static void app_bt_evt_gatt_characteristic_value(const sl_bt_evt_gatt_characteristic_value_t *evt_data)
{
  uint16_t char_value;

  char_value = (uint16_t)(evt_data->value.data[1] << 8) + evt_data->value.data[0];

  switch (characteristic_read.index) {
  case characteristic_index_temperature:
    conn_properties.temperature = (float)(char_value) / 100.0f;
    app_log_info("Temperature raw value: %d\r\n", char_value);
    break;

  case characteristic_index_humidity:
    conn_properties.humidity = (float)(char_value) / 100.0f;
    app_log_info("Humidity raw value: %d\r\n", char_value);
    break;

  default:
    app_log_info("Invalid characteristic index: %d\r\n", characteristic_read.index);
    return;
  }
  read_next_characteristic_value();
  if (characteristic_read.count == CHARACTERISTIC_COUNT) {
    dweet_http_client_update_rht("thunderboard-be-gateway",
                                 conn_properties.humidity,
                                 conn_properties.temperature);
  }
}

static void app_bt_evt_connection_closed(const sl_bt_evt_connection_closed_t *evt_data)
{
  // remove connection from active connections
  remove_connection();
  if (conn_state != scanning) {
    // start scanning again to find new devices
    start_discovery();
  }
  stop_periodic_timer();
  led0_off();
  app_log_info("Connection closed: 0x%x\r\n", evt_data->connection);
}

static void app_bt_evt_system_external_signal(const sl_bt_evt_system_external_signal_t *evt_data)
{
  sl_status_t sc;

  if (evt_data->extsignals & SCANNER_TIMEOUT_EVENT) {
    bd_addr address_zero = {{0, 0, 0, 0, 0, 0}};

    stop_scanner_timeout_timer();
    if (memcmp(&(remote_sensor.address),
               &address_zero,
               sizeof(bd_addr))) {
      // Connect to the thunderboard sense
      start_connect();
    } else {
      // Restart discovery process
      start_discovery();
    }
  }

  if (evt_data->extsignals & CONNECTION_CLOSE_REQUEST_EVENT) {
    if (conn_properties.connection_handle != CONNECTION_HANDLE_INVALID) {
      sc = sl_bt_connection_close(conn_properties.connection_handle);
      app_log_info("Connection timeout: 0x%x\r\n", conn_properties.connection_handle);
      app_assert_status(sc);
    }
    conn_state = stopping;
  }

  if (evt_data->extsignals & DATA_COLLECT_EVENT) {
    read_first_characteristic_value();
  }
}

/***************************************************************************//**
 * Callback on connection timeout timer period.
 ******************************************************************************/
static void scanner_timeout_callback(sl_sleeptimer_timer_handle_t *timer,
                                    void *data)
{
  (void) timer;
  (void) data;

  sl_bt_external_signal(SCANNER_TIMEOUT_EVENT);
}

/***************************************************************************//**
 * Callback on connection timeout timer period.
 ******************************************************************************/
static void connection_timeout_callback(sl_sleeptimer_timer_handle_t *timer,
                                    void *data)
{
  (void) timer;
  (void) data;

  sl_bt_external_signal(CONNECTION_CLOSE_REQUEST_EVENT);
}

/***************************************************************************//**
 * Callback on data collector timer period.
 ******************************************************************************/
static void data_collecter_callback(sl_sleeptimer_timer_handle_t *timer,
                                    void *data)
{
  (void) timer;
  (void) data;

  sl_bt_external_signal(DATA_COLLECT_EVENT);
}
