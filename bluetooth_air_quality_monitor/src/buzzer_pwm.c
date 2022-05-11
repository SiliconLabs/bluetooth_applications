/***************************************************************************//**
 * @file buzzer_pwm.c
 * @brief PWM driver
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/
#include "buzzer_pwm.h"
#include "em_gpio.h"
#include "em_bus.h"
#include "em_cmu.h"

static CMU_Clock_TypeDef get_timer_clock(TIMER_TypeDef *timer)
{
#if defined(_CMU_HFCLKSEL_MASK) || defined(_CMU_CMD_HFCLKSEL_MASK)
  CMU_Clock_TypeDef timer_clock = cmuClock_HF;
#elif defined(_CMU_SYSCLKCTRL_MASK)
  CMU_Clock_TypeDef timer_clock = cmuClock_SYSCLK;
#else
#error "Unknown root of clock tree"
#endif

  switch ((uint32_t)timer) {
#if defined(TIMER0_BASE)
    case TIMER0_BASE:
      timer_clock = cmuClock_TIMER0;
      break;
#endif
#if defined(TIMER1_BASE)
    case TIMER1_BASE:
      timer_clock = cmuClock_TIMER1;
      break;
#endif
#if defined(TIMER2_BASE)
    case TIMER2_BASE:
      timer_clock = cmuClock_TIMER2;
      break;
#endif
#if defined(TIMER3_BASE)
    case TIMER3_BASE:
      timer_clock = cmuClock_TIMER3;
      break;
#endif
#if defined(TIMER4_BASE)
    case TIMER4_BASE:
      timer_clock = cmuClock_TIMER4;
      break;
#endif
#if defined(TIMER5_BASE)
    case TIMER5_BASE:
      timer_clock = cmuClock_TIMER5;
      break;
#endif
#if defined(TIMER6_BASE)
    case TIMER6_BASE:
      timer_clock = cmuClock_TIMER6;
      break;
#endif
#if defined(WTIMER0_BASE)
    case WTIMER0_BASE:
      timer_clock = cmuClock_WTIMER0;
      break;
#endif
#if defined(WTIMER1_BASE)
    case WTIMER1_BASE:
      timer_clock = cmuClock_WTIMER1;
      break;
#endif
#if defined(WTIMER2_BASE)
    case WTIMER2_BASE:
      timer_clock = cmuClock_WTIMER2;
      break;
#endif
#if defined(WTIMER3_BASE)
    case WTIMER3_BASE:
      timer_clock = cmuClock_WTIMER3;
      break;
#endif
    default:
      EFM_ASSERT(0);
      break;
  }
  return timer_clock;
}

sl_status_t buzzer_pwm_init(buzzer_pwm_instance_t *pwm,
                            buzzer_pwm_config_t *config)
{
  uint32_t tmrpresc, inputCount;
  bool prescFound = false;

  CMU_Clock_TypeDef timer_clock = get_timer_clock(pwm->timer);
  CMU_ClockEnable(timer_clock, true);

  // Set PWM pin as output
  CMU_ClockEnable(cmuClock_GPIO, true);
  GPIO_PinModeSet((GPIO_Port_TypeDef)pwm->port,
                  pwm->pin,
                  gpioModePushPull,
                  config->polarity);

  // Set CC channel parameters
  TIMER_InitCC_TypeDef channel_init = TIMER_INITCC_DEFAULT;
  channel_init.mode = timerCCModePWM;
  channel_init.cmoa = timerOutputActionToggle;
  channel_init.edge = timerEdgeBoth;
  channel_init.outInvert = (config->polarity == PWM_ACTIVE_LOW);
  TIMER_InitCC(pwm->timer, pwm->channel, &channel_init);

  // Configure CC channel pinout
#if defined(_SILICON_LABS_32B_SERIES_0)
  BUS_RegMaskedWrite(&pwm->timer->ROUTE,
                     _TIMER_ROUTE_LOCATION_MASK,
                     pwm->location << _TIMER_ROUTE_LOCATION_SHIFT);
#elif defined(_SILICON_LABS_32B_SERIES_1)
  BUS_RegMaskedWrite(&pwm->timer->ROUTELOC0,
                     _TIMER_ROUTELOC0_CC0LOC_MASK << (pwm->channel * 8U),
                     pwm->location << (pwm->channel * 8U));
#elif defined(_SILICON_LABS_32B_SERIES_2)
  volatile uint32_t * route_register = &GPIO->TIMERROUTE[TIMER_NUM(pwm->timer)].CC0ROUTE;
  route_register += pwm->channel;
  *route_register = (pwm->port << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
                    | (pwm->pin << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);
#else
#error "Unknown route setting"
#endif

  /*
   * Find the smallest possible TIMER prescaler that permits counting
   * the 50/60 Hz input without overflowing. The selected TIMER
   * frequency has previously been determined to be at least 1 MHz.
   */
  tmrpresc = timerPrescale1;

  while (!prescFound)
  {
      /*
       * Divide the TIMER frequency by the prescaler, then divide this
       * by 50 Hz (which requires more TIMER counts than does 60 Hz).
       * This needs to be less than the maximum possible timer count
       * (e.g. 0xFFFF for a 16-bit TIMER).
       */
      inputCount = (CMU_ClockFreqGet(timer_clock) >> tmrpresc) / 50;

      if (inputCount < _TIMER_CNT_MASK) {
          prescFound = true;
      } else {
          tmrpresc++;
      }
  }

  // Configure TIMER frequency
  uint32_t top = (CMU_ClockFreqGet(timer_clock) / (config->frequency)) - 1U;
  TIMER_TopSet(pwm->timer, top);

  // Set initial duty cycle to 0%
  TIMER_CompareSet(pwm->timer, pwm->channel, 0U);

  // Initialize TIMER
  TIMER_Init_TypeDef timer_init = TIMER_INIT_DEFAULT;
  timer_init.prescale = tmrpresc;
  TIMER_Init(pwm->timer, &timer_init);

  return SL_STATUS_OK;
}

