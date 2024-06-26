/*
 * @file      task_tcwm.c
 *
 * @brief     task for continuous wave test
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#include "task_tcwm.h"

#include "port.h"
#include "port_common.h"
#include "usb_uart_rx.h"
#include "tcwm.h"
#include "error.h"
#include "deca_dbg.h"

#define TCWM_TASK_DUMMY_TMR_MS    (1000)

// -----------------------------------------------------------------------------

/* @fn        TcwmTask
 * @brief     this starts the Continuous Wave test functionality.
 *
 *            Note: Previous tasks which can call shared resources must be
 *   killed.
 *
 * */
void TcwmTask(void const *argument)
{
  (void) argument;
  const osMutexAttr_t thread_mutex_attr = { .attr_bits = osMutexPrioInherit };
  app.tcwmTask.MutexId = osMutexNew(&thread_mutex_attr);

  diag_printf("TCWMTask start\r\n");

  while (1)
  {
    osMutexRelease(app.tcwmTask.MutexId);

    osDelay(TCWM_TASK_DUMMY_TMR_MS / portTICK_PERIOD_MS);

    osMutexAcquire(app.tcwmTask.MutexId, 0);

    tcwm_process_run();
  }
}

/* @brief
 *        Kill all tasks and timers related to TcwmTask if any
 *
 *        DW1000's RX and IRQ shall be switched off before task termination,
 *        that IRQ will not produce unexpected Signal
 *
 * */
void tcwm_terminate(void)
{
  tcwm_process_terminate();

  TERMINATE_STD_TASK(app.tcwmTask);
}

/* @fn         tcwm_helper
 * @brief      this is a service function which starts the
 *             Continuous Wave Test functionality
 *             Note: Previous tasks which can access shared resources must be
 *   killed.
 *
 * */
void tcwm_helper(void const *arg)
{
  port_disable_dw_irq_and_reset(1);

  set_dw_spi_fast_rate();

  CREATE_NEW_TASK(TcwmTask,
                  NULL,
                  "tcwmTask",
                  128,
                  PRIO_TcwmTask,
                  &app.tcwmTask.Handle);
  app.tcwmTask.Signal = 1;

  if (!app.tcwmTask.Handle) {
    error_handler(1, _ERR_Create_Task_Bad);
  }

  tcwm_process_init();

  UNUSED(arg);
}
