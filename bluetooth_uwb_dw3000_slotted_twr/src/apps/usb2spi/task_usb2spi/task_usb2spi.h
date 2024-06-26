/**
 * @file      task_usb2spi.h
 *
 * @brief     Header file for task_usb2spi.c
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef TASK_USB2SPI_H_
#define TASK_USB2SPI_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

void usb2spi_helper(void const *arg);
void usb2spi_terminate(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_USB2SPI_H_ */
