/***************************************************************************//**
 * @file app_se_manager_secure_identity.c
 * @brief SE manager secure identity functions.
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

// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#include "app_se_manager_secure_identity.h"

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------
/// Command context
static sl_se_command_context_t cmd_ctx;

/// Certificate size buffer
static sl_se_cert_size_type_t cert_size_buf;

/// Challenge buffer
static uint8_t challenge_buf[SL_SE_CHALLENGE_SIZE];

/// Certificate buffer
static uint8_t cert_buf[CERT_SIZE];

/// Public device key buffer
static uint8_t pub_device_key_buf[SL_SE_CERT_KEY_SIZE];

/// Signature buffer
static uint8_t signature_buf[SL_SE_CERT_SIGN_SIZE];

/// Number of bytes actually used in the token.
static size_t token_len;

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * Get certificate size.
 ******************************************************************************/
uint32_t get_cert_size(uint8_t cert_type)
{
  if (cert_type == SL_SE_CERT_BATCH) {
    return cert_size_buf.batch_id_size;
  } else if (cert_type == SL_SE_CERT_DEVICE_SE) {
    return cert_size_buf.se_id_size;
  } else if (cert_type == SL_SE_CERT_DEVICE_HOST) {
    return cert_size_buf.host_id_size;
  } else {
    return 0;
  }
}

/***************************************************************************//**
 * Get challenge buffer pointer.
 ******************************************************************************/
uint8_t * get_challenge_buf_ptr(void)
{
  return(challenge_buf);
}

/***************************************************************************//**
 * Get certificate buffer pointer.
 ******************************************************************************/
uint8_t * get_cert_buf_ptr(void)
{
  return(cert_buf);
}

/***************************************************************************//**
 * Get public key buffer pointer.
 ******************************************************************************/
uint8_t * get_pub_device_key_buf_ptr(void)
{
  return(pub_device_key_buf);
}

/***************************************************************************//**
 * Get token length.
 ******************************************************************************/
size_t get_token_len(void)
{
  return(token_len);
}

/***************************************************************************//**
 * Initialize the SE Manager.
 ******************************************************************************/
sl_status_t init_se_manager(void)
{
  print_error_cycle(sl_se_init(), NULL);
}

/***************************************************************************//**
 * Deinitialize the SE Manager.
 ******************************************************************************/
sl_status_t deinit_se_manager(void)
{
  print_error_cycle(sl_se_deinit(), NULL);
}

/***************************************************************************//**
 * Generate random numbers and save them to a buffer.
 ******************************************************************************/
sl_status_t generate_random_number(uint8_t *buf, uint32_t size)
{
  print_error_cycle(sl_se_get_random(&cmd_ctx, buf, size), &cmd_ctx);
}

/***************************************************************************//**
 * Read size of stored certificates in the SE.
 ******************************************************************************/
sl_status_t read_cert_size(void)
{
  print_error_cycle(sl_se_read_cert_size(&cmd_ctx, &cert_size_buf), &cmd_ctx);
}

/***************************************************************************//**
 * Read stored certificates in SE.
 ******************************************************************************/
sl_status_t read_cert_data(uint8_t cert_type)
{
  print_error_cycle(sl_se_read_cert(&cmd_ctx,
                                    cert_type,
                                    cert_buf,
                                    get_cert_size(cert_type)), &cmd_ctx);
}

/***************************************************************************//**
 * Sign challenge with private device key.
 ******************************************************************************/
sl_status_t sign_challenge(void)
{
  // Set up a key descriptor for private device key
  sl_se_key_descriptor_t private_device_key = SL_SE_APPLICATION_ATTESTATION_KEY;

  // Sign challenge
  print_error_cycle(sl_se_ecc_sign(&cmd_ctx,
                                   &private_device_key,
                                   SL_SE_HASH_SHA256,
                                   false,
                                   challenge_buf,
                                   sizeof(challenge_buf),
                                   signature_buf,
                                   sizeof(signature_buf)), &cmd_ctx);
}

/***************************************************************************//**
 * Get on-chip public device key.
 ******************************************************************************/
sl_status_t get_public_device_key(void)
{
  print_error_cycle(sl_se_read_pubkey(&cmd_ctx,
                                      SL_SE_KEY_TYPE_IMMUTABLE_ATTESTATION,
                                      cert_buf,
                                      SL_SE_CERT_KEY_SIZE), &cmd_ctx);
}

/***************************************************************************//**
 * Verify signature with on-chip public device key.
 ******************************************************************************/
sl_status_t verify_signature_local(void)
{
  // Set up a key descriptor for on-chip public device key
  sl_se_key_descriptor_t pub_device_key = {
    .type = SL_SE_KEY_TYPE_ECC_P256,
    .flags = SL_SE_KEY_FLAG_ASYMMETRIC_BUFFER_HAS_PUBLIC_KEY
             | SL_SE_KEY_FLAG_ASYMMMETRIC_SIGNING_ONLY,
    .storage.method = SL_SE_KEY_STORAGE_EXTERNAL_PLAINTEXT,
    .storage.location.buffer.pointer = cert_buf,
    .storage.location.buffer.size = SL_SE_CERT_KEY_SIZE
  };

  // Verify signature
  print_error_cycle(sl_se_ecc_verify(&cmd_ctx,
                                     &pub_device_key,
                                     SL_SE_HASH_SHA256,
                                     false,
                                     challenge_buf,
                                     sizeof(challenge_buf),
                                     signature_buf,
                                     sizeof(signature_buf)), &cmd_ctx);
}

/***************************************************************************//**
 * Verify signature with public device key in device certificate.
 ******************************************************************************/
sl_status_t verify_signature_remote(uint8_t *buffer,
                                    size_t bufsize,
                                    uint8_t *sig_buffer,
                                    size_t size)
{
  sl_status_t status;
  // Set up a key descriptor for public device key in device certificate
  sl_se_key_descriptor_t pub_device_key = {
    .type = SL_SE_KEY_TYPE_ECC_P256,
    .flags = SL_SE_KEY_FLAG_ASYMMETRIC_BUFFER_HAS_PUBLIC_KEY
             | SL_SE_KEY_FLAG_ASYMMMETRIC_SIGNING_ONLY,
    .storage.method = SL_SE_KEY_STORAGE_EXTERNAL_PLAINTEXT,
    .storage.location.buffer.pointer = pub_device_key_buf,
    .storage.location.buffer.size = sizeof(pub_device_key_buf)
  };

  status = sl_se_validate_key(&pub_device_key);
  if (status) {
    app_log("validate public key: %lX\r\n", status);
  }
  // Verify signature
  // print_error_cycle(
  status = sl_se_ecc_verify(&cmd_ctx,
                            &pub_device_key,
                            SL_SE_HASH_SHA256,
                            false,
                            buffer,
                            bufsize,
                            sig_buffer,
                            size);
  return status;
}

// -----------------------------------------------------------------------------
//                          Static Function Definitions
// -----------------------------------------------------------------------------
