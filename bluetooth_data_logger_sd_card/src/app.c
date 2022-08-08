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
#include "em_common.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app_assert.h"

#include "sl_simple_button_instances.h"
#include "sl_sdc_platform_spi_config.h"
#include "diskio.h"
#include "ff.h"

#include "sl_sensor_rht.h"
#include "logger_sd_card.h"

#include "app_display.h"
#include "app_log.h"
#include "app.h"

// -----------------------------------------------------------------------------
// Local Macros and Definitions

#if defined(SL_CATALOG_SIMPLE_LED_PRESENT) && defined(SL_CATALOG_LED0_PRESENT)
#include "sl_simple_led_instances.h"
#define led0_on()                                   sl_led_turn_on(&sl_led_led0);
#define led0_off()                                  sl_led_turn_off(&sl_led_led0);
#else
#define led0_on()
#define led0_off()
#endif

#define abs(n)                                      ((n) < 0 ? -(n) : (n))
#define UINT16_TO_BYTES(x)                          { (uint8_t)(x), \
                                                      (uint8_t)((x) >> 8) }

#define DATA_LOGGER_INTERVAL_MS                     (10000)

#define DATA_LOGGER_EVENT                           (1 << 0)
#define BUTTON_EVENT                                (1 << 1)

// Main states
#define DISCONNECTED                                0
#define SCANNING                                    1
#define FIND_SERVICE                                2
#define FIND_CHAR                                   3
#define ENABLE_NOTIF                                4
#define DATA_MODE                                   5
#define DISCONNECTING                               6

#define STATE_ADVERTISING                           1
#define STATE_CONNECTED                             2
#define STATE_SPP_MODE                              3

#define LOG_FILE                                    "log.txt"

// Advertising flags (common)
#define ADVERTISE_FLAGS_LENGTH                      2
#define ADVERTISE_FLAGS_TYPE                        0x01

// Bit mask for flags advertising data type.
#define ADVERTISE_FLAGS_LE_LIMITED_DISCOVERABLE     0x01
#define ADVERTISE_FLAGS_LE_GENERAL_DISCOVERABLE     0x02
#define ADVERTISE_FLAGS_BR_EDR_NOT_SUPPORTED        0x04

// Scan Response
#define ADVERTISE_MANDATORY_DATA_LENGTH             5
#define ADVERTISE_MANDATORY_DATA_TYPE_MANUFACTURER  0xFF

#define ADVERTISE_COMPANY_ID                        0x0047 /* Silicon Labs */
#define ADVERTISE_FIRMWARE_ID                       0x0000

// Complete local name.
#define ADVERTISE_TYPE_LOCAL_NAME                   0x09
#define ADVERTISE_DEVICE_NAME_LEN                   11
#define ADVERTISE_DEVICE_NAME                       "Data Logger"

/*
 * Maximum number of iterations when polling UART RX data before sending data
 * over BLE connection
 * set value to 0 to disable optimization -> minimum latency but may decrease
 * throughput
 */
#define UART_POLL_TIMEOUT                           5000

// -----------------------------------------------------------------------------
// Structure that holds Scan Response data

typedef struct {
  uint8_t flags_length;          /**< Length of the Flags field. */
  uint8_t flags_type;            /**< Type of the Flags field. */
  uint8_t flags;                 /**< Flags field. */
  uint8_t mandatory_data_length; /**< Length of the mandata field. */
  uint8_t mandatory_data_type;   /**< Type of the mandata field. */
  uint8_t company_id[2];         /**< Company ID. */
  uint8_t firmware_id[2];        /**< Firmware ID */
  uint8_t local_name_length;     /**< Length of the local name field. */
  uint8_t local_name_type;       /**< Type of the local name field. */
  uint8_t local_name[ADVERTISE_DEVICE_NAME_LEN]; /**< Local name field. */
} advertise_scan_response_t;

