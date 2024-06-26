/**
 * @file      usb_uart_rx.h
 *
 * @brief     Header file for usb_uart_rx.c contains enum for usb data
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef __INC_USB_UART_RX_H_
#define __INC_USB_UART_RX_H_    1

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  NO_DATA = 0,
  DATA_READY,
  COMMAND_READY,
  DATA_SEND,
  DATA_FLUSH,
  DATA_ERROR
}usb_data_e;

usb_data_e usb_uart_rx(void);

#ifdef __cplusplus
}
#endif

#endif /* __INC_USB_UART_TX_H_ */
