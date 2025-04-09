/***************************************************************************//**
 * @file sl_lin.c
 * @brief Slave-side LIN bus driver
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "sl_lin.h"

#include <em_emu.h>
#include <em_cmu.h>
#include <em_core.h>
#include <em_gpio.h>
#include <em_prs.h>
#include <em_usart.h>
#include <em_timer.h>
#include <em_letimer.h>
#include <em_burtc.h>
#include <em_ldma.h>
#include <em_ramfunc.h>

#include <dmadrv.h>

#include <stddef.h>
#include <inttypes.h>
#include <string.h>

#include "sl_component_catalog.h"
#include "sl_sleeptimer.h"
#include "sl_power_manager.h"
#include "gpiointerrupt.h"

#include "system.h"

#define CURRENT_IDX         2

// the baud rate is (HFXO * 2) / USART_CLKDIV_DIV (fractional as-is)
// N bits of time is (500000 * N * USART_CLKDIV_DIV) / HFXO usecs
// which converts to (N * USART_CLKDIV_DIV * 16384) / HFXO ticks
// with N=34 and HFXO=39000000 that translates to
// (8704 * USART_CLKDIV_DIV) / 609375
// (the raw register has an implicit multiplication of 8)
#define SL_BREAKDELIMSYNCPID_TICKS(reg) \
  (((reg & _USART_CLKDIV_DIV_MASK) * 1088) / 609375)

typedef struct sl_lin_endpoint {
  sl_lin_callback_t callback;
  uint8_t bytes; // 0 if unused
  uint8_t next;  // used for triple buffering
  bool writable;
  bool enhcksum;
  uint8_t data[3][9]; // max. 8 bytes of data + 1 bytes of checksum
} sl_lin_endpoint_t;

static volatile sl_lin_endpoint_t sl_lin_endpoints[SL_LIN_MAX_ENDPOINT + 1];

static unsigned int DMA_CH_USART_TX = 0xffffffff;
static unsigned int DMA_CH_USART_RX = 0xffffffff;

#define SL_LIN_SYNC_BYTE    0x55
#define SL_LIN_DIAG_REQUEST 0x3c

// can be 0..5,12..15
#define PRS_USART_RX        5

// RX is pin 3, route to EXTI 5 (0 and 1 are the buttons of the WSTK)
#define LIN_RX_EXTI         5

#define _GPIO_PRS_ASYNCH_ROUTE(n)      ASYNCH ## n ## ROUTE
#define _GPIO_PRS_ASYNCH_ROUTEEN(n)    GPIO_PRS_ROUTEEN_ASYNCH ## n ## PEN
#define _GPIO_PRS_ASYNCH_PORT_SHIFT(n) _GPIO_PRS_ASYNCH ## n ## ROUTE_PORT_SHIFT
#define _GPIO_PRS_ASYNCH_PIN_SHIFT(n)  _GPIO_PRS_ASYNCH ## n ## ROUTE_PIN_SHIFT

#define GPIO_PRS_ASYNCH_ROUTE(n)       _GPIO_PRS_ASYNCH_ROUTE(n)
#define GPIO_PRS_ASYNCH_ROUTEEN(n)     _GPIO_PRS_ASYNCH_ROUTEEN(n)
#define GPIO_PRS_ASYNCH_PORT_SHIFT(n)  _GPIO_PRS_ASYNCH_PORT_SHIFT(n)
#define GPIO_PRS_ASYNCH_PIN_SHIFT(n)   _GPIO_PRS_ASYNCH_PIN_SHIFT(n)

// PA05, the LIN_RX GPIO is the wakeup signal GPIO_EM4WU0
#define WAKEUP_SIGNAL                  0U

static LDMA_Descriptor_t tx_desc =
  LDMA_DESCRIPTOR_SINGLE_M2P_BYTE(NULL, (void *)&(USART0->TXDATA), 0);
static LDMA_TransferCfg_t tx_transfer =
  LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_USART0_TXBL);

static LDMA_Descriptor_t rx_desc =
  LDMA_DESCRIPTOR_SINGLE_P2M_BYTE((void *)&(USART0->RXDATA), NULL, 0);
static LDMA_TransferCfg_t rx_transfer =
  LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_USART0_RXDATAV);

static volatile uint8_t current_frame_id = 0xff;

typedef enum sl_lin_slave_state {
  STATE_ERROR, // something bad has happened, finish this iteration
  STATE_WAITING, // ready to accept messages
  STATE_PROCESSING, // DMA has been started to do the transfer
  STATE_COMPLETED, // DMA callback has been called
} sl_lin_slave_state_t;

static volatile sl_lin_slave_state_t slave_state = STATE_ERROR;

static void wakeup_callback(uint8_t intNo);

SL_RAMFUNC_DECLARATOR
__attribute__((__flatten__))
__attribute__((__noinline__))
static void sl_lin_slave_LETIMER0_IRQHandler(void);

SL_RAMFUNC_DECLARATOR
__attribute__((__flatten__))
static void sl_lin_slave_USART0_TX_IRQHandler(void);

SL_RAMFUNC_DECLARATOR
__attribute__((__flatten__))
static void sl_lin_slave_USART0_RX_IRQHandler(void);

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

static void sl_lin_copy_buffer(uint8_t *dst, const uint8_t *src, int len)
{
  if (unlikely((len < 1) || (len > 9))) {
    return;
  }

  for (int i = 0; i < len; i++)
  {
    *dst++ = *src++;
  }
}

static void sl_lin_copy_buffer_and_checksum(uint8_t *dst,
                                            const uint8_t *src,
                                            int len,
                                            uint8_t init)
{
  uint32_t checksum;

  if (unlikely((len < 1) || (len > 8))) {
    return;
  }

  checksum = init;

  for (int i = 0; i < len; i++)
  {
    uint8_t ch;

    ch = *src++;
    checksum += ch;
    checksum += checksum >> 8;
    checksum &= 0xff;
    *dst++ = ch;
  }

  *dst++ = checksum ^ 0xff;
}

static void sl_lin_clear_buffer_and_checksum(uint8_t *dst, int len,
                                             uint8_t init)
{
  if (unlikely((len < 1) || (len > 8))) {
    return;
  }

  for (int i = 0; i < len; i++)
  {
    *dst++ = 0;
  }

  *dst++ = init ^ 0xff;
}

static void dma_start_tx(int frame_id)
{
  CORE_DECLARE_IRQ_STATE;
  volatile sl_lin_endpoint_t *ep = &sl_lin_endpoints[frame_id];

  __CORE_ENTER_CRITICAL();
  sl_lin_copy_buffer((void *)ep->data[CURRENT_IDX],
                     (void *)ep->data[ep->next],
                     ep->bytes + 1);
  __CORE_EXIT_CRITICAL();

  tx_desc.xfer.srcAddr = (uint32_t)ep->data[CURRENT_IDX];
  tx_desc.xfer.xferCnt = ep->bytes + 1 - 1;
  __DSB();

  USART_IntClear(USART0, USART_IF_TXC);

  LDMA_StartTransfer(DMA_CH_USART_TX, &tx_transfer, &tx_desc);
}

SL_RAMFUNC_DEFINITION_BEGIN
__attribute__((__flatten__))
static bool dma_rx_callback(unsigned int ch,
                            unsigned int sequenceNo,
                            void *param)
{
  (void)ch;
  (void)sequenceNo;
  CORE_DECLARE_IRQ_STATE;
  int frame_id;
  volatile sl_lin_endpoint_t *ep;

#if defined(DMA_ACT_PORT) && defined(DMA_ACT_PIN)
  GPIO_PinOutToggle(DMA_ACT_PORT, DMA_ACT_PIN);
#endif

  LETIMER_IntDisable(LETIMER0, LETIMER_IEN_UF);
  LETIMER_IntClear(LETIMER0, LETIMER_IF_UF);
  NVIC_ClearPendingIRQ(LETIMER0_IRQn);

  frame_id = (int)param;
  ep = &sl_lin_endpoints[frame_id];

  if (likely((slave_state == STATE_PROCESSING) && (ep->bytes > 0))) {
    uint8_t tmp, pid, checksum;

    slave_state = STATE_COMPLETED;

    __CORE_ENTER_CRITICAL();
    tmp = ep->next;
    ep->next = 1 - tmp;
    __CORE_EXIT_CRITICAL();

    pid = sl_lin_frame_id_to_pid(frame_id);
    checksum = sl_lin_calc_checksum((ep->enhcksum) ? pid : 0x00,
                                    (uint8_t *)ep->data[tmp],
                                    ep->bytes + 1);

#if defined(CHECKSUM_ERR_PORT) && defined(CHECKSUM_ERR_PIN)
    if (unlikely(checksum != 0x00)) {
      GPIO_PinOutToggle(CHECKSUM_ERR_PORT, CHECKSUM_ERR_PIN);
    }
#endif

    if (likely(ep->callback != NULL)) {
      ep->callback(frame_id, true, (uint8_t *)ep->data[tmp], ep->bytes,
                   checksum == 0x00);
    }

    current_frame_id = 0xff;
    __DSB();
  }

  sl_lin_slave_LETIMER0_IRQHandler();

#if defined(DMA_ACT_PORT) && defined(DMA_ACT_PIN)
  GPIO_PinOutToggle(DMA_ACT_PORT, DMA_ACT_PIN);
#endif

  return false;
}

SL_RAMFUNC_DEFINITION_END

static void dma_start_rx(int frame_id)
{
  volatile sl_lin_endpoint_t *ep = &sl_lin_endpoints[frame_id];

  rx_desc.xfer.dstAddr = (uint32_t)ep->data[ep->next];
  rx_desc.xfer.xferCnt = ep->bytes + 1 - 1;
  __DSB();

  DMADRV_SetCallbackParam(DMA_CH_USART_RX, (void *)frame_id);
  LDMA_StartTransfer(DMA_CH_USART_RX, &rx_transfer, &rx_desc);
}

static void sl_lin_slave_reinit_uart(void)
{
  CORE_DECLARE_IRQ_STATE;

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

  USART_IntClear(USART0, _USART_IF_MASK);
  USART_IntEnable(USART0, USART_IEN_RXDATAV);

  __CORE_ENTER_CRITICAL();

  // trigger an autobaud detection
  // next time the USART is enabled by a rising edge,
  // it will start detecting the baud rate
  USART0->CLKDIV = 0;

  while (LETIMER0->SYNCBUSY != 0) {}
  LETIMER0->CMD = LETIMER_CMD_CLEAR | LETIMER_CMD_STOP;
  while (LETIMER0->SYNCBUSY != 0) {}
  LETIMER_IntClear(LETIMER0, LETIMER_IF_UF);

  // continue enabling the autobaud detection
  USART0->CLKDIV = USART_CLKDIV_AUTOBAUDEN;

  current_frame_id = 0xff;
  slave_state = STATE_WAITING;

  __CORE_EXIT_CRITICAL();

  sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);

  GPIO_IntClear(1U << (_GPIO_IEN_EM4WUIEN0_SHIFT + WAKEUP_SIGNAL));
  GPIO_IntEnable(1U << (_GPIO_IEN_EM4WUIEN0_SHIFT + WAKEUP_SIGNAL));

  LETIMER_IntEnable(LETIMER0, LETIMER_IEN_UF);

  USART0->TRIGCTRL = USART_TRIGCTRL_RXTEN;
}

void sl_lin_slave_init(void)
{
  Ecode_t ecode;
  USART_InitAsync_TypeDef uart = USART_INITASYNC_DEFAULT;
  LETIMER_Init_TypeDef letimer = LETIMER_INIT_DEFAULT;
  TIMER_Init_TypeDef wakeTimer = TIMER_INIT_DEFAULT;

  sl_lin_LETIMER0_IRQHandler = sl_lin_slave_LETIMER0_IRQHandler;
  sl_lin_USART0_TX_IRQHandler = sl_lin_slave_USART0_TX_IRQHandler;
  sl_lin_USART0_RX_IRQHandler = sl_lin_slave_USART0_RX_IRQHandler;

  ecode = DMADRV_Init();
  if ((ecode != ECODE_OK)
      && (ecode != ECODE_EMDRV_DMADRV_ALREADY_INITIALIZED)) {
    return;
  }

  ecode = DMADRV_AllocateChannel(&DMA_CH_USART_TX, NULL);
  if (ecode != ECODE_EMDRV_DMADRV_OK) {
    return;
  }

  ecode = DMADRV_AllocateChannel(&DMA_CH_USART_RX, NULL);
  if (ecode != ECODE_EMDRV_DMADRV_OK) {
    DMADRV_FreeChannel(DMA_CH_USART_TX);
    DMA_CH_USART_TX = 0xffffffff;
    return;
  }

  // use the TXC interrupt of USART instead
  tx_desc.xfer.doneIfs = 0;

  // 360 - 380 usecs (re-enables and waits for clocks)
  NVIC_SetPriority(GPIO_EVEN_IRQn, CORE_INTERRUPT_DEFAULT_PRIORITY + 1);
  // 3.5 usecs
  NVIC_SetPriority(USART0_TX_IRQn, CORE_INTERRUPT_DEFAULT_PRIORITY + 1);
  // 5.5 - 6.5 usecs
  NVIC_SetPriority(USART0_RX_IRQn, CORE_INTERRUPT_DEFAULT_PRIORITY + 1);
  // 3.5 usecs
  NVIC_SetPriority(LDMA_IRQn, CORE_INTERRUPT_DEFAULT_PRIORITY + 2);
  // 17 - 38 usecs
  NVIC_SetPriority(LETIMER0_IRQn, CORE_INTERRUPT_DEFAULT_PRIORITY + 2);

  CMU_ClockEnable(cmuClock_ICACHE, true);
  BUS_RegBitWrite(&ICACHE0->CTRL, _ICACHE_CTRL_CACHEDIS_SHIFT, 0);
  BUS_RegMaskedWrite(&ICACHE0->LPMODE,
                     _ICACHE_LPMODE_LPLEVEL_MASK,
                     ICACHE_LPMODE_LPLEVEL_ADVANCED);
  CMU_ClockEnable(cmuClock_ICACHE, false);

  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_PRS, true);
  CMU_ClockEnable(cmuClock_LETIMER0, true);
  // the clock source of the TIMER is assumed to be HFXO
  CMU_ClockEnable(cmuClock_TIMER3, true);
  CMU_ClockEnable(cmuClock_BURTC, true);

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

  NVIC_DisableIRQ(BURTC_IRQn);
  BURTC_IntDisable(_BURTC_IEN_MASK);
  BURTC_IntClear(_BURTC_IF_MASK);
  NVIC_ClearPendingIRQ(BURTC_IRQn);

  NVIC_DisableIRQ(USART0_TX_IRQn);
  NVIC_DisableIRQ(USART0_RX_IRQn);
  USART_IntDisable(USART0, _USART_IEN_MASK);
  USART_IntClear(USART0, _USART_IF_MASK);
  NVIC_ClearPendingIRQ(USART0_TX_IRQn);
  NVIC_ClearPendingIRQ(USART0_RX_IRQn);

  DMADRV_SetCallback(DMA_CH_USART_RX, dma_rx_callback, NULL);

  GPIO->P_SET[LIN_TX_PORT].CTRL = GPIO_P_CTRL_DINDISALT;

  GPIO_PinModeSet(LIN_TX_PORT, LIN_TX_PIN, gpioModeWiredAndAlternate, 1);
  GPIO_PinModeSet(LIN_RX_PORT, LIN_RX_PIN, gpioModeInput, 0);

#if defined(DMA_ACT_PORT) && defined(DMA_ACT_PIN)
  GPIO_PinModeSet(DMA_ACT_PORT, DMA_ACT_PIN, gpioModePushPull, 0);
#endif

#if defined(TIMEOUT_ACT_PORT) && defined(TIMEOUT_ACT_PIN)
  GPIO_PinModeSet(TIMEOUT_ACT_PORT, TIMEOUT_ACT_PIN, gpioModePushPull, 0);
#endif

#if defined(UART_ACT_PORT) && defined(UART_ACT_PIN)
  GPIO_PinModeSet(UART_ACT_PORT, UART_ACT_PIN, gpioModePushPull, 0);
#endif

#if defined(WAKEUP_ACT_PORT) && defined(WAKEUP_ACT_PIN)
  GPIO_PinModeSet(WAKEUP_ACT_PORT, WAKEUP_ACT_PIN, gpioModePushPull, 0);
#endif

#if defined(SLEEP_ACT_PORT) && defined(SLEEP_ACT_PIN)
  GPIO_PinModeSet(SLEEP_ACT_PORT, SLEEP_ACT_PIN, gpioModePushPull, 0);
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

  GPIO->USARTROUTE[0].ROUTEEN = 0;
  GPIO->USARTROUTE[0].RXROUTE = (LIN_RX_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT)
                                | (LIN_RX_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].TXROUTE = (LIN_TX_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT)
                                | (LIN_TX_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN
                                | GPIO_USART_ROUTEEN_TXPEN;

  // "PRS output is not affected by the interrupt edge detection logic or gated
  //   by the IEN bits"
  GPIO_ExtIntConfig(LIN_RX_PORT, LIN_RX_PIN, LIN_RX_EXTI, true, true, false);
  PRS_ConnectSignal(PRS_USART_RX,
                    prsTypeAsync,
                    PRS_ASYNC_CH_CTRL_SOURCESEL_GPIO | (LIN_RX_EXTI << 0));

  PRS_ConnectConsumer(PRS_USART_RX, prsTypeAsync, prsConsumerLETIMER0_START);

  // the trigger is a L->H transition
  PRS_ConnectConsumer(PRS_USART_RX, prsTypeAsync, prsConsumerUSART0_TRIGGER);

  letimer.enable = false;
  letimer.comp0Top = true;
  LETIMER0->TOP = SL_LIN_TIMEOUT;
  LETIMER_Init(LETIMER0, &letimer);

  LETIMER0->PRSMODE = LETIMER_PRSMODE_PRSCLEARMODE_NONE
                      | LETIMER_PRSMODE_PRSSTARTMODE_FALLING
                      | LETIMER_PRSMODE_PRSSTOPMODE_NONE;

  wakeTimer.enable = false;
  // configure a 5-usec timer
  wakeTimer.prescale = (CMU_ClockFreqGet(cmuClock_TIMER3) / 200000) - 1;
  wakeTimer.mode = timerModeDown;
  wakeTimer.oneShot = true;
  wakeTimer.disSyncOut = false;
  TIMER_Init(TIMER3, &wakeTimer);

  uart.baudrate = 0;
  USART_InitAsync(USART0, &uart);

  NVIC_EnableIRQ(LETIMER0_IRQn);

  NVIC_EnableIRQ(USART0_TX_IRQn);
  NVIC_EnableIRQ(USART0_RX_IRQn);
  USART_IntEnable(USART0,
                  USART_IEN_TXC
                  | USART_IEN_CCF | USART_IEN_FERR
                  | USART_IEN_RXOF | USART_IEN_RXUF
                  | USART_IEN_RXDATAV);

  USART0->TIMING_CLR = _USART_TIMING_TXDELAY_MASK;
  USART0->TIMING_SET = USART_TIMING_TXDELAY_ONE;
  // NOTE: needs external loopback due to the PRS trickery!
  USART0->CTRL_SET = USART_CTRL_CCEN /* | USART_CTRL_LOOPBK_ENABLE */;

  GPIOINT_CallbackRegister(_GPIO_EM4WUEN_EM4WUEN_SHIFT + WAKEUP_SIGNAL,
                           wakeup_callback);
  // configure wakeup+IRQ on falling edge
  GPIO->EM4WUPOL_CLR = 1U << (_GPIO_EM4WUPOL_EM4WUPOL_SHIFT + WAKEUP_SIGNAL);
  GPIO->EM4WUEN_SET = 1U << (_GPIO_EM4WUEN_EM4WUEN_SHIFT + WAKEUP_SIGNAL);

  // for balancing the release in sl_lin_slave_reinit_uart()
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);

  sl_lin_slave_reinit_uart();
}

