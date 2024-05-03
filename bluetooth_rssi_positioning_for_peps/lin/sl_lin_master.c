/***************************************************************************//**
 * @file sl_lin_master.c
 * @brief Master-side LIN bus driver
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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

#pragma GCC optimize "O2"
#pragma GCC optimize "fast-math"
#pragma GCC optimize "gcse-after-reload"
#pragma GCC optimize "gcse-las"
#pragma GCC optimize "gcse-sm"
#pragma GCC optimize "ira-loop-pressure"
#pragma GCC optimize "ivopts"
#pragma GCC optimize "loop-interchange"
#pragma GCC optimize "modulo-sched"
#pragma GCC optimize "modulo-sched-allow-regmoves"
#pragma GCC optimize "no-delete-null-pointer-checks"
#pragma GCC optimize "no-tree-loop-distribute-patterns"
#pragma GCC optimize "omit-frame-pointer"
#pragma GCC optimize "peel-loops"
#pragma GCC optimize "predictive-commoning"
#pragma GCC optimize "rename-registers"
#pragma GCC optimize "split-loops"
#pragma GCC optimize "split-paths"
#pragma GCC optimize "stdarg-opt"
#pragma GCC optimize "tree-loop-distribution"
#pragma GCC optimize "tree-partial-pre"
#pragma GCC optimize "unswitch-loops"
#pragma GCC optimize "web"

#include "sl_lin_master.h"

#include <em_emu.h>
#include <em_cmu.h>
#include <em_gpio.h>
#include <em_prs.h>
#include <em_usart.h>
#include <em_timer.h>
#include <em_letimer.h>
#include <em_burtc.h>
#include <em_ramfunc.h>

#include <stddef.h>
#include <inttypes.h>
#include <string.h>

#include "sl_component_catalog.h"
#include "sl_sleeptimer.h"
#include "sl_power_manager.h"

#include "system.h"

// 10 msec window in 32 kHz ticks, rounded up
#define SL_LIN_WINDOW_TICKS 328

#define CURRENT_IDX         2

#define DO_WAKEUPS

// the baud rate is (HFXO * 2) / USART_CLKDIV_DIV (fractional as-is)
// N bits of time is (500000 * N * USART_CLKDIV_DIV) / HFXO usecs
// which converts to (N * USART_CLKDIV_DIV * 16384) / HFXO ticks
// with N=34 and HFXO=39000000 that translates to
// (8704 * USART_CLKDIV_DIV) / 609375
// (the raw register has an implicit multiplication of 8)
#define SL_BREAKDELIMSYNCPID_TICKS(reg) \
  (((reg & _USART_CLKDIV_DIV_MASK) * 1088) / 609375)

#define SL_LIN_SYNC_BYTE    0x55
#define SL_LIN_DIAG_REQUEST 0x3c

// indifferent, better kept high
// must be settable for TIMER_TEST if it's enabled
// for PD02 the range is 6..11
#define PRS_TIMER_CC0       11 // master-only

// can be 0..5,12..15
#define PRS_USART_TX        4
#define PRS_USART_RX        5

#define _GPIO_PRS_ASYNCH_ROUTE(n)      ASYNCH ## n ## ROUTE
#define _GPIO_PRS_ASYNCH_ROUTEEN(n)    GPIO_PRS_ROUTEEN_ASYNCH ## n ## PEN
#define _GPIO_PRS_ASYNCH_PORT_SHIFT(n) _GPIO_PRS_ASYNCH ## n ## ROUTE_PORT_SHIFT
#define _GPIO_PRS_ASYNCH_PIN_SHIFT(n)  _GPIO_PRS_ASYNCH ## n ## ROUTE_PIN_SHIFT

#define GPIO_PRS_ASYNCH_ROUTE(n)       _GPIO_PRS_ASYNCH_ROUTE(n)
#define GPIO_PRS_ASYNCH_ROUTEEN(n)     _GPIO_PRS_ASYNCH_ROUTEEN(n)
#define GPIO_PRS_ASYNCH_PORT_SHIFT(n)  _GPIO_PRS_ASYNCH_PORT_SHIFT(n)
#define GPIO_PRS_ASYNCH_PIN_SHIFT(n)   _GPIO_PRS_ASYNCH_PIN_SHIFT(n)

#define USART_TX_ROUTE                 GPIO_PRS_ASYNCH_ROUTE(PRS_USART_TX)
#define PRS_USART_TX_ROUTEEN           GPIO_PRS_ASYNCH_ROUTEEN(PRS_USART_TX)
#define USART_TX_ROUTE_PORT_SHIFT      GPIO_PRS_ASYNCH_PORT_SHIFT(PRS_USART_TX)
#define USART_TX_ROUTE_PIN_SHIFT       GPIO_PRS_ASYNCH_PIN_SHIFT(PRS_USART_TX)

#if defined(TIMER_TEST_PORT)
#define TIMER_CC0_ROUTE                GPIO_PRS_ASYNCH_ROUTE(PRS_TIMER_CC0)
#define PRS_TIMER_CC0_ROUTEEN          GPIO_PRS_ASYNCH_ROUTEEN(PRS_TIMER_CC0)
#define TIMER_CC0_PORT_SHIFT           GPIO_PRS_ASYNCH_PORT_SHIFT(PRS_TIMER_CC0)
#define TIMER_CC0_PIN_SHIFT            GPIO_PRS_ASYNCH_PIN_SHIFT(PRS_TIMER_CC0)
#endif

// PA05, the LIN_RX GPIO is the wakeup signal GPIO_EM0WU
#define WAKEUP_SIGNAL                  0U

#if defined(DO_WAKEUPS)
static volatile bool need_wakeup = true;
#endif

static unsigned int bittime;
static uint32_t last_comm_time = 0;

static uint8_t sleep_data[8] =
{ 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, };

volatile uint32_t sl_lin_counter_master_checksum = 0;
volatile uint32_t sl_lin_counter_master_conflict = 0;
volatile uint32_t sl_lin_counter_master_generic = 0;
volatile uint32_t sl_lin_counter_master_timeout = 0;

volatile uint32_t sl_lin_counter_slave1_checksum = 0;
volatile uint32_t sl_lin_counter_slave1_conflict = 0;
volatile uint32_t sl_lin_counter_slave1_generic = 0;

volatile uint32_t sl_lin_counter_slave2_checksum = 0;
volatile uint32_t sl_lin_counter_slave2_conflict = 0;
volatile uint32_t sl_lin_counter_slave2_generic = 0;

SL_RAMFUNC_DECLARATOR
__attribute__((__flatten__))
static void sl_lin_master_USART0_TX_IRQHandler(void);

SL_RAMFUNC_DECLARATOR
__attribute__((__flatten__))
static void sl_lin_master_USART0_RX_IRQHandler(void);

SL_RAMFUNC_DECLARATOR
__attribute__((__flatten__))
static void sl_lin_master_LETIMER0_IRQHandler(void);

// TODO: update master to use RTCC
#if defined(DO_WAKEUPS)
void BURTC_IRQHandler(void)
{
  // setting this over again without clearing does not affect anything
  need_wakeup = true;
  __DSB();
}

#endif

#if defined(SLAVE1_CHECKSUM_PIN)
#define SLAVE1_CHECKSUM_MASK (1U << SLAVE1_CHECKSUM_PIN) // 1
#else
#define SLAVE1_CHECKSUM_MASK 0
#endif

#if defined(SLAVE1_CONFLICT_PIN)
#define SLAVE1_CONFLICT_MASK (1U << SLAVE1_CONFLICT_PIN) // 3
#else
#define SLAVE1_CONFLICT_MASK 0
#endif

#if defined(SLAVE1_GENERIC_PIN)
#define SLAVE1_GENERIC_MASK  (1U << SLAVE1_GENERIC_PIN)  // 5
#else
#define SLAVE1_GENERIC_MASK  0
#endif

#if defined(SLAVE2_CHECKSUM_PIN)
#define SLAVE2_CHECKSUM_MASK (1U << SLAVE2_CHECKSUM_PIN) // 0
#else
#define SLAVE2_CHECKSUM_MASK 0
#endif

#if defined(SLAVE2_CONFLICT_PIN)
#define SLAVE2_CONFLICT_MASK (1U << SLAVE2_CONFLICT_PIN) // 2
#else
#define SLAVE2_CONFLICT_MASK 0
#endif

#if defined(SLAVE2_GENERIC_PIN)
#define SLAVE2_GENERIC_MASK  (1U << SLAVE2_GENERIC_PIN)  // 6
#else
#define SLAVE2_GENERIC_MASK  0
#endif

#if defined(SLAVE1_CHECKSUM_PORT) && defined(SLAVE1_CHECKSUM_PIN)
static void slave1_checksum_callback(uint8_t intNo)
{
  (void)intNo;
  sl_lin_counter_slave1_checksum++;
}

#endif

#if defined(SLAVE1_CONFLICT_PORT) && defined(SLAVE1_CONFLICT_PIN)
static void slave1_conflict_callback(uint8_t intNo)
{
  (void)intNo;
  sl_lin_counter_slave1_conflict++;
}

#endif

#if defined(SLAVE1_GENERIC_PORT) && defined(SLAVE1_GENERIC_PIN)
static void slave1_generic_callback(uint8_t intNo)
{
  (void)intNo;
  sl_lin_counter_slave1_generic++;
}

#endif

#if defined(SLAVE2_CHECKSUM_PORT) && defined(SLAVE2_CHECKSUM_PIN)
static void slave2_checksum_callback(uint8_t intNo)
{
  (void)intNo;
  sl_lin_counter_slave2_checksum++;
}

#endif

#if defined(SLAVE2_CONFLICT_PORT) && defined(SLAVE2_CONFLICT_PIN)
static void slave2_conflict_callback(uint8_t intNo)
{
  (void)intNo;
  sl_lin_counter_slave2_conflict++;
}

#endif

#if defined(SLAVE2_GENERIC_PORT) && defined(SLAVE2_GENERIC_PIN)
static void slave2_generic_callback(uint8_t intNo)
{
  (void)intNo;
  sl_lin_counter_slave2_generic++;
}

#endif

// blocking, use sparingly on the slave!
// might add a couple usecs of latency
static void sl_lin_delay(int usecs)
{
  usecs = (usecs - 17) / 5;

  TIMER_Enable(TIMER3, false);
  TIMER_TopSet(TIMER3, usecs);
  TIMER_CounterSet(TIMER3, usecs);
  TIMER_IntClear(TIMER3, TIMER_IF_UF);
  TIMER_Enable(TIMER3, true);
  while ((TIMER3->IF & TIMER_IF_UF) == 0) {}
}

// both for master and slave
void sl_lin_master_wakeup_bus(void)
{
  USART0->CTRL_SET = USART_CTRL_TXINV;
  sl_lin_delay(250);
  USART0->CTRL_CLR = USART_CTRL_TXINV;
}

// take care, input is not masked to gain speed
__attribute__((__pure__))
static uint8_t sl_lin_frame_id_to_pid(uint8_t frame_id)
{
  return frame_id
         | ((((frame_id >> 0U) & 1U)
             ^ ((frame_id >> 1U) & 1U)
             ^ ((frame_id >> 2U) & 1U)
             ^ ((frame_id >> 4U) & 1U)) << 6U)
         | ((((frame_id >> 1U) & 1U)
             ^ ((frame_id >> 3U) & 1U)
             ^ ((frame_id >> 4U) & 1U)
             ^ ((frame_id >> 5U) & 1U)
             ^ 1U) << 7U);
}

__attribute__((__pure__))
static uint8_t sl_lin_calc_checksum(uint8_t init, const uint8_t *data, int len)
{
  uint32_t checksum = init;

  if (likely(data != NULL)) {
    for (int i = 0; i < len; i++)
    {
      checksum += *data++;
      checksum += checksum >> 8;
      checksum &= 0xff;
    }
  }

  return checksum ^ 0xff;
}

void sl_lin_master_init(int baud)
{
  USART_InitAsync_TypeDef uart = USART_INITASYNC_DEFAULT;
  LETIMER_Init_TypeDef letimer = LETIMER_INIT_DEFAULT;
  TIMER_Init_TypeDef breakTimer = TIMER_INIT_DEFAULT;
  TIMER_InitCC_TypeDef breakTimerCC0 = TIMER_INITCC_DEFAULT;
  TIMER_Init_TypeDef wakeTimer = TIMER_INIT_DEFAULT;
#if defined(DO_WAKEUPS)
  BURTC_Init_TypeDef burtc = BURTC_INIT_DEFAULT;
#endif

  sl_lin_LETIMER0_IRQHandler = sl_lin_master_LETIMER0_IRQHandler;
  sl_lin_USART0_TX_IRQHandler = sl_lin_master_USART0_TX_IRQHandler;
  sl_lin_USART0_RX_IRQHandler = sl_lin_master_USART0_RX_IRQHandler;

  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_PRS, true);
  CMU_ClockEnable(cmuClock_LETIMER0, true);
  // the clock source of the TIMER is assumed to be HFXO
  CMU_ClockEnable(cmuClock_TIMER3, true);
  CMU_ClockEnable(cmuClock_TIMER4, true);
#if defined(DO_WAKEUPS)
  CMU_ClockEnable(cmuClock_BURTC, true);
#endif

  // CMU_ClockSelectSet(cmuClock_USART0, cmuSelect_EM01GRPACLK);
  CMU_ClockEnable(cmuClock_USART0, true);

  NVIC_DisableIRQ(LETIMER0_IRQn);
  LETIMER_IntDisable(LETIMER0, _LETIMER_IEN_MASK);
  LETIMER_IntClear(LETIMER0, _LETIMER_IF_MASK);
  NVIC_ClearPendingIRQ(LETIMER0_IRQn);

  NVIC_DisableIRQ(TIMER3_IRQn);
  TIMER_IntDisable(TIMER3, _TIMER_IEN_MASK);
  TIMER_IntClear(TIMER3, _TIMER_IF_MASK);
  NVIC_ClearPendingIRQ(TIMER3_IRQn);

  NVIC_DisableIRQ(TIMER4_IRQn);
  TIMER_IntDisable(TIMER4, _TIMER_IEN_MASK);
  TIMER_IntClear(TIMER4, _TIMER_IF_MASK);
  NVIC_ClearPendingIRQ(TIMER4_IRQn);

#if defined(DO_WAKEUPS)
  NVIC_DisableIRQ(BURTC_IRQn);
  BURTC_IntDisable(_BURTC_IEN_MASK);
  BURTC_IntClear(_BURTC_IF_MASK);
  NVIC_ClearPendingIRQ(BURTC_IRQn);
#endif

  NVIC_DisableIRQ(USART0_TX_IRQn);
  NVIC_DisableIRQ(USART0_RX_IRQn);
  USART_IntDisable(USART0, _USART_IEN_MASK);
  USART_IntClear(USART0, _USART_IF_MASK);
  NVIC_ClearPendingIRQ(USART0_TX_IRQn);
  NVIC_ClearPendingIRQ(USART0_RX_IRQn);

  GPIO->P_SET[LIN_TX_PORT].CTRL = GPIO_P_CTRL_DINDISALT;

  GPIO_PinModeSet(LIN_TX_PORT, LIN_TX_PIN, gpioModeWiredAndAlternate, 1);
  GPIO_PinModeSet(LIN_RX_PORT, LIN_RX_PIN, gpioModeInput, 0);

#if defined(TIMER_TEST_PORT)
  GPIO_PinModeSet(TIMER_TEST_PORT, TIMER_TEST_PIN, gpioModePushPull, 0);
#endif

#if defined(CHECKSUM_ERR_PORT) && defined(CHECKSUM_ERR_PIN)
  GPIO_PinModeSet(CHECKSUM_ERR_PORT, CHECKSUM_ERR_PIN, gpioModePushPull, 0);
#endif

#if defined(CONFLICT_ERR_PORT) && defined(CONFLICT_ERR_PIN)
  GPIO_PinModeSet(CONFLICT_ERR_PORT, CONFLICT_ERR_PIN, gpioModePushPull, 0);
#endif

#if defined(GENERIC_ERR_PORT) && defined(GENERIC_ERR_PIN)
  GPIO_PinModeSet(GENERIC_ERR_PORT, GENERIC_ERR_PIN, gpioModePushPull, 0);
#endif

#if defined(TIMEOUT_PORT) && defined(TIMEOUT_PIN)
  GPIO_PinModeSet(TIMEOUT_PORT, TIMEOUT_PIN, gpioModePushPull, 0);
#endif

#if defined(SLAVE1_CHECKSUM_PORT) && defined(SLAVE1_CHECKSUM_PIN)
  GPIOINT_CallbackRegister(SLAVE1_CHECKSUM_PIN, slave1_checksum_callback);
  GPIO_PinModeSet(SLAVE1_CHECKSUM_PORT, SLAVE1_CHECKSUM_PIN, gpioModeInput, 0);
  GPIO_ExtIntConfig(SLAVE1_CHECKSUM_PORT,
                    SLAVE1_CHECKSUM_PIN,
                    SLAVE1_CHECKSUM_PIN,
                    true,
                    true,
                    true);
#endif

#if defined(SLAVE1_CONFLICT_PORT) && defined(SLAVE1_CONFLICT_PIN)
  GPIOINT_CallbackRegister(SLAVE1_CONFLICT_PIN, slave1_conflict_callback);
  GPIO_PinModeSet(SLAVE1_CONFLICT_PORT, SLAVE1_CONFLICT_PIN, gpioModeInput, 0);
  GPIO_ExtIntConfig(SLAVE1_CONFLICT_PORT,
                    SLAVE1_CONFLICT_PIN,
                    SLAVE1_CONFLICT_PIN,
                    true,
                    true,
                    true);
#endif

#if defined(SLAVE1_GENERIC_PORT) && defined(SLAVE1_GENERIC_PIN)
  GPIOINT_CallbackRegister(SLAVE1_GENERIC_PIN, slave1_generic_callback);
  GPIO_PinModeSet(SLAVE1_GENERIC_PORT, SLAVE1_GENERIC_PIN, gpioModeInput, 0);
  GPIO_ExtIntConfig(SLAVE1_GENERIC_PORT,
                    SLAVE1_GENERIC_PIN,
                    SLAVE1_GENERIC_PIN,
                    true,
                    true,
                    true);
#endif

#if defined(SLAVE2_CHECKSUM_PORT) && defined(SLAVE2_CHECKSUM_PIN)
  GPIOINT_CallbackRegister(SLAVE2_CHECKSUM_PIN, slave2_checksum_callback);
  GPIO_PinModeSet(SLAVE2_CHECKSUM_PORT, SLAVE2_CHECKSUM_PIN, gpioModeInput, 0);
  GPIO_ExtIntConfig(SLAVE2_CHECKSUM_PORT,
                    SLAVE2_CHECKSUM_PIN,
                    SLAVE2_CHECKSUM_PIN,
                    true,
                    true,
                    true);
#endif

#if defined(SLAVE2_CONFLICT_PORT) && defined(SLAVE2_CONFLICT_PIN)
  GPIOINT_CallbackRegister(SLAVE2_CONFLICT_PIN, slave2_conflict_callback);
  GPIO_PinModeSet(SLAVE2_CONFLICT_PORT, SLAVE2_CONFLICT_PIN, gpioModeInput, 0);
  GPIO_ExtIntConfig(SLAVE2_CONFLICT_PORT,
                    SLAVE2_CONFLICT_PIN,
                    SLAVE2_CONFLICT_PIN,
                    true,
                    true,
                    true);
#endif

#if defined(SLAVE2_GENERIC_PORT) && defined(SLAVE2_GENERIC_PIN)
  GPIOINT_CallbackRegister(SLAVE2_GENERIC_PIN, slave2_generic_callback);
  GPIO_PinModeSet(SLAVE2_GENERIC_PORT, SLAVE2_GENERIC_PIN, gpioModeInput, 0);
  GPIO_ExtIntConfig(SLAVE2_GENERIC_PORT,
                    SLAVE2_GENERIC_PIN,
                    SLAVE2_GENERIC_PIN,
                    true,
                    true,
                    true);
#endif

  GPIO->USARTROUTE[0].ROUTEEN = 0;
  GPIO->USARTROUTE[0].RXROUTE = (LIN_RX_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT)
                                | (LIN_RX_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN;

  letimer.enable = false;
  letimer.comp0Top = true;
  LETIMER_Init(LETIMER0, &letimer);
  LETIMER0->PRSMODE = LETIMER_PRSMODE_PRSCLEARMODE_NONE
                      | LETIMER_PRSMODE_PRSSTOPMODE_NONE
                      | LETIMER_PRSMODE_PRSSTARTMODE_FALLING;

  wakeTimer.enable = false;
  // configure a 5-usec timer
  wakeTimer.prescale = (CMU_ClockFreqGet(cmuClock_TIMER3) / 200000) - 1;
  wakeTimer.mode = timerModeDown;
  wakeTimer.oneShot = true;
  wakeTimer.disSyncOut = false;
  TIMER_Init(TIMER3, &wakeTimer);

  // configure a microsecond-timer
  breakTimer.enable = false;
  // timer.prescale = (CMU_ClockFreqGet(cmuClock_TIMER4) / 1000000) - 1;
  breakTimer.prescale = 1;
  breakTimer.oneShot = true;
  breakTimer.disSyncOut = false;
  breakTimerCC0.cofoa = timerOutputActionToggle; // L->H transition, starts
                                                 //   RX&TX
  breakTimerCC0.cmoa = timerOutputActionToggle;  // H->L transition just to be
                                                 //   sure
  breakTimerCC0.mode = timerCCModeCompare;
  breakTimerCC0.coist = true;
  breakTimerCC0.prsOutput = timerPrsOutputLevel;
  TIMER_InitCC(TIMER4, 0, &breakTimerCC0);
  TIMER_Init(TIMER4, &breakTimer);

  bittime = CMU_ClockFreqGet(cmuClock_TIMER4)
            / ((breakTimer.prescale + 1) * baud);

  TIMER_TopSet(TIMER4, 14 * bittime - 1); // 13+1 bit time, wraparound & stop
  TIMER_CompareSet(TIMER4, 0, 1 * bittime - 1); // 1 bit time before H->L
                                                //   transition
  TIMER_CounterSet(TIMER4, 0);

  GPIO->PRSROUTE_CLR[0].ROUTEEN = PRS_USART_TX_ROUTEEN;
#if defined(TIMER_TEST_PORT)
  GPIO->PRSROUTE_CLR[0].ROUTEEN = PRS_TIMER_CC0_ROUTEEN;
#endif

  PRS_ConnectSignal(PRS_TIMER_CC0, prsTypeAsync, PRS_TIMER4_CC0);
  PRS_ConnectSignal(PRS_USART_TX, prsTypeAsync, PRS_USART0_TX);
  PRS_Combine(PRS_USART_TX, PRS_TIMER_CC0, prsLogic_A_AND_B);

  // the trigger is a L->H transition (unfortunately COIST=1 activates this)
  PRS_ConnectConsumer(PRS_TIMER_CC0, prsTypeAsync, prsConsumerUSART0_TRIGGER);
  // the trigger is a H->L transition
  PRS_ConnectConsumer(PRS_TIMER_CC0, prsTypeAsync, prsConsumerLETIMER0_START);

  GPIO->PRSROUTE[0].USART_TX_ROUTE =
    (LIN_TX_PORT << USART_TX_ROUTE_PORT_SHIFT)
    | (LIN_TX_PIN << USART_TX_ROUTE_PIN_SHIFT);

  GPIO->PRSROUTE_SET[0].ROUTEEN = PRS_USART_TX_ROUTEEN;

#if defined(TIMER_TEST_PORT)
  GPIO->PRSROUTE[0].TIMER_CC0_ROUTE =
    (TIMER_TEST_PORT << TIMER_CC0_PORT_SHIFT)
    | (TIMER_TEST_PIN << TIMER_CC0_PIN_SHIFT);

  GPIO->PRSROUTE_SET[0].ROUTEEN = PRS_TIMER_CC0_ROUTEEN;
#endif

#if defined(DO_WAKEUPS)
  burtc.start = false;
  burtc.compare0Top = true;
  BURTC_Init(&burtc);

  // slaves might enter sleep mode after 4..10 seconds of inactivity
  // so after at most 4 seconds of inactivity, a wakeup shall be triggered
  BURTC_CompareSet(0, 4 * 32768);
  NVIC_EnableIRQ(BURTC_IRQn);
  BURTC_IntEnable(BURTC_IEN_OF);
  BURTC_Start();
#endif

  uart.enable = usartDisable;
  uart.baudrate = baud;
  USART_InitAsync(USART0, &uart);

  USART0->TIMING_CLR = _USART_TIMING_TXDELAY_MASK;
  USART0->TIMING_SET = USART_TIMING_TXDELAY_ONE;
  USART0->CTRL_SET = USART_CTRL_TXBIL | USART_CTRL_ERRSDMA
                     | USART_CTRL_CCEN /* | USART_CTRL_LOOPBK_ENABLE */;
  USART0->TRIGCTRL = USART_TRIGCTRL_TXTEN | USART_TRIGCTRL_RXTEN;

  __DSB();

  uint32_t timeout = sl_sleeptimer_get_tick_count()
                     + sl_sleeptimer_ms_to_tick(50);

  // the PRS trigger will enable the TX and the RX due to COIST=1
  // wait for it and disable afterwards
  while (time_before(sl_sleeptimer_get_tick_count(), timeout)
         && ((USART0->STATUS & (USART_STATUS_TXENS | USART_STATUS_RXENS))
             != (USART_STATUS_TXENS | USART_STATUS_RXENS))) {}

  USART0->CMD = USART_CMD_TXDIS
                | USART_CMD_RXDIS
                | USART_CMD_RXBLOCKDIS
                | USART_CMD_CLEARTX
                | USART_CMD_CLEARRX;

  while ((USART0->STATUS & (_USART_STATUS_TXBUFCNT_MASK
                            | USART_STATUS_TXIDLE
                            | USART_STATUS_RXDATAV
                            | USART_STATUS_RXBLOCK
                            | USART_STATUS_TXENS
                            | USART_STATUS_RXENS)) != USART_STATUS_TXIDLE) {}

  NVIC_SetPriority(LETIMER0_IRQn, 2);
  NVIC_SetPriority(USART0_TX_IRQn, 2);
  NVIC_SetPriority(USART0_RX_IRQn, 2);
  NVIC_EnableIRQ(LETIMER0_IRQn);
  NVIC_EnableIRQ(USART0_TX_IRQn);
  NVIC_EnableIRQ(USART0_RX_IRQn);
}

