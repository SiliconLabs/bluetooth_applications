/***************************************************************************//**
 * @file app.c
 * @brief application code for the HID tripwire project
 * @version 1.0
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "gatt_db.h"

#include "sl_sleeptimer.h"
#include "sl_led.h"
#include "sl_simple_led_instances.h"
#include "sl_button.h"
#include "sl_simple_button_instances.h"

#include "sl_imu.h"
#include "sl_icm20648.h"
#include "sl_icm20648_config.h"

#include "gpiointerrupt.h"
#include "em_cmu.h"
#include "em_gpio.h"

#define IMU_WOM_THRESHOLD               10
#define IMU_SAMPLE_RATE                 10

#define DELAY_TIMER_MS                  3000UL
#define HEARTBEAT_TIMER_MS              1000UL

// 10% duty cycle
#define SIMPLE_HEARTBEAT_ON_MS          (HEARTBEAT_TIMER_MS * 10UL / 100UL)
#define SIMPLE_HEARTBEAT_OFF_MS         (HEARTBEAT_TIMER_MS \
                                         - SIMPLE_HEARTBEAT_ON_MS)

#define EXT_SIG_UPDATE_FSM              (1UL << 1)
#define EXT_SIG_IMU_WAKEUP              (1UL << 2)
#define EXT_SIG_BUTTON_REL              (1UL << 3)

#define REPORT_MODIFIER_INDEX           0
#define REPORT_DATA_INDEX               2
#define MODIFIER_FORMAT(m)              (1UL << (m))
#define TAB_KEY                         0x2B

enum modifier_key_map {
  LEFT_CTRL = 0,
  LEFT_SHIFT,
  LEFT_ALT,
  LEFT_GUI,
  RIGHT_CTRL,
  RIGHT_SHIFT,
  RIGHT_ALT,
  RIGHT_GUI
};

typedef enum {
  TW_IDLE = 0,
  TW_DELAY,
  TW_PRIMED,
  TW_DISENGAGE,
  TW_TRIPPED,
} tripwire_state_t;

typedef struct {
  bool initialized;
  bool wom_enabled;
  bool gpio_int_enabled;
  uint32_t gpio_intno;
  int16_t o_vec[3];
  int16_t a_vec[3];
} app_imu_t;

static app_imu_t imu = { 0 };

static volatile tripwire_state_t tw_curr_state = TW_IDLE;
static volatile tripwire_state_t tw_next_state = TW_IDLE;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static sl_sleeptimer_timer_handle_t tw_delay_timer = { 0 };
static sl_sleeptimer_timer_handle_t tw_heartbeat_led_timer = { 0 };

static volatile uint8_t heartbeat_state = 0;

static volatile bool tw_ready = false;

static uint8_t input_report_data[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

static void tw_set_heartbeat_led(bool en);
static sl_status_t app_imu_wom_enable(bool en);

// Call to change state from PRIMED state
static void tw_imu_wakeup_callback(uint8_t interrupt_no, void *ctx)
{
  (void)interrupt_no;

  if (!tw_ready) {
    return;
  }

  if (ctx == &imu) {
    if (tw_curr_state == TW_PRIMED) {
      tw_next_state = TW_TRIPPED;

      sl_bt_external_signal(EXT_SIG_UPDATE_FSM);
    }
  }
}

// Call to change state from DELAY state
static void tw_delay_timer_callback(sl_sleeptimer_timer_handle_t *handle,
                                    void *data)
{
  (void)handle;
  (void)data;

  if (!tw_ready) {
    return;
  }

  tw_next_state = TW_PRIMED;

  sl_bt_external_signal(EXT_SIG_UPDATE_FSM);
}

// Call to change state from DELAY or PRIMED state
static void tw_button_release_callback(void)
{
  if (!tw_ready) {
    return;
  }

  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_ATOMIC();

  bool update_fsm = false;

  switch (tw_curr_state) {
    case TW_IDLE:
      update_fsm = true;
      tw_next_state = TW_DELAY;
      break;

    /*
     *    There is a chance that the sleeptimer interrupt can be triggered
     *    before the DISENGAGE state is executed, thus overriding
     *    the tw_next_state from DISENGAGE to PRIMED.
     *    Options:
     *     1. Disable the sleeptimer here, but that prevents all state
     *         operations from being contained in tw_state_machine
     *     2. Have another new variable that indicates a state change is in
     *         progress (.e.g tw_state_change_in_progress) before every
     *         modification of tw_next_state the new variable should be
     *         checked.
     *
     *    If the state machine was more complicated, I would have gone with
     *    encapsulating all operations in a single function for easier code
     *    reading but this is simple so option 1 it is!
     */
    case TW_DELAY:
    case TW_PRIMED:
      sl_sleeptimer_stop_timer(&tw_delay_timer);
      update_fsm = true;
      tw_next_state = TW_DISENGAGE;
      break;

    case TW_TRIPPED:
    // If current state disengaging, do not register user's change
    case TW_DISENGAGE:
    default:
      break;
  }

  if (update_fsm) {
    sl_bt_external_signal(EXT_SIG_UPDATE_FSM);
  }

  CORE_EXIT_ATOMIC();
}

