/*! ----------------------------------------------------------------------------
 * @file    port_wdt.c
 * @brief   HW specific definitions and functions for Watchdog Interface
 *
 * @attention
 *
 * Copyright 2016 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author
 */

#include "port_common.h"
#include "em_cmu.h"
#include "em_wdog.h"

#if defined(WDOG0)
#define WDT_INSTANCE        WDOG0
#define WDT_CMU_CLOCK       cmuClock_WDOG0
#elif defined(WDOG)
#define WDT_INSTANCE        WDOG
#define WDT_CMU_CLOCK       cmuClock_WDOG
#endif
#define WDT_FREQUENCY_HZ    1000 /*ULFRCO*/

void wdt_init(int ms)
{
  WDOG_Init_TypeDef wdog_init = WDOG_INIT_DEFAULT;
#if defined(_WDOG_CTRL_CLKSEL_MASK)
  wdog_init.clkSel = wdogClkSelULFRCO;
#else
  CMU_ClockSelectSet(WDT_CMU_CLOCK, cmuSelect_ULFRCO);
#endif

  // Select the closest period time
  bool found = false;
  WDOG_PeriodSel_TypeDef period_to_use = wdogPeriod_9;
  for (int32_t factor = 8; (period_to_use < wdogPeriod_256k) && !(found);
       factor *= 2) {
    found = (ms < ((1000 * factor) / WDT_FREQUENCY_HZ));
    if (!found) {
      period_to_use++;
    }
  }
  wdog_init.perSel = period_to_use;

  // Initialize and start the wdog
  CMU_ClockEnable(WDT_CMU_CLOCK, true);
  WDOGn_Init(WDT_INSTANCE, &wdog_init);
}

void wdt_refresh(void)
{
  WDOGn_Feed(WDT_INSTANCE);
}
