/**
 *  @file     usb_uart_tx.h
 *
 *  @brief    Header file for usb_uart_tx.c
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef __INC_USB_UART_TX_H_
#define __INC_USB_UART_TX_H_    1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "error.h"

error_e port_tx_msg(uint8_t *str, int len);
error_e flush_report_buf(void);
int reset_report_buf(void);

#ifdef __cplusplus
}
#endif

#endif /* __INC_USB_UART_TX_H_ */
