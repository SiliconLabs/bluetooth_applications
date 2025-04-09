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
#include "em_rmu.h"
#include "em_msc.h"
#include "nvm3.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "app_mbedtls_x509.h"
#include "mbedtls/pk.h"
#include "mbedtls/pem.h"
#include "mbedtls/error.h"
#include "ecdh_util.h"
#include "psa/crypto.h"
#include "sl_sleeptimer.h"

psa_key_id_t private_device_key = 0x8001;

const uint8_t sl_root_pub_key[] =
  "-----BEGIN PUBLIC KEY-----\r\n"
  "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE0Cnl/5yv6/3PF8xAwmPEvFqp0FZ7\r\n"
  "idOsLAeKEe3FhkNixIGB3NndUVS7TFOU/Tt1ay4Iv31pvOVDKpqT611NLQ==\r\n"
  "-----END PUBLIC KEY-----\r\n";

const uint8_t server_private_key[] =
  "-----BEGIN PRIVATE KEY-----\n"
  "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgKNHzn4WP9nsP7q//\n"
  "gyLTfZb5247+SmfI013/LZzuqbmhRANCAATxEaD6th33+JwKHBcNhCxoo5BsW+4C\n"
  "lVjNqjzo1wQoY+mg+XRXozHThLZ5DjGY3VwFNufSgRQOwcIP6pL6WbXe\n"
  "-----END PRIVATE KEY-----\n";

/****    private data****/
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static bool challenge_valid = false;
// and some other stuff
static uint8_t device_attestation_signature[64];
static uint16_t mtu;
static mbedtls_pk_context peer_pubkey;
static uint8_t challenge[CHALLENGE_SIZE];
static mbedtls_pk_context client_root_public_key;
static psa_key_id_t peer_device_pub_key_id,
                    server_ecdh_id,
                    share_derived_key_id;

/**************************************************************************//**
 * Private functions declaration
 *****************************************************************************/
static sl_status_t user_read_request_cb(
  sl_bt_evt_gatt_server_user_read_request_t *request);

static sl_status_t gatt_server_user_write_request_cb(
  sl_bt_evt_gatt_server_user_write_request_t *user_write_request);

sl_status_t pem_to_buffer_with_size(const uint8_t *pem_str,
                                    uint8_t *out,
                                    size_t *size);

static void data_cleanup(void);

static int verify_callback(void *data,
                           mbedtls_x509_crt *crt,
                           int depth,
                           uint32_t *flags);

static bool check_key_initialized(psa_key_id_t key);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////

  sl_status_t sc;
  uint8_t key_buffer[32];
  size_t bytes_written;
  MSC_Status_TypeDef mscStatus;

  mbedtls_pk_init(&peer_pubkey);

  /*import the Client's public key for it's root certificate*/
  mbedtls_pk_init(&client_root_public_key);
  mbedtls_pk_parse_public_key(&client_root_public_key,
                              sl_root_pub_key,
                              sizeof(sl_root_pub_key));
#ifdef USE_CUSTOM_CERTIFICATES

  /* check to see if key is already imported. If it is, skip.
   * Otherwise import the key and zero the PEM string*/
  app_log("initializing private device key\r\n");

  sc = check_key_initialized(private_device_key);
  if (sc == false) {
    sc = import_pem_key((uint8_t *)server_private_key,
                        sizeof(server_private_key),
                        &private_device_key,
                        PSA_KEY_LIFETIME_PERSISTENT);
    app_assert_status(sc);
    memset(key_buffer, 0, sizeof(key_buffer));
    mscStatus = MSC_WriteWord((uint32_t *)server_private_key, key_buffer, 32);
    if (0 != mscStatus) {
      app_log("warning: private device key could not be erased\r\n");
    }
  } else {
    app_log("private_device_key already initialized\r\n");

    /* test exportability of the private key he result should be '-133'*/
    sc = psa_export_key(private_device_key,
                        key_buffer,
                        sizeof(key_buffer),
                        &bytes_written);
    app_log("the result of attempting to export the private key is %ld\r\n",
            sc);
  }
