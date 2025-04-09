/***************************************************************************//**
 * @file
 * @brief Application interface provided to main().
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef APP_H
#define APP_H
#include <stdint.h>
#include <stddef.h>

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void);

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void);

typedef enum {
  scanning,
  opening,
  discover_services,
  discover_characteristics,
  // enable_indication,
  read_device_cert,
  read_batch_cert,
  read_factory_cert,
  read_root_cert,
  send_challenge,
  wait_challenge_response,
  send_batch_cert,
  send_device_cert,
  send_factory_cert,
  send_root_cert,
  wait_for_challenge,
  challenge_notification_enabled,
  send_challenge_response,
  read_server_ecdh_public_key,
  send_client_ecdh_public_key,
  running,
  idle
} conn_state_t;

enum {device_certificate,
      batch_certificate,
      factory_certificate,
      root_certificate,
      challenge,
      challenge_response,
      local_device_certificate,
      local_batch_certificate,
      local_factory_certificate,
      local_root_certificate,
      server_challenge,
      server_challenge_response,
      server_ecdh_pub_key,
      client_ecdh_pub_key,
      test_data,
      nCharacteristics};

typedef struct {
  uint8_t  connection_handle;
  int8_t   rssi;
  uint16_t server_address;
  uint32_t secure_attest_service_handle;
  uint16_t characteristic_handles[nCharacteristics];
  float    temperature;
  char     unit;
} conn_properties_t;

typedef struct {
  uint8_t mantissa_l;
  uint8_t mantissa_m;
  int8_t mantissa_h;
  int8_t exponent;
} IEEE_11073_float;

struct secure_attest_data_t {
  uint8_t device_certificate_buffer[512];
  size_t  device_cert_actual_size;
  uint8_t batch_certificate_buffer[512];
  size_t  batch_cert_actual_size;
  uint8_t factory_certificate_buffer[512];
  size_t  factory_cert_actual_size;
  uint8_t root_certificate_buffer[512];
  size_t  root_cert_actual_size;
  uint8_t challenge[16];
  uint8_t response[64];
};

/* Application config section*/
#define USE_CUSTOM_CERTIFICATES        1
// connection parameters
#define CONN_INTERVAL_MIN              80  // 100ms
#define CONN_INTERVAL_MAX              80  // 100ms
#define CONN_SLAVE_LATENCY             0   // no latency
#define CONN_TIMEOUT                   100 // 1000ms
#define CONN_MIN_CE_LENGTH             0
#define CONN_MAX_CE_LENGTH             0xffff

#define SCAN_INTERVAL                  16  // 10ms
#define SCAN_WINDOW                    16  // 10ms
#define SCAN_PASSIVE                   0

#define TEMP_INVALID                   NAN
#define UNIT_INVALID                   ('?')
#define UNIT_CELSIUS                   ('C')
#define UNIT_FAHRENHEIT                ('F')
#define RSSI_INVALID                   ((int8_t)0x7F)
#define CONNECTION_HANDLE_INVALID      ((uint8_t)0xFF)
#define SERVICE_HANDLE_INVALID         ((uint32_t)0xFFFFFFFF)
#define CHARACTERISTIC_HANDLE_INVALID  ((uint16_t)0xFFFF)
#define TABLE_INDEX_INVALID            ((uint8_t)0xFF)
#define ROOT_CERT_ID                   0x81
#define FACTORY_CERT_ID                0x82

/* secure attestation services UUIDs*/
#define SERVER_DEVICE_CERT_UUID        { 0xfc, 0x06, 0x75, 0xeb, 0x42, 0x8d, \
                                         0x14, 0x77, 0xd4, 0x0a, 0x76, 0xc6, \
                                         0x64, 0xf0, 0xb6, 0x15 }
#define SERVER_BATCH_CERT_UUID         { 0xd7, 0x32, 0xe3, 0x0d, 0x87, 0x87, \
                                         0x69, 0xd5, 0xc2, 0x82, 0x46, 0x40, \
                                         0x76, 0xa2, 0x88, 0x73 }
