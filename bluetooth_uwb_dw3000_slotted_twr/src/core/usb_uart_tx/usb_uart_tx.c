/**
 * @file      usb_uart_tx.c
 *
 * @brief     Puts message to circular buffer which will be transmitted by
 *   flushing thread
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#include "usb_uart_tx.h"

#include "port_common.h"

// -----------------------------------------------------------------------------
//     USB/UART report section

// the size is such long because of possible ACCUMULATORS sending
#define USB_REPORT_BUFSIZE \
  0x8000  /**< the size of USB report buffer, must
           *   be 1<<N, i.e. 0x800,0x1000 etc
           */
#define USB_UART_TX_TIMEOUT_MS \
  ((USB_REPORT_BUFSIZE * 10) / 115)    /**< (USB_REPORT_BUFSIZE * (8+2) / 115)
                                        * = 1400
                                        * the max timeout to output
                                        * of the whole report buffer
                                        * on UART speed 115200, ms
                                        */

static uint8_t ubuf[64]; /**< linear buffer, to transmit next chunk of data */

static struct _txHandle
{
  dw_hal_lockTypeDef   lock;      /**< locking object  */
  struct
  {
    uint16_t   head;
    uint16_t   tail;
    uint8_t    buf[USB_REPORT_BUFSIZE];        /**< Large USB/UART circular Tx
                                                *   buffer */
  }

  Report;                      /**< circular report buffer, data to transmit */
}

txHandle =
{
  .lock = DW_HAL_NODE_UNLOCKED,
  .Report.head = 0,
  .Report.tail = 0
};

// -----------------------------------------------------------------------------
// Implementation

int reset_report_buf(void)
{
  __HAL_LOCK(&txHandle);
  txHandle.Report.head = txHandle.Report.tail = 0;
  __HAL_UNLOCK(&txHandle);
  return _NO_ERR;
}

/* @fn         copy_tx_msg()
 * @brief     put message to circular report buffer
 *             it will be transmitted in background ASAP from flushing thread
 * @return    HAL_BUSY - can not do it now, wait for release
 *             HAL_ERROR- buffer overflow
 *             HAL_OK   - scheduled for transmission
 * */
static error_e copy_tx_msg(uint8_t *str, int len)
{
  error_e  ret = _NO_ERR;
  uint16_t head, tail;
  const uint16_t size = sizeof(txHandle.Report.buf)
                        / sizeof(txHandle.Report.buf[0]);

  /* add TX msg to circular buffer and increase circular buffer length */

  __HAL_LOCK_FAIL_RETURN(&txHandle, _ERR_Busy);   // "return HAL_BUSY;" if
                                                  //   locked
  head = txHandle.Report.head;
  tail = txHandle.Report.tail;

  if (CIRC_SPACE(head, tail, size) > len) {
    while (len > 0)
    {
      txHandle.Report.buf[head] = *(str++);
      head = (head + 1) & (size - 1);
      len--;
    }

    txHandle.Report.head = head;
  } else {
    /* if packet can not fit, setup TX Buffer overflow ERROR and exit */
    error_handler(0, _ERR_TxBuf_Overflow);
    ret = _ERR_TxBuf_Overflow;
  }

  __HAL_UNLOCK(&txHandle);
  return ret;
}

/* @fn         port_tx_msg()
 * @brief     wrap for copy_tx_msg
 *             Puts message to circular report buffer
 *
 * @return    see copy_tx_msg()
 * */
error_e port_tx_msg(uint8_t *str, int len)
{
  error_e ret;

  if (app.maxMsgLen < len) {
    app.maxMsgLen = len;
  }

  ret = copy_tx_msg(str, len);

  if (app.mode != mLISTENER) {
    if (app.flushTask.Handle) {   // RTOS : usbFlushTask can be not started yet
      osThreadFlagsSet(app.flushTask.Handle, app.flushTask.Signal);
    }
  }

  return (ret);
}

// -----------------------------------------------------------------------------
//     USB/UART report : platform - dependent section
//                      can be in platform port file

/* @fn        flush_report_buff()
 * @brief    FLUSH should have higher priority than port_tx_msg()
 *             This shall be called periodically from process, which can not be
 *   locked,
 *             i.e. from independent high priority thread / timer etc.
 * */
error_e flush_report_buf(void)
{
  int         size = sizeof(txHandle.Report.buf)
                     / sizeof(txHandle.Report.buf[0]);
  int         chunk;
  error_e     ret = _NO_ERR;
  uint32_t    tmr;

  if ((app.usbState != USB_CONFIGURED) && (app.pConfig->s.uartEn == 0)) {
    return _ERR_Busy;
  }

  __HAL_LOCK_FAIL_RETURN(&txHandle, _ERR_Busy);   // "return HAL_BUSY;" if
                                                  //   locked

  int head = txHandle.Report.head;
  int tail = txHandle.Report.tail;

  int len = CIRC_CNT(head, tail, size);

  int old_tail = txHandle.Report.tail;

  start_timer(&tmr);

  if (len > 0) {
    do{
      if (check_timer(tmr, USB_UART_TX_TIMEOUT_MS)) {
        break;          // max timeout for any output on the 115200bps rate
                        //   (currently ~1400ms if over the UART)
      }

      /* copy MAX allowed length from circular buffer to linear buffer */
      chunk = MIN((int)sizeof(ubuf), len);

      for (int i = 0; i < chunk; i++)
      {
        ubuf[i] = txHandle.Report.buf[tail];
        tail = (tail + 1) & (size - 1);
      }

      len -= chunk;

      txHandle.Report.tail = tail;

      if (app.pConfig->s.uartEn == 1) {
        /* setup UART DMA transfer */
        if (deca_uart_transmit((char *)ubuf, chunk) != 0) {
          error_handler(0, _ERR_UART_TX);           /**< indicate UART transmit
                                                     *   error */
          ret = _ERR_UART_TX;
          break;
        }
      } else {
        /* setup USB IT transfer */
        if (deca_usb_transmit((char *)ubuf, chunk) != 0) {
          error_handler(0, _ERR_Usb_Tx);           /**< indicate USB transmit
                                                    *   error */
          txHandle.Report.tail = old_tail;
          ret = _ERR_Usb_Tx;
          break;
        } else {
          old_tail = tail;
        }
      }
    }while (len > 0 && app.mode == mUSB2SPI);
  }

  __HAL_UNLOCK(&txHandle);
  return ret;
}

//     END OF Report section
