/**
 * @file      port.h
 *
 * @brief     port headers file to EFR32BG22
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef PORT_COMMON__H_
#define PORT_COMMON__H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "app.h"
#include "error.h"
#include "port_uart.h"
#include "deca_device_api.h"
#include "cmsis_os2.h" // only for the OS priority definitions
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"

// -----------------------------------------------------------------------------

/* Definitions */

enum Priorities {
  /* Interrupts, which cannot use FreeRTOS API functions */
  PRIO_SPI_IRQn           = configMAX_SYSCALL_INTERRUPT_PRIORITY - 2,

  /* Interrupt safe FreeRTOS API functions below is CMSIS IRQ's */
  PRIO_RTC_WKUP_IRQn      = configMAX_SYSCALL_INTERRUPT_PRIORITY, // equivalent
                                                                  //   to the
                                                                  //   highest
                                                                  //   in the
                                                                  //   FreeRTOS
  PRIO_USART_IRQn         = configMAX_SYSCALL_INTERRUPT_PRIORITY + 1,

  /* Application-specific priorities : CMSIS-OS priorities +3..0..-3
   * osPriorityRealtime  is not used by Application level
   * osPriorityHigh       => configMAX_SYSCALL_INTERRUPT_PRIORITY + ?
   * osPriorityAboveNormal
   *
   * */
  PRIO_FlushTask          = osPriorityAboveNormal,   /* FlushTask should have
                                                      *   higher priority than
                                                      *   CalckTask */
  PRIO_CtrlTask           = osPriorityNormal,
  PRIO_StartDefaultTask   = osPriorityLow,

  PRIO_RxTask             = osPriorityHigh,
  PRIO_CalcTask           = osPriorityNormal,
  PRIO_TrilatTask         = osPriorityBelowNormal,

  PRIO_TagPollTask        = osPriorityHigh,
  PRIO_TagRxTask          = osPriorityHigh,
  PRIO_BlinkTask          = osPriorityNormal,

  PRIO_TcfmTask           = osPriorityNormal,
  PRIO_TcwmTask           = osPriorityNormal,
  PRIO_Usb2SpiTask        = osPriorityNormal
};

typedef enum {
  Twr_Tx_Done,
  Twr_Tx_Blink_Sent,            // tag sends blink
  Twr_Tx_Ranging_Config_Sent,   // node sends range init
  Twr_Tx_Poll_Sent,             // tag sends poll
  Twr_Tx_Resp_Sent,             // node sends response
  Twr_Tx_Final_Sent,            // tag sends final
} tx_states_e;

typedef enum
{
  DW_HAL_NODE_UNLOCKED,
  DW_HAL_NODE_LOCKED
} dw_hal_lockTypeDef;

#define __HAL_LOCK_FAIL_RETURN(__HANDLE__, RET_VAL) \
  do{                                               \
    if ((__HANDLE__)->lock == DW_HAL_NODE_LOCKED) { \
      return RET_VAL;                               \
    } else {                                        \
      (__HANDLE__)->lock = DW_HAL_NODE_LOCKED;      \
    }                                               \
  }while (0U)

#define __HAL_LOCK(__HANDLE__)                      \
  do{                                               \
    if ((__HANDLE__)->lock == DW_HAL_NODE_LOCKED) { \
      return DW_HAL_NODE_LOCKED;                    \
    } else {                                        \
      (__HANDLE__)->lock = DW_HAL_NODE_LOCKED;      \
    }                                               \
  }while (0U)

#define __HAL_UNLOCK(__HANDLE__)               \
  do{                                          \
    (__HANDLE__)->lock = DW_HAL_NODE_UNLOCKED; \
  }while (0U)

// -----------------------------------------------------------------------------
// common macros

#ifndef UNUSED
#define UNUSED(x)                    (void)(x)
#endif

#ifndef CIRC_CNT
#define CIRC_CNT(head, tail, size)   (((head) - (tail)) & ((size) - 1))
#endif /* Return count in buffer.  */

#ifndef CIRC_SPACE
#define CIRC_SPACE(head, tail, size) CIRC_CNT((tail), ((head) + 1), (size))
#endif /* Return space available, 0..size-1 */

#ifndef SWAP
#define SWAP(a, b)                   { a ^= b; b ^= a; a ^= b; }
#endif /* SWAP */

#ifndef MIN
#define MIN(a, b)                    (((a) < (b)) ? (a) : (b))
#endif /* MIN */

#ifndef MAX
#define MAX(a, b)                    (((a) > (b)) ? (a) : (b))
#endif /* MAX */

#ifndef TRUE
#define TRUE                  1
#endif /* TRUE */

#ifndef FALSE
#define FALSE                 0
#endif /* FALSE */

#define MASK_40BIT            (0x00FFFFFFFFFFULL)  // DW1000 counter is 40 bits
#define MASK_TXDTS            (0x00FFFFFFFE00ULL)  // The TX timestamp will snap
                                                   //   to 8 ns resolution -
                                                   //   mask lower 9 bits.

// -----------------------------------------------------------------------------
// experimental FREERtos macros: TODO:

#define TERMINATE_STD_TASK(x)                       \
  do{                                               \
    if ((x).Handle) {                               \
      if ((x).MutexId) {                            \
        xSemaphoreTake((x).MutexId, osWaitForever); \
        taskENTER_CRITICAL();                       \
        xSemaphoreGive((x).MutexId);                \
      } else {                                      \
        taskENTER_CRITICAL();                       \
      }                                             \
      vTaskDelete((x).Handle);                      \
      if ((x).MutexId) {                            \
        vSemaphoreDelete((x).MutexId);              \
      }                                             \
      (x).Handle = NULL;                            \
      (x).MutexId = NULL;                           \
      taskEXIT_CRITICAL();                          \
    }                                               \
  } while (0)

#define CREATE_NEW_TASK(func,                                       \
                        func_arg,                                   \
                        task_name,                                  \
                        task_stack_size,                            \
                        task_prio,                                  \
                        thread_id_ptr)                              \
  xTaskCreate((TaskFunction_t)(func), (task_name),                  \
              (uint16_t)(task_stack_size), (func_arg), (task_prio), \
              (TaskHandle_t *)(thread_id_ptr))

/* port functions prototypes
 *
 * */
void error_handler(int block, error_e err);

/* mutex's */
decaIrqStatus_t decamutexon(void);
void decamutexoff(decaIrqStatus_t s);

/* RTC */
void rtc_configure_wakeup_ns(uint32_t period_ns);
void rtc_configure_wakeup_ms(uint32_t period_ms);
void rtc_reload(void);
void rtc_disable_irq(void);
void rtc_enable_irq(void);
uint32_t rtc_counter_get(void);
uint32_t rtc_init(void);
void rtc_deinit(void);
void juniper_configure_hal_rtc_callback(void (*cb)(void));

/* Time section */
uint32_t init_timer(void);
void start_timer(volatile uint32_t *p_timestamp);
bool check_timer(uint32_t timestamp, uint32_t time);
void usleep(uint32_t usec);
void Sleep(uint32_t);

/* Watchdog timer */
void wdt_init(int ms);
void wdt_refresh(void);

/* UART */
#define app_uart_put(char) deca_uart_transmit(&(char), sizeof(char));
void deca_uart_init(void);
void deca_uart_close(void);
int deca_uart_transmit(const char *tx_buffer, int size);
void deca_uart_receive(void);

/* USB (NOT supported)*/
void deca_usb_init(void);
int deca_usb_transmit(char *tx_buffer, int size);

#ifdef __cplusplus
}
#endif

#endif /* PORT__H__ */
