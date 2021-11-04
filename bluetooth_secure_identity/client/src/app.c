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
#include <stdbool.h>
#include <math.h>
#include "em_common.h"
#include "app_log.h"
#include "app_assert.h"
#include "app_se_manager_secure_identity.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif // SL_COMPONENT_CATALOG_PRESENT
#ifdef SL_CATALOG_CLI_PRESENT
#include "sl_cli.h"
#endif // SL_CATALOG_CLI_PRESENT
#include "app.h"
#include "mbedtls/error.h"
#include "mbedtls\x509.h"
#include "mbedtls\x509_crt.h"
#include "mbedtls\pk.h"
#include "ecdh-util.h"

#if SL_BT_CONFIG_MAX_CONNECTIONS < 1
  #error At least 1 connection has to be enabled!
#endif
// Array for holding properties of multiple (parallel) connections
static conn_properties_t conn_properties[SL_BT_CONFIG_MAX_CONNECTIONS];
// Counter of active connections
static uint8_t active_connections_num;
// State of the connection under establishment
static conn_state_t conn_state;
static const uint8_t secure_attest_service[16] = {0x53, 0x69, 0x6c, 0x61, 0x62, 0x73, 0x54, 0x72, 0x61, 0x69, 0x6e, 0x69, 0x6e, 0x67, 0x32, 0x31};
static const uint8_t characteristic_uuids[nCharacteristics][16] =
    {SERVER_DEVICE_CERT_UUID,
     SERVER_BATCH_CERT_UUID,
     SERVER_FACTORY_CERT_UUID,
     SERVER_ROOT_CERT_UUID,
     CLIENT_CHALLENGE_UUID,
     CLIENT_CHALLENGE_RESPONSE_UUID,
     CLIENT_DEVICE_CERT_UUID,
     CLIENT_BATCH_CERT_UUID,
     CLIENT_FACTORY_CERT_UUID,
     CLIENT_ROOT_CERT_UUID,
     SERVER_CHALLENGE_UUID,
     SERVER_CHALLENGE_RESPONSE_UUID,
     SERVER_ECDH_PUBLIC_KEY_UUID,
     CLIENT_ECDH_PUBLIC_KEY_UUID,
     TEST_DATA_UUID};
static struct secure_attest_data_t secure_attest_data;
static psa_key_id_t server_device_pub_key,
                    client_ecdh_id,
                    share_derived_key_id;
static struct signed_key_t peer_key_material;
static mbedtls_x509_crt cert_chain;
static mbedtls_x509_crt root_trust;
mbedtls_pk_context root_pub_pk;
#ifdef USE_CUSTOM_CERTIFICATES


static const uint8_t server_root_pub_key[]=
   "-----BEGIN PUBLIC KEY-----\n"
   "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAED2tCORj7BQ3DGVwKO2L07ABmFRFe\n"
   "f8UDyPZDfMxMJ0JqFM4YKPpTh5C4ok5Zj+YleOJE8ydXuemdM8bOyf3tMw==\n"
   "-----END PUBLIC KEY-----\n";
#endif

/************************************** private functions *******************************************************************/
static void init_properties(void);
static uint8_t find_service_in_advertisement(uint8_t *data, uint8_t len, const uint8_t *service, uint8_t service_len);
static uint8_t find_index_by_connection_handle(uint8_t connection);
static void add_connection(uint8_t connection, uint16_t address);
// Remove a connection from the connection_properties array
static void remove_connection(uint8_t connection);
static bd_addr *read_and_cache_bluetooth_address(uint8_t *address_type_out);
static void print_bluetooth_address(void);
static void set_attribute_system_id(void);
static void gatt_proc_complete_cb(sl_bt_evt_gatt_procedure_completed_t *procedure_completed);
static sl_status_t send_cert(uint8_t connection, uint16_t characteristic, uint8_t cert_id);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
   sl_status_t status;

   // Initialize connection properties
   init_properties();
   // app_log("soc_thermometer_client initialised\n");
   /////////////////////////////////////////////////////////////////////////////
   // Put your additional application init code here!                         //
   // This is called once during start-up.                                    //
   /////////////////////////////////////////////////////////////////////////////
   memset(&secure_attest_data,0,sizeof(secure_attest_data));
   mbedtls_pk_init(&root_pub_pk);
   status = mbedtls_pk_parse_public_key(&root_pub_pk,server_root_pub_key,sizeof(server_root_pub_key));
   if(SL_STATUS_OK!=status){
       app_log_debug("mbedtls_pk_parse_public_key(): error %d\r\n", status);
   }
}

#define SE_MANAGER_PRINT_CERT 1
static int verify_callback(void *data,
                           mbedtls_x509_crt *crt,
                           int depth,
                           uint32_t *flags)
{
   (void) data;
   char *p, *q, *r;
   char buf[1024];
#if (SE_MANAGER_PRINT_CERT == 1)
   int32_t i;
   int32_t ret;
#else
  (void) depth;
#endif

   // Get information about the certificate
#if (SE_MANAGER_PRINT_CERT == 1)
   ret = mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "      ", crt);
   //printf("  + Verify requested for (Depth %d) ... OK\n", depth);
   /*for (i = 0; i < ret; i++) {
    printf("%c", buf[i]); */
   if(depth==3){
      p = strstr((const char *)buf,"subject");
      q = strstr((const char *)p,"O=Silicon Labs");
      r = strstr((const char *)q,"issued");
      if(q>p && q<r){
         app_log("Certificate subject includes Silicon Labs Inc - appears to be valid\r\n");
      }
      else{
         app_log("Certificate subject does not include Silicon Labs Inc\r\n");
         app_log_append(p);
     }
   }