#endif
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
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
      app_log("\r\n------ Silicon Labs BLE Application Layer Encryption "
              "Demo: Server Role-----\r\n");

      /* recommend replacing with sleep"timer*/
      sc = sl_bt_sm_delete_bondings();
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

      // configure security
      sc = sl_bt_sm_configure(0, sl_bt_sm_io_capability_noinputnooutput);
      app_assert_status(sc);
      sc = sl_bt_sm_set_bondable_mode(1);
      app_assert_status(sc);
      sc = sl_bt_sm_store_bonding_configuration(4, 0x2);
      app_assert_status(sc);

      // Start general advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log("connection opened\r\n");
      sl_bt_sm_increase_security(evt->data.evt_connection_opened.connection);
      break;

    // -------------------------------
    case sl_bt_evt_gatt_mtu_exchanged_id:
      mtu = evt->data.evt_gatt_mtu_exchanged.mtu;
      break;

    case sl_bt_evt_gatt_server_user_read_request_id:
      user_read_request_cb(&evt->data.evt_gatt_server_user_read_request);
      break;

    case sl_bt_evt_gatt_server_user_write_request_id:
      sc = gatt_server_user_write_request_cb(
        &evt->data.evt_gatt_server_user_write_request);
      if (SL_STATUS_OK != sc) {
        app_log("error in gatt_server_user_write_request_cb(): %ld, "
                "on handle %d\r\n",
                sc,
                evt->data.evt_gatt_server_user_write_request.characteristic);
      }
      break;

    case sl_bt_evt_gatt_server_attribute_value_id:
      /*placeholder for gatt_server_attribute_value event handler*/
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (evt->data.evt_gatt_server_characteristic_status.characteristic
          == gattdb_peer_challenge) {
        if ((evt->data.evt_gatt_server_characteristic_status.status_flags
             == sl_bt_gatt_server_client_config)
            && (evt->data.evt_gatt_server_characteristic_status.
                client_config_flags > 0)) {
          sc = sl_bt_gatt_server_send_notification(
            evt->data.evt_gatt_server_characteristic_status.connection,
            gattdb_peer_challenge,
            sizeof(challenge),
            challenge);
          if (SL_STATUS_OK == sc) {
            app_log("Sending random challenge to Client\r\n");
          } else {
            app_log("error sending indication %ld\r\n", sc);
          }
        }
      }
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log("connection closed, reason 0x%2X\r\n",
              evt->data.evt_connection_closed.reason);
      sc = sl_bt_sm_delete_bondings();
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      data_cleanup();
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    case sl_bt_evt_connection_parameters_id:
      /*optional event handler code here*/
      break;

    case sl_bt_evt_connection_phy_status_id:
      /*optional event handler code here*/
      break;

    case sl_bt_evt_connection_remote_used_features_id:
      /*optional event handler code here*/
      break;

    case sl_bt_evt_sm_bonded_id:
      app_log("sm_bonded event: bond handle %d and security mode %d\r\n",
              evt->data.evt_sm_bonded.bonding,
              evt->data.evt_sm_bonded.security_mode + 1);
      break;

    case sl_bt_evt_system_error_id:
      app_log("system error 0x%4X\r\n",
              evt->data.evt_system_error.reason);
      break;

    case sl_bt_evt_sm_bonding_failed_id:
      app_log("bonding failed, reason 0x%4X\r\n",
              evt->data.evt_sm_bonding_failed.reason);
      break;

    // -------------------------------
    // Default event handler.
    default:
      app_log("unhandled event 0x%2lX\r\n", SL_BT_MSG_ID(evt->header));
      break;
  }
}

static void data_cleanup(void)
{
  challenge_valid = false;
  memset(challenge, 0, sizeof(challenge));
  mtu = 0;
}

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

  /*printf("  + Verify requested for (Depth %d) ... OK\n", depth);
   *    for (i = 0; i < ret; i++) {
   *    printf("%c", buf[i]);
   *    }*/
#else
  mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "      ", crt);
#endif

  if (depth == 3) {
    p = strstr((const char *)buf, "subject");
    q = strstr((const char *)p, "O=Silicon Labs");
    r = strstr((const char *)q, "issued");
    if ((q > p) && (q < r)) {
      app_log(
        "Certificate subject includes Silicon Labs Inc - appears to be valid\r\n");
    } else {
      app_log("Certificate subject does not include Silicon Labs Inc\r\n");
      app_log_append(p);
    }
  }
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

