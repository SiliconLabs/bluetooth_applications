/*******************************************************************************
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
#include "em_common.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app_assert.h"
#include "app_log.h"
#include "app.h"

#include "sl_iostream_handles.h"
#include "sl_simple_button_instances.h"
#include "sl_sleeptimer.h"

/*******************************************************************************
 *    Local Macros and Definitions
 ******************************************************************************/

/* Set here the operation mode of the SPP device: */
#define SPP_SERVER_MODE       0
#define SPP_CLIENT_MODE       1

/* soft timer handles */
#define RESTART_TIMER         1
#define GPIO_POLL_TIMER       3

#define EVENT_RECONNECT       (1 << 0)
#define EVENT_PASSKEY_CONFIRM (1 << 1)

/*Main states */
enum STATE {
  STATE_INIT,
  STATE_DISCONNECTED,
  STATE_SCANNING,
  STATE_FIND_SERVICE,
  STATE_FIND_CHAR,
  STATE_ENABLE_NOTIF,
  STATE_ADVERTISING,
  STATE_CONNECTED,
  STATE_SPP_MODE,
  STATE_AUTHENTICATING
};

// SPP service UUID: 4880c12c-fdcb-4077-8920-a450d7f9b907
const uint8_t serviceUUID[16] = { 0x07,
                                  0xb9,
                                  0xf9,
                                  0xd7,
                                  0x50,
                                  0xa4,
                                  0x20,
                                  0x89,
                                  0x77,
                                  0x40,
                                  0xcb,
                                  0xfd,
                                  0x2c,
                                  0xc1,
                                  0x80,
                                  0x48 };

// SPP data UUID: fec26ec4-6d71-4442-9f81-55bc21d658d6
const uint8_t charUUID[16] = { 0xd6,
                               0x58,
                               0xd6,
                               0x21,
                               0xbc,
                               0x55,
                               0x81,
                               0x9f,
                               0x42,
                               0x44,
                               0x71,
                               0x6d,
                               0xc4,
                               0x6e,
                               0xc2,
                               0xfe };

/* Maximum number of iterations when polling UART RX data before sending data
 * over BLE connection
 * set value to 0 to disable optimization -> minimum latency but may decrease
 * throughput */
#define UART_POLL_TIMEOUT 5000

static sl_sleeptimer_timer_handle_t reconnect_timer;

/*Bookkeeping struct for storing amount of received/sent data  */
typedef struct
{
  uint32_t num_pack_sent;
  uint32_t num_bytes_sent;
  uint32_t num_pack_received;
  uint32_t num_bytes_received;
  uint32_t num_writes; /* Total number of send attempts */
} tsCounters;

/* Function is only used in SPP client mode */
static bool process_scan_response(
  sl_bt_evt_scanner_legacy_advertisement_report_t *pResp);

/* Common local functions*/
static void printStats(tsCounters *psCounters);
static void reset_variables();
static void send_spp_data();
static void setup_advertising_or_scanning(sl_bt_msg_t *evt);
static void discover_primary_services(void);
static sl_bt_connection_security_t get_connection_security_mode(
  uint8_t connection);
static void reconnect_callback(sl_sleeptimer_timer_handle_t *handle,
                               void *data);

/*******************************************************************************
 *    Local Variables
 ******************************************************************************/
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static uint32_t _service_handle;
static uint16_t _char_handle;

static uint8_t _spp_operation_mode = SPP_SERVER_MODE;
// static uint8_t _spp_operation_mode = SPP_CLIENT_MODE;
static uint8_t _conn_handle = 0xFF;
static enum STATE _main_state = STATE_INIT;

tsCounters _sCounters;

static uint8_t _max_packet_size = 20; // Maximum bytes per one packet
static uint8_t _min_packet_size = 20; // Target minimum bytes for one packet

uint8_t connectionToIncreasSec = 0xFF;
enum SOFT_TIMER_HANDLES
{
  INCREASE_SEC_TMR_HANDLE = 1,
};

static void reset_variables()
{
  _conn_handle = 0xFF;
  if (_spp_operation_mode == SPP_SERVER_MODE) {
    _main_state = STATE_ADVERTISING;
  } else {
    _main_state = STATE_DISCONNECTED;
  }
  _max_packet_size = 20;

  memset(&_sCounters, 0, sizeof(_sCounters));
}

