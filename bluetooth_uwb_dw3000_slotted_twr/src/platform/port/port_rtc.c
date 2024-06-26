/*! ----------------------------------------------------------------------------
 * @file    port_rtc.c
 * @brief   HW specific definitions and functions for RTC Interface
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
#include "em_cmu.h"
#include "em_burtc.h" // Sleeptimer could be also used but callaback overhead is
                      //   bigger --> delay --> "global Super Frame" timestamp
                      //   would be less accurate
#include "sl_common.h"

#define RTC_FREQUENCY 32768ULL

static uint32_t rtc_wakeup_time_in_ticks;

void juniper_configure_hal_rtc_callback(void (*cb)(void))
{
  app.HAL_RTCEx_WakeUpTimerEventCb = cb;
}

void BURTC_IRQHandler(void)
{
  BURTC_IntClear(_BURTC_IF_MASK);
  if (app.HAL_RTCEx_WakeUpTimerEventCb) {
    app.HAL_RTCEx_WakeUpTimerEventCb();
  }
}

/*
 * @brief   setup RTC Wakeup timer
 *          period_ms is awaiting time in ms
 * */
void rtc_configure_wakeup_ns(uint32_t period_ns)
{
#if ((UINT32_MAX * RTC_FREQUENCY) / 1000000000ULL) > UINT32_MAX
#error period_ns saturation is needed!
#endif
  // This API is bit of an overkill, most of the RTC counter increments with
  //   ~30.5 us
  rtc_wakeup_time_in_ticks =
    (uint32_t)(((uint64_t)period_ns * RTC_FREQUENCY) / 1000000000ULL);
  rtc_reload();
}

/*
 * @brief   setup RTC Wakeup timer
 *          period_ms is awaiting time in ms
 * */
void rtc_configure_wakeup_ms(uint32_t period_ms)
{
  period_ms = SL_MIN(((UINT32_MAX * 1000ULL) / RTC_FREQUENCY), period_ms);
  rtc_wakeup_time_in_ticks =
    (uint32_t)(((uint64_t)period_ms * RTC_FREQUENCY) / 1000);
  rtc_reload();
}

void rtc_reload(void)
{
  uint32_t new_cc = BURTC_CounterGet() + rtc_wakeup_time_in_ticks;
  BURTC_CompareSet(0, new_cc);
  rtc_enable_irq();
}

void rtc_disable_irq(void)
{
  BURTC_IntDisable(BURTC_IF_COMP);
}

void rtc_enable_irq(void)
{
  BURTC_IntClear(BURTC_IF_COMP);
  BURTC_IntEnable(BURTC_IF_COMP);
}

uint32_t rtc_counter_get(void)
{
  return BURTC_CounterGet();
}

/** @brief Initialization of the RTC driver instance
 */
uint32_t rtc_init(void)
{
  const BURTC_Init_TypeDef burtc_init_structure = {
    .start = true,
    .debugRun = false,
    .clkDiv = burtcClkDiv_1,
    .compare0Top = false,     // wrap to 0 on compare match event
    .em4comp = false,
    .em4overflow = false
  };
  CMU_ClockEnable(cmuClock_BURTC, true);
  BURTC_Init(&burtc_init_structure);

  // configure the RTC Wakeup timer with a high priority;
  // this timer is saving global Super Frame Timestamp,
  // so we want this timestamp as stable as we can.
  NVIC_SetPriority(BURTC_IRQn, PRIO_RTC_WKUP_IRQn);
  NVIC_ClearPendingIRQ(BURTC_IRQn);
  NVIC_EnableIRQ(BURTC_IRQn);

  return _NO_ERR;
}

void rtc_deinit(void)
{
  // stop the RTC timer
  BURTC_Reset();
  NVIC_DisableIRQ(BURTC_IRQn);
}