sl_status_t buzzer_pwm_deinit(buzzer_pwm_instance_t *pwm)
{
  // Reset TIMER routes
  buzzer_pwm_stop(pwm);

#if defined(_SILICON_LABS_32B_SERIES_0)
  BUS_RegMaskedClear(&pwm->timer->ROUTE, _TIMER_ROUTE_LOCATION_MASK);
#elif defined(_SILICON_LABS_32B_SERIES_1)
  BUS_RegMaskedClear(&pwm->timer->ROUTELOC0, _TIMER_ROUTELOC0_CC0LOC_MASK << (pwm->channel * 8));
#elif defined(_SILICON_LABS_32B_SERIES_2)
  volatile uint32_t * route_register = &GPIO->TIMERROUTE[TIMER_NUM(pwm->timer)].CC0ROUTE;
  route_register += pwm->channel;
  *route_register = 0;
#else
#error "Unknown route setting"
#endif

  // Reset TIMER
  TIMER_Reset(pwm->timer);

  // Reset GPIO
  GPIO_PinModeSet((GPIO_Port_TypeDef)pwm->port,
                  pwm->pin,
                  gpioModeDisabled,
                  0);

  CMU_Clock_TypeDef timer_clock = get_timer_clock(pwm->timer);
  CMU_ClockEnable(timer_clock, false);

  return SL_STATUS_OK;
}

void buzzer_pwm_start(buzzer_pwm_instance_t *pwm)
{
  // Enable PWM output
#if defined(_SILICON_LABS_32B_SERIES_0)
  BUS_RegMaskedSet(&pwm->timer->ROUTE,
                   1 << (pwm->channel + _TIMER_ROUTE_CC0PEN_SHIFT));
#elif defined(_SILICON_LABS_32B_SERIES_1)
  BUS_RegMaskedSet(&pwm->timer->ROUTEPEN,
                   1 << (pwm->channel + _TIMER_ROUTEPEN_CC0PEN_SHIFT));
#elif defined(_SILICON_LABS_32B_SERIES_2)
  GPIO->TIMERROUTE_SET[TIMER_NUM(pwm->timer)].ROUTEEN = 1 << (pwm->channel + _GPIO_TIMER_ROUTEEN_CC0PEN_SHIFT);
#else
#error "Unknown route setting"
#endif
}

void buzzer_pwm_stop(buzzer_pwm_instance_t *pwm)
{
  // Disable PWM output
#if defined(_SILICON_LABS_32B_SERIES_0)
  BUS_RegMaskedClear(&pwm->timer->ROUTE,
                     1 << (pwm->channel + _TIMER_ROUTE_CC0PEN_SHIFT));
#elif defined(_SILICON_LABS_32B_SERIES_1)
  BUS_RegMaskedClear(&pwm->timer->ROUTEPEN,
                     1 << (pwm->channel + _TIMER_ROUTEPEN_CC0PEN_SHIFT));
#elif defined(_SILICON_LABS_32B_SERIES_2)
  GPIO->TIMERROUTE_CLR[TIMER_NUM(pwm->timer)].ROUTEEN = 1 << (pwm->channel + _GPIO_TIMER_ROUTEEN_CC0PEN_SHIFT);
#else
#error "Unknown route setting"
#endif

  // Keep timer running in case other channels are in use
}

void buzzer_pwm_set_duty_cycle(buzzer_pwm_instance_t *pwm, uint8_t percent)
{
  uint32_t top = TIMER_TopGet(pwm->timer);

  // Set compare value
  TIMER_CompareBufSet(pwm->timer, pwm->channel, (top * percent) / 100);
}

uint8_t buzzer_pwm_get_duty_cycle(buzzer_pwm_instance_t *pwm)
{
  uint32_t top = TIMER_TopGet(pwm->timer);
  uint32_t compare = TIMER_CaptureGet(pwm->timer, pwm->channel);

  uint8_t percent = (uint8_t)((compare * 100) / top);

  return percent;
}

void buzzer_pwm_set_frequency(buzzer_pwm_instance_t *pwm, uint32_t freq)
{
  CMU_Clock_TypeDef timer_clock = get_timer_clock(pwm->timer);
  uint32_t top = (CMU_ClockFreqGet(timer_clock) / freq - 1U);

  TIMER_TopBufSet(pwm->timer, top);
}

uint32_t buzzer_pwm_get_frequency(buzzer_pwm_instance_t *pwm)
{
  CMU_Clock_TypeDef timer_clock = get_timer_clock(pwm->timer);
  uint32_t freq = (uint32_t)CMU_ClockFreqGet(timer_clock)/TIMER_TopGet(pwm->timer);

  return freq;
}
