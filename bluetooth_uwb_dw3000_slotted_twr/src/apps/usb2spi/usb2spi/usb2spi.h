/**
 * @file      usb2spi.h
 *
 * @brief     Header file for usb2spi
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef __INC_USB2SPI_H_
#define __INC_USB2SPI_H_    1

#ifdef __cplusplus
extern "C" {
#endif

#include "usb_uart_rx.h"
#include <stdint.h>
#include <error.h>

/* Typedefs & Enumerations */
enum {
  Usb_Msg_Header    = 0x02,
  Usb_Msg_Footer    = 0x03
};

#define USB2SPI_MAX_CMD_LEN 5 // Length of deca?

usb_data_e usb2spi_frame_complete(uint8_t *p, uint16_t len);
usb_data_e usb2spi_command_check(uint8_t *p, uint16_t len);
error_e    usb2spi_process_init(void);
void usb2spi_process_run(void);
void usb2spi_process_terminate(void);

#ifdef __cplusplus
}
#endif

#endif /* __INC_USB2SPI_H_ */