static volatile uint8_t *lin_bus_data_ptr = NULL;
static volatile int lin_bus_data_len = 0;
static volatile unsigned int lin_bus_data_checksum = 0x00;
static volatile bool lin_bus_receiving = false;
static volatile bool lin_bus_completed = false;
static volatile sl_status_t lin_bus_result = SL_STATUS_OK;

SL_RAMFUNC_DEFINITION_BEGIN
__attribute__((__flatten__))
static void sl_lin_master_USART0_TX_IRQHandler(void)
{
  uint32_t ints = USART_IntGetEnabled(USART0);

  if (unlikely(ints & USART_IF_CCF)) {
    LETIMER0->CMD = LETIMER_CMD_STOP;

    USART0->CMD = USART_CMD_TXDIS | USART_CMD_RXBLOCKDIS | USART_CMD_RXDIS;
    // while ((USART0->STATUS & (USART_STATUS_TXENS | USART_STATUS_RXBLOCK |
    //   USART_STATUS_RXENS)) != 0);

    LETIMER_IntDisable(LETIMER0, _LETIMER_IEN_MASK);
    LETIMER_IntClear(LETIMER0, _LETIMER_IF_MASK);
    USART_IntDisable(USART0, _USART_IEN_MASK);
    USART_IntClear(USART0, _USART_IF_MASK);
    NVIC_ClearPendingIRQ(LETIMER0_IRQn);
    NVIC_ClearPendingIRQ(USART0_TX_IRQn);
    NVIC_ClearPendingIRQ(USART0_RX_IRQn);

#if defined(CONFLICT_ERR_PORT) && defined(CONFLICT_ERR_PIN)
    GPIO_PinOutToggle(CONFLICT_ERR_PORT, CONFLICT_ERR_PIN);
#endif
    sl_lin_counter_master_conflict++;

    lin_bus_result = SL_STATUS_ABORT;
    lin_bus_completed = true;

    return;
  }

  if (ints & USART_IF_TXC) {
    if (lin_bus_receiving) {
      USART0->CMD = USART_CMD_TXDIS | USART_CMD_RXBLOCKDIS;
      // while ((USART0->STATUS & (USART_STATUS_TXENS | USART_STATUS_RXBLOCK))
      //   != 0);

      USART_IntDisable(USART0, USART_IEN_TXC | USART_IEN_CCF);
      USART_IntClear(USART0, USART_IF_TXC | USART_IF_CCF);
      USART_IntEnable(USART0,
                      USART_IEN_RXDATAV | USART_IEN_FERR | USART_IEN_RXOF);
    } else {
      LETIMER0->CMD = LETIMER_CMD_STOP;

      USART0->CMD = USART_CMD_TXDIS | USART_CMD_RXBLOCKDIS | USART_CMD_RXDIS;
      // while ((USART0->STATUS & (USART_STATUS_TXENS | USART_STATUS_RXBLOCK |
      //   USART_STATUS_RXENS)) != 0);

      LETIMER_IntDisable(LETIMER0, _LETIMER_IEN_MASK);
      LETIMER_IntClear(LETIMER0, _LETIMER_IF_MASK);
      USART_IntDisable(USART0, _USART_IEN_MASK);
      USART_IntClear(USART0, _USART_IF_MASK);
      NVIC_ClearPendingIRQ(LETIMER0_IRQn);
      NVIC_ClearPendingIRQ(USART0_TX_IRQn);
      NVIC_ClearPendingIRQ(USART0_RX_IRQn);

      lin_bus_completed = true;
    }
  }

  // this is only enabled while sending
  if (ints & USART_IF_TXBL) {
    if (lin_bus_data_len == 0) {
      USART_IntDisable(USART0, USART_IEN_TXBL);
      USART0->TXDATA = lin_bus_data_checksum;
    } else {
      USART0->TXDATA = *lin_bus_data_ptr++;
    }

    USART_IntClear(USART0, USART_IF_TXBL);
    lin_bus_data_len--;

    return;
  }
}