/**************************************************************************/ /**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  if (sl_button_get_state(&sl_button_btn0) == SL_SIMPLE_BUTTON_PRESSED) {
    sl_status_t sc;
    uint8_t count_down = 10;

    app_log("\r\n");
    while (count_down) {
      if (sl_button_get_state(&sl_button_btn0) == SL_SIMPLE_BUTTON_RELEASED) {
        break;
      }
      sl_sleeptimer_delay_millisecond(500);

      if (count_down % 2) {
        app_log("Hold button for %d (sec) to delete all bonding\r",
                (count_down + 1) / 2);
      }
      count_down--;
    }
    app_log("\r\n");

    if (count_down == 0) {
      sc = sl_bt_sm_delete_bondings();
      app_assert_status(sc);
//      sl_bt_system_reset();
    } else {
      app_log("Button holding is timed out, keep old bonding\r\n");
    }
    _spp_operation_mode = SPP_CLIENT_MODE;
    app_log("SPP Role: SPP client\r\n");
  }
}

/**************************************************************************/ /**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
  if (STATE_SPP_MODE == _main_state) {
    send_spp_data();
  }
}

/**************************************************************************/ /**
 * Bluetooth stack event handler for SPP Server mode.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header))
  {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      reset_variables();
      // Security configurations
      sc = sl_bt_sm_configure(0x0F, sm_io_capability_displayyesno);
      app_assert_status(sc);

      sc = sl_bt_sm_set_bondable_mode(1);
      app_assert_status(sc);

      // Maximum allowed bonding count: 8
      // New bonding overwrite the bonding that was used the longest time ago
      sc = sl_bt_sm_store_bonding_configuration(8, 0x2);
      app_assert_status(sc);

      setup_advertising_or_scanning(evt);

      sl_button_enable(&sl_button_btn0);
      break;

    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      if (process_scan_response(&evt->data.
                                evt_scanner_legacy_advertisement_report)) {
        sc = sl_bt_scanner_stop();
        app_assert_status(sc);

        sc = sl_bt_connection_open(evt->data.evt_scanner_scan_report.address,
                                   evt->data.evt_scanner_scan_report.address_type,
                                   sl_bt_gap_1m_phy,
                                   &_conn_handle);
        app_assert_status(sc);
      }
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      _conn_handle = evt->data.evt_connection_opened.connection;

      app_log("--------------------------------------\r\n\n");
      app_log("Connected\r\n\n");
      if (_spp_operation_mode == SPP_SERVER_MODE) {
        _main_state = STATE_CONNECTED;
        sc = sl_bt_advertiser_stop(advertising_set_handle);
        app_assert_status(sc);

        /* Request connection parameter update.
         * conn.interval min 20ms, max 40ms, slave latency 4 intervals,
         * supervision timeout 2 seconds
         * (These should be compliant with Apple Bluetooth Accessory Design
         * Guidelines, both R7 and R8) */
        sc = sl_bt_connection_set_parameters(_conn_handle,
                                             24,
                                             40,
                                             0,
                                             200,
                                             0,
                                             0xFFFF);
        app_assert_status(sc);
      } else {
        discover_primary_services();
        _main_state = STATE_FIND_SERVICE;
      }
      break;

    case sl_bt_evt_connection_parameters_id:
      if (_main_state == STATE_AUTHENTICATING) {
        app_log("The security is enhanced\r\n");

        // After enhance the security, restart discovering services
        discover_primary_services();
        _main_state = STATE_FIND_SERVICE;
      }
      break;

    case sl_bt_evt_gatt_mtu_exchanged_id:
      /* Calculate maximum data per one notification / write-without-response,
       * this depends on the MTU.
       * up to ATT_MTU-3 bytes can be sent at once  */
      _max_packet_size = evt->data.evt_gatt_mtu_exchanged.mtu - 3;
      _min_packet_size = _max_packet_size;

      /* Try to send maximum length packets whenever possible */
      app_log("MTU exchanged: %d\r\n\n", evt->data.evt_gatt_mtu_exchanged.mtu);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("DISCONNECTED!\r\n\n");

      /* Show statistics (RX/TX counters) after disconnect: */
      printStats(&_sCounters);
      // Enable sleeping
      sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM2);
      reset_variables();
      if (_spp_operation_mode == SPP_SERVER_MODE) {
        // Restart advertising after client has disconnected.
        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                           sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);
      } else {
        sc = sl_sleeptimer_restart_timer_ms(&reconnect_timer,
                                            1000,
                                            reconnect_callback,
                                            NULL,
                                            0,
                                            0);
        app_assert_status(sc);
      }
      break;

    case sl_bt_evt_sm_confirm_bonding_id:
      app_log("Bonding confirm\r\n");
      sc = sl_bt_sm_bonding_confirm(_conn_handle, 1);
      app_assert_status(sc);
      break;

    // Event raised when bonding is successful
    case sl_bt_evt_sm_bonded_id:
      app_log("Bonded\r\n\n");
      break;

    // Event raised when bonding failed
    case sl_bt_evt_sm_bonding_failed_id:
      app_log("Bonding failed\r\n\n");
      app_log("--------------------------------------\r\n\n");
      sc = sl_bt_sm_increase_security(_conn_handle);
      app_assert_status(sc);

      _main_state = STATE_AUTHENTICATING;
      break;

    case sl_bt_evt_sm_passkey_display_id:
      app_log("Passkey: %06lu", evt->data.evt_sm_passkey_display.passkey);
      break;

    // Event raised by the security manager when a passkey needs to be confirmed
    case sl_bt_evt_sm_confirm_passkey_id:
      app_log("Do you see this passkey on the other device: %06lu? (y/n)\r\n\n",
              evt->data.evt_sm_confirm_passkey.passkey);
      sl_bt_external_signal(EVENT_PASSKEY_CONFIRM);
      break;

    case sl_bt_evt_gatt_service_id:
      if (evt->data.evt_gatt_service.uuid.len == 16) {
        if (memcmp(serviceUUID, evt->data.evt_gatt_service.uuid.data,
                   16) == 0) {
          app_log("Service discovered\r\n");
          _service_handle = evt->data.evt_gatt_service.service;
        }
      }
      break;

    case sl_bt_evt_gatt_procedure_completed_id:
      if (_spp_operation_mode != SPP_CLIENT_MODE) {
        break;
      }
      switch (_main_state)
      {
        case STATE_FIND_SERVICE:
          if (_service_handle > 0) {
            // Service found, next step: search for characteristics
            sc = sl_bt_gatt_discover_characteristics(_conn_handle,
                                                     _service_handle);
            app_assert_status(sc);

            _main_state = STATE_FIND_CHAR;
          } else {
            // Service is not found: disconnect
            app_log("SPP service not found!\r\n");
            sc = sl_bt_connection_close(_conn_handle);
            app_assert_status(sc);
          }
          break;

        case STATE_FIND_CHAR:
          if (_char_handle > 0) {
            // Characteristic found, turn on notifications
            sc = sl_bt_gatt_set_characteristic_notification(_conn_handle,
                                                            _char_handle,
                                                            sl_bt_gatt_notification);
            app_assert_status(sc);

            app_log("Characteristic notification enabled\r\n");

            _main_state = STATE_ENABLE_NOTIF;
          } else {
            // Characteristic is not found? -> disconnect
            app_log("SPP characteristic not found?\r\n");
            sc = sl_bt_connection_close(_conn_handle);
            app_assert_status(sc);
          }
          break;

        case STATE_ENABLE_NOTIF:
        {
          bool increase_security = false;

          if (evt->data.evt_gatt_procedure_completed.result != SL_STATUS_OK) {
            app_log("Setting characteristic notification is failed: 0x%x\r\n",
                    evt->data.evt_gatt_procedure_completed.result);

            if (evt->data.evt_gatt_procedure_completed.result
                == SL_STATUS_BT_ATT_INSUFFICIENT_ENCRYPTION) {
              app_log(
                "Insufficient encryption, need to increase security level\r\n\n");
              increase_security = true;
            } else {
              // Characteristic found, turn on notifications
              sc = sl_bt_gatt_set_characteristic_notification(_conn_handle,
                                                              _char_handle,
                                                              sl_bt_gatt_notification);
              app_assert_status(sc);
              _main_state = STATE_FIND_CHAR;
              break;
            }
          }

          if (!increase_security
              && (get_connection_security_mode(_conn_handle)
                  > sl_bt_connection_mode1_level2)) {
            app_log("--------------------------------------\r\n\n");
            app_log("Already bonded.\r\n");
            app_log("Authentication not needed.\r\n\n");
            app_log("--------------------------------------\r\n\n");
            app_log("** Client: SPP mode ON **\r\n\n");
            _main_state = STATE_SPP_MODE;
            // disable sleeping when SPP mode active
            sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM2);
          } else {
            // If not yet bonded or need to increase security,
            // start authenticating
            app_log("--------------------------------------\r\n\n");
            app_log("Re-authenticating...\r\n\n");

            sc = sl_bt_sm_increase_security(_conn_handle);
            app_assert_status(sc);

            _main_state = STATE_AUTHENTICATING;
          }
        }
        break;

        default:
          break;
      }
      break;

    case sl_bt_evt_gatt_characteristic_id:
      if (evt->data.evt_gatt_characteristic.uuid.len == 16) {
        if (memcmp(charUUID, evt->data.evt_gatt_characteristic.uuid.data,
                   16) == 0) {
          app_log("Char discovered\r\n");
          _char_handle = evt->data.evt_gatt_characteristic.characteristic;
        }
      }

      break;

    case sl_bt_evt_gatt_characteristic_value_id:
      if (evt->data.evt_gatt_characteristic_value.characteristic
          == _char_handle) {
        for (uint8_t i = 0;
             i < evt->data.evt_gatt_server_attribute_value.value.len; i++)
        {
          sl_iostream_putchar(sl_iostream_vcom_handle,
                              evt->data.evt_gatt_server_attribute_value.value.data[
                                i]);
        }
        _sCounters.num_pack_received++;
        _sCounters.num_bytes_received +=
          evt->data.evt_gatt_server_attribute_value.value.len;
      }
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
    {
      sl_bt_evt_gatt_server_characteristic_status_t pStatus;
      pStatus = evt->data.evt_gatt_server_characteristic_status;

      if (pStatus.characteristic == gattdb_gatt_spp_data) {
        if (pStatus.status_flags == gatt_server_client_config) {
          // Characteristic client configuration (CCC)
          // for spp_data has been changed
          if (pStatus.client_config_flags == gatt_notification) {
            _main_state = STATE_SPP_MODE;
            // Disable sleeping when SPP mode active
            sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM2);

            // If not yet bonded, start authentication.
            if (get_connection_security_mode(_conn_handle)
                <= sl_bt_connection_mode1_level2) {
              app_log("--------------------------------------\r\n\n");
              app_log("Authenticating...\r\n\n");

              sc = sl_bt_sm_increase_security(_conn_handle);
              app_assert_status(sc);

              _main_state = STATE_AUTHENTICATING;
            } else {
              app_log("--------------------------------------\r\n\n");
              app_log("Already bonded.\r\n");
              app_log("Authentication not needed.\r\n\n");
              app_log("--------------------------------------\r\n\n");
              app_log("** Server: SPP Mode ON **\r\n\n");
              _main_state = STATE_SPP_MODE;
            }
          } else {
            app_log("** Server: SPP Mode OFF **\r\n\n");
            app_log("--------------------------------------\r\n\n");
            _main_state = STATE_CONNECTED;
            // Enable sleeping
            sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM2);
          }
        }
      }
    }
    break;

    case sl_bt_evt_gatt_server_attribute_value_id:
    {
      for (int i = 0; i < evt->data.evt_gatt_server_attribute_value.value.len;
           i++)
      {
        sl_iostream_putchar(sl_iostream_vcom_handle,
                            evt->data.evt_gatt_server_attribute_value.value.data[
                              i]);
      }

      _sCounters.num_pack_received++;
      _sCounters.num_bytes_received +=
        evt->data.evt_gatt_server_attribute_value.value.len;
    }
    break;

    case sl_bt_evt_system_external_signal_id:
    {
      if (evt->data.evt_system_external_signal.extsignals
          & EVENT_PASSKEY_CONFIRM) {
        char _ch;
        sl_iostream_getchar(sl_iostream_vcom_handle, &_ch);
        while ((_ch != 'y') && (_ch != 'n') && (_ch != 'Y') && (_ch != 'N'))
        {
          sl_iostream_getchar(sl_iostream_vcom_handle, &_ch);
        }
        app_log("\r\n");
        if ((_ch == 'y') || (_ch == 'Y')) {
          sc = sl_bt_sm_passkey_confirm(_conn_handle, 1);
          app_log("Waiting for other device to confirm...\r\n\n");
        } else {
          sc = sl_bt_sm_passkey_confirm(_conn_handle, 0);
        }
        app_assert_status(sc);
      }

      if (evt->data.evt_system_external_signal.extsignals & EVENT_RECONNECT) {
        if (_conn_handle != 0xFF) {
          sc = sl_bt_connection_close(_conn_handle);
          app_assert_status(sc);
          sc = sl_sleeptimer_restart_timer_ms(&reconnect_timer,
                                              1000,
                                              reconnect_callback,
                                              NULL,
                                              0,
                                              0);
          app_assert_status(sc);
        } else {
          if (_main_state != STATE_INIT) {
            sl_sleeptimer_stop_timer(&reconnect_timer);

            sc = sl_bt_scanner_start(sl_bt_gap_1m_phy,
                                     sl_bt_scanner_discover_generic);
            app_assert_status(sc);
          }
        }
      }
    }
    break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