#else
  mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "      ", crt);
#endif

  // Get the verification status of a certificate
#if (SE_MANAGER_PRINT_CERT == 1)
   if ((*flags) != 0) {
      ret = mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", *flags);
      for (i = 0; i < ret; i++) {
         printf("%c", buf[i]);
      }
   }
   if (depth == 0) {
      printf("  + Verify the certificate chain with root certificate... ");
   }
#else
  if ((*flags) != 0) {
    mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", *flags);
  }
#endif
   return 0;
}
/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
   /////////////////////////////////////////////////////////////////////////////
   // Put your additional application code here!                              //
   // This is called infinitely.                                              //
   // Do not call blocking functions from here!                               //
   /////////////////////////////////////////////////////////////////////////////
}

sl_status_t gatt_characteristic_value_dev_cert_cb(sl_bt_evt_gatt_characteristic_value_t *value, uint8_t table_index)
{
   if((value->value.len + value->offset) > sizeof(secure_attest_data.device_certificate_buffer)){
       return SL_STATUS_WOULD_OVERFLOW;
   }
   if(value->characteristic == conn_properties[table_index].characteristic_handles[device_certificate]){
         memcpy(&secure_attest_data.device_certificate_buffer[value->offset],
                value->value.data,
                value->value.len);
         secure_attest_data.device_cert_actual_size += value->value.len;
   }
   return SL_STATUS_OK;
}

sl_status_t gatt_characteristic_value_batch_cert_cb(sl_bt_evt_gatt_characteristic_value_t *value, uint8_t table_index)
{
   // app_log("got %d bytes of the server's batch cert\r\n", value->value.len);
   if((value->value.len + value->offset)> sizeof(secure_attest_data.batch_certificate_buffer)){
      return SL_STATUS_WOULD_OVERFLOW;
   }
   if(value->characteristic == conn_properties[table_index].characteristic_handles[batch_certificate]){
      memcpy(&secure_attest_data.batch_certificate_buffer[value->offset],
             value->value.data,
             value->value.len);
             secure_attest_data.batch_cert_actual_size += value->value.len;
   }
   return SL_STATUS_OK;
}

sl_status_t gatt_characteristic_value_factory_cert_cb(sl_bt_evt_gatt_characteristic_value_t *value, uint8_t table_index)
{
   //app_log("got %d bytes of the server's factory cert\r\n", value->value.len);
   if((value->value.len+value->offset) > sizeof(secure_attest_data.factory_certificate_buffer)){
      return SL_STATUS_WOULD_OVERFLOW;
   }
   if(value->characteristic == conn_properties[table_index].characteristic_handles[factory_certificate]){
      memcpy(&secure_attest_data.factory_certificate_buffer[value->offset],
         value->value.data,
         value->value.len);
      secure_attest_data.factory_cert_actual_size += value->value.len;
   }
   return SL_STATUS_OK;
}

sl_status_t gatt_characteristic_value_root_cert_cb(sl_bt_evt_gatt_characteristic_value_t *value, uint8_t table_index)
{
   if((value->value.len+value->offset)> sizeof(secure_attest_data.root_certificate_buffer)){
      return SL_STATUS_WOULD_OVERFLOW;
   }
   if(value->characteristic == conn_properties[table_index].characteristic_handles[root_certificate]){
      memcpy(&secure_attest_data.root_certificate_buffer[value->offset],
             value->value.data,
             value->value.len);
             secure_attest_data.root_cert_actual_size += value->value.len;
   }
   return SL_STATUS_OK;
}

sl_status_t gatt_characteristic_value_challenge_response_cb(sl_bt_evt_gatt_characteristic_value_t *value, uint8_t table_index)
{
   if(value->value.len > sizeof(secure_attest_data.response)){
      return SL_STATUS_WOULD_OVERFLOW;
   }
   if(value->characteristic == conn_properties[table_index].characteristic_handles[challenge_response]){
      /* the response *should* come in a single packet, but let's not assume anything */
      app_log("saving challenge response from server\r\n");
      memcpy(&secure_attest_data.response[value->offset],
             value->value.data,
             value->value.len);
   }
   return SL_STATUS_OK;
}

sl_status_t gatt_characteristic_value_server_ecdh_pub_key_cb(sl_bt_evt_gatt_characteristic_value_t *value, uint8_t table_index)
{
   size_t max_size = sizeof(peer_key_material.Q) + sizeof(peer_key_material.signature_raw);
   if(value->value.len > max_size){
      return SL_STATUS_WOULD_OVERFLOW;
   }
   if(value->characteristic == conn_properties[table_index].characteristic_handles[server_ecdh_pub_key]){
      memcpy(&peer_key_material.Q,
             value->value.data,
             65);
      memcpy(&peer_key_material.signature_raw,
             value->value.data+65,
             64);
   }
   return SL_STATUS_OK;
}

