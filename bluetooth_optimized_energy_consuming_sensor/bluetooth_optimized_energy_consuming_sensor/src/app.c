/***************************************************************************//**
 * @file
 * @brief Core application logic.
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
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "sensor_app.h"
#include "app.h"

#define EM2_SLEEP_TIME                 1000 // 1s
#define EM4_SLEEP_TIME                 5000 // 5s

// Advertising flags (common).
#define ADVERTISE_FLAGS_LENGTH         0x02
#define ADVERTISE_FLAGS_TYPE           0x01
#define ADVERTISE_FLAGS_DATA           0x06

// Complete local name.
#define DEVICE_NAME_LENGTH             7
#define DEVICE_NAME_TYPE               0x09
#define DEVICE_NAME                    "BG22_SE"

// Manufacturer ID (0x02FF - Silicon Labs' company ID)
#define MANUF_ID                       0x02FF
// 1+2+8 bytes for type, company ID and the payload
#define MANUF_LENGTH                   7
#define MANUF_TYPE                     0xFF

SL_PACK_START(1)
typedef struct
{
  uint8_t len_flags;
  uint8_t type_flags;
  uint8_t val_flags;

  uint8_t len_manuf;
  uint8_t type_manuf;
  // First two bytes must contain the manufacturer ID (little-endian order)
  uint8_t company_LO;
  uint8_t company_HI;

  uint32_t internal_temp;

  // length of the name AD element is variable,
  // adding it last to keep things simple
  uint8_t len_name;
  uint8_t type_name;

  // NAME_MAX_LENGTH must be sized
  // so that total length of data does not exceed 31 bytes
  uint8_t name[DEVICE_NAME_LENGTH];

  // These values are NOT included in the actual advertising payload,
  // just for bookkeeping
  char dummy;        // Space for null terminator
  uint8_t data_size; // Actual length of advertising data
} SL_ATTRIBUTE_PACKED advertising_packet_t;
SL_PACK_END()

/**************************************************************************//**
 * Local variables.
 *****************************************************************************/
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// Advertising data.
static advertising_packet_t advertising_data = {
  .len_flags = ADVERTISE_FLAGS_LENGTH,
  .type_flags = ADVERTISE_FLAGS_TYPE,
  .val_flags = ADVERTISE_FLAGS_DATA,

  .len_manuf = MANUF_LENGTH,
  .type_manuf = MANUF_TYPE,
  .company_LO = MANUF_ID & 0xFF,
  .company_HI = (MANUF_ID >> 8) & 0xFF,

  // length of name element is the name string length + 1 for the AD type
  .len_name = DEVICE_NAME_LENGTH + 1,
  .type_name = DEVICE_NAME_TYPE,

  // Initialize for custom data
  .internal_temp = 0,

  .name = DEVICE_NAME,
  // Calculate total length of advertising data
  .data_size = 3 + (1 + MANUF_LENGTH) + (1 + DEVICE_NAME_LENGTH + 1),
};

/**************************************************************************//**
 * Local function prototypes.
 *****************************************************************************/
// Advertiser time out event handler.
static void bt_advertiser_timeout_event_handle(void);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  app_log("BLE - Optimized energy consuming sensor\r\n");
  sensor_app_init();
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
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
  float temp;
  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Reset and stop BURTC counter until advertising timeout.
      BURTC_CounterReset();
      BURTC_Stop();
      BURTC_SyncWait(); // Wait for the stop to synchronize

      temp = EMU_TemperatureGet();
      advertising_data.internal_temp = (temp > 0) ? (uint32_t)(temp * 100) : 0;
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160,
        160,
        100,
        0);
      app_assert_status(sc);

      // Set custom advertising payload
      sc = sl_bt_legacy_advertiser_set_data(
        advertising_set_handle,
        sl_bt_advertiser_advertising_data_packet,
        advertising_data.data_size,
        (uint8_t *)&advertising_data);
      app_assert_status(sc);

      sc = sl_bt_advertiser_set_tx_power(advertising_set_handle, 0, NULL);
      app_assert_status(sc);

      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(
        advertising_set_handle,
        sl_bt_advertiser_scannable_non_connectable);
      app_assert_status(sc);
      app_log("Start advertising ...\n");
      break;

    case sl_bt_evt_advertiser_timeout_id:
      bt_advertiser_timeout_event_handle();
      break;

    default:
      break;
  }
}

/**************************************************************************//**
 * Handler function for advertiser time out event.
 *****************************************************************************/
static void bt_advertiser_timeout_event_handle(void)
{
  sl_status_t sc;
  static uint8_t wakeup_number = 0;

  if (wakeup_number < 2) {
    BURTC_CounterReset();
    BURTC_CompareSet(0, EM2_SLEEP_TIME - 1);
    BURTC_Start();
    BURTC_SyncWait();
    EMU_EnterEM2(true);
    sc = sl_bt_legacy_advertiser_start(
      advertising_set_handle,
      sl_bt_advertiser_scannable_non_connectable);
    app_assert_status(sc);
    app_log("Start advertising ...\n");
  }
  if (wakeup_number == 2) {
    BURTC_CounterReset();
    BURTC_CompareSet(0, EM4_SLEEP_TIME - 1);
    BURTC_Start();
    BURTC_SyncWait();
    EMU_EnterEM4();
  }
  wakeup_number++;
}

/**************************************************************************//**
 * BURTC IRQ Handle function.
 *****************************************************************************/
void BURTC_IRQHandler(void)
{
  // Clear interrupt source
  BURTC_IntClear(BURTC_IEN_COMP);
}
