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
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"

#include "app_log.h"
#include "sl_iostream_handles.h"

/***************************************************************************************************
 Local Macros and Definitions
 **************************************************************************************************/
/* Set here the operation mode of the SPP device: */
#define SPP_SERVER_MODE 0
#define SPP_CLIENT_MODE 1

#define SPP_OPERATION_MODE SPP_SERVER_MODE//SPP_CLIENT_MODE

/*Main states */
#define DISCONNECTED  0
#define SCANNING      1
#define FIND_SERVICE  2
#define FIND_CHAR     3
#define ENABLE_NOTIF  4
#define DATA_MODE     5
#define DISCONNECTING 6

#define STATE_ADVERTISING 1
#define STATE_CONNECTED   2
#define STATE_SPP_MODE    3

// SPP service UUID: 4880c12c-fdcb-4077-8920-a450d7f9b907
const uint8_t serviceUUID[16] = {0x07,
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
                                 0x48};

// SPP data UUID: fec26ec4-6d71-4442-9f81-55bc21d658d6
const uint8_t charUUID[16] = {0xd6,
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
                              0xfe};


/* maximum number of iterations when polling UART RX data before sending data over BLE connection
 * set value to 0 to disable optimization -> minimum latency but may decrease throughput */
#define UART_POLL_TIMEOUT  5000

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
#if (SPP_OPERATION_MODE == SPP_CLIENT_MODE)
static bool process_scan_response(sl_bt_evt_scanner_legacy_advertisement_report_t *pResp);
#endif

/* Common local functions*/
static void printStats(tsCounters *psCounters);
static void reset_variables();
static void send_spp_data();

/***************************************************************************************************
 Local Variables
 **************************************************************************************************/
#if (SPP_OPERATION_MODE == SPP_SERVER_MODE)
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
#endif

static uint8_t _conn_handle = 0xFF;
static uint8_t _main_state;
static uint32_t _service_handle;
static uint16_t _char_handle;

tsCounters _sCounters;

/* Default maximum packet size is 20 bytes. This is adjusted after connection is opened based
 * on the connection parameters */
static uint8_t _max_packet_size = 20;
static uint8_t _min_packet_size = 20;  // Target minimum bytes for one packet

static void reset_variables()
{
  _conn_handle = 0xFF;
  _main_state = STATE_ADVERTISING;
  _service_handle = 0;
  _char_handle = 0;
  _max_packet_size = 20;

  memset(&_sCounters, 0, sizeof(_sCounters));
}

/**************************************************************************
 Application Init code
 ***************************************************************************/
SL_WEAK void app_init(void)
{

}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  if(STATE_SPP_MODE == _main_state){
    send_spp_data();
  }
}

#if (SPP_OPERATION_MODE == SPP_SERVER_MODE)
/**************************************************************************//**
 * Bluetooth stack event handler for SPP Server mode
 *
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  uint16_t max_mtu_out;
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {

    case sl_bt_evt_system_boot_id:
      app_log("SPP Role: SPP Server\r\n");
      reset_variables();
      sl_bt_gatt_server_set_max_mtu(247, &max_mtu_out);
      sl_bt_advertiser_create_set(&advertising_set_handle);
      sl_bt_advertiser_set_timing( advertising_set_handle,
                                160, // min. adv. interval (milliseconds * 1.6)
                                160, // max. adv. interval (milliseconds * 1.6)
                                0,   // adv. duration
                                0);  // max. num. adv. events

      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle, sl_bt_advertiser_general_discoverable);
      // Start  advertising and enable connections
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle, sl_bt_legacy_advertiser_connectable);

      break;

    case sl_bt_evt_connection_opened_id:
      _conn_handle = evt->data.evt_connection_opened.connection;
      app_log("Connection opened\r\n");
      _main_state = STATE_CONNECTED;

      /* Request connection parameter update.
       * conn.interval min 20ms, max 40ms, slave latency 4 intervals,
       * supervision timeout 2 seconds
       * (These should be compliant with Apple Bluetooth Accessory Design Guidelines, both R7 and R8) */
      sl_bt_connection_set_parameters(_conn_handle, 24, 40, 0, 200, 0, 0xFFFF);
      break;

    case sl_bt_evt_connection_parameters_id:
      app_log("Conn.parameters: interval %u units, txsize %u\r\n", evt->data.evt_connection_parameters.interval, evt->data.evt_connection_parameters.txsize);
      break;

    case sl_bt_evt_gatt_mtu_exchanged_id:
      /* Calculate maximum data per one notification / write-without-response, this depends on the MTU.
       * up to ATT_MTU-3 bytes can be sent at once  */
      _max_packet_size = evt->data.evt_gatt_mtu_exchanged.mtu - 3;
      _min_packet_size = _max_packet_size; /* Try to send maximum length packets whenever possible */
      app_log("MTU exchanged: %d\r\n", evt->data.evt_gatt_mtu_exchanged.mtu);
      break;

    case sl_bt_evt_connection_closed_id:
      printStats(&_sCounters);
      if (STATE_SPP_MODE == _main_state){
         sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
      }
      reset_variables();
      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle, sl_bt_legacy_advertiser_connectable);
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      {
        sl_bt_evt_gatt_server_characteristic_status_t char_status;
        char_status = evt->data.evt_gatt_server_characteristic_status;

        if (char_status.characteristic == gattdb_spp_data) {
          if (char_status.status_flags == gatt_server_client_config) {
            // Characteristic client configuration (CCC) for spp_data has been changed
            if (char_status.client_config_flags == gatt_notification) {
              _main_state = STATE_SPP_MODE;
              sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
              app_log("SPP Mode ON\r\n");
              }
            else {
              app_log("SPP Mode OFF\r\n");
              _main_state = STATE_CONNECTED;
              sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
              }
            }
          }
      }
      break;

    case sl_bt_evt_gatt_server_attribute_value_id:
    {
       for(uint8_t i = 0; i < evt->data.evt_gatt_server_attribute_value.value.len; i++) {
           sl_iostream_putchar(sl_iostream_vcom_handle, evt->data.evt_gatt_server_attribute_value.value.data[i]);
       }
       _sCounters.num_pack_received++;
       _sCounters.num_bytes_received += evt->data.evt_gatt_server_attribute_value.value.len;
    }
    break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