#define SERVER_FACTORY_CERT_UUID       { 0x4e, 0x67, 0xbe, 0x48, 0x0e, 0x89, \
                                         0x10, 0xa6, 0x9e, 0x4f, 0xa3, 0xe1, \
                                         0xd3, 0xd3, 0xc0, 0x82 }
#define SERVER_ROOT_CERT_UUID          { 0x3c, 0xa1, 0x8c, 0x61, 0x37, 0x5c, \
                                         0xfc, 0x83, 0xb8, 0x4d, 0xf8, 0xb1, \
                                         0x36, 0x7c, 0x54, 0x5e }
#define CLIENT_CHALLENGE_UUID          { 0xdc, 0x59, 0xaf, 0x67, 0x05, 0x33, \
                                         0xe4, 0x04, 0x85, 0xa1, 0x39, 0xeb, \
                                         0x22, 0x06, 0x61, 0x7b }
#define CLIENT_CHALLENGE_RESPONSE_UUID { 0xeb, 0xf6, 0x58, 0x7a, 0xa1, 0x91, \
                                         0x65, 0x65, 0x11, 0x35, 0x64, 0x1a, \
                                         0x38, 0xf6, 0x33, 0xad }
#define CLIENT_DEVICE_CERT_UUID        { 0xdd, 0x35, 0xa2, 0x08, 0x47, 0x40, \
                                         0x09, 0xb2, 0xb3, 0x4c, 0x04, 0xe6, \
                                         0x4c, 0x0b, 0xd3, 0xaa }
#define CLIENT_BATCH_CERT_UUID         { 0x78, 0x8f, 0xe8, 0x69, 0x21, 0x7b, \
                                         0x28, 0xb2, 0xa5, 0x4d, 0x80, 0xa5, \
                                         0x1a, 0xbc, 0x0a, 0x33 }
#define CLIENT_FACTORY_CERT_UUID       { 0xd8, 0x34, 0xe4, 0x0b, 0xa4, 0xb3, \
                                         0x3b, 0x8f, 0xca, 0x42, 0xdb, 0xa3, \
                                         0x9c, 0x9a, 0x8d, 0x54 }
#define CLIENT_ROOT_CERT_UUID          { 0x92, 0xfc, 0xf4, 0xa2, 0xa9, 0xe2, \
                                         0xed, 0xaa, 0xc5, 0x43, 0x82, 0x4f, \
                                         0xdc, 0xc4, 0x63, 0xcf }
#define SERVER_CHALLENGE_UUID          { 0x9a, 0x4c, 0x1f, 0xbe, 0x80, 0xe6, \
                                         0xd2, 0xb4, 0xa8, 0x49, 0x1c, 0xd1, \
                                         0x68, 0xd3, 0x24, 0x47 }
#define SERVER_CHALLENGE_RESPONSE_UUID { 0x95, 0x4d, 0x0e, 0x1b, 0x19, 0x87, \
                                         0x5b, 0xa4, 0xe2, 0x40, 0x09, 0x15, \
                                         0x15, 0x7e, 0xa6, 0xd7 }
#define SERVER_ECDH_PUBLIC_KEY_UUID    { 0xf7, 0x4c, 0x84, 0x53, 0x06, 0x56, \
                                         0x89, 0xac, 0xd2, 0x44, 0x3c, 0xaa, \
                                         0x34, 0xd8, 0x3a, 0xb5 }
#define CLIENT_ECDH_PUBLIC_KEY_UUID    { 0x78, 0x92, 0xb3, 0xbd, 0x97, 0x99, \
                                         0xec, 0x97, 0xf9, 0x45, 0x4e, 0xd7, \
                                         0x80, 0x66, 0x41, 0xbe }
#define TEST_DATA_UUID                 { 0x6e, 0x1e, 0x74, 0x11, 0xe3, 0xef, \
                                         0x19, 0xbd, 0xb6, 0x40, 0x87, 0xae, \
                                         0x7a, 0xbe, 0xdf, 0xc9 }

extern const uint8_t factory[];
extern const uint8_t root[];
#define FACTORY_CERT                   factory
#define ROOT_CERT                      root
#endif // APP_H