SL_RAMFUNC_DEFINITION_END

SL_RAMFUNC_DEFINITION_BEGIN
__attribute__((__flatten__))
static void sl_lin_master_USART0_RX_IRQHandler(void)
{
  uint32_t ints = USART_IntGetEnabled(USART0);

  if (ints & (USART_IF_FERR | USART_IF_RXOF)) {
    LETIMER0->CMD = LETIMER_CMD_STOP;

    USART0->CMD = USART_CMD_RXDIS;
    // while ((USART0->STATUS & USART_STATUS_RXENS) != 0);

    LETIMER_IntDisable(LETIMER0, _LETIMER_IEN_MASK);
    LETIMER_IntClear(LETIMER0, _LETIMER_IF_MASK);
    USART_IntDisable(USART0, _USART_IEN_MASK);
    USART_IntClear(USART0, _USART_IF_MASK);
    NVIC_ClearPendingIRQ(LETIMER0_IRQn);
    NVIC_ClearPendingIRQ(USART0_TX_IRQn);
    NVIC_ClearPendingIRQ(USART0_RX_IRQn);

#if defined(GENERIC_ERR_PORT) && defined(GENERIC_ERR_PIN)
    GPIO_PinOutToggle(GENERIC_ERR_PORT, GENERIC_ERR_PIN);
#endif
    sl_lin_counter_master_generic++;

    lin_bus_result = SL_STATUS_IO;
    lin_bus_completed = true;

    return;
  }

  if (ints & USART_IF_RXDATAV) {
    uint8_t ch;

    ch = USART0->RXDATA;
    USART_IntClear(USART0, USART_IF_RXDATAV);

    if (lin_bus_data_len == 0) {
      uint32_t checksum;

      LETIMER0->CMD = LETIMER_CMD_STOP;

      USART0->CMD = USART_CMD_RXDIS;
      // while ((USART0->STATUS & USART_STATUS_RXENS) != 0);

      LETIMER_IntDisable(LETIMER0, _LETIMER_IEN_MASK);
      LETIMER_IntClear(LETIMER0, _LETIMER_IF_MASK);
      USART_IntDisable(USART0, _USART_IEN_MASK);
      USART_IntClear(USART0, _USART_IF_MASK);
      NVIC_ClearPendingIRQ(LETIMER0_IRQn);
      NVIC_ClearPendingIRQ(USART0_TX_IRQn);
      NVIC_ClearPendingIRQ(USART0_RX_IRQn);

      checksum = lin_bus_data_checksum + ch;
      while (checksum > 0xff)
      {
        checksum -= 0xff;
      }

      if (unlikely(checksum != 0xff)) {
#if defined(CHECKSUM_ERR_PORT) && defined(CHECKSUM_ERR_PIN)
        GPIO_PinOutToggle(CHECKSUM_ERR_PORT, CHECKSUM_ERR_PIN);
#endif
        sl_lin_counter_master_checksum++;

        lin_bus_result = SL_STATUS_IO;
      }

      lin_bus_completed = true;
    } else {
      uint8_t ch = USART0->RXDATA;
      *lin_bus_data_ptr++ = ch;
      lin_bus_data_len--;
      lin_bus_data_checksum += ch;
    }
  }
}

