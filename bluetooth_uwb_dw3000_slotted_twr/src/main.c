/*! ----------------------------------------------------------------------------
 * @file    main.c
 * @brief   This is the a variant of implementation of the Slotted TWR PDoA Node
 *          on Nordic nRF52840 platform with FreeRTOS
 *
 * @author  Decawave Applications
 *
 * @attention Copyright 2017-2019 (c) DecaWave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

/* Includes ------------------------------------------------------------------*/

#include "em_gpio.h"
#include "sl_system_init.h"
#include "sl_event_handler.h"
#include "port_dw3000.h"
#include "app.h"
#include "crc16.h"
#include "config.h"
#include "task_usb2spi.h"
#include "node.h"
#include "task_node.h"
#include "task_tag.h"
#include "task_tcfm.h"
#include "task_tcwm.h"
#include "task_ctrl.h"
#include "task_flush.h"
#include "deca_dbg.h"
#include "deca_probe_interface.h"
#include "task_listener.h"

#define BUTTON_TIME_TAG_MS  (3000)
#define USB_DRV_UPDATE_MS   5000
#define DEFAULT_EVENT       (Ev_Stop_All)

/* Private variables ---------------------------------------------------------*/
osThreadId_t      defaultTaskHandle;
gDiagPrintFStr_t  gDiagPrintFStr;
app_t             app;    /**< All global variables are in the "app" structure
                           */

// -----------------------------------------------------------------------------
void trilat_helper(void const *argument);
void trilat_terminate(void);

static void StartDefaultTask(void const *argument);

/**
 * @brief Function for application main entry.
 */
int main(void)
{
  // Initialize Silicon Labs device, system, service(s) and protocol stack(s).
  // Note that if the kernel is present, processing task(s) will be created by
  // this call.
  sl_system_init();

  wdt_init(30000);
  rtc_init();
  init_timer();
  port_init_dw_chip();
  dw_irq_init();

  init_crc16();

  load_bssConfig();                   /**< load the RAM Configuration parameters
                                       *   from NVM block */
  app.pConfig = get_pbssConfig();
  app.xStartTaskEvent = xEventGroupCreate();   /**< xStartTaskEvent indicates
                                                *   which tasks to be started */

#ifdef ENABLE_USB_PRINT
  if (!app.pConfig->s.uartEn) {
    deca_usb_init();
  } else
#endif
  {
    deca_uart_init();
  }

  reset_DW3000();   // this will reset DW device

  if (dwt_probe((struct dwt_probe_s *)&dw3000_probe_interf)) {
    error_handler(1, _ERR_INIT);
  }

  /* This initialization is added to avoid crashing the board when calling APIs
   *   that writes inside local data
   *    like setxtaltrim */
  if (dwt_initialise(DWT_DW_INIT) != DWT_SUCCESS) {
    error_handler(1, _ERR_INIT);
  }

  /* initialize inter-task communication mail queue for Node :
   *
   * The RxTask need to send the rxPckt to the CalcTask.
   *
   * TODO: rxPcktPool_q_id should be a part of NodeInfo, but
   * FreeRTOS cannot free resources efficiently on task deletion.
   *
   * Current code has an implementation where NodeInfo is statically defined
   * and rxPcktPool_q is a part of FreeRtos Heap.
   *
   * Note, the debug accumulator & diagnostics readings are a part of
   * mail queue. Every rx_mail_t has a size of ~6kB.
   *
   * */
  app.rxPcktPool_q_id = osMemoryPoolNew(RX_MAIL_QUEUE_SIZE,
                                        sizeof(rx_mail_t),
                                        NULL);
  app.rxPcktQueue_q_id =
    osMessageQueueNew(RX_MAIL_QUEUE_SIZE, sizeof(rx_mail_t *), NULL);
  if (!app.rxPcktPool_q_id) {
    error_handler(1, _ERR_Cannot_Alloc_Mail);
  }

  /* Create the thread(s)
   * definition and creation of defaultTask
   * Note. The DefaultTask is responsible for starting & stopping of TOP Level
   *   applications.
   */
  CREATE_NEW_TASK(StartDefaultTask,
                  NULL,
                  "defaultTask",
                  configMINIMAL_STACK_SIZE * 2,
                  PRIO_StartDefaultTask,
                  &defaultTaskHandle);

  /* FlushTask is always working and flushing the output buffer to uart/usb */
  CREATE_NEW_TASK(FlushTask,
                  NULL,
                  "flushTask",
                  configMINIMAL_STACK_SIZE,
                  PRIO_FlushTask,
                  &app.flushTask.Handle);
  app.flushTask.Signal = 1;

  /* ctrlTask is always working serving rx from uart/usb */
  // 2K for CTRL task: it needs a lot of memory: it uses mallocs(512),
  //   sscanf(212bytes)
  CREATE_NEW_TASK(CtrlTask,
                  NULL,
                  "ctrlTask",
                  512,
                  PRIO_CtrlTask,
                  &app.ctrlTask.Handle);
  app.ctrlTask.Signal = 1;

  if (!defaultTaskHandle | !app.flushTask.Handle | !app.ctrlTask.Handle) {
    error_handler(1, _ERR_Create_Task_Bad);
  }

  /* Start scheduler */
  sl_kernel_start();

  /* We should never get here as control is now taken by the scheduler */
  while (1)
  {
  }
}