static bool check_key_initialized(psa_key_id_t key)
{
  psa_key_attributes_t private_device_key_attributes;
  psa_key_type_t type;
  size_t key_bits;
  psa_key_usage_t usage;
  psa_algorithm_t algo;

  private_device_key_attributes = psa_key_attributes_init();
  psa_get_key_attributes(key, &private_device_key_attributes);
  usage = psa_get_key_usage_flags(&private_device_key_attributes);
  type = psa_get_key_type(&private_device_key_attributes);
  key_bits = psa_get_key_bits(&private_device_key_attributes);
  algo = psa_get_key_algorithm(&private_device_key_attributes);

  /*if any of the usage, type, bits or algorithm are zero, then the key is not
   *   valid*/
  return (usage != 0 && type != 0 && key_bits != 0 && algo != 0);
}

// bytes_to_send = cert_size;
static sl_status_t user_read_request_cb(
  sl_bt_evt_gatt_server_user_read_request_t *request)
{
  uint8_t buffer[512];
  size_t bytes_to_send = 0;
  uint16_t sent_bytes = 0;
  sl_status_t sc;

#ifndef USE_CUSTOM_CERTIFICATES
  sl_se_command_context_t cmd_ctx;
  sl_se_cert_type_t cert_type;

  sl_se_cert_size_type_t cert_size_buffer;
  sl_se_read_cert_size(&cmd_ctx, &cert_size_buffer);
#endif

  switch (request->characteristic) {
    case gattdb_batch_cert:
      if (request->offset == 0) {
        app_log("sending device identity certificate chain to client\r\n");
      }
#ifdef USE_CUSTOM_CERTIFICATES
      pem_to_buffer_with_size(custom_batch_certificate,
                              buffer,
                              &bytes_to_send);

#else
      cert_type = SL_SE_CERT_BATCH;
      cert_size = cert_size_buffer.batch_id_size;
      sc = sl_se_read_cert(&cmd_ctx,
                           cert_type,
                           buffer,
                           cert_size);
#endif
      break;

    case gattdb_device_cert:
    {
#ifdef USE_CUSTOM_CERTIFICATES
      pem_to_buffer_with_size(custom_device_certificate, buffer,
                              &bytes_to_send);

#else
      cert_type = SL_SE_CERT_DEVICE_HOST;
      cert_size = cert_size_buffer.host_id_size;
      sc = sl_se_read_cert(&cmd_ctx,
                           cert_type,
                           buffer,
                           cert_size);
#endif
    }
    break;

    case gattdb_factory_cert:
      sc = pem_to_buffer_with_size(custom_factory_certificate,
                                   buffer, &bytes_to_send);
      if (SL_STATUS_OK != sc) {
        app_log("error parsing factory certificate %ld\r\n", sc);
      }
      break;

    case gattdb_root_cert:
      sc = pem_to_buffer_with_size(custom_root_certificate,
                                   buffer,
                                   &bytes_to_send);
      if (SL_STATUS_OK != sc) {
        app_log("error parsing root certificate %ld\r\n", sc);
      }
      break;

    case gattdb_device_attestation_challenge_response:
      if (sizeof(device_attestation_signature) > sizeof(buffer)) {
        return SL_STATUS_WOULD_OVERFLOW;
      }
      memcpy(buffer,
             device_attestation_signature,
             sizeof(device_attestation_signature));
      bytes_to_send = sizeof(device_attestation_signature);
      sc = SL_STATUS_OK;
      break;

    case gattdb_server_pubkey:
    {
      struct signed_key_t server_pubkey;
      sl_status_t sc;

      if (challenge_valid == false) {
        app_log("attempt to key agreement without valid device attestation\r\n");
        return SL_STATUS_NOT_READY;
      }
      sc = signPublicKey(&server_ecdh_id, &server_pubkey);
      app_assert_status(sc);
      app_log("ECDH key signed OK\r\n");
      if (sizeof(server_pubkey) > sizeof(buffer)) {
        return SL_STATUS_WOULD_OVERFLOW;
      }
      memcpy(buffer, &server_pubkey, sizeof(server_pubkey));
      bytes_to_send = sizeof(server_pubkey);
      sc = SL_STATUS_OK;
    }
    break;

    case gattdb_test_data:
    {
      const uint8_t plaintext[] = "Silabs success";
      static uint8_t ciphertext[32];
      uint8_t nonce[13];
      uint8_t additional[16] = "";
      sl_status_t sc;
      size_t bytes_written;
      psa_algorithm_t alg = PSA_ALG_CCM;

      sc = psa_generate_random(nonce, sizeof(nonce));
      app_assert_status(sc);
      sc = psa_aead_encrypt(share_derived_key_id,
                            alg,
                            nonce, sizeof(nonce),
                            additional, 0,
                            plaintext, sizeof(plaintext),
                            ciphertext, sizeof(ciphertext),
                            &bytes_written);
      app_assert_status(sc);
      app_log("sending encrypted message\r\n");
      if (sizeof(nonce) + sizeof(ciphertext) > sizeof(buffer)) {
        return SL_STATUS_WOULD_OVERFLOW;
      }
      memcpy(buffer, nonce, sizeof(nonce));
      memcpy(buffer + sizeof(nonce), ciphertext, bytes_written);
      bytes_to_send = sizeof(nonce) + bytes_written;
    }
    break;

    default:
      sc = SL_STATUS_BT_ATT_INVALID_HANDLE;
      app_log("attempt to read invalid handle %d\r\n",
              request->characteristic);
      memset(buffer, 0, sizeof(buffer));
      bytes_to_send = 0;
      break;
  }

  bytes_to_send -= request->offset;
  if (bytes_to_send > mtu - 1U) { // replace with mtu
    bytes_to_send = mtu - 1U;
  }
  sc = sl_bt_gatt_server_send_user_read_response(request->connection,
                                                 request->characteristic,
                                                 SL_STATUS_OK,
                                                 bytes_to_send,
                                                 &buffer[request->offset],
                                                 &sent_bytes);
  return sc;
}