static void sl_lin_slave_abort_uart(void)
{
  USART0->TRIGCTRL = 0;

  slave_state = STATE_ERROR;
  __DSB();

  LDMA_StopTransfer(DMA_CH_USART_TX);
  LDMA_StopTransfer(DMA_CH_USART_RX);

  USART0->CMD = USART_CMD_TXDIS | USART_CMD_RXDIS | USART_CMD_RXBLOCKEN;
  USART_IntClear(USART0, _USART_IF_MASK);

  NVIC_ClearPendingIRQ(USART0_TX_IRQn);
  NVIC_ClearPendingIRQ(USART0_RX_IRQn);

  if (current_frame_id != 0xff) {
    volatile sl_lin_endpoint_t *ep = &sl_lin_endpoints[current_frame_id];

    if (likely((ep->callback != NULL) && (ep->bytes > 0))) {
      // NOTE: it might be impossible to detect the correct 'data'
      // for RX (writable endpoints) it is '_current'
      // but for TX (readable endpoints) it might be '_current' or '_next'
      // so just report NULL instead
      // the
      ep->callback(current_frame_id, ep->writable, NULL, ep->bytes, false);
    }

    current_frame_id = 0xff;
    __DSB();
  }
}

void sl_lin_slave_wakeup_bus(void)
{
  CORE_DECLARE_IRQ_STATE;

  __CORE_ENTER_CRITICAL();

  sl_lin_slave_abort_uart();

  USART0->CTRL_SET = USART_CTRL_TXINV;
  sl_lin_delay(250);
  USART0->CTRL_CLR = USART_CTRL_TXINV;

  sl_lin_slave_reinit_uart();

  __CORE_EXIT_CRITICAL();
}