static bool process_scan_response(
  sl_bt_evt_scanner_legacy_advertisement_report_t *pResp)
{
  // Decoding advertising packets is done here.
  // The list of AD types can be found at:
  // https://www.bluetooth.com/specifications/assigned-numbers/Generic-Access-Profile

  uint8_t i = 0, ad_len, ad_type;
  bool ad_match_found = false;

  char name[32];

  while (i < (pResp->data.len - 1))
  {
    ad_len = pResp->data.data[i];
    ad_type = pResp->data.data[i + 1];

    if ((ad_type == 0x08) || (ad_type == 0x09)) {
      // Type 0x08 = Shortened Local Name
      // Type 0x09 = Complete Local Name
      memcpy(name, &(pResp->data.data[i + 2]), ad_len - 1);
      name[ad_len - 1] = 0;
      app_log("%s\r\n", name);
    }

    // 4880c12c-fdcb-4077-8920-a450d7f9b907
    if ((ad_type == 0x06) || (ad_type == 0x07)) {
      // Type 0x06 = Incomplete List of 128-bit Service Class UUIDs
      // Type 0x07 = Complete List of 128-bit Service Class UUIDs
      if (memcmp(serviceUUID, &(pResp->data.data[i + 2]), 16) == 0) {
        app_log("Found SPP device\r\n");
        ad_match_found = true;
      }
    }
    // Jump to next AD record
    i = i + ad_len + 1;
  }

  return ad_match_found;
}

