/**
 * @file      crc16.c
 *
 * @brief     Checks crc16 of frame
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 */

#include "crc16.h"

#define GP 0x11021    /* x^16 + x^12 + x^5 + 1 */
#define DI 0x1021

static uint16_t crc16Table[256];     /* 8-bit table */
static uint8_t     reverseByte[256];

/*local functions*/
static void init_table_crc16(void);
static void init_reverse_byte(void);

void init_crc16(void)
{
  init_table_crc16();
  init_reverse_byte();
}

/* @brief Should be called to init crc tables.
 *
 * */
void init_reverse_byte(void)
{
  int i;
  for (i = 0; i < 256; i++)
  {
    reverseByte[i] = ((i & 0x01) << 7)
                     | ((i & 0x02) << 5)
                     | ((i & 0x04) << 3)
                     | ((i & 0x08) << 1)
                     | ((i & 0x10) >> 1)
                     | ((i & 0x20) >> 3)
                     | ((i & 0x40) >> 5)
                     | ((i & 0x80) >> 7);
  }
}

/* @brief Should be called to init crc tables.
 *
 * */
void init_table_crc16(void)
{
  int i, j;
  uint32_t crc;

  for (i = 0; i < 256; i++) {
    crc = (i << 8);
    for (j = 0; j < 8; j++) {
      crc = (crc << 1) ^ ((crc & 0x8000) ? DI : 0);
    }
    crc16Table[i] = crc & 0xFFFF;
  }
}

/* @brief check CRC16 of frame which includes a CRC
 * @param flen - frame length + 2 bytes of CRC
 *
 * */
crc_err_e check_crc16(uint8_t *frame, uint16_t flen)
{
  uint8_t  crcHiRev, crcLoRev;
  uint16_t crcvalue;
  uint16_t i;

  crc_err_e ret = CRC_ERROR;
  crcvalue = 0;

  for (i = 0; i < flen - 2; i++)
  {
    crcvalue =
      crc16Table[(((crcvalue) >>
                   8) ^ reverseByte[frame[i]]) & 0xFF] ^ ((crcvalue) << 8);
  }
  crcHiRev = reverseByte[(crcvalue >> 8) & 0xFF];
  crcLoRev = reverseByte[crcvalue & 0xFF];

  if ((frame[flen - 2] == crcHiRev) && (frame[flen - 1] == crcLoRev)) {
    ret = CRC_OKAY;
  }

  return (ret);
}

/* @brief calculates crc16 of frame
 * @param flen - frame length to calculate crc
 * @return crc16 value of frame
 * */
uint16_t calc_crc16(uint8_t *frame, uint16_t flen)
{
  uint8_t crcHiRev, crcLoRev;
  int16_t crcvalue;
  int16_t i;

  crcvalue = 0;

  for (i = 0; i < flen; i++)
  {
    crcvalue =
      crc16Table[(((crcvalue) >>
                   8) ^ reverseByte[frame[i]]) & 0xFF] ^ ((crcvalue) << 8);
  }
  crcHiRev = reverseByte[(crcvalue >> 8) & 0xFF];
  crcLoRev = reverseByte[crcvalue & 0xFF];

  return ((crcHiRev << 8) + crcLoRev);
}
