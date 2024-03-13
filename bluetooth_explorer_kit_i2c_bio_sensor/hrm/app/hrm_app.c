/**************************************************************************//**
* @file hrm_app.c
* @brief BLE HRM/SpO2 demo using Maxm86161 sensor
* @version 1.0.0
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
#include "hrm/app/hrm_helper.h"
#include "hrm/config/maxm86161_hrm_config.h"
#include "hrm/lib/maxm86161_hrm_spo2.h"
#include "em_gpio.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "sl_i2cspm.h"
#include "sl_bt_api.h"
#include "sl_simple_button_instances.h"
#include "sl_sleeptimer.h"
#include "hrm_app.h"
#include "gpiointerrupt.h"

/**************************************************************************//**
 * Local defines
 *****************************************************************************/
// configuration pinout for BGM220 Explorer Kit
#ifdef BGM220PC22HNA
#define MAXM86161_INT_GPIO_PORT         gpioPortB
#define MAXM86161_INT_GPIO_PIN          3

#define MAXM86161_EN_GPIO_PORT          gpioPortC
#define MAXM86161_EN_GPIO_PIN           3
#endif

// configuration pinout for BG22 Explorer Kit
#ifdef EFR32BG22C224F512IM40
#define MAXM86161_INT_GPIO_PORT         gpioPortB
#define MAXM86161_INT_GPIO_PIN          3

#define MAXM86161_EN_GPIO_PORT          gpioPortC
#define MAXM86161_EN_GPIO_PIN           3
#endif

// configuration pinout for xG24 Explorer Kit
#ifdef EFR32MG24B210F1536IM48
#define MAXM86161_INT_GPIO_PORT         gpioPortB
#define MAXM86161_INT_GPIO_PIN          1

#define MAXM86161_EN_GPIO_PORT          gpioPortC
#define MAXM86161_EN_GPIO_PIN           0
#endif

/**************************************************************************//**
 * Global variables
 *****************************************************************************/

/** Sensor handle for the maxm86161 hrm algorithm */
maxm_hrm_handle_t *hrmHandle;

/** Data Storage memory bock required by the maxm86161 hrm algorithm */
mamx86161_hrm_data_storage_t hrm_data_storage;
maxm86161_spo2_data_storage_t spo2_data;
maxm86161_data_storage_t data_storage;

/** Heart Rate result from the si117xhrm algorithm */
static int16_t heart_rate;

/** SpO2 result from the si117xhrm algorithm */
static int16_t spo2;

static int32_t hrm_status = 0;

static bool hrm_contac_status = false;

mamx86161_hrm_data_t hrm_data;

/** Flag used to indicate HRM/SpO2 state */
static volatile hrm_spo2_state_t hrm_spo2_state = HRM_STATE_IDLE;

static void max86161_int_callback(uint8_t intNo);

/**************************************************************************//**
 * Local prototypes
 *****************************************************************************/
static void hrm_gpio_setup(void);

/**************************************************************************//**
 * @brief Initialize the HRM application.
 *****************************************************************************/
int32_t hrm_init_app(void)
{
  int32_t err;

  /* Initialize peripherals*/
  hrm_gpio_setup();

  data_storage.spo2 = &spo2_data;
  data_storage.hrm = &hrm_data_storage;

  err = maxm86161_hrm_initialize(&data_storage, &hrmHandle);
  if (err != SL_STATUS_OK) {
    return err;
  }

  err = maxm86161_hrm_configure(hrmHandle, NULL, true);
  if (err != SL_STATUS_OK) {
    return err;
  }

  return SL_STATUS_OK;
}

/**************************************************************************//**
 * Simple Button
 * Button state changed callback
 * @param[in] handle
 *    Button event handle
 *****************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  // Button pressed.
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED) {
    if (&sl_button_btn0 == handle) {
      sl_bt_external_signal(BTN0_IRQ_EVENT);
    }
  }
}

/**************************************************************************//**
 * @brief Setup GPIO, enable sensor isolation switch
 *****************************************************************************/