static void tw_heartbeat_led_timer_callback(
  sl_sleeptimer_timer_handle_t *handle,
  void *data)
{
  (void)handle;
  (void)data;

  if (!tw_ready) {
    return;
  }

  heartbeat_state ^= 0x01;
  tw_set_heartbeat_led(heartbeat_state);

  uint32_t time_period =
    heartbeat_state ? SIMPLE_HEARTBEAT_ON_MS : SIMPLE_HEARTBEAT_OFF_MS;
  sl_sleeptimer_start_timer_ms(&tw_heartbeat_led_timer,
                               time_period,
                               tw_heartbeat_led_timer_callback,
                               NULL,
                               32,
                               0);
}

static void tw_set_heartbeat_led(bool en)
{
  if (!tw_ready) {
    return;
  }

  if (en) {
    sl_led_turn_on(&sl_led_led0);
    heartbeat_state = 0x01;
    sl_sleeptimer_start_timer_ms(&tw_heartbeat_led_timer,
                                 SIMPLE_HEARTBEAT_ON_MS,
                                 tw_heartbeat_led_timer_callback,
                                 NULL,
                                 32,
                                 0);
  } else {
    sl_sleeptimer_stop_timer(&tw_heartbeat_led_timer);
    sl_led_turn_off(&sl_led_led0);
    heartbeat_state = 0x00;
  }
}

// Mealy machine
static void tw_state_machine(void)
{
  if (!tw_ready) {
    return;
  }

  sl_led_toggle(&sl_led_led0);

  sl_status_t sc;
  tripwire_state_t next_state = tw_next_state;

  switch (next_state) {
    case TW_DELAY:
      // Turn on 3s timer to delay priming the device after button 0 is
      //   released.
      // This allows the user to have enough time to readjust the device on the
      // doorknob after the button is released and prevent any unnecessary
      // triggering.
      // Case: Let the user press the button and give the user enough time to
      //   make
      // the device stay still from any potential swaying since this is hanging
      // from a doorknob.

      sl_sleeptimer_start_timer_ms(&tw_delay_timer,
                                   DELAY_TIMER_MS,
                                   tw_delay_timer_callback,
                                   NULL,
                                   32,
                                   0);

      break;

    case TW_PRIMED:
      // Turn on heartbeat to indicate to the user that the device is primed.
      // This LED is crucial to indicate that the device is also connected to
      //   your
      // computer.
      // Case: When the user looks at the device from across the room, the
      //   heartbeat
      // would indicate the device is on and play games safely.

      app_imu_wom_enable(true);

      tw_set_heartbeat_led(true);

      break;

    case TW_DISENGAGE:
      // The user can press button 0 to disengage the tripwire.
      // Case: The user can leave the room without triggering the tripwire.

      app_imu_wom_enable(false);

      tw_next_state = TW_IDLE;
      sl_bt_external_signal(EXT_SIG_UPDATE_FSM);

      tw_set_heartbeat_led(false);

      break;

    case TW_TRIPPED:
      // Send the ALT+TAB keyboard command to your computer. Turn off the
      //   heartbeat.
      // Once the command has been sent, the device should be in idle/standby
      //   state
      // and wait for the next button press.
      // Case: Tab out of your game into another workspace.

      app_imu_wom_enable(false);

      memset(input_report_data, 0, sizeof(input_report_data));

      input_report_data[REPORT_MODIFIER_INDEX] = MODIFIER_FORMAT(LEFT_ALT);
      input_report_data[REPORT_DATA_INDEX] = TAB_KEY;

      // "Press" ALT+TAB
      sc = sl_bt_gatt_server_notify_all(gattdb_report,
                                        sizeof(input_report_data),
                                        input_report_data);
      app_assert_status(sc);

      // "Release" ALT+TAB
      memset(input_report_data, 0, sizeof(input_report_data));
      sc = sl_bt_gatt_server_notify_all(gattdb_report,
                                        sizeof(input_report_data),
                                        input_report_data);
      app_assert_status(sc);

      tw_next_state = TW_IDLE;
      sl_bt_external_signal(EXT_SIG_UPDATE_FSM);

      tw_set_heartbeat_led(false);

      break;

    case TW_IDLE:
    default:
      tw_set_heartbeat_led(false);
      break;
  }

  tw_curr_state = next_state;
}

