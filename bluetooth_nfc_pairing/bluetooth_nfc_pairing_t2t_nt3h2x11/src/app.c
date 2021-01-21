 /***************************************************************************//**
 * @file   app.c
 * @brief  Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
 ******************************************************************************/

/* ...
 *
 * EXPERIMENTAL QUALITY
 * This code has not been formally tested and is provided as-is.  It is not suitable for production environments.
 * This code will not be maintained.
 *
... */


#include <stdio.h>
#include "em_common.h"
#include "sl_app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "nt3h2x11.h"
#include "ndef_message.h"
#include "tlv.h"
#include "t2t.h"
#include "bluetooth_handover.h"
#include "bt_evt_dbg.h"

#define BRD4314A  1

/* EFR32xG12 Boards*/
#if    (BRD4103A == 1)\
    || (BRD4161A == 1)\
    || (BRD4162A == 1)\
    || (BRD4163A == 1)\
    || (BRD4164A == 1)\
    || (BRD4170A == 1)\
    || (BRD4172A == 1)\
    || (BRD4172B == 1)\
    || (BRD4173A == 1)

#define NT3H2X11_I2C             I2C0
#define NT3H2X11_I2C_SDA_PORT    gpioPortC
#define NT3H2X11_I2C_SCL_PORT    gpioPortC
#define NT3H2X11_I2C_SDA_PIN     11
#define NT3H2X11_I2C_SCL_PIN     10
#define NT3H2X11_I2C_SDA_LOC     _I2C_ROUTELOC0_SDALOC_LOC16
#define NT3H2X11_I2C_SCL_LOC     _I2C_ROUTELOC0_SCLLOC_LOC14

#elif  (BRD4166A == 1)\
    || (BRD4304A == 1)

#define NT3H2X11_I2C             I2C0
#define NT3H2X11_I2C_SDA_PORT    gpioPortC
#define NT3H2X11_I2C_SCL_PORT    gpioPortC
#define NT3H2X11_I2C_SDA_PIN     10
#define NT3H2X11_I2C_SCL_PIN     11
#define NT3H2X11_I2C_SDA_LOC     _I2C_ROUTELOC0_SDALOC_LOC15
#define NT3H2X11_I2C_SCL_LOC     _I2C_ROUTELOC0_SCLLOC_LOC15

/* EFR32xG13 Boards*/
#elif  (BRD4104A == 1)\
    || (BRD4158A == 1)\
    || (BRD4159A == 1)\
    || (BRD4165B == 1)\
    || (BRD4167A == 1)\
    || (BRD4168A == 1)\
    || (BRD4174A == 1)\
    || (BRD4174B == 1)\
    || (BRD4175A == 1)\
    || (BRD4305A == 1)\
    || (BRD4305C == 1)\
    || (BRD4305D == 1)\
    || (BRD4305E == 1)

#define NT3H2X11_I2C             I2C0
#define NT3H2X11_I2C_SDA_PORT    gpioPortC
#define NT3H2X11_I2C_SCL_PORT    gpioPortC
#define NT3H2X11_I2C_SDA_PIN     11
#define NT3H2X11_I2C_SCL_PIN     10
#define NT3H2X11_I2C_SDA_LOC     _I2C_ROUTELOC0_SDALOC_LOC16
#define NT3H2X11_I2C_SCL_LOC     _I2C_ROUTELOC0_SCLLOC_LOC14

#elif  (BRD4306A == 1)\
    || (BRD4306B == 1)\
    || (BRD4306C == 1)\
    || (BRD4306D == 1)

#define NT3H2X11_I2C             I2C0
#define NT3H2X11_I2C_SDA_PORT    gpioPortC
#define NT3H2X11_I2C_SCL_PORT    gpioPortC
#define NT3H2X11_I2C_SDA_PIN     10
#define NT3H2X11_I2C_SCL_PIN     11
#define NT3H2X11_I2C_SDA_LOC     _I2C_ROUTELOC0_SDALOC_LOC15
#define NT3H2X11_I2C_SCL_LOC     _I2C_ROUTELOC0_SCLLOC_LOC15

/* EFR32xG21 Boards*/
#elif  (BRD4180A == 1)\
    || (BRD4180B == 1)\
    || (BRD4181A == 1)\
    || (BRD4181B == 1)\
    || (BRD4181C == 1)\
    || (BRD4308A == 1)\
    || (BRD4308B == 1)