static void hrm_gpio_setup(void)
{
  /* Enable GPIO clock */
  CMU_ClockEnable(cmuClock_GPIO, true);

  GPIO_PinModeSet(MAXM86161_EN_GPIO_PORT,
                  MAXM86161_EN_GPIO_PIN,
                  gpioModePushPull,
                  1);
  GPIO_PinModeSet(MAXM86161_INT_GPIO_PORT,
                  MAXM86161_INT_GPIO_PIN,
                  gpioModeInputPull,
                  1);

  GPIOINT_Init();
  GPIOINT_CallbackRegister(MAXM86161_INT_GPIO_PIN, max86161_int_callback);
  GPIO_ExtIntConfig(MAXM86161_INT_GPIO_PORT,
                    MAXM86161_INT_GPIO_PIN,
                    MAXM86161_INT_GPIO_PIN,
                    true,
                    true,
                    true);

  // need delay to wait the Maxim ready before we can read and write to register
  sl_sleeptimer_delay_millisecond(5);
}

/**************************************************************************//**
 * @brief Process HRM IRQ events.
 *****************************************************************************/
void hrm_process_event(uint32_t event_flags)
{
  if (event_flags == MAXM86161_IRQ_EVENT) {
    maxm86161_hrm_helper_process_irq();
  }

  if (event_flags == BTN0_IRQ_EVENT) {
    if (hrm_spo2_state == HRM_STATE_IDLE) {
      hrm_spo2_state = HRM_STATE_ACQUIRING;
      maxm86161_hrm_run(hrmHandle);
    } else {
      hrm_spo2_state = HRM_STATE_IDLE;
      maxm86161_hrm_pause();
    }
  }
}

/**************************************************************************//**
 * @brief HRM process main loop.
 *****************************************************************************/
void hrm_loop(void)
{
  int16_t num_samples_processed;
  int32_t err;

  err = maxm86161_hrm_process(hrmHandle,
                              &heart_rate,
                              &spo2,
                              1,
                              &num_samples_processed,
                              &hrm_status, &hrm_data);

  switch (hrm_spo2_state) {
    case HRM_STATE_IDLE:
      break;
    case HRM_STATE_ACQUIRING:
    case HRM_STATE_ACTIVE:
      if ((err == MAXM86161_HRM_SUCCESS)
          && (hrm_status & MAXM86161_HRM_STATUS_FRAME_PROCESSED)) {
        hrm_status &= ~MAXM86161_HRM_STATUS_FRAME_PROCESSED;

#if (UART_DEBUG & HRM_LEVEL)
        hrm_helper_output_debug_message(heart_rate, spo2);
#endif
        hrm_spo2_state = HRM_STATE_ACTIVE;
      }
#ifdef PROXIMITY
      else if ((hrm_status & MAXM86161_HRM_STATUS_FINGER_OFF)
               || (hrm_status & MAXM86161_HRM_STATUS_SPO2_FINGER_OFF)
               || (hrm_status & MAXM86161_HRM_STATUS_ZERO_CROSSING_INVALID)
               || (maxm86161_get_prox_mode())) {
#else
      else if ((hrm_status & MAXM86161_HRM_STATUS_FINGER_OFF)
               || (hrm_status & MAXM86161_HRM_STATUS_SPO2_FINGER_OFF)
               || (hrm_status & MAXM86161_HRM_STATUS_ZERO_CROSSING_INVALID)) {
#endif
        heart_rate = 0;
        spo2 = 0;
        hrm_spo2_state = HRM_STATE_ACQUIRING;
      }
      break;

    default:
      break;
  }
}

/**************************************************************************//**
 * @brief This function returns the current heart rate.
 *****************************************************************************/
int16_t hrm_get_heart_rate(void)
{
  return heart_rate;
}

/**************************************************************************//**
 * @brief This function returns the current finger contact status.
 *****************************************************************************/
bool hrm_get_status(void)
{
  hrm_contac_status = (hrm_spo2_state == HRM_STATE_ACTIVE);
  return hrm_contac_status;
}

/**************************************************************************//**
 * @brief This function returns the SpO2.
 *****************************************************************************/
int16_t hrm_get_spo2(void)
{
  return spo2;
}

/**************************************************************************//**
 * @brief Callback function for interrupt.
 *****************************************************************************/
static void max86161_int_callback(uint8_t intNo)
{
  (void) intNo;
  if (!GPIO_PinInGet(MAXM86161_INT_GPIO_PORT, MAXM86161_INT_GPIO_PIN)) {
    sl_bt_external_signal(MAXM86161_IRQ_EVENT);
  }
}
