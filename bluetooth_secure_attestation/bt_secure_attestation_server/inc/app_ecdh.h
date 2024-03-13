#ifndef APP_ECDH_H
#define APP_ECDH_H
#include "mbedtls/ecdh.h"
#define MBEDTLS_ECC_ID                 (MBEDTLS_ECP_DP_SECP256R1)

#define mbedtls_fprintf                fprintf
#define mbedtls_printf                 printf

#define MODE_ENCRYPT                   (1)
#define MODE_DECRYPT                   (2)

#define AES_BLOCK_SIZE                 (16)
#define IV_SIZE                        (16)
#define TAG_SIZE                       (32)
#define MAX_MESSAGE_SIZE_ENCRYPTION    (1024)
#define MAX_MESSAGE_SIZE_DECRYPTION \
  (2 * MAX_MESSAGE_SIZE_ENCRYPTION + 2 * IV_SIZE + 2 * TAG_SIZE + 1)

#define STDIN_FILENO                   (0)
#define STDOUT_FILENO                  (1)

typedef struct {
  uint8_t  connectionHandle;
  int8_t   rssi;
  uint16_t serverAddress;
  uint32_t ecdhServiceHandle;
  uint32_t testDataServiceHandle;
  uint16_t m_valCharacteristicHandle;
  uint32_t temperature;
} ConnProperties;

/* keep all of the UUIDs related to ECDH together*/

/*
 * maybe it's time to think about splitting this up into multiple structs since
 *   many of these fields are not actually related to ECDH
 * */
struct ecdh_gatt_info {
  uint8_t ecdhService[2];
  uint32_t serviceHandle;
  uint8_t server_local_uuid[16];
  uint8_t server_remote_uuid[16];
  uint16_t server_local_handle;
  uint16_t server_remote_handle;
  uint32_t testDataServiceHandle;  // not ECDH
  uint8_t testDataServiceUUID[16];  // not ECDH
  uint16_t serverDataOutCharacteristicHandle;  // not ECDH
  uint8_t serverDataOutCharacteristicUUID[16];  // not ECDH
  uint16_t testDataNonceHandle;  // not ECDH
  uint8_t testDataNonceUUID[16];  // not ECDH
  uint16_t testDataAeadHandle;  // not ECDH
  uint8_t  testDataAeadUUID[16]; // not ECDH
  uint16_t testDataAeadNonceHandle;
  uint8_t testDataAeadNonceUUID[16];
};

#ifndef NOT_READY_YET
struct testDataServiceGattInfo {
  uint32_t testDataServiceHandle; // not ECDH
  uint8_t testDataServiceUUID[16];    // not ECDH
  uint16_t serverDataOutCharacteristicHandle;    // not ECDH
  uint8_t serverDataOutCharacteristicUUID[16];    // not ECDH
  uint16_t testDataNonceHandle;    // not ECDH
  uint8_t testDataNonceUUID[16];    // not ECDH
  uint16_t testDataAeadHandle;    // not ECDH
  uint8_t  testDataAeadUUID[16];   // not ECDH
  uint16_t testDataAeadNonceHandle;
  uint8_t testDataAeadNonceUUID[16];
};

#endif

struct xy_coord ecdh_generate_xy_coords();
int compute_shared_secret(struct xy_coord *coords);
mbedtls_mpi  get_shared_secret();

#endif
