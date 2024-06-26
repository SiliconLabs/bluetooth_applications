/**
 * @file      crc16.h
 *
 * @brief     Header file for crc16 checksum
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 */

#ifndef CRC16_H_
#define CRC16_H_

#include <stdint.h>

typedef enum {
  CRC_OKAY = 0,
  CRC_ERROR = -1
}crc_err_e;

uint16_t calc_crc16(uint8_t *frame, uint16_t flen);

crc_err_e check_crc16(uint8_t *frame, uint16_t flen);

void init_crc16(void);

#endif /* CRC16_H_ */