#else
/**************************************************************************//**
 * Bluetooth stack event handler for SPP Client mode
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  uint16_t max_mtu_out;
  sl_status_t status;

  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_bt_evt_system_boot_id:
      app_log("SPP Role: SPP client\r\n");
      reset_variables();
      sl_bt_gatt_server_set_max_mtu(247, &max_mtu_out);
      sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
      _main_state = SCANNING;
      break;

   /* case sl_bt_evt_scanner_scan_report_id:
      if(process_scan_response(&evt->data.evt_scanner_scan_report)){
         status = sl_bt_connection_open(evt->data.evt_scanner_scan_report.address,
                                         evt->data.evt_scanner_scan_report.address_type,
                                         sl_bt_gap_1m_phy,
                                         &_conn_handle);
         if(SL_STATUS_OK == status){
           sl_bt_scanner_stop();
           }
        }
      break;
*/
    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      if(process_scan_response(&evt->data.evt_scanner_legacy_advertisement_report)){
         status = sl_bt_connection_open(evt->data.evt_scanner_scan_report.address,
                                         evt->data.evt_scanner_scan_report.address_type,
                                         sl_bt_gap_1m_phy,
                                         &_conn_handle);
         if(SL_STATUS_OK == status){
           sl_bt_scanner_stop();
           }
        }
      break;

    case sl_bt_evt_connection_opened_id:
      app_log("Connection opened!\r\n");
      _main_state = FIND_SERVICE;
      sl_bt_gatt_discover_primary_services_by_uuid(_conn_handle, 16, serviceUUID);
      break;

    case sl_bt_evt_connection_closed_id:
      if (STATE_SPP_MODE == _main_state){
          sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
      }
      printStats(&_sCounters);
      reset_variables();
      sl_bt_scanner_start(sl_bt_gap_1m_phy, sl_bt_scanner_discover_generic);
      _main_state = SCANNING;
      break;

    case sl_bt_evt_connection_parameters_id:
       app_log("Conn.parameters: interval %u units, txsize %u\r\n",
                                  evt->data.evt_connection_parameters.interval,
                                  evt->data.evt_connection_parameters.txsize);
      break;

    case sl_bt_evt_gatt_mtu_exchanged_id:
       /* Calculate maximum data per one notification / write-without-response, this depends on the MTU.
        * up to ATT_MTU-3 bytes can be sent at once  */
       _max_packet_size = evt->data.evt_gatt_mtu_exchanged.mtu - 3;
       _min_packet_size = _max_packet_size; /* Try to send maximum length packets whenever possible */
       app_log("MTU exchanged: %d\r\n", evt->data.evt_gatt_mtu_exchanged.mtu);
       break;

    case sl_bt_evt_gatt_service_id:
      if (evt->data.evt_gatt_service.uuid.len == 16) {
          if (memcmp(serviceUUID, evt->data.evt_gatt_service.uuid.data, 16) == 0) {
            app_log("Service discovered\r\n");
            _service_handle = evt->data.evt_gatt_service.service;
          }
        }
      break;

    case sl_bt_evt_gatt_procedure_completed_id:
      switch (_main_state) {
         case FIND_SERVICE:
           if (_service_handle > 0) {
             // Service found, next step: search for characteristics
             sl_bt_gatt_discover_characteristics(_conn_handle, _service_handle);
             _main_state = FIND_CHAR;
           } else {
             //Service is not found: disconnect
               app_log("SPP service not found!\r\n");
               sl_bt_connection_close(_conn_handle);
           }
           break;

         case FIND_CHAR:
           if (_char_handle > 0) {
             // Characteristic found, turn on indications
             sl_bt_gatt_set_characteristic_notification(_conn_handle, _char_handle, gatt_notification);
             _main_state = ENABLE_NOTIF;
           } else {
             //Characteristic is not found: disconnect
               app_log("SPP char not found?\r\n");
               sl_bt_connection_close(_conn_handle);
           }
           break;

         case ENABLE_NOTIF:
           _main_state = STATE_SPP_MODE;
           app_log("SPP Mode ON\r\n");
           //disable deep sleep (for using USART)
           sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
           break;

         default:
           break;
         }
      break;

      case sl_bt_evt_gatt_characteristic_id:
        if (evt->data.evt_gatt_characteristic.uuid.len == 16) {
          if (memcmp(charUUID, evt->data.evt_gatt_characteristic.uuid.data, 16) == 0) {
            app_log("Char discovered\r\n");
            _char_handle = evt->data.evt_gatt_characteristic.characteristic;
          }
        }
        break;

      case sl_bt_evt_gatt_characteristic_value_id:
        if (evt->data.evt_gatt_characteristic_value.characteristic == _char_handle) {
           for(uint8_t i = 0; i < evt->data.evt_gatt_server_attribute_value.value.len; i++) {
               sl_iostream_putchar(sl_iostream_vcom_handle, evt->data.evt_gatt_server_attribute_value.value.data[i]);
           }
           _sCounters.num_pack_received++;
           _sCounters.num_bytes_received += evt->data.evt_gatt_server_attribute_value.value.len;
        }
        break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

