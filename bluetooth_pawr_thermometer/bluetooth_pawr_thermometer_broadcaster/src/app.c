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
#include "sl_bt_api.h"
#include "sl_sleeptimer.h"
#include "sl_simple_button_instances.h"

#include "app.h"
#include "app_log.h"
#include "app_assert.h"

#include "glib.h"
#include "dmd.h"
#include "stdio.h"

// Define the number of devices the application needs.
#define PAWR_DEVICES_MAX_SYNC                   3

// Value for no flag present for the advertiser
#define PERIODIC_ADVERTISER_FLAG_NONE           0
// Periodic advertisement interval for PAwR train.
// Value in units of 1.25 ms.
#define PAWR_PERIODIC_ADV_MIN_INTERVAL_DEFAULT  500
#define PAWR_PERIODIC_ADV_MAX_INTERVAL_DEFAULT  500
// Scalable up to 255 Sensors in 1 groups
#define PAWR_SUBEVENT_COUNT_DEFAULT             1
// 7.5 ms
#define PAWR_SUBEVENT_INTERVAL_DEFAULT          100
// Time between the advertising packet in a subevent and the first response
// slot. Value in units of 1.25 ms.
#define PAWR_RESPONSE_SLOT_DELAY_DEFAULT        6
// Time between response slots. Value in units of 0.125 ms.
// 0.75 ms enough for up to 75 bytes on 1M phy
// (including LL overhead plus T_IFS)
#define PAWR_RESPONSE_SLOT_SPACING_DEFAULT      6
// Number of subevent response slots
#define PAWR_RESPONSE_SLOT_COUNT_DEFAULT        6

// calculate timeout as follows:
// tmeout_value_ms = 6 * (pawr.adv_interval * 1.25f) [ms]
#define PAST_TIMEOUT                            (6                                            \
                                                 * ((                                         \
                                                      PAWR_PERIODIC_ADV_MAX_INTERVAL_DEFAULT) \
                                                    + (                                       \
                                                      PAWR_PERIODIC_ADV_MAX_INTERVAL_DEFAULT  \
                                                      >> 2)))
#define TABLE_INDEX_INVALID                     ((uint8_t)0xFFu)

// Ping opcode
#define PAWR_PING_OPCODE                        0x00
// Read data sensor opcode
#define PAWR_READ_DATA_SENSOR_OPCODE            0x01
// Control LED opcode
#define PAWR_CONTROL_LED_OPCODE                 0x02

typedef enum {
  scanning,
  opening,
  discover_services,
  discover_characteristics,
  write_characteristic,
  synchronized,
  unsynchronized
} connection_state_t;

typedef struct device_address_s {
  uint8_t group_id; // ranging from 0-127
  uint8_t device_id; // valid in the range of 1-255.
                     // 0 is the default (reset) value meaning no address
                     // assigned to the device
} device_address_t;

typedef struct device_data_s {
  device_address_t addr;
  uint8_t temp;
  uint8_t led_stt;
} device_data_t;

/// Array type
typedef struct pawr_array_s {
  uint8_t len;     ///< Number of bytes stored in @p data
  // byte 0: group id (sub event or 0xFF to broadcast)
  // byte 1: device id (response slot or 0xFF to broadcast)
  // byte 2: opcode (0x00 - ping, 0x01 - Read Data Sensor, 0x02 - Control LED)
  uint8_t data[3]; ///< Data bytes
} pawr_array_t;

// PAwR service UUID defined by user
// 74e2e878-e82c-4e07-b276-5d2affe4239f
const uint8_t pawr_service[16] =
{ 0x9f, 0x23, 0xe4, 0xff, 0x2a, 0x5d, 0x76, 0xb2,
  0x07, 0x4e, 0x2c, 0xe8, 0x78, 0xe8, 0xe2, 0x74 };
// DeviceID characteristic UUID defined by user
// 6a02d89c-80d5-4f4a-9162-4946120aab7c
const uint8_t device_addr_char[16] =
{ 0x7c, 0xab, 0x0a, 0x12, 0x46, 0x49, 0x62, 0x91,
  0x4a, 0x4f, 0xd5, 0x80, 0x9c, 0xd8, 0x02, 0x6a };

// The advertising set handle allocated from Bluetooth stack.
static uint8_t pawr_handle = 0xff;
static uint8_t conn_handle = 0xff;
static uint32_t pawr_service_handle = 0xff;
static uint16_t device_addr_char_handle = 0xff;
// State of the connection under establishment
static uint8_t running_state;
static device_address_t device_addr = {
  .group_id = 0,
  .device_id = 1,
};

