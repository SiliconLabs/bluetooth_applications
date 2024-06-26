/**
 * @file      usb_uart_rx.c
 *
 * @brief     This file supports Decawave USB-TO-SPI and Control modes.
 *            Functions can be used in both bare-metal and RTOS implementations.
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

/* Includes */
#include "usb_uart_rx.h"

#include <string.h>

#include "port_common.h"
#include "error.h"

#include "app.h"
#include "usb2spi.h"
#include "cmd.h"
#include "usb_uart_tx.h"

/* For bare-metal implementation this critical defines may be required. */
#define USB_UART_ENTER_CRITICAL()
#define USB_UART_EXIT_CRITICAL()

usb_data_e waitForCommand(uint8_t *pBuf,
                          uint16_t len,
                          uint16_t *read_offset,
                          uint16_t cyclic_size);
usb_data_e waitForData(uint8_t *pBuf,
                       uint16_t len,
                       uint16_t *read_offset,
                       uint16_t cyclic_size);

/* Receiving command type status */
typedef enum
{
  cmdREGULAR=0,/* Regular command */
  cmdJSON,/* JSON command */
  cmdUNKNOWN_TYPE/* Unknown command */
}command_type_e;

// -----------------------------------------------------------------------------
// IMPLEMENTATION

/*
 * @brief    Waits only commands from incoming stream.
 *             The binary interface (deca_usb2spi stream) is not allowed.
 *
 * @return  COMMAND_READY : the data for future processing can be found in
 *   app.local_buff : app.local_buff_len
 *          NO_DATA : no command yet
 */
usb_data_e waitForCommand(uint8_t *pBuf,
                          uint16_t len,
                          uint16_t *read_offset,
                          uint16_t cyclic_size)
{
  usb_data_e          ret = NO_DATA;
  static uint16_t     cmdLen = 0;
  static uint8_t      cmdBuf[MAX_CMD_LENGTH];   /* Commands buffer */
  uint16_t        cnt;
  static command_type_e command_type = cmdUNKNOWN_TYPE;
  static uint8_t      brackets_cnt;

  app.local_buff_length = 0;

  for (cnt = 0; cnt < len; cnt++)// Loop over the buffer rx data
  {
    if (pBuf[*read_offset] == '\b') { // erase of a char in the terminal
      port_tx_msg((uint8_t *)"\b\x20\b", 3);
      if (cmdLen) {
        cmdLen--;
      }
    } else {
      port_tx_msg(&pBuf[*read_offset], 1);
      if ((pBuf[*read_offset] == '\n') || (pBuf[*read_offset] == '\r')) {
        if ((cmdLen != 0) && (command_type == cmdREGULAR)) {// Checks if need to
                                                            //   handle regular
                                                            //   command
          // Need to update the app commands buffer
          memcpy(&app.local_buff[app.local_buff_length], cmdBuf, cmdLen);
          app.local_buff[app.local_buff_length + cmdLen] = '\n';
          app.local_buff_length += (cmdLen + 1);
          cmdLen = 0;
          command_type = cmdUNKNOWN_TYPE;
          ret = COMMAND_READY;
        }
      } else if (command_type == cmdUNKNOWN_TYPE) {// Need to find out if
                                                   //   getting regular command
                                                   //   or JSON
        cmdBuf[cmdLen] = pBuf[*read_offset];
        if (pBuf[*read_offset] == '{') {// Start Json command
          command_type = cmdJSON;
          brackets_cnt = 1;
        } else {// Start regular command
          command_type = cmdREGULAR;
        }
        cmdLen++;
      } else if (command_type == cmdREGULAR) {// Regular command
        cmdBuf[cmdLen] = pBuf[*read_offset];
        cmdLen++;
      } else {// Json command
        cmdBuf[cmdLen] = pBuf[*read_offset];
        cmdLen++;
        if (pBuf[*read_offset] == '{') {
          brackets_cnt++;
        } else if (pBuf[*read_offset] == '}') {
          brackets_cnt--;
          if (brackets_cnt == 0) {// Got a full Json command
            // Need to update the app commands buffer
            memcpy(&app.local_buff[app.local_buff_length], cmdBuf, cmdLen);
            app.local_buff[app.local_buff_length + cmdLen] = '\n';
            app.local_buff_length += (cmdLen + 1);
            cmdLen = 0;
            command_type = cmdUNKNOWN_TYPE;
            ret = COMMAND_READY;
          }
        }
      }
    }
    *read_offset = (*read_offset + 1) & cyclic_size;
    if (cmdLen >= sizeof(cmdBuf)) {/* Checks if command too long and we need to
                                    *   reset it */
      cmdLen = 0;
      command_type = cmdUNKNOWN_TYPE;
    }
  }

  if (ret == COMMAND_READY) {// If there is at least 1 command, add 0 at the end
    app.local_buff[app.local_buff_length] = 0;
    app.local_buff_length++;
  }
  return (ret);
}

