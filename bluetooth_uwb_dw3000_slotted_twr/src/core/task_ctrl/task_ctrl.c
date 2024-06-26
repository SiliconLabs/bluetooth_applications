/**
 * @file      task_ctrl.c
 *
 * @brief     Control task for USB/UART
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 */
#include "cmd.h"
#include "usb_uart_rx.h"
#include "usb_uart_tx.h"
#include "app.h"

/**
 * @brief     this is a Command Control and Data task.
 *             this task is activated on the startup
 *             there 2 sources of control data: Uart and Usb.
 *
 * */
void CtrlTask(void const *arg)
{
  (void) arg;
  usb_data_e    res;

  while (1)
  {
    osThreadFlagsWait(app.ctrlTask.Signal,
                      osFlagsWaitAny,
                      osWaitForever);        /*   signal from USB/UART
                                              *   that some data has been
                                              *   received
                                              */

    taskENTER_CRITICAL();

    /* mutex if usb2spiTask using the app.local_buf*/
    res = usb_uart_rx();        /*< processes usb/uart input :
                                 *      copy the input to the app.local_buff[
                                 *   local_buff_length ]
                                 *      for future processing */
    taskEXIT_CRITICAL();

    if (res == COMMAND_READY) {
      command_parser((char *)app.local_buff);      // parse and execute the
                                                   //   command
    } else if (res == DATA_READY) {
      if (app.usb2spiTask.Handle) {
        osThreadFlagsSet(app.usb2spiTask.Handle, app.usb2spiTask.Signal);
      }
    }
  }
}
