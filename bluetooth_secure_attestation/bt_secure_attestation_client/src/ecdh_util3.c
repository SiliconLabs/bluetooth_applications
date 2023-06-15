#include "em_cmu.h"
#include "app.h"
#include <string.h>
#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif
#include "mbedtls/aes.h"
#include "mbedtls/md.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/timing.h"
#include "mbedtls/ecp.h"
#include "mbedtls/pk.h"
#include "mbedtls/pem.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/error.h"
#include "psa/crypto.h"
#include "psa/crypto_values.h"
#include "ecdh_util.h"
#include "app_assert.h"
#include "sl_se_manager_internal_keys.h"
#define mbedtls_fprintf            fprintf
#define mbedtls_printf             printf

#define KEY_SIZE                   32

/*
 * Function : ecdh_generate_xy_coords
 * Description:
 *
 *
 * */

/*
 *
 * following code comes from github
 *
 * */
#if ((_SILICON_LABS_SECURITY_FEATURE == _SILICON_LABS_SECURITY_FEATURE_VAULT) \
  || defined(DOXYGEN))
psa_key_id_t private_device_key = SL_SE_BUILTIN_KEY_APPLICATION_ATTESTATION_ID;
#else
extern psa_key_id_t private_device_key;
#endif

/*****************************************************************************
 * @brief Setup ECDH for client and server
 * @param mode True for client, false for server
 * @param peerCtr Pointer to ECDH context
 * @param ecpPointX Pointer to ECP point X buffer
 * @param ecpPointY Pointer to ECP point Y buffer
 * @return true if successful and false otherwise.
 ******************************************************************************/

/*****************************************************************************
 * @brief Use peer's key to generate shared secret
 * @param mode True to use client key, false to use server key
 * @param peerCtr Pointer to ECDH context
 * @param ecpPointX Pointer to ECP point X buffer
 * @param ecpPointY Pointer to ECP point Y buffer
 * @return true if successful and false otherwise.
 ******************************************************************************/

#define DERIVED_KEY_BITS 256

sl_status_t create_import_key(psa_key_id_t *id,
                              psa_key_usage_t usage,
                              psa_key_type_t type,
                              psa_algorithm_t algo,
                              psa_key_lifetime_t lifetime,
                              psa_key_bits_t bits,
                              uint8_t *key_material,
                              size_t key_len)
{
  psa_key_attributes_t attributes;
  sl_status_t status;

  attributes = psa_key_attributes_init();
  psa_set_key_type(&attributes, type);
  psa_set_key_usage_flags(&attributes, usage);
  psa_set_key_lifetime(&attributes, lifetime);
  psa_set_key_algorithm(&attributes, algo);
  psa_set_key_bits(&attributes, bits);
  psa_set_key_type(&attributes, type);
  if (lifetime == PSA_KEY_LIFETIME_PERSISTENT) {
    psa_set_key_id(&attributes, *id);
  }
  status = psa_import_key(&attributes, key_material, key_len, id);
  return status;
}

sl_status_t create_new_key(psa_key_id_t *id,
                           psa_key_usage_t usage,
                           psa_key_type_t type,
                           psa_algorithm_t algo,
                           psa_key_lifetime_t lifetime,
                           psa_key_bits_t bits)
{
  psa_key_attributes_t attributes;
  sl_status_t status;

  attributes = psa_key_attributes_init();
  psa_set_key_type(&attributes, type);
  psa_set_key_usage_flags(&attributes, usage);
  psa_set_key_lifetime(&attributes, lifetime);
  psa_set_key_algorithm(&attributes, algo);
  psa_set_key_type(&attributes, type);
  psa_set_key_bits(&attributes, bits);
  status = psa_generate_key(&attributes, id);
  return status;
}