SL_RAMFUNC_DEFINITION_END

SL_RAMFUNC_DEFINITION_BEGIN
__attribute__((__flatten__))
static void sl_lin_master_LETIMER0_IRQHandler(void)
{
  LETIMER0->CMD = LETIMER_CMD_STOP;

  USART0->CMD = USART_CMD_TXDIS | USART_CMD_RXBLOCKDIS | USART_CMD_RXDIS;
  // while ((USART0->STATUS & (USART_STATUS_TXENS | USART_STATUS_RXBLOCK |
  //   USART_STATUS_RXENS)) != 0);

  LETIMER_IntDisable(LETIMER0, _LETIMER_IEN_MASK);
  LETIMER_IntClear(LETIMER0, _LETIMER_IF_MASK);
  USART_IntDisable(USART0, _USART_IEN_MASK);
  USART_IntClear(USART0, _USART_IF_MASK);
  NVIC_ClearPendingIRQ(LETIMER0_IRQn);
  NVIC_ClearPendingIRQ(USART0_TX_IRQn);
  NVIC_ClearPendingIRQ(USART0_RX_IRQn);

#if defined(TIMEOUT_PORT) && defined(TIMEOUT_PIN)
  GPIO_PinOutToggle(TIMEOUT_PORT, TIMEOUT_PIN);
#endif
  sl_lin_counter_master_timeout++;

  lin_bus_result = SL_STATUS_TIMEOUT;
  lin_bus_completed = true;
}