#define ADVERTISE_SCAN_RESPONSE_DEFAULT                                \
  {                                                                    \
    .flags_length = ADVERTISE_FLAGS_LENGTH,                            \
    .flags_type = ADVERTISE_FLAGS_TYPE,                                \
    .flags = ADVERTISE_FLAGS_LE_GENERAL_DISCOVERABLE                   \
             | ADVERTISE_FLAGS_BR_EDR_NOT_SUPPORTED,                   \
    .mandatory_data_length = ADVERTISE_MANDATORY_DATA_LENGTH,          \
    .mandatory_data_type = ADVERTISE_MANDATORY_DATA_TYPE_MANUFACTURER, \
    .company_id = UINT16_TO_BYTES(ADVERTISE_COMPANY_ID),               \
    .firmware_id = UINT16_TO_BYTES(ADVERTISE_FIRMWARE_ID),             \
    .local_name_length = ADVERTISE_DEVICE_NAME_LEN + 1,                \
    .local_name_type = ADVERTISE_TYPE_LOCAL_NAME,                      \
    .local_name = ADVERTISE_DEVICE_NAME                                \
  }

// Bookkeeping struct for storing amount of received/sent data
typedef struct
{
  uint32_t num_pack_sent;
  uint32_t num_bytes_sent;
  uint32_t num_writes; /* Total number of send attempts */
} ts_counters_t;

typedef struct {
  uint8_t conn_handle;
  uint8_t main_state;
  bool data_logger_enable;
  sl_sleeptimer_timer_handle_t data_logger_timer;
} app_properties_t;

// -----------------------------------------------------------------------------
// Local Variables

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static const advertise_scan_response_t adv_scan_response =
  ADVERTISE_SCAN_RESPONSE_DEFAULT;

static app_properties_t app_properties;

static ts_counters_t s_counters;

/*
 * Default maximum packet size is 20 bytes. This is adjusted after connection is
 * opened based
 * on the connection parameters
 */
static uint8_t max_packet_size = 20;
static uint8_t min_packet_size = 20;  // Target minimum bytes for one packet

// -----------------------------------------------------------------------------
// Common local function declarations

static sl_status_t send_notification(uint8_t connection,
                                     uint16_t characteristic,
                                     size_t value_len,
                                     const uint8_t *value);
static void print_stats(ts_counters_t *ps_counters);
static void reset_variables();
static void data_logger_callback(sl_sleeptimer_timer_handle_t *timer,
                                 void *data);
static void send_log_data_to_spp(void);
static void send_log_data_to_spp(void);
static void get_2_of_3_decimal_digit(uint32_t decimal,
                                     uint8_t decimal_digit[static 2]);
static void sensor_rht_process_log_data(void);
static bool get_data_logger_enable_config(void);

static void app_bt_system_boot(void);
static void app_bt_connection_opened(
  const sl_bt_evt_connection_opened_t *evt_data);
static void app_bt_gatt_mtu_exchanged(
  const sl_bt_evt_gatt_mtu_exchanged_t *evt_data);
static void app_bt_connection_closed(void);
static void app_bt_gatt_server_characteristic_status(
  const sl_bt_evt_gatt_server_characteristic_status_t *evt_data);
static void app_bt_evt_system_external_signal(
  const sl_bt_evt_system_external_signal_t *evt_data);

/***************************************************************************//**
 *    Application Init code
 ******************************************************************************/
void app_init(void)
{
#if !FF_FS_NORTC && !FF_FS_READONLY
  sl_sleeptimer_date_t date = {
    .year = 122,
    .month = 7,
    .month_day = 1,
    .hour = 10,
    .min = 34,
    .sec = 0,
  };
  // Initialize default date time
  sl_sleeptimer_set_datetime(&date);

  DWORD time_data = get_fattime();
  app_log("\nCurrent time is %u/%u/%u %2u:%02u:%02u.\r\n",
          (time_data >> 25) + 1980,
          (time_data >> 21) & 0x0f,
          (time_data >> 16) & 0x1f,
          (time_data >> 11) & 0x1f,
          (time_data >> 5) & 0x3f,
          (time_data << 1) & 0x1f);
#endif

  app_properties.data_logger_enable = get_data_logger_enable_config();
  if (app_properties.data_logger_enable) {
    led0_on();
  } else {
    led0_off();
  }

  // Initialize OLED display
  app_display_init();

  // Initialize logger SD card
  logger_sd_card_init();

  // Initialize si7021 sensor
  sl_sensor_rht_init();

  // Process 1st log data
  sensor_rht_process_log_data();

  // Create sensor sampling & oled display periodic timer
  sl_sleeptimer_start_periodic_timer_ms(&app_properties.data_logger_timer,
                                        DATA_LOGGER_INTERVAL_MS,
                                        data_logger_callback,
                                        NULL,
                                        0,
                                        0);
}