psa_status_t generatePeerKeySecret(mbedtls_svc_key_id_t key_id,
                                   uint8_t *Q,
                                   size_t peer_pub_key_size,
                                   uint8_t *shared_secret,
                                   size_t shared_secret_size,
                                   psa_key_id_t *derived)
{
  size_t shared_secret_bytes_written, key_bytes_written;
  sl_status_t status;
  uint8_t key_buffer[DERIVED_KEY_BITS / 8];
  psa_key_lifetime_t lifetime = PSA_KEY_LIFETIME_VOLATILE;
  psa_algorithm_t algo = PSA_ALG_CCM,
                  hash_algo = PSA_ALG_SHA_256;
  psa_key_type_t type = PSA_KEY_TYPE_AES;
  size_t keybits = DERIVED_KEY_BITS;
  psa_key_usage_t usage = PSA_KEY_USAGE_DECRYPT | PSA_KEY_USAGE_ENCRYPT;

  status = psa_raw_key_agreement(PSA_ALG_ECDH,
                                 key_id,
                                 Q,
                                 peer_pub_key_size,
                                 shared_secret,
                                 shared_secret_size,
                                 &shared_secret_bytes_written);
  app_assert(status == 0, "error in psa_raw_key_agreement() %ld\r\n", status);
  psa_hash_compute(hash_algo,
                   shared_secret, shared_secret_bytes_written,
                   key_buffer, sizeof(key_buffer),
                   &key_bytes_written);
  app_assert(status == 0, "error in psa_hash_compute() %ld\r\n", status);

  status = create_import_key(derived,
                             usage,
                             type,
                             algo,
                             lifetime,
                             keybits,
                             key_buffer,
                             sizeof(key_buffer));
  app_assert(PSA_SUCCESS == status, "error importing key %ld\r\n", status);

#ifdef TEST_KEY_EXPORT

  /*try to export the derived key - this must fail! */
  status = psa_export_key(derived,
                          export_buffer,
                          sizeof(export_buffer),
                          &key_bytes_written);
  app_log("result of trying to export the derived key is %x\r\n",
          status);
#endif
  return status;
}

/*
 * signPublicKey
 * description: generates and signs an ECDH public key
 * parameters:
 *    local - id of the ECDH keypair. must be persistent since it will be used
 *   after the function returns
 *    key -  pointer to the struct containing the public key and signature
 * returns - status
 *
 * */
sl_status_t signPublicKey(psa_key_id_t *local, struct signed_key_t *key)
{
  sl_status_t status;
  uint8_t hash[32];
  psa_algorithm_t algo = PSA_ALG_ECDH;
  psa_algorithm_t sign_algo = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
  psa_key_attributes_t ecdh_key_attr;
  psa_key_type_t type = PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1);
  psa_key_lifetime_t lifetime = PSA_KEY_LIFETIME_VOLATILE;
  size_t keybits = 256;
  psa_key_usage_t usage = PSA_KEY_USAGE_DERIVE;
  size_t output_size, bytes_written;

  /*set up ECDH key*/
  psa_generate_key(&ecdh_key_attr, local);
  status = create_new_key(local, usage, type, algo, lifetime, keybits);
  status = psa_export_public_key(*local,
                                 key->Q,
                                 sizeof(key->Q) + 1,
                                 &output_size);
  app_assert(status == 0, "psa_export_public_key(): %ld\r\n", status);

  status = psa_hash_compute(PSA_ALG_SHA_256,
                            key->Q,
                            sizeof(key->Q),
                            hash,
                            sizeof(hash),
                            &bytes_written);
  app_assert_status(status);

  status = psa_sign_hash(private_device_key,
                         sign_algo,
                         hash,
                         sizeof(hash),
                         key->signature_raw,
                         sizeof(key->signature_raw),
                         &bytes_written);
  app_assert_status(status);
  return status;
}

/*
 * verifyPublicKey
 * Description: verifies the signature of an ECDH public key
 * parameters:
 *     signed_key - pointer to a struct containing the ECDH public key and
 *   signature
 *     pubkey_from_cert - key id of the public key in the peer's device
 *   certificate
 *
 * */
sl_status_t verifyPublicKey(struct signed_key_t *signed_key,
                            psa_key_id_t pubkey_from_cert)
{
  sl_status_t status;
  psa_algorithm_t algo = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
  uint8_t hash[32];
  size_t bytes_written;

  status = psa_hash_compute(PSA_ALG_SHA_256,
                            signed_key->Q,
                            sizeof(signed_key->Q),
                            hash,
                            sizeof(hash),
                            &bytes_written);
  app_assert_status(status);
  status = psa_verify_hash(pubkey_from_cert,
                           algo,
                           hash,
                           sizeof(hash),
                           signed_key->signature_raw,
                           sizeof(signed_key->signature_raw));
  app_assert_status(status);
  return status;
}

/*
 *  complete_key_agreement()
 *  Desription: call this function to complete a key agreement with verifcation
 *              of the key's signature
 *  Parameters:
 *      private_key_id: id of this device's ECDH keypair
 *      pubkey_from_cert: id of the public key sent by the peer in it's device
 *      certificate
 *      key : struct containing the peer's public key and signature
 *      shared_derived_key : id of the key derived from the shared secret
 **/