SL_RAMFUNC_DEFINITION_END

sl_status_t sl_lin_master_transmit(uint8_t frame_id,
                                   const uint8_t *data,
                                   int len,
                                   bool enhanced_checksum,
                                   bool inject_checksum_error,
                                   bool limiter)
{
  CORE_DECLARE_IRQ_STATE;
  uint8_t pid, checksum;
  uint32_t now, flags = USART_IEN_TXC | USART_IEN_CCF;

  if (unlikely((data == NULL) || (len == 0))) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  if (unlikely(frame_id >= 0x3e)) {
    return SL_STATUS_INVALID_RANGE;
  }

  if (unlikely(frame_id >= 0x3c)) {
    enhanced_checksum = false;
  }

  pid = sl_lin_frame_id_to_pid(frame_id);
  checksum = sl_lin_calc_checksum((enhanced_checksum) ? pid : 0x00,
                                  data, len);
  if (unlikely(inject_checksum_error)) {
    checksum ^= 0xff;
  }

  // have to block RX for collision detection
  USART0->CMD = USART_CMD_RXBLOCKEN
                | USART_CMD_CLEARTX
                | USART_CMD_CLEARRX;

  while ((USART0->STATUS & (_USART_STATUS_TXBUFCNT_MASK
                            | USART_STATUS_TXIDLE
                            | USART_STATUS_RXDATAV
                            | USART_STATUS_RXBLOCK))
         != (USART_STATUS_TXIDLE
             | USART_STATUS_RXBLOCK)) {}

  USART_IntClear(USART0, _USART_IF_MASK);

  // the FIFO is assumed to be at least 2-level deep
  USART0->TXDATA = SL_LIN_SYNC_BYTE;
  USART0->TXDATA = pid;

  // prime more data if there's space in the FIFO
  while (len && ((USART0->STATUS & USART_STATUS_TXBL) != 0))
  {
    USART0->TXDATA = *data++;
    len--;
  }

  // also try to put the checksum if there's more room left
  if ((USART0->STATUS & USART_STATUS_TXBL) != 0) {
    USART0->TXDATA = checksum;
  } else {
    flags |= USART_IEN_TXBL;
  }

#if defined(DO_WAKEUPS)
  if (unlikely(need_wakeup)) {
    sl_lin_master_wakeup_bus();
    sl_lin_delay(100000);
  }

  BURTC_CounterReset();
  BURTC_IntClear(_BURTC_IF_MASK);
  NVIC_ClearPendingIRQ(BURTC_IRQn);
  need_wakeup = false;
#endif

  now = sl_sleeptimer_get_tick_count();
  if (limiter) {
    last_comm_time += SL_LIN_WINDOW_TICKS;

    while (time_before(now, last_comm_time))
    {
      now = sl_sleeptimer_get_tick_count();
    }
  }

  LETIMER0->TOP = SL_LIN_TIMEOUT;
  while (LETIMER0->SYNCBUSY != 0) {}
  LETIMER0->CMD = LETIMER_CMD_STOP | LETIMER_CMD_CLEAR;
  while (LETIMER0->SYNCBUSY != 0) {}
  LETIMER_IntClear(LETIMER0, LETIMER_IF_UF);
  LETIMER_IntEnable(LETIMER0, LETIMER_IF_UF);

  USART_IntClear(USART0, flags);
  USART_IntEnable(USART0, flags);

  lin_bus_data_ptr = (uint8_t *)data;
  lin_bus_data_len = len;
  lin_bus_data_checksum = checksum;
  lin_bus_receiving = false;
  lin_bus_completed = false;
  lin_bus_result = SL_STATUS_OK;

  __CORE_ENTER_CRITICAL();
  TIMER_Enable(TIMER4, true);
  last_comm_time = sl_sleeptimer_get_tick_count();
  __CORE_EXIT_CRITICAL();

  while (lin_bus_completed == false) {}

  return lin_bus_result;
}