/***************************************************************************//**
 * Application Process Action.
 ******************************************************************************/
SL_WEAK void app_process_action(void)
{
}

/***************************************************************************//**
 * Bluetooth stack event handler for SPP Server mode
 *
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 ******************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  switch (SL_BT_MSG_ID(evt->header)) {
  case sl_bt_evt_system_boot_id:
    app_bt_system_boot();
    break;

  case sl_bt_evt_connection_opened_id:
    app_bt_connection_opened(&(evt->data.evt_connection_opened));
    break;

  case sl_bt_evt_connection_parameters_id:
    app_log("Conn.parameters: interval %u units, txsize %u\r\n",
            evt->data.evt_connection_parameters.interval,
            evt->data.evt_connection_parameters.txsize);
    break;

  case sl_bt_evt_gatt_mtu_exchanged_id:
    app_bt_gatt_mtu_exchanged(&(evt->data.evt_gatt_mtu_exchanged));
    break;

  case sl_bt_evt_connection_closed_id:
    app_bt_connection_closed();
    break;

  case sl_bt_evt_gatt_server_characteristic_status_id:
    app_bt_gatt_server_characteristic_status(
      &(evt->data.evt_gatt_server_characteristic_status));
    break;

  case sl_bt_evt_system_external_signal_id:
    app_bt_evt_system_external_signal(&(evt->data.evt_system_external_signal));
    break;

  // Default event handler.
  default:
    break;
  }
}

// -----------------------------------------------------------------------------
// Common local function definitions
static void print_stats(ts_counters_t *ps_counters)
{
  app_log("Outgoing data:\r\n");
  app_log(" bytes/packets sent: %lu / %lu ",
          ps_counters->num_bytes_sent,
          ps_counters->num_pack_sent);
  app_log(", num writes: %lu\r\n", ps_counters->num_writes);
  app_log("(RX buffer overflow is not tracked)\r\n");
  return;
}

static sl_status_t send_notification(uint8_t connection,
                                     uint16_t characteristic,
                                     size_t value_len,
                                     const uint8_t *value)
{
  sl_status_t sc = SL_STATUS_OK;
  size_t send_len;

  while (value_len) {
    if (value_len > max_packet_size) {
      send_len = max_packet_size;
      value_len -= max_packet_size;
    } else {
      send_len = value_len;
      value_len = 0;
    }
    do {
      sc = sl_bt_gatt_server_send_notification(connection,
                                               characteristic,
                                               send_len,
                                               value);
      s_counters.num_writes++;
    } while (sc == SL_STATUS_NO_MORE_RESOURCE);
    if (SL_STATUS_OK == sc) {
      s_counters.num_pack_sent++;
      s_counters.num_bytes_sent += send_len;
    } else {
      app_log_error("Unexpected error: %x\r\n", sc);
      break;
    }
    value += send_len;
  }
  return sc;
}

static void reset_variables()
{
  app_properties.conn_handle = 0xFF;
  app_properties.main_state = STATE_ADVERTISING;
  max_packet_size = 20;

  memset(&s_counters, 0, sizeof(s_counters));
}

static void send_log_data_to_spp_callback(const char *line, int length)
{
  sl_status_t sc;

  if (STATE_SPP_MODE == app_properties.main_state) {
    sc = send_notification(app_properties.conn_handle,
                           gattdb_spp_data,
                           length,
                           (uint8_t *)line);
    app_assert_status(sc);
  }
}

static void send_log_data_to_spp(void)
{
  sl_status_t sc;

  sc = logger_sd_card_readline(LOG_FILE,
                               send_log_data_to_spp_callback);
  if (SL_STATUS_OK == sc) {
    app_log("All log from sd card is sent, file: '%s'\r\n", LOG_FILE);
  }
  logger_sd_card_clear_log(LOG_FILE);
}

/***************************************************************************//**
 * @brief
 *    This function take 2 of 3 digit from decimal part
 * @param[in] decimal
 *    The decimal part of the float number,
 *    It is an unsigned integer which equal to
 *    the decimal part of the float number multiplied by 1000
 * @param[out] decimal_digit
 *    The 2 digit of the decimal part, we only take 2 digit of the decimal part
 ******************************************************************************/