#define NT3H2X11_I2C             I2C0
#define NT3H2X11_I2C_SDA_PORT    gpioPortC
#define NT3H2X11_I2C_SCL_PORT    gpioPortC
#define NT3H2X11_I2C_SDA_PIN     2
#define NT3H2X11_I2C_SCL_PIN     3

#elif (BRD4309B == 1)

#warning "Limited pins on BRD4309B, VCOM can not be used when using this board with I2C."

#define NT3H2X11_I2C             I2C0
#define NT3H2X11_I2C_SDA_PORT    gpioPortC
#define NT3H2X11_I2C_SCL_PORT    gpioPortC
#define NT3H2X11_I2C_SDA_PIN     0
#define NT3H2X11_I2C_SCL_PIN     1

/* EFR32xG22 Boards*/
#elif  (BRD4182A == 1)\
    || (BRD4310A == 1)\
    || (BRD4311A == 1)

#define NT3H2X11_I2C             I2C0
#define NT3H2X11_I2C_SDA_PORT    gpioPortB
#define NT3H2X11_I2C_SCL_PORT    gpioPortB
#define NT3H2X11_I2C_SDA_PIN     3
#define NT3H2X11_I2C_SCL_PIN     2

#elif  (BRD4183A == 1)

#warning "Limited pins on BRD4183A, VCOM can not be used when using this board with I2C."

#define NT3H2X11_I2C             I2C0

#define NT3H2X11_I2C_SDA_PORT    gpioPortA
#define NT3H2X11_I2C_SCL_PORT    gpioPortA
#define NT3H2X11_I2C_SDA_PIN     5
#define NT3H2X11_I2C_SCL_PIN     6

#elif  (BRD4184A == 1)

#define NT3H2X11_I2C             I2C0
#define NT3H2X11_I2C_SDA_PORT    gpioPortD
#define NT3H2X11_I2C_SCL_PORT    gpioPortD
#define NT3H2X11_I2C_SDA_PIN     2
#define NT3H2X11_I2C_SCL_PIN     3

#elif  (BRD4314A == 1)

#define NT3H2X11_I2C             I2C0
#define NT3H2X11_I2C_SDA_PORT    gpioPortD
#define NT3H2X11_I2C_SCL_PORT    gpioPortD
#define NT3H2X11_I2C_SDA_PIN     3
#define NT3H2X11_I2C_SCL_PIN     2

#else

// Custom board
#define NT3H2X11_I2C
#define NT3H2X11_I2C_SDA_PORT
#define NT3H2X11_I2C_SCL_PORT
#define NT3H2X11_I2C_SDA_PIN
#define NT3H2X11_I2C_SCL_PIN

#endif

#if (_SILICON_LABS_32B_SERIES == 1)
#define NT3H2X11_I2C_INIT_DEFAULT {                           \
    true,                        /* Enable */                 \
    NT3H2X11_I2C,                /* Use I2C instance 0 */     \
    NT3H2X11_I2C_SCL_PORT,       /* SCL port */               \
    NT3H2X11_I2C_SCL_PIN,        /* SCL pin */                \
    NT3H2X11_I2C_SDA_PORT,       /* SDA port */               \
    NT3H2X11_I2C_SDA_PIN,        /* SDA pin */                \
    NT3H2X11_I2C_SCL_LOC,        /* SCL Location */           \
    NT3H2X11_I2C_SDA_LOC,        /* SDA Location  */          \
}
#elif (_SILICON_LABS_32B_SERIES == 2)
#define NT3H2X11_I2C_INIT_DEFAULT {                           \
    true,                        /* Enable */                 \
    NT3H2X11_I2C,                /* Use I2C instance 0 */     \
    NT3H2X11_I2C_SCL_PORT,       /* SCL port */               \
    NT3H2X11_I2C_SCL_PIN,        /* SCL pin */                \
    NT3H2X11_I2C_SDA_PORT,       /* SDA port */               \
    NT3H2X11_I2C_SDA_PIN,        /* SDA pin */                \
}
#endif

#define SECURE_CONNECTION           1

#define TLV_BUFFER_SIZE             200
#define PAYLOAD_BUFFER_SIZE         200
#define RECORD_TYPE_BUFFER_SIZE     50

nt3h2x11_init_t nt3h2x11_init_only_i2c = {
    .i2c_init = NT3H2X11_I2C_INIT_DEFAULT,
    .fd_init = { .enable = false }
};