sl_status_t save_certificate_chunk(
  sl_bt_evt_gatt_server_user_write_request_t *attribute_value,
  uint8_t *cert_buffer,
  size_t *cert_size)
{
  size_t copy_size = attribute_value->value.len - 1;
  static uint16_t offset = 0;
  static size_t size = 0;

  /*if this is the first segment, offset is zero. Otherwise
   * offset must attribute_value->offset-1 because we discard the first byte*/
  if (attribute_value->offset + attribute_value->value.len
      > CERTIFICATE_BUFFER_SIZE) {
    app_log("received data larger than buffer");
    return SL_STATUS_WOULD_OVERFLOW;
  } else {
    memcpy(cert_buffer + offset,
           attribute_value->value.data + 1,
           copy_size);
    offset += copy_size;
    size += copy_size;
  }
  if (attribute_value->value.data[0] == 0) {
    *cert_size = size;
    size = 0;
    offset = 0;
    return SL_STATUS_OK;
  }
  return SL_STATUS_IN_PROGRESS;
}

static sl_status_t gatt_server_user_write_request_cb(
  sl_bt_evt_gatt_server_user_write_request_t *user_write_request)
{
  static uint8_t cert_buffer[CERTIFICATE_BUFFER_SIZE];
  static bool peer_device_cert_complete = false;
  static bool peer_batch_cert_complete = false;
  static bool peer_factory_cert_complete = false;
  static bool peer_root_cert_complete = false;
  static mbedtls_x509_crt cert_chain;
  static mbedtls_x509_crt root_trust;
  size_t cert_size;
  sl_status_t sc = SL_STATUS_FAIL, result;
  uint32_t flags = 1;

  switch (user_write_request->characteristic) {
    /* first byte indicates whether more data to follow or not*/
    case gattdb_peer_device_cert:
      sc = save_certificate_chunk(user_write_request,
                                  cert_buffer,
                                  &cert_size);
      if (SL_STATUS_OK == sc) {
        peer_device_cert_complete = true;
        mbedtls_x509_crt_init(&cert_chain);
        sc = mbedtls_x509_crt_parse_der(&cert_chain,
                                        cert_buffer,
                                        cert_size);
        if (sc) {
          app_log("error parsing device certificate %s %s\r\n",
                  mbedtls_high_level_strerr(sc),
                  mbedtls_low_level_strerr(sc));
          cert_size = 0;
          return sc;
        }
      }
      break;
    case gattdb_peer_batch_cert:
      sc = save_certificate_chunk(user_write_request,
                                  cert_buffer,
                                  &cert_size);
      if (SL_STATUS_OK == sc) {
        peer_batch_cert_complete = true;
        sc = mbedtls_x509_crt_parse_der(&cert_chain,
                                        cert_buffer,
                                        cert_size);
        if (sc) {
          app_log("error parsing batch certificate %s %s\r\n",
                  mbedtls_high_level_strerr(sc),
                  mbedtls_low_level_strerr(sc));
          cert_size = 0;
          return sc;
        }
      }
      break;
#ifdef USE_CUSTOM_CERTIFICATES
    case gattdb_peer_factory_cert:
      sc = save_certificate_chunk(user_write_request,
                                  cert_buffer,
                                  &cert_size);
      if (SL_STATUS_OK == sc) {
        peer_factory_cert_complete = true;
        result = mbedtls_x509_crt_parse_der(&cert_chain,
                                            cert_buffer,
                                            cert_size);
        if (result) {
          app_log("error parsing factory certificate %s %s\r\n",
                  mbedtls_high_level_strerr(result),
                  mbedtls_low_level_strerr(result));
          cert_size = 0;
          return result;
        }
      }
      break;
    case gattdb_peer_root_cert:
      sc = save_certificate_chunk(user_write_request,
                                  cert_buffer,
                                  &cert_size);
      if (SL_STATUS_OK == sc) {
        peer_root_cert_complete = true;
        mbedtls_x509_crt_init(&root_trust);
        result = mbedtls_x509_crt_parse_der(&root_trust,
                                            cert_buffer,
                                            cert_size);
        if (result) {
          app_log("error parsing root certificate %s %s\r\n",
                  mbedtls_high_level_strerr(result),
                  mbedtls_low_level_strerr(result));
          cert_size = 0;
          return result;
        }
      }
      break;
#endif
    case gattdb_client_pubkey:
    {
      struct signed_key_t peer_key_material;
      sl_status_t sc;
      if (user_write_request->value.len > sizeof(peer_key_material)) {
        return SL_STATUS_WOULD_OVERFLOW;
      }
      memcpy(&peer_key_material,
             user_write_request->value.data,
             user_write_request->value.len);
      sc = finalize_key_agreement(server_ecdh_id,
                                  peer_device_pub_key_id,
                                  &peer_key_material,
                                  &share_derived_key_id);
      app_assert_status(sc);
    }
    break;
    case gattdb_peer_challenge_response:
    {
      uint8_t hash[32];
      size_t bytes_written;

      psa_hash_compute(PSA_ALG_SHA_256, challenge,
                       sizeof(challenge),
                       hash,
                       sizeof(hash),
                       &bytes_written);
      result = psa_verify_hash(peer_device_pub_key_id,
                               PSA_ALG_ECDSA(PSA_ALG_SHA_256),
                               hash, sizeof(hash),
                               user_write_request->value.data,
                               user_write_request->value.len);
      if (PSA_SUCCESS == result) {
        challenge_valid = true;
      }
    }
    break;

    case gattdb_device_attestation_challenge:
      app_log("signing challenge\r\n");
      sc = ble_sign_challenge(user_write_request->value.data,
                              user_write_request->value.len,
                              device_attestation_signature,
                              sizeof(device_attestation_signature));

      break;
    default:
      app_log("unhandled attribute update\r\n");
      break;
  }
  result = sl_bt_gatt_server_send_user_write_response(
    user_write_request->connection,
    user_write_request->characteristic,
    SL_STATUS_OK);
  if ((peer_device_cert_complete == true) && peer_batch_cert_complete
      && (peer_factory_cert_complete == true)
      && (peer_root_cert_complete == true)) {
    app_log("received certificate chain from client, now verify it\r\n");
    peer_device_cert_complete = peer_batch_cert_complete = false;
    // uint8_t flag = 1; /*enable printing of cert info*/
    sc = check_root_of_trust(&client_root_public_key, &root_trust);
    app_assert_status(sc);
    sc = mbedtls_x509_crt_verify(&cert_chain,
                                 &root_trust,
                                 NULL,
                                 NULL,
                                 &flags,
                                 verify_callback,
                                 NULL);
    if (SL_STATUS_OK == sc) {
      app_log("remote device cert chain ok\r\n");

      /* good place to put the peer's public key from the device certificate
       * in a persistent psa key*/
      sc = save_peer_pub_key(&cert_chain.pk, &peer_device_pub_key_id);
      app_assert_status(sc);
      sc = psa_generate_random(challenge, sizeof(challenge));
      app_assert_status(sc);
    } else {
      app_log(" %s\r\n", mbedtls_high_level_strerr(sc));
      sl_bt_connection_close(user_write_request->connection);
      return SL_STATUS_INVALID_SIGNATURE;
    }
  }
  return result;
}