static bool process_scan_response(sl_bt_evt_scanner_legacy_advertisement_report_t *pResp)
{
  // Decoding advertising packets is done here. The list of AD types can be found
  // at: https://www.bluetooth.com/specifications/assigned-numbers/Generic-Access-Profile

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
      app_log("%s\r\n", name);
    }

    // 4880c12c-fdcb-4077-8920-a450d7f9b907
    if (ad_type == 0x06 || ad_type == 0x07) {
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

#endif //SPP_OPERATION_MODE


static void printStats(tsCounters *psCounters)
{
  app_log("Outgoing data:\r\n");
  app_log(" bytes/packets sent: %lu / %lu ", psCounters->num_bytes_sent, psCounters->num_pack_sent);
  app_log(", num writes: %lu\r\n", psCounters->num_writes);
  app_log("(RX buffer overflow is not tracked)\r\n");
  app_log("Incoming data:\r\n");
  app_log(" bytes/packets received: %lu / %lu\r\n", psCounters->num_bytes_received, psCounters->num_pack_received);

  return;
}

static void send_spp_data()
{
  uint8_t len = 0;
  uint8_t data[256];
  sl_status_t result, read_result;
  uint16_t sent_len = 0;
  uint32_t timeout = 0;
  char c;


  // Read up to _max_packet_size characters from local buffer
  while (len < _max_packet_size) {
      read_result = sl_iostream_getchar(sl_iostream_vcom_handle, &c);
      if(SL_STATUS_OK == read_result ) {
        data[len++] = (uint8_t)c;
      } else if(len == 0) {
        /* If the first ReadChar() fails then return immediately */
        return;
      } else {
        /* Speed optimization: if there are some bytes to be sent but the length is still
         * below the preferred minimum packet size, then wait for additional bytes
         * until timeout. Target is to put as many bytes as possible into each air packet */
        // Conditions for exiting the while loop and proceed to send data:
        if(timeout++ > UART_POLL_TIMEOUT) {
          break;
        } else if(len >= _min_packet_size) {
          break;
        }
      }
  }

  if (len > 0) {
    // Stack may return "out-of-memory" (SL_STATUS_NO_MORE_RESOURCE) error if the local buffer is full -> in that case, just keep trying until the command succeeds
    do {
#if (SPP_OPERATION_MODE == SPP_SERVER_MODE)
      result = sl_bt_gatt_server_send_notification(_conn_handle, gattdb_spp_data, len, data);
#else
      result = sl_bt_gatt_write_characteristic_value_without_response(_conn_handle, _char_handle, len, data, &sent_len);
#endif
      _sCounters.num_writes++;
    } while(result == SL_STATUS_NO_MORE_RESOURCE);

    if (result != 0) {
      app_log("Unexpected error: %x\r\n", result);
    } else {
      _sCounters.num_pack_sent++;
      _sCounters.num_bytes_sent += len;
    }
  }
  return;
}