sl_status_t gatt_characteristic_value_test_data_cb(sl_bt_evt_gatt_characteristic_value_t *value, uint8_t table_index)
{
   sl_status_t status;
   if(value->characteristic == conn_properties[table_index].characteristic_handles[test_data]){
      uint8_t plaintext[16], ciphertext[32], additional[16], nonce[13];
      psa_algorithm_t alg = PSA_ALG_CCM;
      size_t bytes_decrypted;
      uint8_t *p = value->value.data;

      /* check that there is sufficient room for ciphertext and nonce */
      if(value->value.len > (sizeof(ciphertext)+sizeof(nonce))){
          return SL_STATUS_WOULD_OVERFLOW;
      }
      memcpy(nonce,p,sizeof(nonce));
      memcpy(ciphertext,
             p + sizeof(nonce),
             value->value.len - sizeof(nonce));
      status = psa_aead_decrypt(share_derived_key_id,alg,
                                nonce,sizeof(nonce),
                                additional,0,
                                ciphertext,value->value.len - sizeof(nonce),
                                plaintext,sizeof(plaintext),
                                &bytes_decrypted);

      app_assert(status==0,"authentication/decryption failed, reason %d\r\n", status);
      app_log("Decrypted the following message %s\r\n", plaintext);
   }
   return status;
}
sl_status_t gatt_characteristic_value_server_challenge_cb(sl_bt_evt_gatt_characteristic_value_t *value, uint8_t table_index)
{
   sl_status_t status=SL_STATUS_IN_PROGRESS;
   uint8_t challenge[16], response[64];
   if(value->value.len > sizeof(challenge)){
       return SL_STATUS_WOULD_OVERFLOW;
   }
   app_log("got a random challenge from the server\r\n");
   if(value->characteristic == conn_properties[table_index].characteristic_handles[server_challenge]){

      uint16_t response_handle = conn_properties[table_index].characteristic_handles[server_challenge_response];
      memcpy(challenge,value->value.data,
                 value->value.len);
      app_log("signing challenge\r\n");
      status =  ble_sign_challenge(challenge,
                             sizeof(challenge),
                             response,
                             sizeof(response));
      /*if signing fails for any reason then return before sending the response*/
      if(status){
         return status;
      }
      if(conn_state == challenge_notification_enabled){
          app_log("sending challenge response to server\r\n");
      status = sl_bt_gatt_write_characteristic_value(value->connection,
                response_handle,
                sizeof(response),
                response);
      if(status){
          app_log("error sending challenge response to server %d\r\n", status);
        }
       conn_state = send_challenge_response;
      }
      else{
          app_log("received a challenge from server, but the subscription operation hasn't completed yet\r\n");
      }
   }
   return status;
}

sl_status_t (*gatt_characteristic_callback[nCharacteristics])(sl_bt_evt_gatt_characteristic_value_t *value, uint8_t table_index) =
   {gatt_characteristic_value_dev_cert_cb,
    gatt_characteristic_value_batch_cert_cb,
    gatt_characteristic_value_factory_cert_cb,
    gatt_characteristic_value_root_cert_cb,
    NULL, //challenge characteristic is write-only
    gatt_characteristic_value_challenge_response_cb,
    /* next 4 are client's certificate chain*/
    NULL,//characteristic is write-only
    NULL,//characteristic is write-only
    NULL,
    NULL,
    gatt_characteristic_value_server_challenge_cb,
    NULL,//characteristic is write-only
    gatt_characteristic_value_server_ecdh_pub_key_cb,
    NULL,//characteristic is write-only
    gatt_characteristic_value_test_data_cb       };