static void get_2_of_3_decimal_digit(uint32_t decimal,
                                     uint8_t decimal_digit[2])
{
  decimal_digit[0] = decimal / 100;
  decimal %= 100;
  decimal_digit[1] = decimal / 10;
  if ((decimal % 10) >= 5) {
    if (decimal_digit[1] < 9) {
      decimal_digit[1] += 1;
    } else {
      if (decimal_digit[0] < 9) {
        decimal_digit[1] = 0;
        decimal_digit[0] += 1;
      }
    }
  }
}

static void sensor_rht_process_log_data(void)
{
  sl_status_t sc;
  uint32_t rh;
  uint8_t rh_decimal[2];
  int32_t t;
  uint8_t t_decimal[2];
  const char *line;
  int length;
  static sl_sleeptimer_date_t date;

  if (sl_sleeptimer_get_datetime(&date) != SL_STATUS_OK) {
    return;
  }
  sc = sl_sensor_rht_get(&rh, &t);
  if (SL_STATUS_NOT_INITIALIZED == sc) {
    app_log_info(
      "Relative Humidity and Temperature sensor is not initialized.\r\n");
    return;
  } else if (SL_STATUS_OK != sc) {
    app_log_status_error_f(sc, "RHT sensor measurement failed.\r\n");
    return;
  }

  // we only take 2 digit from decimal part
  get_2_of_3_decimal_digit(rh % 1000,
                           rh_decimal);
  get_2_of_3_decimal_digit((uint32_t)abs(t) % 1000,
                           t_decimal);
  rh /= 1000;
  t /= 1000;
  app_log_info("%04d/%02d/%02d %02d:%02d:%02d "
               "Humidity = %d.%d%d %%RH, Temperature = %d.%d%d C\r\n",
               date.year + 1900,
               date.month,
               date.month_day,
               date.hour,
               date.min,
               date.sec,
               rh,
               rh_decimal[0],
               rh_decimal[1],
               t,
               t_decimal[0],
               t_decimal[1]);

  app_display_show_sensor_rht_data(rh,
                                   rh_decimal,
                                   t,
                                   t_decimal);
  logger_sd_card_create_log_entry("%04d/%02d/%02d %02d:%02d:%02d "
                                  "Humidity = %d.%d%d %%RH, Temperature = %d.%d%d C",
                                  date.year + 1900,
                                  date.month,
                                  date.month_day,
                                  date.hour,
                                  date.min,
                                  date.sec,
                                  rh,
                                  rh_decimal[0],
                                  rh_decimal[1],
                                  t,
                                  t_decimal[0],
                                  t_decimal[1]);
  if (STATE_SPP_MODE == app_properties.main_state) {
    // Send log that is queued to sd card
    send_log_data_to_spp();

    // Send current log entry
    sc = logger_sd_card_get_current_log_entry(&line, &length);
    if (SL_STATUS_OK == sc) {
      sc = send_notification(app_properties.conn_handle,
                             gattdb_spp_data,
                             length,
                             (uint8_t *)line);
      app_assert_status(sc);
    }
  } else {
    if (app_properties.data_logger_enable) {
      sc = logger_sd_card_append_current_log_entry(LOG_FILE);
      if (SL_STATUS_OK == sc) {
        app_log_info("Append log entry to file: '%s' success\r\n", LOG_FILE);
      } else {
        app_log_info("Append log entry to file: '%s' failed\r\n", LOG_FILE);
      }
    }
  }
}

static bool get_data_logger_enable_config(void)
{
  uint8_t value;
  size_t value_length;
  sl_status_t sc;

  sc = sl_bt_nvm_load(0x4001,
                      1,
                      &value_length,
                      &value);
  if ((sc == SL_STATUS_OK) && (value_length == 1)) {
    return value ? true : false;
  }
  // Data logger default config is: enable
  return true;
}

static sl_status_t set_data_logger_enable_config(uint8_t value)
{
  return sl_bt_nvm_save(0x4001,
                        1,
                        &value);
}