/*
 * @brief    Waits for binary interface (deca_usb2spi stream) from incoming
 *   stream.
 *
 * @return  DATA_READY : the data is ready for future processing in usb2spi
 *   application
 *                       data can be found in app.local_buff :
 *   app.local_buff_len
 *          NO_DATA    : no valid data yet
 */
usb_data_e waitForData(uint8_t *pBuf,
                       uint16_t len,
                       uint16_t *read_offset,
                       uint16_t cyclic_size)
{
  static uint16_t     dataLen = 0;
  usb_data_e          ret;
  uint16_t            cnt;
  static bool frame_started = false;

  ret = NO_DATA;

  /* wait for valid usb2spi message from pBuf */
  if ((len + dataLen) < sizeof(app.local_buff) - 1) {
    for (cnt = 0; cnt < len && ret != DATA_READY; cnt++)
    {
      if (!frame_started) {
        if ((pBuf[*read_offset] == '\n') || (pBuf[*read_offset] == '\r')) {
          // Do not store CR or LF when not receiving USB2SPI frames. E.g
          //   Decaranging
          // adds CRLF to the end of some USB2SPI commands.
          // Reset the dataLen to allow CR or LF to reset any command.
          dataLen = 0;
          *read_offset = (*read_offset + 1) & cyclic_size;
          continue;
        } else if (pBuf[*read_offset] == Usb_Msg_Header) {
          // Remove any old data. If it contained a USB2SPI command,
          // it was handled before.
          dataLen = 0;
          frame_started = true;
        }
      }

      app.local_buff[dataLen] = pBuf[*read_offset];
      *read_offset = (*read_offset + 1) & cyclic_size;
      dataLen++;

      if (frame_started) {
        if (usb2spi_frame_complete(app.local_buff, dataLen) == DATA_READY) {
          frame_started = false;
          ret = DATA_READY;
        }
      } else {
        ret = usb2spi_command_check(app.local_buff, dataLen);
      }
    }
    if (ret == DATA_READY) {
      app.local_buff_length = dataLen;
      app.local_buff[dataLen] = 0;
      dataLen = 0;
    }
  } else { /* overflow in usb2spi protocol : flush the buffer */
    dataLen = 0;
  }

  return (ret);
}

/* @fn     usb_uart_rx
 * @brief  this should be calling on a reception of a data from UART or USB.
 *         uses platform-dependent
 *
 * */
usb_data_e usb_uart_rx(void)
{
  usb_data_e      ret = NO_DATA;
  uint16_t    uartLen, usbLen;
  uint16_t    headUart, tailUart;
  uint16_t    headUsb, tailUsb;

  /* USART control prevails over USB control if both at the same time */

  USB_UART_ENTER_CRITICAL();
  headUart = app.uartRx.head;

  headUsb = app.usbRx.head;
  USB_UART_EXIT_CRITICAL();

  tailUart = app.uartRx.tail;
  tailUsb = app.usbRx.tail;

  uartLen = CIRC_CNT(headUart, tailUart, sizeof(app.uartRx.buf));
  usbLen = CIRC_CNT(headUsb, tailUsb, sizeof(app.usbRx.buf));

  if (uartLen > 0) {
    if (app.pConfig->s.uartEn) {
      ret = (app.mode == mUSB2SPI)
            ?waitForData(app.uartRx.buf, uartLen, &tailUart,
                         sizeof(app.uartRx.buf) - 1)
            :waitForCommand(app.uartRx.buf, uartLen, &tailUart,
                            sizeof(app.uartRx.buf) - 1);

      USB_UART_ENTER_CRITICAL();
      app.uartRx.tail = tailUart;
      USB_UART_EXIT_CRITICAL();
    } else {
      // Ignore data in UART buffer. Should not happen since UART is disabled.
      app.uartRx.tail = (app.uartRx.tail + uartLen)
                        & (sizeof(app.uartRx.buf) - 1);
    }
  }
  if (usbLen > 0) {
    if (!app.pConfig->s.uartEn && (app.usbState == USB_CONFIGURED)) {
      ret = (app.mode == mUSB2SPI)
            ?waitForData(app.usbRx.buf,
                         usbLen,
                         &tailUsb,
                         sizeof(app.usbRx.buf) - 1)
            :waitForCommand(app.usbRx.buf, usbLen, &tailUsb,
                            sizeof(app.usbRx.buf) - 1);

      USB_UART_ENTER_CRITICAL();
      app.usbRx.tail = tailUsb;
      USB_UART_EXIT_CRITICAL();
    } else {
      // Ignore data from USB
      app.usbRx.tail = (app.usbRx.tail + usbLen) & (sizeof(app.usbRx.buf) - 1);
    }
  }

  return ret;
}