/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t* evt)
{
   sl_status_t sc;
   //uint8_t *char_value;
   uint16_t addr_value;
   uint8_t table_index = 0;

   // Handle stack events
   switch (SL_BT_MSG_ID(evt->header)) {
      // -------------------------------
      // This event indicates the device has started and the radio is ready.
      // Do not call any stack command before receiving this boot event!
      case sl_bt_evt_system_boot_id:
         // Print boot message.
         app_log("\r\n----- Silicon Labs Application Layer Security Demo -----\r\n");
         /*  app_log("Bluetooth stack booted: v%d.%d.%d-b%d\n",
                 evt->data.evt_system_boot.major,
                 evt->data.evt_system_boot.minor,
                 evt->data.evt_system_boot.patch,
                 evt->data.evt_system_boot.build); */
         // Print bluetooth address.
         print_bluetooth_address();
         // Set GATT Attribute System ID
         set_attribute_system_id();

         // Set passive scanning on 1Mb PHY
         sc = sl_bt_scanner_set_mode(gap_1m_phy, SCAN_PASSIVE);
         app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to set discovery type\n",
                    (int)sc);
         // Set scan interval and scan window
         sc = sl_bt_scanner_set_timing(gap_1m_phy, SCAN_INTERVAL, SCAN_WINDOW);
         app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to set discovery timing\n",
                    (int)sc);
         // Set the default connection parameters for subsequent connections
         sc = sl_bt_connection_set_default_parameters(CONN_INTERVAL_MIN,
                                                      CONN_INTERVAL_MAX,
                                                      CONN_SLAVE_LATENCY,
                                                      CONN_TIMEOUT,
                                                      CONN_MIN_CE_LENGTH,
                                                      CONN_MAX_CE_LENGTH);
         app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to set connection timing\n",
                    (int)sc);
         // Start scanning - looking for thermometer devices
         sc = sl_bt_scanner_start(gap_1m_phy, scanner_discover_generic);
         app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start discovery #1\n",
                    (int)sc);
         conn_state = scanning;
         psa_crypto_init();
         break;

   case sl_bt_evt_connection_parameters_id:
      /*placeholder connection parameters updated event*/
      break;
   case sl_bt_evt_gatt_mtu_exchanged_id:
      /*placeholder for gatt mtu exchanged event*/
      break;
   // -------------------------------
   // This event is generated when an advertisement packet or a scan response
   // is received from a slave
   case sl_bt_evt_scanner_scan_report_id:
      // Parse advertisement packets
      if (evt->data.evt_scanner_scan_report.packet_type == 0) {
         if (find_service_in_advertisement(&(evt->data.evt_scanner_scan_report.data.data[0]),
                                           evt->data.evt_scanner_scan_report.data.len,
                                           secure_attest_service,
                                           sizeof(secure_attest_service)) != 0) {
            // then stop scanning for a while
            sc = sl_bt_scanner_stop();
            app_assert(sc == SL_STATUS_OK,
                       "[E: 0x%04x] Failed to stop discovery\n",
                       (int)sc);
            // and connect to that device
            if (active_connections_num < SL_BT_CONFIG_MAX_CONNECTIONS) {
               sc = sl_bt_connection_open(evt->data.evt_scanner_scan_report.address,
                                          evt->data.evt_scanner_scan_report.address_type,
                                          gap_1m_phy,
                                          NULL);
               app_assert(sc == SL_STATUS_OK,
                          "[E: 0x%04x] Failed to connect\n",
                          (int)sc);
               conn_state = opening;
            }
         }
      }
      break;

   // -------------------------------
   // This event is generated when a new connection is established
   case sl_bt_evt_connection_opened_id:
      // Get last two bytes of sender address
      app_log("connection opened\r\n");
      addr_value = (uint16_t)(evt->data.evt_connection_opened.address.addr[1] << 8) + evt->data.evt_connection_opened.address.addr[0];
      // Add connection to the connection_properties array
      add_connection(evt->data.evt_connection_opened.connection, addr_value);
      // would it be better to wait for an encrypted connection to do this?
      sc = sl_bt_gatt_discover_primary_services_by_uuid(evt->data.evt_connection_opened.connection,
                                                        sizeof(secure_attest_service),
                                                        secure_attest_service);
      app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to discover primary services\n",
                    (int)sc);
      conn_state = discover_services;
      sl_bt_sm_increase_security(evt->data.evt_connection_opened.connection);
      break;
   case sl_bt_evt_connection_closed_id:
      app_log("connection closed, reason 0x%2X\r\n",
              evt->data.evt_connection_closed.reason);
      remove_connection(evt->data.evt_connection_opened.connection);
      break;
   // -------------------------------
   // This event is generated when a new service is discovered
   case sl_bt_evt_gatt_service_id:
      table_index = find_index_by_connection_handle(evt->data.evt_gatt_service.connection);
      if (table_index != TABLE_INDEX_INVALID) {
         // Save service handle for future reference
         conn_properties[table_index].secure_attest_service_handle = evt->data.evt_gatt_service.service;
      }
      break;

   // -------------------------------
   // This event is generated when a new characteristic is discovered
   case sl_bt_evt_gatt_characteristic_id:
      table_index = find_index_by_connection_handle(evt->data.evt_gatt_characteristic.connection);
      for(int i = 0; i<nCharacteristics;i++){
         // static uint8_t count;
         if(memcmp(evt->data.evt_gatt_characteristic.uuid.data, characteristic_uuids[i],16) == 0){
            conn_properties[table_index].characteristic_handles[i] = evt->data.evt_gatt_characteristic.characteristic;
            break;
         }
      }
      break;

   // -------------------------------
   // This event is generated for various procedure completions, e.g. when a
   // write procedure is completed, or service discovery is completed
   case sl_bt_evt_gatt_procedure_completed_id:
      gatt_proc_complete_cb(&evt->data.evt_gatt_procedure_completed);
      break;

   // -------------------------------
   // This event is generated when a characteristic value was received e.g. an indication
   case sl_bt_evt_gatt_characteristic_value_id:
   {
      // table_index = find_index_by_connection_handle(evt->data.evt_gatt_characteristic_value.connection);
      int index;
      uint16_t characteristic = evt->data.evt_gatt_characteristic_value.characteristic;
      for(index = 0; index < nCharacteristics;index++){
         if(characteristic == conn_properties[table_index].characteristic_handles[index]){
            /* check to make sure that the callback pointer is valid
             * and call the appropriate handler */
            if(gatt_characteristic_callback[index] != NULL){
               sc = gatt_characteristic_callback[index](&evt->data.evt_gatt_characteristic_value,table_index);
               app_assert(sc==0,"error handling characteristic value %d\r\n", characteristic);
            }
            break;
         }
      }
      if(index == nCharacteristics){
          app_log("unhandled characteristic value %d\r\n", characteristic);
      }
      break;
   }
   // -------------------------------
   // This event is generated when RSSI value was measured
   case sl_bt_evt_connection_rssi_id:
      table_index = find_index_by_connection_handle(evt->data.evt_connection_rssi.connection);
      if (table_index != TABLE_INDEX_INVALID) {
        conn_properties[table_index].rssi = evt->data.evt_connection_rssi.rssi;
      }
      // Print the values
      // print_values(conn_properties);
      break;
   case sl_bt_evt_connection_phy_status_id:
      break;
   case sl_bt_evt_connection_remote_used_features_id:
      break;
   case sl_bt_evt_sm_bonded_id:
      break;
   default:
      app_log_debug("unhandled event 0x%2X\r\n",
                     SL_BT_MSG_ID(evt->header));
      break;
   }
}