pawr_array_t subevent_data;
// device info contain sub event and response slot
static device_data_t device_data[PAWR_DEVICES_MAX_SYNC];
static uint8_t device_count = 0;

static volatile uint8_t enable_refresh_display = false;
static GLIB_Context_t glibContext;
static int current_line = 0;
static char text_string[20];

sl_sleeptimer_timer_handle_t read_data_timer;
static void read_data_timeout(sl_sleeptimer_timer_handle_t *timer, void *data);

sl_sleeptimer_timer_handle_t connection_timer;
static void connection_timeout(sl_sleeptimer_timer_handle_t *timer, void *data);

static uint8_t find_service_in_adv(uint8_t *data, uint8_t len);
static void gatt_procedure_completed_handler(sl_bt_msg_t *evt);
static void refresh_display(void);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  uint32_t status;

  status = DMD_init(0);
  EFM_ASSERT(status == SL_STATUS_OK);

  status = GLIB_contextInit(&glibContext);
  EFM_ASSERT(status == SL_STATUS_OK);

  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;

  GLIB_setFont(&glibContext, (GLIB_Font_t *) &GLIB_FontNarrow6x8);
  GLIB_clear(&glibContext);

  GLIB_drawStringOnLine(&glibContext,
                        "PAwR Thermometer",
                        current_line++,
                        GLIB_ALIGN_LEFT,
                        5,
                        5,
                        true);
  DMD_updateDisplay();
  app_log("\r\n------  BLE - PAwR Thermometer "
          "Demo: Broadcaster Role-----\r\n");
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
  if (enable_refresh_display) {
    enable_refresh_display = false;
    refresh_display();
  }
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
      sc = sl_bt_sm_set_bondable_mode(1);
      app_assert_status(sc);
      sc = sl_bt_sm_configure(0, sl_bt_sm_io_capability_noinputnooutput);
      app_assert_status(sc);
      sc = sl_bt_sm_store_bonding_configuration(8, 2);
      app_assert_status(sc);
      sc = sl_bt_sm_delete_bondings();
      app_assert_status(sc);
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&pawr_handle);
      app_assert_status(sc);
      // Start pawr advertising
      sc = sl_bt_pawr_advertiser_start(pawr_handle,
                                       PAWR_PERIODIC_ADV_MIN_INTERVAL_DEFAULT,
                                       PAWR_PERIODIC_ADV_MAX_INTERVAL_DEFAULT,
                                       PERIODIC_ADVERTISER_FLAG_NONE,
                                       PAWR_SUBEVENT_COUNT_DEFAULT,
                                       PAWR_SUBEVENT_INTERVAL_DEFAULT,
                                       PAWR_RESPONSE_SLOT_DELAY_DEFAULT,
                                       PAWR_RESPONSE_SLOT_SPACING_DEFAULT,
                                       PAWR_RESPONSE_SLOT_COUNT_DEFAULT);
      app_assert_status(sc);

      sc = sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m,
                               sl_bt_scanner_discover_generic);
      app_assert_status(sc);
      running_state = scanning;

      // Start a timer to Read data sensor
      sc = sl_sleeptimer_start_periodic_timer_ms(&read_data_timer,
                                                 1000,
                                                 read_data_timeout,
                                                 (void *)NULL, 0, 0);
      app_assert_status(sc);
      break;

    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      // Parse advertisement packets
      if (evt->data.evt_scanner_legacy_advertisement_report.event_flags
          == (SL_BT_SCANNER_EVENT_FLAG_CONNECTABLE
              | SL_BT_SCANNER_EVENT_FLAG_SCANNABLE)) {
        // If a thermometer advertisement is found...
        if (find_service_in_adv(
              evt->data.evt_scanner_legacy_advertisement_report.data.data,
              evt->data.evt_scanner_legacy_advertisement_report.data.len)) {
          // then stop scanning for a while
          sc = sl_bt_scanner_stop();
          app_assert_status(sc);
          app_log("Found PAwR Observer device\r\n");
          // and connect to that device
          sc = sl_bt_connection_open(
            evt->data.evt_scanner_legacy_advertisement_report.address,
            evt->data.evt_scanner_legacy_advertisement_report.address_type,
            sl_bt_gap_1m_phy,
            NULL);
          app_assert_status(sc);
          running_state = opening;
        }
      }
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("connection opened\r\n");
      conn_handle = evt->data.evt_connection_opened.connection;
      sc =
        sl_bt_sm_increase_security(evt->data.evt_connection_opened.connection);
      app_assert_status(sc);
      break;

    case sl_bt_evt_sm_confirm_bonding_id:
      sl_bt_sm_bonding_confirm(evt->data.evt_sm_confirm_bonding.connection, 1);
      break;

    case sl_bt_evt_sm_bonded_id:
      app_log("Bonding done: %d\r\n", evt->data.evt_sm_bonded.bonding);
      // Discover PAwR service on the observer device
      sc = sl_bt_gatt_discover_primary_services_by_uuid(conn_handle,
                                                        sizeof(pawr_service),
                                                        (const uint8_t *)pawr_service);
      app_assert_status(sc);
      running_state = discover_services;
      break;

    case sl_bt_evt_sm_bonding_failed_id:
      app_log("Bonding failed, reason: %x\r\n",
              evt->data.evt_sm_bonding_failed.reason);
      // Try to close the connection
      sc = sl_bt_connection_close(conn_handle);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event is generated when a new service is discovered
    case sl_bt_evt_gatt_service_id:
      pawr_service_handle = evt->data.evt_gatt_service.service;
      break;

    // -------------------------------
    // This event is generated when a new characteristic is discovered
    case sl_bt_evt_gatt_characteristic_id:
      device_addr_char_handle
        = evt->data.evt_gatt_characteristic.characteristic;
      break;

    case sl_bt_evt_gatt_procedure_completed_id:
      gatt_procedure_completed_handler(evt);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("Connection closed, reason: %d \r\n",
              evt->data.evt_connection_closed.reason);
      conn_handle = 0xff;
      pawr_service_handle = 0xff;
      device_addr_char_handle = 0xff;
      (void)sl_sleeptimer_stop_timer(&connection_timer);

      if (running_state == synchronized) {
        device_addr.device_id++;
        device_count++;
      }
      // more devices can be connected
      if (device_count < PAWR_DEVICES_MAX_SYNC) {
        if (running_state != scanning) {
          // start scanning again to find new devices
          sc = sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m,
                                   sl_bt_scanner_discover_generic);
          app_assert_status(sc);
          running_state = scanning;
        }
      }
      break;

    case sl_bt_evt_pawr_advertiser_subevent_data_request_id:
    {
      // Check if command is already prepared
      if (subevent_data.len) {
        sc = sl_bt_pawr_advertiser_set_subevent_data(pawr_handle,
                                                     device_addr.group_id,
                                                     0,
                                                     device_addr.device_id,
                                                     subevent_data.len,
                                                     subevent_data.data);
        app_assert_status(sc);
        subevent_data.len = 0;
      }
    }
    break;

    case sl_bt_evt_pawr_advertiser_response_report_id:
      if (evt->data.evt_pawr_advertiser_response_report.data_status != 255) {
        uint8_t device_index =
          evt->data.evt_pawr_advertiser_response_report.response_slot - 1;

        device_data[device_index].addr.group_id =
          evt->data.evt_pawr_advertiser_response_report.subevent;
        device_data[device_index].addr.device_id =
          evt->data.evt_pawr_advertiser_response_report.response_slot;
        device_data[device_index].temp =
          evt->data.evt_pawr_advertiser_response_report.data.data[0];
        device_data[device_index].led_stt =
          evt->data.evt_pawr_advertiser_response_report.data.data[1];
        app_log(
          "Response received in group id: %d, device id: %d, status: %d\r\n",
          evt->data.evt_pawr_advertiser_response_report.subevent,
          evt->data.evt_pawr_advertiser_response_report.response_slot,
          evt->data.evt_pawr_advertiser_response_report.data_status);
        app_log("Temp = %d, led status = %d\n\r",
                evt->data.evt_pawr_advertiser_response_report.data.data[0],
                evt->data.evt_pawr_advertiser_response_report.data.data[1]);
      }
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