static void app_bt_system_boot(void)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];
  uint16_t max_mtu_out;

  app_log("SPP Role: SPP Server\r\n");
  reset_variables();
  sc = sl_bt_gatt_server_set_max_mtu(247, &max_mtu_out);
  app_assert_status(sc);

  // Extract unique ID from BT Address.
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

  sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id,
                                               0,
                                               sizeof(system_id),
                                               system_id);
  app_assert_status(sc);

  // Create an advertising set.
  sc = sl_bt_advertiser_create_set(&advertising_set_handle);
  app_assert_status(sc);

  // Set advertising interval to 100ms.
  sc = sl_bt_advertiser_set_timing(
    advertising_set_handle,
    160, // min. adv. interval (milliseconds * 1.6)
    160, // max. adv. interval (milliseconds * 1.6)
    0,   // adv. duration
    0);  // max. num. adv. events
  app_assert_status(sc);

  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                        sl_bt_advertiser_scan_response_packet,
                                        sizeof(adv_scan_response),
                                        (uint8_t *)&adv_scan_response);
  app_assert_status(sc);

  // Start advertising and enable connections
  sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                     sl_bt_advertiser_connectable_scannable);
  app_assert_status(sc);
}

static void app_bt_connection_opened
(
  const sl_bt_evt_connection_opened_t *evt_data
)
{
  app_properties.conn_handle = evt_data->connection;
  app_log("Connection opened\r\n");
  app_properties.main_state = STATE_CONNECTED;

  /*
   * Request connection parameter update.
   * conn.interval min 20ms, max 40ms, slave latency 4 intervals,
   * supervision timeout 2 seconds
   * (These should be compliant with Apple Bluetooth Accessory Design
   * Guidelines, both R7 and R8)
   */
  sl_bt_connection_set_parameters(app_properties.conn_handle,
                                  24,
                                  40,
                                  0,
                                  200,
                                  0,
                                  0xFFFF);
}

static void app_bt_gatt_mtu_exchanged
(
  const sl_bt_evt_gatt_mtu_exchanged_t *evt_data
)
{
  /*
   * Calculate maximum data per one notification / write-without-response,
   * this depends on the MTU. up to ATT_MTU-3 bytes can be sent at once
   */
  max_packet_size = evt_data->mtu - 3;

  // Try to send maximum length packets whenever possible
  min_packet_size = max_packet_size;
  app_log_info("MTU exchanged: %d\r\n", evt_data->mtu);
}

static void app_bt_connection_closed(void)
{
  sl_status_t sc;

  print_stats(&s_counters);
  if (STATE_SPP_MODE == app_properties.main_state) {
    sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
  }
  reset_variables();
  // Start advertising and enable connections
  sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                     sl_bt_advertiser_connectable_scannable);
  app_assert_status(sc);
}

static void app_bt_gatt_server_characteristic_status
(
  const sl_bt_evt_gatt_server_characteristic_status_t *evt_data
)
{
  if (evt_data->characteristic == gattdb_spp_data) {
    if (evt_data->status_flags == gatt_server_client_config) {
      // Characteristic client configuration (CCC) for spp_data has been changed
      if (evt_data->client_config_flags == gatt_notification) {
        app_properties.main_state = STATE_SPP_MODE;
        sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
        app_log("SPP Mode ON\r\n");
        send_log_data_to_spp();
      } else {
        app_log("SPP Mode OFF\r\n");
        app_properties.main_state = STATE_CONNECTED;
        sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
      }
    }
  }
}

static void app_bt_evt_system_external_signal
(
  const sl_bt_evt_system_external_signal_t *evt_data
)
{
  if (evt_data->extsignals & DATA_LOGGER_EVENT) {
    sensor_rht_process_log_data();
  }
  if (evt_data->extsignals & BUTTON_EVENT) {
    app_properties.data_logger_enable = !app_properties.data_logger_enable;
    if (app_properties.data_logger_enable) {
      led0_on();
      app_log_info("Data logger is enabled!\r\n");
    } else {
      led0_off();
      app_log_info("Data logger is disabled!\r\n");
    }
    set_data_logger_enable_config(app_properties.data_logger_enable);
  }
}

/***************************************************************************//**
 * Callback on timer period.
 ******************************************************************************/
static void data_logger_callback(sl_sleeptimer_timer_handle_t *timer,
                                 void *data)
{
  (void) timer;
  (void) data;
  sl_bt_external_signal(DATA_LOGGER_EVENT);
}

/***************************************************************************//**
 * Callback on button change.
 ******************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED) {
    if (&sl_button_btn0 == handle) {
      sl_bt_external_signal(BUTTON_EVENT);
    }
  }
}
