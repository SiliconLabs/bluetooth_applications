#ifndef ECDH_UTIL_H
#define ECDH_UTIL_H
#include "psa\crypto.h"
#include "sl_status.h"
#define ECDH_COORDINATE_SIZE 32
struct xy_coord{
	uint8_t X[ECDH_COORDINATE_SIZE];
	uint8_t Y[ECDH_COORDINATE_SIZE];

};
__packed struct signed_key_t {
  uint8_t Q[65]; // the ECDH public key to be signed
  uint8_t signature_raw[64]; //resulting signature
};

psa_status_t generatePeerKeySecret(mbedtls_svc_key_id_t key_id,
                                   uint8_t *Q,
                                   size_t peer_pub_key_size,
                                   uint8_t *shared_secret,
                                   size_t shared_secret_size,
                                   psa_key_id_t *derived);
psa_status_t setupEcdhPeer(uint8_t *public,size_t size, psa_key_id_t key_id);

sl_status_t finalize_key_agreement(psa_key_id_t private_key_id,
                                   psa_key_id_t pubkey_from_cert,
                                   struct signed_key_t *key,
                                   psa_key_id_t *shared_derived_key);
psa_status_t derive_key(psa_key_id_t private_key_id,
                        uint8_t *input_key,
                        size_t input_key_size,
                        psa_key_id_t *derived_key_id);
sl_status_t save_peer_pub_key(mbedtls_pk_context *pubkey, psa_key_id_t *pubkey_id);
sl_status_t signPublicKey(psa_key_id_t *local, struct signed_key_t *key);
sl_status_t ble_sign_challenge(uint8_t *, size_t, uint8_t *, size_t);
void print_hex(const char *message, uint8_t *block, size_t size);
sl_status_t import_pem_key(const uint8_t *pem_key_str,
                           size_t key_size,
                           psa_key_id_t *key_id,
                           psa_key_lifetime_t lifetime);
sl_status_t import_raw_key(psa_key_id_t *key_id,
                           uint8_t *keymaterial,
                           size_t key_size,
                           psa_key_lifetime_t lifetime);
sl_status_t pem_to_buffer_with_size(const uint8_t * pem_str,
                                    uint8_t *out,
                                    size_t *size);
sl_status_t check_root_of_trust(mbedtls_pk_context *root_public_key,
                                mbedtls_x509_crt *root);
#endif