/***************************************************************************//**
 * Callback on button change.
 *
 * This function overrides a weak implementation defined in the simple_button
 * module. It is triggered when the user activates one of the buttons.
 *
 ******************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
    subevent_data.len = 3;
    subevent_data.data[0] = device_addr.group_id;
    subevent_data.data[1] = 0xff;
    subevent_data.data[2] = PAWR_CONTROL_LED_OPCODE;
  }
}

static uint8_t find_service_in_adv(uint8_t *data, uint8_t len)
{
  uint8_t ad_length;
  uint8_t ad_type;
  uint8_t i = 0;

  // Parse advertisement packet
  while (i < len) {
    ad_length = data[i];
    ad_type = data[i + 1];
    // Partial ($02) or complete ($03) list of 128-bit UUIDs
    if ((ad_type == 0x06) || (ad_type == 0x07)) {
      // compare UUID to service UUID
      if (memcmp(&data[i + 2], pawr_service, 16) == 0) {
        return 1;
      }
    }
    // advance to the next AD element
    i = i + ad_length + 1;
  }
  return 0;
}

void gatt_procedure_completed_handler(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (running_state) {
    case discover_services:
      sc = sl_bt_gatt_discover_characteristics_by_uuid(conn_handle,
                                                       pawr_service_handle,
                                                       16,
                                                       device_addr_char);
      app_assert_status(sc);
      running_state = discover_characteristics;
      break;

    case discover_characteristics:
    {
      uint8_t data[2] = { device_addr.group_id, device_addr.device_id };

      sc = sl_bt_gatt_write_characteristic_value(conn_handle,
                                                 device_addr_char_handle,
                                                 sizeof(data),
                                                 data);
      app_assert_status(sc);

      running_state = write_characteristic;
    }
    break;

    case write_characteristic:
      app_log("Observer Address is configured, result: %x\r\n",
              evt->data.evt_gatt_procedure_completed.result);
      if (evt->data.evt_gatt_procedure_completed.result == SL_STATUS_OK) {
        sc = sl_bt_advertiser_past_transfer(conn_handle,
                                            0,
                                            pawr_handle);
        app_assert_status(sc);
        // calculate timeout as follows:
        // tmeout_value_ms = 6 * (pawr.adv_interval * 1.25f) [ms]
        (void)sl_sleeptimer_stop_timer(&connection_timer);
        running_state = synchronized;
        // Start a timer to check timeout
        sc = sl_sleeptimer_start_timer_ms(&connection_timer,
                                          PAST_TIMEOUT,
                                          connection_timeout,
                                          0,
                                          0,
                                          false);
        app_assert_status(sc);
      } else {
        // Try to close the connection
        (void)sl_bt_connection_close(conn_handle);
        running_state = unsynchronized;
      }
      break;

    default:
      break;
  }
}

static void read_data_timeout(sl_sleeptimer_timer_handle_t *timer, void *data)
{
  (void) timer;
  (void) data;

  if (!subevent_data.len) {
    subevent_data.len = 3;
    subevent_data.data[0] = device_addr.group_id;
    subevent_data.data[1] = 0xff;
    subevent_data.data[2] = PAWR_READ_DATA_SENSOR_OPCODE;
  }

  enable_refresh_display = true;
}

static void connection_timeout(sl_sleeptimer_timer_handle_t *timer,
                               void *data)
{
  (void)timer;
  (void)data;

  // Try to close the connection
  (void)sl_bt_connection_close(conn_handle);
  running_state = unsynchronized;
}

static void refresh_display(void)
{
  GLIB_clear(&glibContext);
  current_line = 0;
  for (uint8_t i = 0; i < device_count; i++) {
    if (device_data[i].addr.device_id) {
      snprintf(text_string, 20, "GroupID: %d", device_data[i].addr.group_id);
      GLIB_drawStringOnLine(&glibContext,
                            text_string,
                            current_line++,
                            GLIB_ALIGN_LEFT,
                            5,
                            5,
                            true);
      snprintf(text_string, 20, "DeviceID: %d", i + 1);

      /* Draw text on the memory lcd display*/
      GLIB_drawStringOnLine(&glibContext,
                            text_string,
                            current_line++,
                            GLIB_ALIGN_LEFT,
                            5,
                            5,
                            true);
      snprintf(text_string, 20, "  TEMP  %d *C", device_data[i].temp);

      /* Draw text on the memory lcd display*/
      GLIB_drawStringOnLine(&glibContext,
                            text_string,
                            current_line++,
                            GLIB_ALIGN_LEFT,
                            5,
                            5,
                            true);

      snprintf(text_string, 20, "  LED   %s",
               device_data[i].led_stt? "On" : "Off");

      /* Draw text on the memory lcd display*/
      GLIB_drawStringOnLine(&glibContext,
                            text_string,
                            current_line++,
                            GLIB_ALIGN_LEFT,
                            5,
                            5,
                            true);
    }
  }
  DMD_updateDisplay();
}