// Init connection properties
static void init_properties(void)
{
   uint8_t i;
   active_connections_num = 0;

   for (i = 0; i < SL_BT_CONFIG_MAX_CONNECTIONS; i++) {
      conn_properties[i].connection_handle = CONNECTION_HANDLE_INVALID;
      // conn_properties[i].thermometer_service_handle = SERVICE_HANDLE_INVALID;
      conn_properties[i].secure_attest_service_handle = SERVICE_HANDLE_INVALID;
      //conn_properties[i].thermometer_characteristic_handle = CHARACTERISTIC_HANDLE_INVALID;
      for(int j = 0;j<4;j++){
         conn_properties[i].characteristic_handles[j] = CHARACTERISTIC_HANDLE_INVALID;
      }
      conn_properties[i].temperature = TEMP_INVALID;
      conn_properties[i].unit = UNIT_INVALID;
      conn_properties[i].rssi = RSSI_INVALID;
   }
}

// Parse advertisements looking for advertised Health Thermometer service
static uint8_t find_service_in_advertisement(uint8_t *data, uint8_t len, const uint8_t *service, uint8_t service_len)
{
   uint8_t ad_field_length;
   uint8_t ad_field_type;
   uint8_t i = 0;
   // Parse advertisement packet
   while (i < len) {
      ad_field_length = data[i];
      ad_field_type = data[i + 1];
      // Partial ($02) or complete ($03) list of 16-bit UUIDs
      if (ad_field_type == 0x02 || ad_field_type == 0x03 || ad_field_type == 0x06 ||ad_field_type == 0x07) {
        if (memcmp(&data[i + 2], service, service_len) == 0) {
           return 1;
      }
    }
    // advance to the next AD struct
    i = i + ad_field_length + 1;
   }
   return 0;
}

// Find the index of a given connection in the connection_properties array
static uint8_t find_index_by_connection_handle(uint8_t connection)
{
   for (uint8_t i = 0; i < active_connections_num; i++) {
      if (conn_properties[i].connection_handle == connection) {
         return i;
      }
  }
  return TABLE_INDEX_INVALID;
}

// Add a new connection to the connection_properties array
static void add_connection(uint8_t connection, uint16_t address)
{
   conn_properties[active_connections_num].connection_handle = connection;
   conn_properties[active_connections_num].server_address    = address;
   active_connections_num++;
}

// Remove a connection from the connection_properties array
static void remove_connection(uint8_t connection)
{
   uint8_t i;
   uint8_t table_index = find_index_by_connection_handle(connection);

   if (active_connections_num > 0) {
      active_connections_num--;
   }
   // Shift entries after the removed connection toward 0 index
   for (i = table_index; i < active_connections_num; i++) {
      conn_properties[i] = conn_properties[i + 1];
   }
   // Clear the slots we've just removed so no junk values appear
   for (i = active_connections_num; i < SL_BT_CONFIG_MAX_CONNECTIONS; i++) {
      conn_properties[i].connection_handle = CONNECTION_HANDLE_INVALID;
      // conn_properties[i].thermometer_service_handle = SERVICE_HANDLE_INVALID;
      conn_properties[i].secure_attest_service_handle = SERVICE_HANDLE_INVALID;
      // conn_properties[i].thermometer_characteristic_handle = CHARACTERISTIC_HANDLE_INVALID;
      for(int j= 0; j<4;j++){
         conn_properties[i].characteristic_handles[j] = CHARACTERISTIC_HANDLE_INVALID;
      }
      conn_properties[i].temperature = TEMP_INVALID;
      conn_properties[i].unit = UNIT_INVALID;
      conn_properties[i].rssi = RSSI_INVALID;
   }
}


/**************************************************************************//**
 * @brief
 *   Function to Read and Cache Bluetooth Address.
 * @param address_type_out [out]
 *   A pointer to the outgoing address_type. This pointer can be NULL.
 * @return
 *   Pointer to the cached Bluetooth Address
 *****************************************************************************/
static bd_addr *read_and_cache_bluetooth_address(uint8_t *address_type_out)
{
   static bd_addr address;
   static uint8_t address_type;
   static bool cached = false;

   if (!cached) {
      sl_status_t sc = sl_bt_system_get_identity_address(&address, &address_type);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to get Bluetooth address\n",
                 (int)sc);
      cached = true;
   }

   if (address_type_out) {
      *address_type_out = address_type;
   }

   return &address;
}

/**************************************************************************//**
 * @brief
 *   Function to Print Bluetooth Address.
 * @return
 *   None
 *****************************************************************************/
static void print_bluetooth_address(void)
{
   uint8_t address_type;
   bd_addr *address = read_and_cache_bluetooth_address(&address_type);

   app_log("Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\n",
           address_type ? "static random" : "public device",
           address->addr[5],
           address->addr[4],
           address->addr[3],
           address->addr[2],
           address->addr[1],
           address->addr[0]);
}

/**************************************************************************//**
 * @brief
 *   Function to Set GATT Attribute System ID.
 * @return
 *   None
 *****************************************************************************/