SL_RAMFUNC_DEFINITION_BEGIN
__attribute__((__flatten__))
__attribute__((__noinline__))
static void sl_lin_slave_LETIMER0_IRQHandler(void)
{
#if defined(TIMEOUT_ACT_PORT) && defined(TIMEOUT_ACT_PIN)
  GPIO_PinOutToggle(TIMEOUT_ACT_PORT, TIMEOUT_ACT_PIN);
#endif
  sl_lin_slave_abort_uart();
  sl_lin_slave_reinit_uart();
#if defined(TIMEOUT_ACT_PORT) && defined(TIMEOUT_ACT_PIN)
  GPIO_PinOutToggle(TIMEOUT_ACT_PORT, TIMEOUT_ACT_PIN);
#endif
}

SL_RAMFUNC_DEFINITION_END

SL_RAMFUNC_DEFINITION_BEGIN
__attribute__((__flatten__))
static void sl_lin_slave_USART0_TX_IRQHandler(void)
{
  uint32_t flags;

#if defined(UART_ACT_PORT) && defined(UART_ACT_PIN)
  GPIO_PinOutToggle(UART_ACT_PORT, UART_ACT_PIN);
#endif

  flags = USART0->IF & (USART_IF_CCF | USART_IF_TXC);
  USART_IntClear(USART0, flags);

  if (unlikely((flags & USART_IF_CCF) != 0)) {
#if defined(CONFLICT_ERR_PORT) && defined(CONFLICT_ERR_PIN)
    GPIO_PinOutToggle(CONFLICT_ERR_PORT, CONFLICT_ERR_PIN);
#endif

    sl_lin_slave_abort_uart();
  } else {
    int frame_id;
    volatile sl_lin_endpoint_t *ep;

    frame_id = current_frame_id;
    ep = &sl_lin_endpoints[frame_id];

    if (likely((slave_state == STATE_PROCESSING) && (ep->bytes > 0))) {
      slave_state = STATE_COMPLETED;
      __DSB();

      LETIMER_IntDisable(LETIMER0, LETIMER_IEN_UF);
      LETIMER_IntClear(LETIMER0, LETIMER_IF_UF);
      NVIC_ClearPendingIRQ(LETIMER0_IRQn);

      if (likely(ep->callback != NULL)) {
        ep->callback(frame_id,
                     false,
                     (uint8_t *)ep->data[CURRENT_IDX],
                     ep->bytes,
                     (USART0->IF & USART_IF_CCF) == 0);
      }

      current_frame_id = 0xff;
      __DSB();

      sl_lin_slave_LETIMER0_IRQHandler();
    }
  }

#if defined(UART_ACT_PORT) && defined(UART_ACT_PIN)
  GPIO_PinOutToggle(UART_ACT_PORT, UART_ACT_PIN);
#endif
}

