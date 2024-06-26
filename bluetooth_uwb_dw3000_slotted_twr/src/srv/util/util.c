/**---------------------------------------
 * @file    util.c
 * @brief   utility functions:
 *          Seconds to/from DW1000 internal time conversions.
 *          1sy = 1us / 1.0256
 *          1dt  = 1s / 499.2e6 / 128.0
 *          sfd timeout calculation
 *
 * @author Decawave
 *
 * Copyright 2017 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author
 */

#include "util.h"
#include "math.h"

uint64_t util_us_to_dev_time(double microsecu)
{
  uint64_t dt;
  long double dtime;

  dtime = (microsecu / (double) DWT_TIME_UNITS) / 1e6;

  dt = (uint64_t) (dtime);

  return (dt);
}

double util_dev_time_to_sec(uint64_t dt)
{
  double f = 0;

  f = dt * DWT_TIME_UNITS;      // seconds #define TIME_UNITS
                                //            (1.0/499.2e6/128.0) = 15.65e-12

  return (f);
}

uint64_t util_sec_to_dev_time(double secu)
{
  uint64_t dt;
  double dtime;

  dtime = (secu / (double) DWT_TIME_UNITS);

  dt = 0x0FFFFFFFFFULL & (uint64_t) (dtime);

  return (dt);
}

double util_us_to_sy(double us)
{
  return (double)(us / 1.0256);
}

/* @fn      calc_sfd_to()
 * @param   caclulates SFDTimeout based on given dwt_config_t:
 *          sfdType, txPreambLength , rxPAC
 *          sfdto = {txPreambLength} + 1 + {SFDType} - {rxPAC}
 *
 *          Calculation based on deca_device_api.h driver definition
 *
 * @return  sfdto value
 *
 * */
int16_t calc_sfd_to(void *p)
{
  dwt_config_t *pCfg = p;

  int16_t ret = 1;

  /* + length of SFD based on the SFD type */
  switch (pCfg->sfdType)
  {
    case 0x0:   // IEEE standard 8
    case 0x1:   // DW non-standard 8
      ret += 0x08;
      break;

    case 0x2:   // DW non-standard 16
      ret += 0x10;
      break;

    default:         // IEEE 4z 8
      ret += 0x08;
      break;
  }

  /* + {txPreambLength} */
  switch (pCfg->txPreambLength)
  {
    case DWT_PLEN_64:
    case DWT_PLEN_128:
    case DWT_PLEN_256:
    case DWT_PLEN_512:
      ret += (0x40 << (pCfg->txPreambLength >> 4));
      break;

    case DWT_PLEN_1024:
    case DWT_PLEN_1536:
    case DWT_PLEN_2048:
      ret += (0x200 + (0x200 << (pCfg->txPreambLength >> 4)));
      break;

    default:         // Preamble length 4096 and any
      ret += 0x1000;
      break;
  }

  /* - {rxPAC} */
  switch (pCfg->rxPAC)
  {
    case DWT_PAC4:
      ret -= 4;
      break;

    case DWT_PAC16:
      ret -= 16;
      break;

    case DWT_PAC32:
      ret -= 32;
      break;

    default:         // PAC8 and any
      ret -= 8;
      break;
  }

  return (ret);
}