sl_status_t sl_lin_master_request(uint8_t frame_id,
                                  uint8_t *data,
                                  int len,
                                  bool enhanced_checksum,
                                  bool limiter)
{
  CORE_DECLARE_IRQ_STATE;
  uint32_t now, checksum = 0;
  uint8_t pid;

  if (unlikely((data == NULL) || (len == 0))) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  if (unlikely(frame_id >= 0x3e)) {
    return SL_STATUS_INVALID_RANGE;
  }

  if (unlikely(frame_id >= 0x3c)) {
    enhanced_checksum = false;
  }

  pid = sl_lin_frame_id_to_pid(frame_id);

  if (enhanced_checksum) {
    checksum += pid;
  }

  // have to block RX for collision detection
  USART0->CMD = USART_CMD_RXBLOCKEN
                | USART_CMD_CLEARTX
                | USART_CMD_CLEARRX;

  while ((USART0->STATUS & (_USART_STATUS_TXBUFCNT_MASK
                            | USART_STATUS_TXIDLE
                            | USART_STATUS_RXDATAV
                            | USART_STATUS_RXBLOCK))
         != (USART_STATUS_TXIDLE
             | USART_STATUS_RXBLOCK)) {}

  USART_IntClear(USART0, _USART_IF_MASK);

  // the FIFO is assumed to be at least 2-level deep
  USART0->TXDATA = SL_LIN_SYNC_BYTE;
  USART0->TXDATA = pid;

#if defined(DO_WAKEUPS)
  if (unlikely(need_wakeup)) {
    sl_lin_master_wakeup_bus();
    sl_lin_delay(100000);
  }

  BURTC_CounterReset();
  BURTC_IntClear(_BURTC_IF_MASK);
  NVIC_ClearPendingIRQ(BURTC_IRQn);
  need_wakeup = false;
#endif

  now = sl_sleeptimer_get_tick_count();
  if (limiter) {
    last_comm_time += SL_LIN_WINDOW_TICKS;

    while (time_before(now, last_comm_time))
    {
      now = sl_sleeptimer_get_tick_count();
    }
  }

  LETIMER0->TOP = SL_LIN_TIMEOUT;
  while (LETIMER0->SYNCBUSY != 0) {}
  LETIMER0->CMD = LETIMER_CMD_STOP | LETIMER_CMD_CLEAR;
  while (LETIMER0->SYNCBUSY != 0) {}
  LETIMER_IntClear(LETIMER0, LETIMER_IF_UF);
  LETIMER_IntEnable(LETIMER0, LETIMER_IF_UF);

  USART_IntEnable(USART0, USART_IEN_TXC | USART_IEN_CCF);

  lin_bus_data_ptr = data;
  lin_bus_data_len = len;
  lin_bus_data_checksum = checksum;
  lin_bus_receiving = true;
  lin_bus_completed = false;
  lin_bus_result = SL_STATUS_OK;

  __CORE_ENTER_CRITICAL();
  TIMER_Enable(TIMER4, true);
  last_comm_time = sl_sleeptimer_get_tick_count();
  __CORE_EXIT_CRITICAL();

  while (lin_bus_completed == false) {}

  return lin_bus_result;
}

sl_status_t sl_lin_master_bus_sleep(bool force)
{
  (void)force;
  sl_status_t ret;

#if defined(DO_WAKEUPS)
  if (!force && need_wakeup) {
    return SL_STATUS_OK;
  }
#endif

  ret = sl_lin_master_transmit(SL_LIN_DIAG_REQUEST, sleep_data,
                               sizeof(sleep_data), false, false,
                               true);

#if defined(DO_WAKEUPS)
  need_wakeup = true;
#endif

  return ret;
}

void sl_lin_master_reset_counters(void)
{
  sl_lin_counter_master_checksum = 0;
  sl_lin_counter_master_conflict = 0;
  sl_lin_counter_master_generic = 0;
  sl_lin_counter_master_timeout = 0;

  sl_lin_counter_slave1_checksum = 0;
  sl_lin_counter_slave1_conflict = 0;
  sl_lin_counter_slave1_generic = 0;

  sl_lin_counter_slave2_checksum = 0;
  sl_lin_counter_slave2_conflict = 0;
  sl_lin_counter_slave2_generic = 0;
}