static void set_attribute_system_id(void)
{
   sl_status_t sc;
   uint8_t system_id[8];
   bd_addr *address = read_and_cache_bluetooth_address(NULL);

   // Pad and reverse unique ID to get System ID
   system_id[0] = address->addr[5];
   system_id[1] = address->addr[4];
   system_id[2] = address->addr[3];
   system_id[3] = 0xFF;
   system_id[4] = 0xFE;
   system_id[5] = address->addr[2];
   system_id[6] = address->addr[1];
   system_id[7] = address->addr[0];

   sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id,
                                                0,
                                                sizeof(system_id),
                                                system_id);
   app_assert(sc == SL_STATUS_OK,
              "[E: 0x%04x] Failed to write attribute System ID\n",
              (int)sc);
}


#ifdef SL_CATALOG_CLI_PRESENT
void hello(sl_cli_command_arg_t *arguments)
{
   (void) arguments;
   print_bluetooth_address();
}
void toggle_print_tx_power(sl_cli_command_arg_t *arguments)
{
   (void)arguments;
}
#endif // SL_CATALOG_CLI_PRESENT

/*
 *
 *
 * GATT procedure completed callback
 *
 * */
void gatt_proc_complete_cb(sl_bt_evt_gatt_procedure_completed_t *procedure_completed){
   uint8_t table_index;
   static sl_status_t sc;
   uint32_t flags = 1;
   sl_status_t result;

   table_index = find_index_by_connection_handle(procedure_completed->connection);
   if (table_index == TABLE_INDEX_INVALID) {
       return;
      }

   app_assert(procedure_completed->result == SL_STATUS_OK,
              "GATT procedure completed with error 0x%02X\r\n",
              procedure_completed->result);
   if(conn_properties[table_index].secure_attest_service_handle != SERVICE_HANDLE_INVALID){
      switch(conn_state){
         case discover_services:
            sc = sl_bt_gatt_discover_characteristics(procedure_completed->connection,
                                                     conn_properties[table_index].secure_attest_service_handle);

            app_assert(sc == SL_STATUS_OK,
                       "[E: 0x%04x] Failed to discover characteristics\n",
                       (int)sc);
            conn_state = discover_characteristics;
            break;

         case discover_characteristics:
            sc = sl_bt_gatt_read_characteristic_value(procedure_completed->connection,
                                                  conn_properties[table_index].characteristic_handles[device_certificate]);
            app_assert(sc == SL_STATUS_OK,
                       "[E: 0x%04x] Failed to read device certificate characteristic\n",
                       (int)sc);
            conn_state = read_device_cert;
            break;

          case read_device_cert:
            //printf("reading batch certificate from handle %d\r\n", conn_properties[table_index].characteristic_handles[batch_certificate]);
            sl_bt_scanner_stop();
            sc = sl_bt_gatt_read_characteristic_value(procedure_completed->connection,
                                                      conn_properties[table_index].characteristic_handles[batch_certificate]);
             app_assert(sc == SL_STATUS_OK,
                        "[E: 0x%04x] Failed to read batch certificate characteristic\n",
                        (int)sc);
             conn_state = read_batch_cert;
             break;

           case read_batch_cert:
               sc = sl_bt_gatt_read_characteristic_value(procedure_completed->connection,
                                                         conn_properties[table_index].characteristic_handles[factory_certificate]);
               app_assert(sc == SL_STATUS_OK,
                          "[E: 0x%04x] Failed to read factory certificate characteristic\n",
                          (int)sc);
               conn_state = read_factory_cert;
               break;
            case read_factory_cert:
               sc = sl_bt_gatt_read_characteristic_value(procedure_completed->connection,
                                                         conn_properties[table_index].characteristic_handles[root_certificate]);
               app_assert(sc == SL_STATUS_OK,
                          "[E: 0x%04x] Failed to read root certificate characteristic\n",
                          (int)sc);
               conn_state = read_root_cert;
               break;
            case read_root_cert:
               /*probably need to verify certificate chain here first before sending the challenge.
                               * no reason to challenge if the certificate chain is invalid */
               app_log("Received secure identity certificate chain from server\r\n");
                       mbedtls_x509_crt_init(&cert_chain);
                            /*maybe roll this all into a single wrapper?*/
               result = mbedtls_x509_crt_parse(&cert_chain,
                                           (const uint8_t *)secure_attest_data.device_certificate_buffer,
                                           secure_attest_data.device_cert_actual_size);

               app_assert(result==0,"failed to parse device cert %d\r\n", result);
               result = mbedtls_x509_crt_parse(&cert_chain,
                                               (const uint8_t *)secure_attest_data.batch_certificate_buffer,
                                               secure_attest_data.batch_cert_actual_size);
               app_assert(result==0,"failed to parse batch cert, reason %s\r\n", mbedtls_high_level_strerr(result));

               /* factory */
               result = mbedtls_x509_crt_parse(&cert_chain,
                                              (const uint8_t *)secure_attest_data.factory_certificate_buffer,
                                               secure_attest_data.factory_cert_actual_size);
               app_assert(result==0,"failed to parse factory cert  %s\r\n", mbedtls_high_level_strerr(result));

               /* root */
               mbedtls_x509_crt_init(&root_trust);
               //app_log("parsing root certificate of %d bytes\r\n", secure_attest_data.root_cert_actual_size);
               result = mbedtls_x509_crt_parse(&root_trust,
                                               (const uint8_t *)secure_attest_data.root_certificate_buffer,
                                               secure_attest_data.root_cert_actual_size);
               app_assert(result==0,"failed to parse root cert  %s\r\n", mbedtls_high_level_strerr(result));

               result = check_root_of_trust(&root_pub_pk,&root_trust);
               app_assert(result==0,"server's root certificate does not match public key: reason %d\r\n", result);
               result =  mbedtls_x509_crt_verify(&cert_chain,
                                                 &root_trust,
                                                 NULL,
                                                 NULL,
                                                 &flags,
                                                 verify_callback,
                                                 NULL);
               if(SL_STATUS_OK == result){
                  /*generate a random number and get the remote device to sign it*/
                  generate_random_number(secure_attest_data.challenge,sizeof(secure_attest_data.challenge));
                  app_log("sending challenge to server\r\n");
                  sl_bt_gatt_write_characteristic_value(procedure_completed->connection,
                                                        conn_properties[table_index].characteristic_handles[challenge],
                                                        sizeof(secure_attest_data.challenge),
                                                        secure_attest_data.challenge);
                  result = save_peer_pub_key(&cert_chain.pk, &server_device_pub_key);
                  app_assert(result==0,"saving public key from certificate failed\r\n");
                  conn_state = send_challenge;
               }
               else{ /* If the certificate chain is invalid, close the connection*/
                  app_log("certificate chain verification failed ... closing connection\r\n");
                  sl_bt_connection_close(conn_properties[table_index].connection_handle);
               }
               app_assert(sc == SL_STATUS_OK,
                          "[E: 0x%04x] Failed to read batch certificate characteristic\n",
                          (int)sc);
               conn_state = send_challenge;
               break;
            case send_challenge:
               sc = sl_bt_gatt_read_characteristic_value(procedure_completed->connection,
                                                         conn_properties[table_index].characteristic_handles[challenge_response]);
               app_assert(sc==0,"error reading challenge response characteristic\r\n");
               conn_state = wait_challenge_response;
               break;
            case wait_challenge_response:
            {
               uint8_t hash[32];
               size_t bytes_written;
               /*verify the response*/
               psa_hash_compute(PSA_ALG_SHA_256,secure_attest_data.challenge,
                                sizeof(secure_attest_data.challenge),hash,sizeof(hash),&bytes_written);
               sc = psa_verify_hash(server_device_pub_key,
                                    PSA_ALG_ECDSA(PSA_ALG_SHA_256),
                                    hash,sizeof(hash),
                                    secure_attest_data.response,
                                    sizeof(secure_attest_data.response));
               if(PSA_SUCCESS==sc){
                  app_log("challenge response is OK. Device appears to be authentic.\r\n");
                  app_log("sending certificate chain to the server\r\n");
                  sc = send_cert(procedure_completed->connection,
                                 conn_properties[table_index].characteristic_handles[local_device_certificate],
                                 SL_SE_CERT_DEVICE_HOST);
                  conn_state = send_device_cert;
               }
               else{
                  app_log("challenge response verification failed reason %d. closing connection\r\n",
                          sc);
                  sl_bt_connection_close(procedure_completed->connection);
               }
            }
            break;
            case send_device_cert:
               if(sc == SL_STATUS_IS_WAITING){
                  sc = send_cert(procedure_completed->connection,
                                 conn_properties[table_index].characteristic_handles[local_device_certificate],
                                 SL_SE_CERT_DEVICE_HOST);
               }
               else if(sc == SL_STATUS_OK){
                  sc = send_cert(procedure_completed->connection,
                                 conn_properties[table_index].characteristic_handles[local_batch_certificate],
                                 SL_SE_CERT_BATCH);
                  conn_state = send_batch_cert;
               }
               break;
        case send_batch_cert:
            if(sc == SL_STATUS_IS_WAITING){
               sc = send_cert(procedure_completed->connection,
                              conn_properties[table_index].characteristic_handles[local_batch_certificate],
                              SL_SE_CERT_BATCH);
               break;
            }
            else if(sc == SL_STATUS_OK){
               sc = send_cert(procedure_completed->connection,
                              conn_properties[table_index].characteristic_handles[local_factory_certificate],
                              FACTORY_CERT_ID/*factory cert goes here*/);
                conn_state = send_factory_cert;
            }
            break;
         case send_factory_cert:
         {
            if(sc == SL_STATUS_IS_WAITING){
               sc = send_cert(procedure_completed->connection,
                              conn_properties[table_index].characteristic_handles[local_factory_certificate],
                              FACTORY_CERT_ID/*factory cert goes here*/);
            }
            else if(sc == SL_STATUS_OK){
               sc = send_cert(procedure_completed->connection,
                              conn_properties[table_index].characteristic_handles[local_root_certificate],
                              ROOT_CERT_ID/*root cert goes here*/);
               conn_state = send_root_cert;
            }
         }
         break;
         case send_root_cert:
         {
            if(sc == SL_STATUS_IS_WAITING){
                sc = send_cert(procedure_completed->connection,
                               conn_properties[table_index].characteristic_handles[local_root_certificate],
                               ROOT_CERT_ID/*root cert goes here*/);
            }
            else if(sc == SL_STATUS_OK){
                uint16_t server_challenge_characteristic =
                    conn_properties[table_index].characteristic_handles[server_challenge];
                if(server_challenge_characteristic!=CHARACTERISTIC_HANDLE_INVALID){
                   sc = sl_bt_gatt_set_characteristic_notification(procedure_completed->connection,
                                                              server_challenge_characteristic,
                                                              sl_bt_gatt_notification);
                   if(sc){
                       app_log("error subscribing to challenge notifications\r\n");
                   }
                }
                conn_state = challenge_notification_enabled;

            }
          }
          break;
         case challenge_notification_enabled:
            /*need to wait until previous operation completes before sending challenge response*/
            break;

         case send_challenge_response:
         {
            uint16_t server_challenge_characteristic = conn_properties[table_index].characteristic_handles[server_challenge];
            /*turn off notifications since we don't need them anymore*/
            sl_bt_gatt_set_characteristic_notification(procedure_completed->connection,
                                                       server_challenge_characteristic,
                                                       sl_bt_gatt_disable);
            conn_state = read_server_ecdh_public_key;
         }
         break;
         case read_server_ecdh_public_key:
            app_log("reading server ECDH key\r\n");
            sl_bt_gatt_read_characteristic_value(procedure_completed->connection,
                                                 conn_properties[table_index].characteristic_handles[server_ecdh_pub_key]);
            conn_state =  send_client_ecdh_public_key;
            break;
         case send_client_ecdh_public_key:
         {
            struct signed_key_t client_key;
            sl_status_t status;

            app_log("client signing ECDH key\r\n");
            signPublicKey(&client_ecdh_id,&client_key);
            status = sl_bt_gatt_write_characteristic_value(procedure_completed->connection,
                                                           conn_properties[table_index].characteristic_handles[client_ecdh_pub_key],
                                                           sizeof(client_key),
                                                           (const uint8_t *) &client_key);
            app_assert(status==0,"failed to write characteristic, 0x%2X\r\n",status);
            app_log("client finalizing key agreement\r\n");
            status = finalize_key_agreement(client_ecdh_id,
                                            server_device_pub_key,
                                            &peer_key_material/*need the key material from the server here*/,
                                            &share_derived_key_id);
            app_assert(0==status,"complete_key_agreement(): returns %d\r\n", status);
            app_log("ECDH key OK\r\n");

            conn_state = running;
         }
         break;
         case running:
            result = sl_bt_gatt_read_characteristic_value(procedure_completed->connection,
                                                          conn_properties[table_index].characteristic_handles[test_data]);
            app_assert(0==result,"sl_bt_gatt_read_characteristic_value(): returns %d\r\n", result);
            conn_state = idle;
            break;
         case opening:
         case wait_for_challenge:
         case idle:
         default:
            break;
        }
      }
}