static void tw_reset(void)
{
  tw_curr_state = TW_IDLE;
  tw_next_state = TW_IDLE;

  app_imu_wom_enable(false);
  tw_set_heartbeat_led(false);

  sl_sleeptimer_stop_timer(&tw_delay_timer);
}

// Not interrupt safe
static sl_status_t app_imu_gpio_int_init(void (*callback)(uint8_t interrupt_no,
                                                          void *ctx))
{
  sl_status_t sc = SL_STATUS_OK;

  uint32_t interrupt = INTERRUPT_UNAVAILABLE;
  CMU_ClockEnable(cmuClock_GPIO, true);

  GPIO_PinModeSet(SL_ICM20648_INT_PORT,
                  SL_ICM20648_INT_PIN,
                  gpioModeInputPull,
                  1);

  GPIOINT_Init();   // Calls NVIC functions

  interrupt = GPIOINT_CallbackRegisterExt(SL_ICM20648_INT_PIN,
                                          (GPIOINT_IrqCallbackPtrExt_t)callback,
                                          &imu);
  if (interrupt == INTERRUPT_UNAVAILABLE) {
    return SL_STATUS_NOT_AVAILABLE;
  }

  imu.gpio_intno = interrupt;

  return sc;
}

static sl_status_t app_imu_init(void (*callback)(uint8_t interrupt_no,
                                                 void *ctx))
{
  sl_status_t sc = SL_STATUS_OK;

  if (imu.initialized) {
    return SL_STATUS_OK;
  }

  sc = sl_imu_init();
  if (sc != SL_STATUS_OK) {
    return sc;
  }

  sc = app_imu_gpio_int_init(callback);
  if (sc != SL_STATUS_OK) {
    return sc;
  }

  imu.initialized = true;

  return sc;
}

static sl_status_t app_imu_wom_enable(bool en)
{
  sl_status_t sc = SL_STATUS_OK;

  GPIO_ExtIntConfig(SL_ICM20648_INT_PORT,
                    SL_ICM20648_INT_PIN,
                    imu.gpio_intno,
                    true,
                    false,
                    en);
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);

  sc = sl_icm20648_enable_wake_on_motion_interrupt(en,
                                                   IMU_WOM_THRESHOLD,
                                                   IMU_SAMPLE_RATE);

  if (sc == SL_STATUS_OK) {
    imu.wom_enabled = en;
  }

  return sc;
}

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  app_imu_init(tw_imu_wakeup_callback);
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

      // Just works pairing
      sc = sl_bt_sm_configure(0, sl_bt_sm_io_capability_noinputnooutput);
      app_assert_status(sc);

      // Bonding required for HID device
      sc = sl_bt_sm_set_bondable_mode(1);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160,     // min. adv. interval (milliseconds * 1.6)
        160,     // max. adv. interval (milliseconds * 1.6)
        0,       // adv. duration
        0);      // max. num. adv. events
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      sc =
        sl_bt_sm_increase_security(evt->data.evt_connection_opened.connection);
      app_assert_status(sc);

      tw_reset();

      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      tw_reset();
      tw_ready = false;

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    case sl_bt_evt_sm_bonded_id:
      // Successful bonding
      break;

    case sl_bt_evt_sm_bonding_failed_id:
      /* Previous bond is broken, delete it and close connection,
       *  host must retry at least once */
      sl_bt_sm_delete_bondings();
      sl_bt_connection_close(evt->data.evt_sm_bonding_failed.connection);
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (evt->data.evt_gatt_server_characteristic_status.characteristic
          == gattdb_report) {
        // client characteristic configuration changed by remote GATT client
        if (evt->data.evt_gatt_server_characteristic_status.status_flags
            == sl_bt_gatt_server_client_config) {
          if (evt->data.evt_gatt_server_characteristic_status.
              client_config_flags == sl_bt_gatt_notification) {
            tw_ready = true;
          } else {
            tw_ready = false;
            tw_reset();
          }
        }
      }
      break;

    case  sl_bt_evt_system_external_signal_id: {
      uint32_t extsignals = evt->data.evt_system_external_signal.extsignals;

      if (extsignals & EXT_SIG_UPDATE_FSM) {
        tw_state_machine();
      }

      break;
    }

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

void sl_button_on_change(const sl_button_t *handle)
{
  // button 0
  if (handle == SL_SIMPLE_BUTTON_INSTANCE(0)) {
    if (sl_button_get_state(SL_SIMPLE_BUTTON_INSTANCE(0))
        == SL_SIMPLE_BUTTON_RELEASED) {
      tw_button_release_callback();
    }
  }
}
