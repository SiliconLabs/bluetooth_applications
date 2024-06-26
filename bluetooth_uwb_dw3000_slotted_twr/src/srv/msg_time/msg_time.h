/**
 * @file    msg_time.h
 * @brief   used to calculate frames duration
 *
 *
 * @author Decawave
 *
 * @attention Copyright 2017 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef __MSG_TIME__H__
#define __MSG_TIME__H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

struct msg_s
{
  int     dataRate;                     // Deca define: e.g. DWT_BR_850K
  int     txPreambLength;               // Deca define: e.g. DWT_PLEN_4096
  int     stsLength;                    // STS length in symbols
  int     sfdType;                      // 0-3
  int     txPcode;                      // Preamble code
  int     msg_len;
};

typedef struct msg_s msg_t;

struct msg_time_s
{
  uint16_t    preamble_us;
  uint16_t    sts_us;
  uint16_t    phr_us;
  uint16_t    data_us;
  uint16_t    phrAndData_us;

  uint16_t    us;
  uint16_t    sy;
  uint64_t    dt64;
  uint8_t     dt[5];
};

typedef struct msg_time_s msg_time_t;

/* exported functions prototypes */
void calculate_msg_time(msg_t *msg, msg_time_t *msg_time);

#ifdef __cplusplus
}
#endif

#endif