SL_RAMFUNC_DEFINITION_END

SL_RAMFUNC_DEFINITION_BEGIN
__attribute__((__flatten__))
static void sl_lin_slave_USART0_RX_IRQHandler(void)
{
  uint32_t flags, now;

#if defined(UART_ACT_PORT) && defined(UART_ACT_PIN)
  GPIO_PinOutToggle(UART_ACT_PORT, UART_ACT_PIN);
#endif

  now = LETIMER_CounterGet(LETIMER0);
  flags = USART_IntGetEnabled(USART0);

  if (unlikely(flags & (USART_IF_FERR | USART_IF_RXOF | USART_IF_RXUF))) {
#if defined(GENERIC_ERR_PORT) && defined(GENERIC_ERR_PIN)
    GPIO_PinOutToggle(GENERIC_ERR_PORT, GENERIC_ERR_PIN);
#endif

    sl_lin_slave_abort_uart();
  } else if (likely(flags & USART_IF_RXDATAV)) {
    uint32_t ticks;
    uint8_t pid;

    pid = USART0->RXDATA;

    // TODO: test maximum time
    // start a HFXO-based timer at the rising edge of the BREAK detection
    // it's followed by at least 1-bit time of delimiter (calculate with 2)
    // next is the 10-bit time of SYNC
    // the ID field is again 10 bits wide
    // finally add 4 bits for various overheads
    // and 40% for the time variation
    // that's 36.4 bits time
    // or 1820 usecs at 20 kbit/sec

    // TODO: or add RTCC + SR latch -based BREAK measurement instead
    // to capture the timestamp of the falling and rising edge
    // their distance shall be at least 11 bits by spec
    // (550 usec = 18.0224 ticks)
    // or 12 if better because of rounding to LF ticks (19.6608 ticks)
    // 18 ticks = 10.986 bits, 19 ticks = 11.596 bits
    // difference between the rising edge and the current time shall be
    // less than the maximum time defined above

    // TODO: ALL this is error-prone as the baud rate could be misdetected
    // USART_DIV_DIV was 6688 for example, instead of 3807
    // or 3808 (with 38.4 MHz HFXO)

    // the BREAK, DELIM, SYNC and PID fields have already passed by now
    // that's at least 34 bits of time, so anything closer to the falling edge
    // of the break is an error
    // LETIMER starts counting down from SL_LIN_TIMEOUT at the falling edge
    ticks = SL_BREAKDELIMSYNCPID_TICKS(USART0->CLKDIV);
    // seems like this is too sensitive or something got delayed by some ticks
    // as 'now' contained 246 and 247, but it should have at most 245
    // adjust the timeout constant for now
    // it might be perfecly valid, btw.
    // 2 ticks = 1.22 bits at 20 kbit/sec, and:
    // 1) the UART might report the received character before processing
    // the STOP bit
    // 2) the UART is asynchronous to the LETIMER, so an off-by-1 error
    // might happen
    if (likely((slave_state == STATE_WAITING)
               && ((now + ticks) <= (SL_LIN_TIMEOUT + 2)))) {
      volatile sl_lin_endpoint_t *ep = NULL;
      uint8_t check, frame_id;
      uint8_t bytes = 0;

      frame_id = pid & 0x3f;
      check = sl_lin_frame_id_to_pid(frame_id);

      if (likely((check == pid) && (frame_id <= SL_LIN_MAX_ENDPOINT))) {
        current_frame_id = frame_id;
        ep = &sl_lin_endpoints[frame_id];
        bytes = ep->bytes;
      }

      if (bytes) {
        slave_state = STATE_PROCESSING;
        __DSB();

        if (ep->writable) {
          dma_start_rx(frame_id);
        } else {
          // time to enable the transmitter
          // the TX FIFO is going to be filled by the DMA
          // have to keep the RX enabled for collision detection
          // but RX has to be blocked to not receive our transmission
          USART0->CMD = USART_CMD_TXEN | USART_CMD_RXBLOCKEN;
          dma_start_tx(frame_id);
        }
      } else {
        // unknown/invalid/damaged frame ID, just drop it
        sl_lin_slave_abort_uart();
      }
    } else {
      // out of sync reception
      sl_lin_slave_abort_uart();
    }

    // error or DMA will do the rest...
    USART_IntClear(USART0, USART_IF_RXDATAV);
    USART_IntDisable(USART0, USART_IF_RXDATAV);
  }

#if defined(UART_ACT_PORT) && defined(UART_ACT_PIN)
  GPIO_PinOutToggle(UART_ACT_PORT, UART_ACT_PIN);
#endif
}

