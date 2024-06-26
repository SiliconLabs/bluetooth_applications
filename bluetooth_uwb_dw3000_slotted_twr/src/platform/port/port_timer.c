/*! ----------------------------------------------------------------------------
 * @file    port_timer.c
 * @brief   HW specific definitions and functions for Timing Interface(s)
 *
 * @attention
 *
 * Copyright 2016 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author
 */

#include "app.h"
#include "port_common.h"
#include "sl_common.h"
#include "sl_udelay.h"
#include "sl_component_catalog.h"
#include "sl_sleeptimer.h"
#include "em_cmu.h"
#ifdef SL_CATALOG_KERNEL_PRESENT
#include "FreeRTOS.h"
#include "task.h"
#endif

// sl_udelay_wait() is a SW loop thus its' precision is very bad let's use
//   DWT->CYCNT instead if this macro is enabled.
#define SLEEP_USE_DWT_AS_US_TIMER 1

// OS delay is less accurate but allows the MCU to sleep
#define SLEEP_USE_OS_DELAY        1

/* @fn         init_timer(void)
 * @brief     initiate timestamp (in CLOCKS_PER_SEC)
 * @parm     p_timestamp pointer on current system timestamp
 */
uint32_t init_timer(void)
{
  // Standard timer function(s), sleeptimer can be used (already initialized by
  //   GSDK)
  return 0;
}

/* @fn         start_timer(uint32 *p_timestamp)
 * @brief     save system timestamp (in CLOCKS_PER_SEC)
 * @parm     p_timestamp pointer on current system timestamp
 */
void start_timer(volatile uint32_t *p_timestamp)
{
  if (p_timestamp) {
    *p_timestamp = sl_sleeptimer_get_tick_count();
  }
}

/* @fn         check_timer(uint32 timestamp , uint32 time)
 * @brief     check if time from current timestamp over expectation
 * @param     [in] timestamp - current timestamp
 * @param     [in] time - time expectation (in CLOCKS_PER_SEC)
 * @return     true - time is over
 *             false - time is not over yet
 */
bool check_timer(uint32_t timestamp, uint32_t time)
{
  // Will wrap correctly no additional effort needed
  uint32_t ticks_elapsed = sl_sleeptimer_get_tick_count() - timestamp;
  return (sl_sleeptimer_tick_to_ms(ticks_elapsed) >= time);
}

/* @brief    Sleep
 *             -DDEBUG defined in Makefile prevents __WFI
 */
void Sleep(uint32_t dwMs)
{
#if defined(SL_CATALOG_KERNEL_PRESENT) && SLEEP_USE_OS_DELAY
  // if kernel is present and running use its' delay function because it allow
  //   the system to sleep
  if (osKernelRunning == osKernelGetState()) {
    osDelay(dwMs / osKernelGetTickFreq());
  } else
#endif
  {
    sl_sleeptimer_delay_millisecond(dwMs);
  }
}

/* @fn    usleep
 * @brief precise usleep() delay
 * */
void usleep(unsigned long time_us)
{
#if SLEEP_USE_DWT_AS_US_TIMER
  // init DWT
  ITM->LAR = 0xC5ACCE55;
  const uint32_t orig_DEMCR = CoreDebug->DEMCR;   // CoreDebug_DEMCR_TRCENA_Msk
                                                  //   might be set during
                                                  //   debugging
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  // WAIT
  const uint32_t start_tick = DWT->CYCCNT;
  const uint32_t counter_freq_mhz = CMU_ClockFreqGet(cmuClock_CORE) / 1000000UL;
  time_us = SL_MIN((UINT32_MAX / counter_freq_mhz) - 1, time_us);
  while ((DWT->CYCCNT - start_tick) < (time_us * counter_freq_mhz)) {}

  // deinit DWT
  CoreDebug->DEMCR = orig_DEMCR;
  DWT->CTRL &= (~DWT_CTRL_CYCCNTENA_Msk);
  ITM->LAR = 0;
#else
  sl_udelay_wait(time_us);
#endif
}
