/*! ----------------------------------------------------------------------------
 * @file    deca_usb.c
 * @brief   HW specific definitions and functions for USB Interface
 *
 * @author  Decawave
 *
 * @attention
 *
 * Copyright 2018 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */

//
// USB IS NOT SUPPORTED BY EFR32xG FAMILY!
//

int deca_usb_transmit(char *tx_buffer, int size)
{
  (void)tx_buffer;
  (void)size;
  return -1;
}

void deca_usb_init(void)
{
}

/** @} */
