/***************************************************************************//**
 * @file system.h
 * @brief Low-level support functions
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: LicenseRef-MSLA
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement
 * By installing, copying or otherwise using this software, you agree to the
 * terms of the MSLA.
 *
 ******************************************************************************/

#ifndef SYSTEM__H
#define SYSTEM__H

#include <em_emu.h>
#include <em_prs.h>

#define likely(x) \
  __builtin_expect((x), 1)
#define unlikely(x) \
  __builtin_expect((x), 0)

#define READ_ONCE(var) \
  (*(const volatile typeof(var) *) & (var))

#define WRITE_ONCE(var, val)                   \
  do                                           \
  {                                            \
    *(volatile typeof(var) *) & (var) = (val); \
  } while (0)

#define __CORE_ENTER_CRITICAL() \
  {                             \
    irqState = __get_PRIMASK(); \
    __disable_irq();            \
  }

#define __CORE_EXIT_CRITICAL() \
  {                            \
    if (irqState == 0U)        \
    {                          \
      __enable_irq();          \
    }                          \
  }

#define __CORE_YIELD_CRITICAL()       \
  {                                   \
    if ((__get_PRIMASK() & 1U) != 0U) \
    {                                 \
      __enable_irq();                 \
      __ISB();                        \
      __disable_irq();                \
    }                                 \
  }

// For an explanation of the time comparison, see these links:
// https://stackoverflow.com/a/37872151
// https://stackoverflow.com/questions/4714598/jiffies-counter-over-flow-case-linux/
// They work reliably if the difference is less than the half of the total
//   range,
// without doing fancy math

/**
 * @brief      Compare two 32-bit timestamps considering wraparond
 *
 * @param[in]  a           first timestamp
 * @param[in]  b           second timestamp
 *
 * @returns    true if a > b
 */
#define time_after(a, b) \
  ((int32_t)((b) - (a)) < 0)

/**
 * @brief      Compare two 32-bit timestamps considering wraparond
 *
 * @param[in]  a           first timestamp
 * @param[in]  b           second timestamp
 *
 * @returns    true if a < b
 */
#define time_before(a, b) \
  time_after(b, a)

#if defined(_SILICON_LABS_32B_SERIES_1)
enum
{
  prsTypeDefault = prsTypeSync,
};

#define PRS_Setup(channel, sync, sourcesignal, port, pin, loc) \
  do {                                                         \
    unsigned int __channel = (channel);                        \
    unsigned int __sync = (sync);                              \
    unsigned int __sourcesignal = (sourcesignal);              \
    unsigned int __port = (port);                              \
    unsigned int __pin = (pin);                                \
    unsigned int __loc = (loc);                                \
    BUS_RegBitWrite(&PRS->ROUTEPEN,                            \
                    _PRS_ROUTEPEN_CH0PEN_SHIFT + __channel,    \
                    0);                                        \
    PRS_ConnectSignal(__channel, __sync, __sourcesignal);      \
    PRS_GpioOutputLocation(__channel, __loc);                  \
  } while (0);
#elif defined(_SILICON_LABS_32B_SERIES_2)
enum
{
  prsTypeDefault = prsTypeAsync,
};

#define PRS_Setup(channel, sync, sourcesignal, port, pin, loc) \
  do {                                                         \
    unsigned int __channel = (channel);                        \
    unsigned int __sync = (sync);                              \
    unsigned int __sourcesignal = (sourcesignal);              \
    unsigned int __port = (port);                              \
    unsigned int __pin = (pin);                                \
    BUS_RegBitWrite(&GPIO->PRSROUTE[0].ROUTEEN,                \
                    __channel                                  \
                    + ((__sync                                 \
                        == prsTypeAsync) ?                     \
                       _GPIO_PRS_ROUTEEN_ASYNCH0PEN_SHIFT :    \
                       _GPIO_PRS_ROUTEEN_SYNCH0PEN_SHIFT),     \
                    0);                                        \
    PRS_ConnectSignal(__channel, __sync, __sourcesignal);      \
    PRS_PinOutput(__channel, __sync, __port, __pin);           \
  } while (0);
#else
#error "Please add an implementation for PRS_Setup()"
#endif

#endif // __SYSTEM_H