static void printStats(tsCounters *psCounters)
{
  app_log("Outgoing data:\r\n");
  app_log(" bytes/packets sent: %lu / %lu ",
          psCounters->num_bytes_sent,
          psCounters->num_pack_sent);
  app_log(", num writes: %lu\r\n",
          psCounters->num_writes);
  app_log("(RX buffer overflow is not tracked)\r\n");
  app_log("Incoming data:\r\n");
  app_log(" bytes/packets received: %lu / %lu\r\n",
          psCounters->num_bytes_received,
          psCounters->num_pack_received);

  return;
}

static void send_spp_data()
{
  uint8_t len = 0;
  uint8_t data[256];
  sl_status_t result, read_result;
  uint32_t timeout = 0;
  char c;

  // Read up to _max_packet_size characters from local buffer
  while (len < _max_packet_size)
  {
    read_result = sl_iostream_getchar(sl_iostream_vcom_handle, &c);
    if (SL_STATUS_OK == read_result) {
      data[len++] = (uint8_t)c;
    } else if (len == 0) {
      /* If the first sl_iostream_getchar() fails then return immediately */
      return;
    } else {
      /* Speed optimization: if there are some bytes to be sent but the length
       * is still below the preferred minimum packet size, then wait for
       * additional bytes until timeout. Target is to put as many bytes as
       * possible into each air packet */
      // Conditions for exiting the while loop and proceed to send data:
      if (timeout++ > UART_POLL_TIMEOUT) {
        break;
      } else if (len >= _min_packet_size) {
        break;
      }
    }
  }

  if (len > 0) {
    /* Stack may return "out-of-memory" (SL_STATUS_NO_MORE_RESOURCE)
     * error if the local buffer is full -> in that case, just keep trying
     * until the command succeeds
     */
    do
    {
      if (_spp_operation_mode == SPP_SERVER_MODE) {
        result = sl_bt_gatt_server_send_notification(_conn_handle,
                                                     gattdb_gatt_spp_data,
                                                     len,
                                                     data);
      } else {
        result = sl_bt_gatt_write_characteristic_value_without_response(
          _conn_handle,
          _char_handle,
          len,
          data,
          NULL);
      }
      _sCounters.num_writes++;
    } while (result == SL_STATUS_NO_MORE_RESOURCE);
    app_assert_status(result);

    _sCounters.num_pack_sent++;
    _sCounters.num_bytes_sent += len;
  }
  return;
}