/*
 *
 * send  a certificate to a remote device
 *
 * If there are more bytes to send than an MTU,
 *
 * */

sl_status_t send_cert(uint8_t connection, uint16_t characteristic, uint8_t cert_id)
{
   static uint8_t cert_buffer[768];
   uint8_t send_buffer[255];
   static size_t cert_size = 0;
   sl_status_t status, write_status;
   sl_se_command_context_t cmd_ctx;
   sl_se_cert_size_type_t cert_sizes;
   static size_t bytes_to_send, offset = 0;
   static uint16_t mtu;
   if(cert_size==0){
      sl_bt_gatt_server_get_mtu(connection,&mtu);
      //app_log("mtu size %d bytes\r\n", mtu);
      if(cert_id < 0x10){
         cert_size = sl_se_read_cert_size(&cmd_ctx,&cert_sizes);//get_cert_size(cert_id);
         if(cert_id==SL_SE_CERT_DEVICE_HOST){
            cert_size = cert_sizes.host_id_size;
         }
         else if(cert_id==SL_SE_CERT_BATCH){
            cert_size = cert_sizes.batch_id_size;
         }
         status = sl_se_read_cert(&cmd_ctx,
                                  cert_id,
                                  cert_buffer,
                                  cert_size);
         if(status){
            app_log("error reading certificate id %d is 0x%2X\r\n", cert_id,status);
            return status;
         }
      }
      else if(cert_id == FACTORY_CERT_ID){
         pem_to_buffer_with_size(FACTORY_CERT,cert_buffer,&cert_size);
      }
      else if(cert_id == ROOT_CERT_ID){
         pem_to_buffer_with_size(ROOT_CERT,cert_buffer,&cert_size);
      }
   }

   if(cert_size>mtu - 6U){
      bytes_to_send = mtu-6;
      send_buffer[0] = 1;
      status = SL_STATUS_IS_WAITING;
   }
   else {
      bytes_to_send = cert_size;
      send_buffer[0] = 0;
      status = SL_STATUS_OK;
   }

   memcpy(send_buffer+1,cert_buffer+offset,bytes_to_send);
   write_status = sl_bt_gatt_write_characteristic_value(connection,
                                                        characteristic,
                                                        bytes_to_send+1,
                                                        send_buffer);
   if(write_status){
      app_log("error 0x%.2X writing to characteristic handle %d\r\n",
              write_status,
              characteristic);
      return status;
   }else{
      cert_size -= bytes_to_send -1;
      offset += bytes_to_send;
   }
   if(status == SL_STATUS_OK){
      offset = 0;
      bytes_to_send = 0;
      send_buffer[0] = 0;
      cert_size = 0;
      memset(cert_buffer,768,0);
   }
   return status;
}

/* end of file */