static EventBits_t check_the_user_button(void)
{
  EventBits_t ret = DEFAULT_EVENT;

#if defined(HAL_IO_PORT_BUTTON) && defined(HAL_IO_PIN_BUTTON)
  GPIO_PinModeSet(HAL_IO_PORT_BUTTON, HAL_IO_PIN_BUTTON, gpioModeInput, 0);
  if (GPIO_PinInGet(HAL_IO_PORT_BUTTON,
                    HAL_IO_PIN_BUTTON) != HAL_IO_BUTTON_ACTIVE_STATE) {
    if (app.pConfig->s.default_event != 0) {
      ret = app.pConfig->s.default_event;
    }
  } else {
    uint32_t tmr;
    start_timer(&tmr);
    bool tmp = false;

    while ((GPIO_PinInGet(HAL_IO_PORT_BUTTON,
                          HAL_IO_PIN_BUTTON) == HAL_IO_BUTTON_ACTIVE_STATE)
           && !(tmp = check_timer(tmr, BUTTON_TIME_TAG_MS)))
    {
      wdt_refresh();
    }
    ret = (tmp) ? (Ev_Node_Task) : (Ev_Tag_Task);
  }
#endif
  return ret;
}

/* StartDefaultTask function */
static void StartDefaultTask(void const *argument)
{
  (void)argument;

  const EventBits_t bitsWaitForAny =
    (Ev_Node_Task | Ev_Tag_Task | Ev_Trilat_N_Task
     | Ev_Usb2spi_Task | Ev_Tcwm_Task
     | Ev_Tcfm_Task | Ev_Listener_Task | Ev_Stop_All);

  EventBits_t    uxBits;

  uxBits = check_the_user_button();

  xEventGroupSetBits(app.xStartTaskEvent, uxBits);

  /* Infinite loop */
  while (1)
  {
    uxBits = xEventGroupWaitBits(app.xStartTaskEvent, bitsWaitForAny,
                                 pdTRUE, pdFALSE,
                                 USB_DRV_UPDATE_MS / portTICK_PERIOD_MS);

    wdt_refresh();        // WDG_Refresh

    uxBits &= bitsWaitForAny;

    if (uxBits) {
      app.lastErrorCode = _NO_ERR;

      /*   need to switch off DW chip's RX and IRQ before killing tasks */
      if (app.mode != mIDLE) {
        disable_dw3000_irq();
        reset_DW3000();
        dwt_setcallbacks(NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL); /* DW_IRQ is disabled: safe to cancel
                                 * all user call-backs
                                 */
      }

      /* Event to start/stop task received */
      /* 1. free the resources: kill all user threads and timers */
      tag_terminate();
      node_terminate();
      usb2spi_terminate();
      tcfm_terminate();
      tcwm_terminate();
      trilat_terminate();
      listener_terminate();

      FlushTask_reset();

      app.lastErrorCode = _NO_ERR;

      // incoming Events are slow, and usually User-events, i.e. from a slow
      //   I/O,
      // however the Ev_Stop_All can be generated internally and may OR with
      //   other event,
      // because they can be received asynchronously.
      // Ev_Stop_All event should be tracked separately.
      app.mode = mIDLE;
      uxBits &= ~Ev_Stop_All;
    }

    wdt_refresh();       // WDG_Refresh
    osThreadYield();     // force switch of context

    taskENTER_CRITICAL();

    /* 2. Start appropriate RTOS top-level application or run a
     *   usb_vbus_driver() if a dummy loop */
    switch (uxBits)
    {
      case Ev_Listener_Task:
        app.mode = mLISTENER;
        listener_helper(NULL);       /* call Listener helper function which will
                                      *   setup sub-tasks for Listener process
                                      */
        break;

#if (PDOA_TAG == 1)

      /* PDoA */
      case Ev_Tag_Task:
        app.mode = mPTAG;
        tag_helper(NULL);       /* call Tag helper function which will setup
                                 *   sub-tasks for Tag process */
        break;
#endif

#if (PDOA_NODE == 1)
      case Ev_Node_Task:
        app.mode = mPNODE;
        node_helper(NULL);       /* call Node helper function which will setup
                                  *   sub-tasks for Node process */
        break;
#endif

#if (CFG_LE_TRILAT == 1)
      case Ev_Trilat_N_Task:
        app.mode = mTRILAT_N;
        trilat_helper(NULL);       /* call Trilat helper function which will
                                    *   setup sub-tasks for Node process &
                                    *   Trilat */
        break;
#endif

      /* Service apps */
      case Ev_Usb2spi_Task:
        /* Setup a Usb2Spi task : 8K of stack is required to this task */
        app.mode = mUSB2SPI;
        usb2spi_helper(NULL);
        break;

      case Ev_Tcfm_Task:
        /* Setup a TCFM task */
        app.mode = mTCFM;
        tcfm_helper(NULL);          /* call tcfm helper function which will
                                     *   setup sub-tasks for Power test */
        break;

      case Ev_Tcwm_Task:
        /* Setup a TCWM task */
        app.mode = mTCWM;
        tcwm_helper(NULL);          /* call tcwm helper function which will
                                     *   setup sub-tasks for Power test */
        break;

      case Ev_Stop_All:
        app.mode = mIDLE;
        break;

      default:
        break;
    }
    taskEXIT_CRITICAL();          // ready to switch to a created task

    wdt_refresh();          // WDG_Refresh
    osThreadYield();
  }
}

/** @} */