static void setup_advertising_or_scanning(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];
  uint8_t i;

  if (_spp_operation_mode == SPP_SERVER_MODE) {
    app_log("SPP Role: SPP Server\r\n");
    app_log("stack version: %u.%u.%u\r\r\n",
            evt->data.evt_system_boot.major,
            evt->data.evt_system_boot.minor,
            evt->data.evt_system_boot.patch);
    // Extract unique ID from BT Address.
    sc = sl_bt_system_get_identity_address(&address, &address_type);
    app_assert_status(sc);

    app_log("local BT device address: ");
    for (i = 0; i < 5; i++)
    {
      app_log("%2.2x:", address.addr[5 - i]);
    }
    app_log("%2.2x\r\r\n", address.addr[0]);
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

    // Generate data for advertising
    sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                               sl_bt_advertiser_general_discoverable);
    app_assert_status(sc);

    // Set advertising interval to 100ms.
    sc = sl_bt_advertiser_set_timing(
      advertising_set_handle,
      160,   // min. adv. interval (milliseconds * 1.6)
      160,   // max. adv. interval (milliseconds * 1.6)
      0,     // adv. duration
      0);    // max. num. adv. events
    app_assert_status(sc);
    // Start advertising and enable connections.
    sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                       sl_bt_advertiser_connectable_scannable);
    app_assert_status(sc);
  } else {
    // Set scanning parameters.
    sc = sl_bt_scanner_set_parameters(sl_bt_scanner_scan_mode_passive, 16, 16);
    app_assert_status(sc);

    sc = sl_bt_gatt_server_set_max_mtu(247, NULL);
    app_assert_status(sc);

    sc = sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
    app_assert_status(sc);

    _main_state = STATE_SCANNING;
    app_log("SCAN results:\r\n\n");
  }
}

static void discover_primary_services(void)
{
  sl_status_t sc;

  sc = sl_bt_gatt_discover_primary_services_by_uuid(_conn_handle,
                                                    16,
                                                    serviceUUID);
  app_assert_status(sc);
}

static sl_bt_connection_security_t get_connection_security_mode(
  uint8_t connection)
{
  sl_status_t sc;
  uint8_t security_mode;
  uint8_t key_size;
  uint8_t bonding_handle;

  sc = sl_bt_connection_get_security_status(connection,
                                            &security_mode,
                                            &key_size,
                                            &bonding_handle);
  app_assert_status(sc);
  return security_mode;
}

static void reconnect_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  (void)data;

  sl_bt_external_signal(EVENT_RECONNECT);
}

/***************************************************************************//**
 * Callback on button change.
 ******************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  if ((sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED)
      && (_main_state != STATE_INIT)) {
    sl_bt_external_signal(EVENT_RECONNECT);
  }
}
