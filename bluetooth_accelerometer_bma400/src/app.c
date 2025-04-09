/***************************************************************************//**
 * @file app.c
 * @brief Explorer Kit Bluetooth accelerometer example using I2C bus BMA400
 *   accelerometer
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided \'as-is\', without any express or implied
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
#include "gpiointerrupt.h"
#include "sl_simple_led_instances.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "app_assert.h"
#include "app_log.h"

#ifdef SL_CATALOG_MIKROE_ACCEL5_BMA400_SPI_PRESENT
#include "sl_spidrv_instances.h"
#include "mikroe_bma400_spi.h"
#include "mikroe_bma400_spi_config.h"
#endif

#ifdef SL_CATALOG_MIKROE_ACCEL5_BMA400_I2C_PRESENT
#include "sl_i2cspm_instances.h"
#include "mikroe_bma400_i2c.h"
#include "mikroe_bma400_i2c_config.h"
#endif

// Flag set each time sleep timer callback to trigger bluetooth external signal
#define TIMER_CALLBACK_FLAG   (1 << 0)

// Earth's gravity in m/s^2
#define GRAVITY_EARTH         (9.80665f)
// 39.0625us per tick
#define SENSOR_TICK_TO_S      (0.0000390625f)

static struct bma400_dev bma;
// set sleep timer handle
static sl_sleeptimer_timer_handle_t led_blinky_timer;
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
// If the notification is enabled or not
static uint8_t notification_enabled = 0;
static int16_t connection_handle = 0xff;

static void app_gpio_int_cb(uint8_t intNo);
static void app_bma400_config(void);
static void app_bma400_get_data(uint32_t extsignals);
static void led_blinky_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                      void *data);
static float lsb_to_ms2(int16_t accel_data, uint8_t g_range, uint8_t bit_width);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void)
{
  app_bma400_config();
  app_log("> Start measuring...\n");
  sl_sleeptimer_start_periodic_timer_ms(&led_blinky_timer,
                                        800,
                                        led_blinky_timer_callback,
                                        NULL,
                                        0,
                                        0);
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
  sl_status_t sc = SL_STATUS_OK;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

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
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      app_log("> Start advertising...\n");
      sl_bt_external_signal(TIMER_CALLBACK_FLAG);

      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:

      notification_enabled = 0;
      sc = sl_sleeptimer_stop_timer(&led_blinky_timer);
      app_assert_status(sc);
      sl_led_turn_off(SL_SIMPLE_LED_INSTANCE(0));
      connection_handle = evt->data.evt_connection_opened.connection;
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:

      notification_enabled = 0;
      connection_handle = 0xff;
      sl_sleeptimer_start_periodic_timer_ms(&led_blinky_timer,
                                            800,
                                            led_blinky_timer_callback,
                                            NULL,
                                            0,
                                            0);
      app_assert_status(sc);
      app_log("Connection closed. -> Start advertising..\r\n");
      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    case sl_bt_evt_gatt_server_characteristic_status_id:

      if (evt->data.evt_gatt_server_characteristic_status.characteristic
          ==
          gattdb_acceleration) {
        // client characteristic configuration changed by remote GATT client
        if (evt->data.evt_gatt_server_characteristic_status.client_config_flags
            & sl_bt_gatt_notification) {
          notification_enabled = 1;
        } else {
          notification_enabled = 0;
        }
      }

      break;

    case sl_bt_evt_system_external_signal_id:
      app_bma400_get_data(evt->data.evt_system_external_signal.extsignals);
      break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

static void app_bma400_config(void)
{
  int8_t rslt;
  struct bma400_sensor_conf conf;
  struct bma400_int_enable int_en;

  GPIO_PinModeSet(MIKROE_BMA400_INT1_PORT,
                  MIKROE_BMA400_INT1_PIN,
                  gpioModeInputPullFilter,
                  1);
  GPIO_ExtIntConfig(MIKROE_BMA400_INT1_PORT,
                    MIKROE_BMA400_INT1_PIN,
                    MIKROE_BMA400_INT1_PIN,
                    1,
                    0,
                    1);
  GPIOINT_CallbackRegister(MIKROE_BMA400_INT1_PIN, app_gpio_int_cb);
  GPIO_IntEnable(MIKROE_BMA400_INT1_PIN);

  app_log(
    "Bluetooth - Accelerometer (BMA400) example.\n");

#ifdef SL_CATALOG_MIKROE_ACCEL5_BMA400_SPI_PRESENT
  rslt = bma400_spi_init(sl_spidrv_mikroe_handle, &bma);
#endif

#ifdef SL_CATALOG_MIKROE_ACCEL5_BMA400_I2C_PRESENT
  rslt = bma400_i2c_init(sl_i2cspm_mikroe, MIKROE_BMA400_ADDR, &bma);
#endif
  app_assert(rslt == BMA400_OK,
             "[E: 0x%04x] Failed to init BMA400 interface\r\n",
             (int)rslt);

  rslt = bma400_soft_reset(&bma);
  app_assert(rslt == BMA400_OK,
             "[E: 0x%04x] Failed to init BMA400 interface\r\n",
             (int)rslt);

  rslt = bma400_init(&bma);
  app_assert(rslt == BMA400_OK,
             "[E: 0x%04x] Failed to init BMA400 interface\r\n",
             (int)rslt);

  // Select the type of configuration to be modified
  conf.type = BMA400_ACCEL;

  // Get the accelerometer configurations which are set in the sensor
  rslt = bma400_get_sensor_conf(&conf, 1, &bma);
  app_assert(rslt == BMA400_OK,
             "[E: 0x%04x] Failed to init BMA400 interface\r\n",
             (int)rslt);
  conf.param.accel.int_chan = BMA400_INT_CHANNEL_1;
  conf.param.accel.odr = BMA400_ODR_25HZ;
  conf.param.accel.range = BMA400_RANGE_2G;
  conf.param.accel.data_src = BMA400_DATA_SRC_ACCEL_FILT_1;
  // Set the desired configurations to the sensor
  rslt = bma400_set_sensor_conf(&conf, 1, &bma);
  app_assert(rslt == BMA400_OK,
             "[E: 0x%04x] Failed to init BMA400 interface\r\n",
             (int)rslt);

  rslt = bma400_set_power_mode(BMA400_MODE_NORMAL, &bma);
  app_assert(rslt == BMA400_OK,
             "[E: 0x%04x] Failed to init BMA400 interface\r\n",
             (int)rslt);

  int_en.type = BMA400_DRDY_INT_EN;
  int_en.conf = BMA400_ENABLE;
  rslt = bma400_enable_interrupt(&int_en, 1, &bma);
  app_assert(rslt == BMA400_OK,
             "[E: 0x%04x] Failed to init BMA400 interface\r\n",
             (int)rslt);
}

static void led_blinky_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                      void *data)
{
  (void)handle;
  (void)data;
  sl_led_toggle(SL_SIMPLE_LED_INSTANCE(0));
}

static void app_bma400_get_data(uint32_t extsignals)
{
  if (extsignals & TIMER_CALLBACK_FLAG) {
    sl_status_t sc;
    int8_t rslt;
    struct bma400_sensor_data accel_data_raw;
    struct bma400_sensor_data accel_data_ms2;
    uint16_t int_status = 0;
    uint8_t accel_buffer[3];

    rslt = bma400_get_interrupt_status(&int_status, &bma);
    if (rslt != BMA400_OK) {
      app_log("[E: 0x%04x] Failed to get interrupt status\r\n", (int)rslt);
      return;
    }

    if (int_status & BMA400_ASSERTED_DRDY_INT) {
      rslt = bma400_get_accel_data(BMA400_DATA_SENSOR_TIME,
                                   &accel_data_raw,
                                   &bma);
      if (rslt != BMA400_OK) {
        app_log("[E: 0x%04x] Failed to get accel data\r\n", (int)rslt);
        return;
      }

      /* 12-bit accelerometer at range 2G */
      accel_data_ms2.x = (int16_t) (10 * lsb_to_ms2(accel_data_raw.x, 2, 12));
      accel_buffer[0] = (uint8_t) abs(accel_data_ms2.x);
      accel_data_ms2.y = (int16_t) (10 * lsb_to_ms2(accel_data_raw.y, 2, 12));
      accel_buffer[1] = (uint8_t) abs(accel_data_ms2.y);
      accel_data_ms2.z = (int16_t) (10 * lsb_to_ms2(accel_data_raw.z, 2, 12));
      accel_buffer[2] = (uint8_t) abs(accel_data_ms2.z);

      sc = sl_bt_gatt_server_write_attribute_value(gattdb_acceleration,
                                                   0,
                                                   3,
                                                   accel_buffer);
      app_assert_status(sc);

      if ((notification_enabled == 1) && (connection_handle != 0xff)) {
        sc = sl_bt_gatt_server_send_notification(connection_handle,
                                                 gattdb_acceleration,
                                                 3,
                                                 accel_buffer);

        app_assert_status(sc);
      }
    }
  }
}

static float lsb_to_ms2(int16_t accel_data, uint8_t g_range, uint8_t bit_width)
{
  float accel_ms2;
  int16_t half_scale;

  half_scale = 1 << (bit_width - 1);
  accel_ms2 = (GRAVITY_EARTH * accel_data * g_range) / half_scale;

  return accel_ms2;
}

static void app_gpio_int_cb(uint8_t intNo)
{
  (void) intNo;

  sl_bt_external_signal(TIMER_CALLBACK_FLAG);
}