sl_status_t finalize_key_agreement(psa_key_id_t private_key_id,
                                   psa_key_id_t pubkey_from_cert,
                                   struct signed_key_t *key,
                                   psa_key_id_t *shared_derived_key)
{
  sl_status_t status;
  uint8_t shared_secret[64];

  app_log("verifying signature of peer's public key\r\n");

  status = verifyPublicKey(key, pubkey_from_cert);
  app_assert_status(status);
//   if (SL_STATUS_OK ==status){
  status = generatePeerKeySecret(private_key_id,
                                 key->Q,
                                 sizeof(key->Q),
                                 shared_secret,
                                 sizeof(shared_secret) + 1,
                                 shared_derived_key);
  app_assert_status(status);
//     }
  return status;
}

sl_status_t save_peer_pub_key(mbedtls_pk_context *pubkey,
                              psa_key_id_t *pubkey_id)
{
  sl_status_t status;
  psa_algorithm_t algo = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
  psa_key_type_t type = PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1);
  size_t keybits = 256;
  psa_key_lifetime_t lifetime = PSA_KEY_PERSISTENCE_VOLATILE;
  psa_key_usage_t usage = PSA_KEY_USAGE_VERIFY_HASH;
  mbedtls_ecp_keypair *key;
  mbedtls_ecp_group group;
  uint8_t buffer[65];
  size_t nWritten;

  /* now set up all the attributes */

  /*get the plain binary form of the public key*/
  key = mbedtls_pk_ec(*pubkey);
  mbedtls_ecp_group_init(&group);
  status = mbedtls_ecp_group_load(&group, MBEDTLS_ECP_DP_SECP256R1);
  app_assert_status(status);
  status = mbedtls_ecp_point_write_binary(&group,
                                          &key->MBEDTLS_PRIVATE(Q),
                                          MBEDTLS_ECP_PF_UNCOMPRESSED,
                                          &nWritten,
                                          buffer,
                                          65);
  app_assert_status(status);

  /*now import into the psa key*/
  status = create_import_key(pubkey_id,
                             usage,
                             type,
                             algo,
                             lifetime,
                             keybits,
                             buffer,
                             sizeof(buffer));
  app_assert_status(status);
  return status;
}

sl_status_t ble_sign_challenge(uint8_t *challenge,
                               size_t size,
                               uint8_t *signature,
                               size_t siglen)
{
  sl_status_t status;
  size_t bytes_written;
  psa_algorithm_t algo = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
  uint8_t hash[32];

  status = psa_hash_compute(PSA_ALG_SHA_256,
                            challenge,
                            size,
                            hash,
                            sizeof(hash),
                            &bytes_written);
  if (status != SL_STATUS_OK) {
    return status;
  }
  // app_log("signing hash with key id %d\r\n", private_device_key);
  status = psa_sign_hash(private_device_key,
                         algo,
                         hash,
                         sizeof(hash),
                         signature,
                         siglen,
                         &bytes_written);
  return status;
}

sl_status_t pem_to_buffer_with_size(const uint8_t *pem_str,
                                    uint8_t *out,
                                    size_t *size)
{
  mbedtls_pem_context pem;
  uint8_t pwd[] = "";
  size_t use_len;
  sl_status_t status;
  static const char header[] = "-----BEGIN CERTIFICATE-----";
  static const char footer[] = "-----END CERTIFICATE-----";

  mbedtls_pem_init(&pem);
  status = mbedtls_pem_read_buffer(&pem,
                                   header, footer,
                                   pem_str,
                                   pwd, sizeof(pwd), &use_len);
  memcpy(out, pem.MBEDTLS_PRIVATE(buf), pem.MBEDTLS_PRIVATE(buflen));
  app_assert_status(status);
  *size = pem.MBEDTLS_PRIVATE(buflen);
  mbedtls_pem_free(&pem);
  return status;
}

void print_hex(const char *message, uint8_t *block, size_t size)
{
  app_log(message);
  for (size_t i = 0; i < size; i++) {
    app_log("%2.2X:", *(block + i));
    if ((i + 1) % 16 == 0) {
      app_log("\r\n");
    }
  }
  app_log("\r\n");
}