SL_RAMFUNC_DEFINITION_END

// it is assumed that no other GPIO interrupts are present!
SL_RAMFUNC_DEFINITION_BEGIN
__attribute__((__flatten__))
static void wakeup_callback(uint8_t intNo)
{
  (void)intNo;

#if defined(WAKEUP_ACT_PORT) && defined(WAKEUP_ACT_PIN)
  GPIO_PinOutToggle(WAKEUP_ACT_PORT, WAKEUP_ACT_PIN);
#endif

  // waits until the high frequency clock gets stable
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);

  GPIO_IntClear(1U << (_GPIO_IEN_EM4WUIEN0_SHIFT + WAKEUP_SIGNAL));
  GPIO_IntDisable(1U << (_GPIO_IEN_EM4WUIEN0_SHIFT + WAKEUP_SIGNAL));

#if defined(WAKEUP_ACT_PORT) && defined(WAKEUP_ACT_PIN)
  GPIO_PinOutToggle(WAKEUP_ACT_PORT, WAKEUP_ACT_PIN);
#endif
}

SL_RAMFUNC_DEFINITION_END

static sl_status_t register_endpoint(uint8_t frame_id,
                                     bool writable,
                                     int len,
                                     sl_lin_callback_t callback,
                                     const uint8_t *initdata,
                                     bool enhanced_checksum)
{
  volatile sl_lin_endpoint_t *ep;
  uint8_t pid;

  if (unlikely((frame_id > SL_LIN_MAX_ENDPOINT) || (len <= 0) || (len > 8))) {
    return SL_STATUS_INVALID_RANGE;
  }

  if (unlikely(frame_id >= 0x3c)) {
    enhanced_checksum = false;
  }

  ep = &sl_lin_endpoints[frame_id];
  if (unlikely(ep->bytes)) {
    return SL_STATUS_ALREADY_EXISTS;
  }

  ep->writable = writable;
  ep->enhcksum = enhanced_checksum;
  ep->callback = callback;
  ep->next = 0;

  pid = sl_lin_frame_id_to_pid(frame_id);

  if (initdata) {
    sl_lin_copy_buffer_and_checksum((void *)ep->data[0], initdata, len,
                                    (enhanced_checksum) ? pid : 0x00);
  } else {
    sl_lin_clear_buffer_and_checksum((void *)ep->data[0], len,
                                     (enhanced_checksum) ? pid : 0x00);
  }

  sl_lin_copy_buffer((void *)ep->data[1], (void *)ep->data[0], len + 1);
  sl_lin_copy_buffer((void *)ep->data[2], (void *)ep->data[0], len + 1);
  __DSB();

  ep->bytes = len;
  __DSB();

  return SL_STATUS_OK;
}

