/* @file    deca_translate.c
 * @brief     translate DW1000 parameters from Deca to Human and from Human to
 *   Deca format
 *
 *            return translated value or (-1) if out of allowed range
 *
 * @author Decawave
 * @attention Copyright 2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 */

#include "deca_device_api.h"

/* Channel */
int chan_to_deca(int i)
{
  int ret = -1;

  if ((i == 5) || (i == 9)) {
    ret = i;
  }

  return (ret);
}

int deca_to_chan(int i)
{
  return(chan_to_deca(i));
}

/* Bitrate */
int bitrate_to_deca(int i)
{
  switch (i)
  {
    case 850:
      return DWT_BR_850K;
    case 6810:
      return DWT_BR_6M8;
    default:
      return -1;
  }
}

int deca_to_bitrate(int i)
{
  switch (i)
  {
    case DWT_BR_850K:
      return 850;
    case DWT_BR_6M8:
      return 6810;
    default:
      return -1;
  }
}

/* PAC */
int pac_to_deca(int i)
{
  switch (i)
  {
    case 8:
      return DWT_PAC8;
    case 16:
      return DWT_PAC16;
    case 32:
      return DWT_PAC32;
    case 4:
      return DWT_PAC4;
    default:
      return -1;
  }
}

int deca_to_pac(int i)
{
  switch (i)
  {
    case DWT_PAC8:
      return 8;
    case DWT_PAC16:
      return 16;
    case DWT_PAC32:
      return 32;
    case DWT_PAC4:
      return 4;
    default:
      return -1;
  }
}

/* PLEN */
int plen_to_deca(int i)
{
  switch (i)
  {
//    case 4096 :
//        return DWT_PLEN_4096;
    case 2048:
      return DWT_PLEN_2048;
    case 1536:
      return DWT_PLEN_1536;
    case 1024:
      return DWT_PLEN_1024;
    case 512:
      return DWT_PLEN_512;
    case 256:
      return DWT_PLEN_256;
    case 128:
      return DWT_PLEN_128;
    case 64:
      return DWT_PLEN_64;
    default:
      return -1;
  }
}

int deca_to_plen(int i)
{
  switch (i)
  {
//    case DWT_PLEN_4096 :
//        return 4096;
    case DWT_PLEN_2048:
      return 2048;
    case DWT_PLEN_1536:
      return 1536;
    case DWT_PLEN_1024:
      return 1024;
    case DWT_PLEN_512:
      return 512;
    case DWT_PLEN_256:
      return 256;
    case DWT_PLEN_128:
      return 128;
    case DWT_PLEN_64:
      return 64;
    default:
      return -1;
  }
}

/*STS Length*/
int sts_length_to_deca(int i)
{
  switch (i)
  {
    case 2048:
      return DWT_STS_LEN_2048;
    case 1024:
      return DWT_STS_LEN_1024;
    case 512:
      return DWT_STS_LEN_512;
    case 256:
      return DWT_STS_LEN_256;
    case 128:
      return DWT_STS_LEN_128;
    case 64:
      return DWT_STS_LEN_64;
    case 32:
      return DWT_STS_LEN_32;
    default:
      return -1;
  }
}

int deca_to_sts_length(int i)
{
  switch (i)
  {
    case DWT_STS_LEN_2048:
      return 2048;
    case DWT_STS_LEN_1024:
      return 1024;
    case DWT_STS_LEN_512:
      return 512;
    case DWT_STS_LEN_256:
      return 256;
    case DWT_STS_LEN_128:
      return 128;
    case DWT_STS_LEN_64:
      return 64;
    case DWT_STS_LEN_32:
      return 32;
    default:
      return -1;
  }
}

/* END of translate */