uint8_t payload[PAYLOAD_BUFFER_SIZE];
uint8_t record_type[RECORD_TYPE_BUFFER_SIZE];
uint8_t btssp_config_addr[BLUETOOTH_LE_MAC_ADDR_FIELD_LEN];
uint8_t btssp_config_le_role[BLUETOOTH_LE_ROLE_FIELD_LEN];
uint8_t btssp_config_sm_tk_value[BLUETOOTH_SM_TK_VALUE_FIELD_LEN];
uint8_t btssp_config_sc_confirmation_value[BLUETOOTH_SC_CONFIRMATION_VALUE_FIELD_LEN];
uint8_t btssp_config_sc_random_value[BLUETOOTH_SC_RANDOM_VALUE_FIELD_LEN];
uint8_t btssp_config_appearance[BLUETOOTH_APPEARANCE_FIELD_LEN];
uint8_t btssp_config_local_name[] = "Silabs Bluetooth NFC Pairing";
uint8_t btssp_config_flags_value[BLUETOOTH_FLAGS_FIELD_LEN];


ndef_record_t record = {
        .payload = payload,
        .type = record_type
};

bt_carrier_config_t btssp_config = {
        .addr = {
                .value = btssp_config_addr
        },
        .le_role = {
                .value = btssp_config_le_role
        },
        //        .security_manager_tk_value = {
        //                .value = btssp_config_sm_tk_value
        //        },
#if (SECURE_CONNECTION == 1)
        .secure_connections_confirm_value = {
                .value = btssp_config_sc_confirmation_value
        },
        .secure_connectinos_random_value = {
                .value = btssp_config_sc_random_value
        },
#endif
        //        .appearance = {
        //                .value = btssp_config_appearance
        //        },
        .local_name = {
                .is_set = true,
                .type = BLUETOOTH_AD_TYPE_COMPLETE_LOCAL_NAME,
                /* Eliminate string termination character. */
                .length = sizeof(btssp_config_local_name) - 1,
                .value = btssp_config_local_name
        },
        //        .flags = {
        //                .value = btssp_config_flags_value
        //        }
};

uint8_t ndef_message_buff[TLV_BUFFER_SIZE - 3];
uint8_t tlv_buff[TLV_BUFFER_SIZE];