sl_status_t sl_lin_slave_register_writable_endpoint(uint8_t frame_id,
                                                    int len,
                                                    sl_lin_callback_t callback,
                                                    bool enhanced_checksum)
{
  return register_endpoint(frame_id,
                           true,
                           len,
                           callback,
                           NULL,
                           enhanced_checksum);
}

sl_status_t sl_lin_slave_register_readable_endpoint(uint8_t frame_id,
                                                    int len,
                                                    sl_lin_callback_t callback,
                                                    const uint8_t *initdata,
                                                    bool enhanced_checksum)
{
  return register_endpoint(frame_id,
                           false,
                           len,
                           callback,
                           initdata,
                           enhanced_checksum);
}

sl_status_t sl_lin_slave_unregister_endpoint(uint8_t frame_id)
{
  volatile sl_lin_endpoint_t *ep;

  if (unlikely(frame_id > SL_LIN_MAX_ENDPOINT)) {
    return SL_STATUS_INVALID_RANGE;
  }

  ep = &sl_lin_endpoints[frame_id];
  if (unlikely(!ep->bytes)) {
    return SL_STATUS_EMPTY;
  }

  ep->bytes = 0;
  __DSB();

  return SL_STATUS_OK;
}

sl_status_t sl_lin_slave_update_readable_endpoint(uint8_t frame_id,
                                                  const uint8_t *data)
{
  volatile sl_lin_endpoint_t *ep;
  volatile uint8_t *dest;
  int len;
  uint8_t tmp, pid;

  if (unlikely(frame_id > SL_LIN_MAX_ENDPOINT)) {
    return SL_STATUS_INVALID_RANGE;
  }

  if (unlikely(!data)) {
    return SL_STATUS_NULL_POINTER;
  }

  ep = &sl_lin_endpoints[frame_id];
  len = ep->bytes;
  if (unlikely(!len || ep->writable)) {
    return SL_STATUS_INVALID_KEY;
  }

  tmp = 1 - ep->next;
  dest = ep->data[tmp];
  pid = sl_lin_frame_id_to_pid(frame_id);

  sl_lin_copy_buffer_and_checksum((void *)dest, data, len,
                                  (ep->enhcksum) ? pid : 0x00);

  __DSB();

  ep->next = tmp;
  __DSB();

  return SL_STATUS_OK;
}

sl_status_t sl_lin_slave_inject_checksum_error(uint8_t frame_id)
{
  volatile sl_lin_endpoint_t *ep;
  int len;

  if (unlikely(frame_id > SL_LIN_MAX_ENDPOINT)) {
    return SL_STATUS_INVALID_RANGE;
  }

  ep = &sl_lin_endpoints[frame_id];
  len = ep->bytes;
  if (unlikely(!len || ep->writable)) {
    return SL_STATUS_INVALID_KEY;
  }

  // invert the checksum, so the master will detect an error
  ep->data[0][len] ^= 0xff;
  ep->data[1][len] ^= 0xff;
  ep->data[2][len] ^= 0xff;
  __DSB();

  return SL_STATUS_OK;
}