sl_status_t import_pem_key(const uint8_t *pem_key_str,
                           size_t key_size,
                           psa_key_id_t *key_id,
                           psa_key_lifetime_t lifetime)
{
  uint8_t pwd[] = "";
  sl_status_t status;
  uint8_t buffer[65];
  size_t bytes_to_import = 0;

  /*need to specify secure storage for PSA descriptor*/

  psa_algorithm_t       algo = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
  psa_key_type_t        type = PSA_KEY_TYPE_NONE;
  size_t                keybits = 256;
  psa_key_usage_t usage = 0;
  mbedtls_pk_context pk;
  mbedtls_ecp_keypair *key;
  mbedtls_ecp_group p256;

  mbedtls_ecp_group_init(&p256);
  mbedtls_ecp_group_load(&p256, MBEDTLS_ECP_DP_SECP256R1);

  mbedtls_pk_init(&pk);

  /*set the key usage depending on the type of key.
   * If there is a public key , enable verifying.
   * If there is a private key enable signing */
  if (strstr((const char *)pem_key_str, "PRIVATE") != NULL) {
    status = mbedtls_pk_parse_key(&pk,
                                  pem_key_str,
                                  key_size,
                                  pwd,
                                  sizeof(pwd),
                                  NULL,
                                  0);
    app_assert_status(status);
    key = mbedtls_pk_ec(pk);
    if (mbedtls_ecp_check_privkey(&p256, &key->MBEDTLS_PRIVATE(d)) == 0) {
      usage |= PSA_KEY_USAGE_SIGN_HASH;
      type = PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1);
      bytes_to_import = 32;
      mbedtls_mpi_write_binary(&key->MBEDTLS_PRIVATE(d), buffer,
                               bytes_to_import);
    }
  } else if (strstr((const char *)pem_key_str, "PUBLIC") != NULL) {
    status = mbedtls_pk_parse_public_key(&pk, pem_key_str, key_size);
    app_assert_status(status);
    key = mbedtls_pk_ec(pk);
    if (mbedtls_ecp_check_pubkey(&p256, &key->MBEDTLS_PRIVATE(Q)) == 0) {
      usage |= PSA_KEY_USAGE_VERIFY_HASH;
      type = PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1);
      key = mbedtls_pk_ec(pk);
      mbedtls_ecp_point_write_binary(&p256,
                                     &key->MBEDTLS_PRIVATE(Q),
                                     MBEDTLS_ECP_PF_UNCOMPRESSED,
                                     &bytes_to_import,
                                     buffer,
                                     sizeof(buffer));
    }
  } else {
    /* not a public key nor private key*/
    return SL_STATUS_FAIL;
  }

  /*write the key in raw binary format*/
  /*import to a PSA key*/
  status = create_import_key(key_id,
                             usage,
                             type,
                             algo,
                             lifetime,
                             keybits,
                             buffer,
                             bytes_to_import);
  app_assert_status(status);
  return status;
}

sl_status_t import_raw_key(psa_key_id_t *key_id,
                           uint8_t *keymaterial,
                           size_t key_size,
                           psa_key_lifetime_t lifetime)
{
  sl_status_t status;
  psa_algorithm_t algo = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
  psa_key_type_t  type = PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1);
  size_t          keybits = 256;
  psa_key_usage_t usage = PSA_KEY_USAGE_SIGN_HASH;

  status = create_import_key(key_id,
                             usage,
                             type,
                             algo,
                             lifetime,
                             keybits,
                             keymaterial,
                             key_size);
  return status;
}

/*   confirm that the remote device's root certificate is authentic
 *   by checking it's signature against a provisioned copy of the
 *   corresponding public key*/
sl_status_t check_root_of_trust(mbedtls_pk_context *root_public_key,
                                mbedtls_x509_crt *root)
{
  sl_status_t status;
  uint8_t hash[32];
  size_t bytes_written;

  /*compute hash of the 'to be signed' portion of the certificate*/
  status = psa_hash_compute(PSA_ALG_SHA_256,
                            root->tbs.p,
                            root->tbs.len,
                            hash, sizeof(hash),
                            &bytes_written);
  if (PSA_SUCCESS != status) {
    app_log("error hashing tbs : %ld\r\n", status);
  }

  /*verify the hash using the preprovisioned key */

  status = mbedtls_pk_verify(root_public_key,
                             MBEDTLS_MD_SHA256,
                             hash, sizeof(hash),
                             root->MBEDTLS_PRIVATE(sig).p,
                             root->MBEDTLS_PRIVATE(sig).len);
  return status;
}