ndef_record_t ndef_message[1];

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static void btssp_record_create (void) {

  sl_status_t sc;

  printf("NT3H2x11 static handover example.\r\n");

  /* Initialize NT3H2x11 I2C communication. */
  nt3h2x11_init(nt3h2x11_init_only_i2c);

  bd_addr static_addr = { .addr = {0x11, 0x11, 0x11, 0x11, 0x11, 0xC0} };

  sc = sl_bt_advertiser_set_random_address(advertising_set_handle, 1, static_addr, NULL);

  sl_app_assert(sc == SL_STATUS_OK,
                "[E: 0x%04x] Failed to set random address\n",
                (int)sc);

  sc = sl_bt_sm_delete_bondings();

  sl_app_assert(sc == SL_STATUS_OK,
                "[E: 0x%04x] Failed to delete all bondings\n",
                (int)sc);

  printf("All bonding deleted\r\n");

#if (SECURE_CONNECTION == 1)

  sc = sl_bt_sm_configure(0x0a, sm_io_capability_noinputnooutput);

  sl_app_assert(sc == SL_STATUS_OK,
                "[E: 0x%04x] Failed to configure security manager\n",
                (int)sc);

  sc = sl_bt_sm_set_bondable_mode(1);

  sl_app_assert(sc == SL_STATUS_OK,
                "[E: 0x%04x] Failed to set bondable mode\n",
                (int)sc);

  printf("Use Secure Connection Pairing.\r\n");

  uint8_t oob_data[32];

  size_t len;

  sc = sl_bt_sm_use_sc_oob(1, 32, &len, oob_data);

  sl_app_assert(sc == SL_STATUS_OK,
                "[E: 0x%04x] Failed to enable secure connection OOB\n",
                (int)sc);
#endif

  /** Address */

  uint8_t addr_buff[BLUETOOTH_MAC_ADDR_LEN];

  bt_le_mac_addr_t bt_le_addr = {
          .type = random_address,
          .value = addr_buff
  };

  /* Copy device address. */
  // Public address, change the type in the bt_le_mac_addr_t struct above to use public address
//            memcpy(bd_addr.value, gecko_cmd_system_get_bt_address()->address.addr, BLUETOOTH_MAC_ADDR_LEN);
  /* Random static address, for android NFC pairing with secure connection, this has to be used for some reason */
  memcpy(bt_le_addr.value, static_addr.addr, BLUETOOTH_MAC_ADDR_LEN);
  /* Encode address and its type. */
  ch_bluetooth_bd_addr_encode(btssp_config.addr.value, bt_le_addr);

  /** LE Role. */
  /* This device only acts as peripheral. */
  btssp_config.le_role.value[0] = only_peripheral;

#if (SECURE_CONNECTION == 1)

  /** LE Secure Connections Confirmation Value. */

  printf("16-byte confirm value: ");
  for (uint8_t i = 16; i < 32; i++) {
      printf("0x%02X ", oob_data[i]);
      btssp_config.secure_connections_confirm_value.value[i - 16] = oob_data[i];
  }
  printf("\r\n");

  btssp_config.secure_connections_confirm_value.is_set = true;

  /** LE Secure Connections Random Value. */

  printf("16-byte random value: ");
  for (uint8_t i = 0; i < 16; i++) {
      printf("0x%02X ", oob_data[i]);
      btssp_config.secure_connectinos_random_value.value[i] = oob_data[i];
  }

  printf("\r\n");

  btssp_config.secure_connectinos_random_value.is_set = true;

#endif

  /* Encode bluetooth carrier configuration record. */
  ch_bluetooth_le_carrier_configuration_record_encode(&record, &btssp_config);

  ndef_message[0] = record;

  // encode ndef message
  ndef_message_encode_result_t ndef_message_encode_result;
  ndef_message_encode_result = ndef_message_encode(ndef_message_buff, ndef_message);
  if ( ndef_message_encode_result.err == ndefMessageEncodeFail ) {
      printf("NDEF message encode failed:(\r\n");
      while(1);
  }

  uint32_t write_size = ndef_message_encode_result.size;

  // encode ndef tlv
  if (tlv_encode(tlv_buff,
                 TLV_BUFFER_SIZE,
                 NFC_T2T_NDEF_MESSAGE_TLV,
                 ndef_message_encode_result.size,
                 ndef_message_buff
                 ) != tlvEncodeCompleted ) {
      printf("NDEF TLV encode failed:(\r\n");
      while(1);
  }

  write_size += 2;
  // encode terminator tlv
  if ( tlv_encode(&tlv_buff[write_size],
                  (TLV_BUFFER_SIZE - write_size),
                   NFC_T2T_TERMINATOR_TLV,
                   0,
                   NULL
                  )!= tlvEncodeCompleted ) {
      printf("Terminator TLV encode failed:(\r\n");
      while(1);
  }

  write_size++;

  for (uint32_t i = 1; (i - 1) * 16 <= write_size; i++) {
      while (nt3h2x11_write_block(i, &tlv_buff[(i - 1) * 16]) != i2cTransferDone);
  }

}

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////

  printf("\r\nNT3H2x11 static handover example.\r\n");

  /* Initialize NT3H2x11 I2C communication. */
  nt3h2x11_init(nt3h2x11_init_only_i2c);

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

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];

  sl_bt_evt_log(evt);

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to get Bluetooth address\n",
                    (int)sc);

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
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to write attribute\n",
                    (int)sc);

      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to create advertising set\n",
                    (int)sc);


      btssp_record_create();

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to set advertising timing\n",
                    (int)sc);
      // Start general advertising and enable connections.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start advertising\n",
                    (int)sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Restart advertising after client has disconnected.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start advertising\n",
                    (int)sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    case sl_bt_evt_sm_confirm_bonding_id:

      sc = sl_bt_sm_bonding_confirm(evt->data.evt_sm_confirm_bonding.connection, 1);

//      sl_app_assert(sc == SL_STATUS_OK,
//                    "[E: 0x%04x] Failed to get Bluetooth address\n",
//                    (int)sc);

      if (sc == SL_STATUS_OK) {
          printf("bonding accepted\r\n");
      } else {
          printf("bonding error code: 0x%x\r\n", (unsigned int)sc);
      }
      break;

    case sl_bt_evt_sm_bonding_failed_id:

      break;

    case sl_bt_evt_sm_bonded_id:
      sl_bt_sm_list_all_bondings();
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}
