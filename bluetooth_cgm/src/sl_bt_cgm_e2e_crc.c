/***************************************************************************//**
 * @file
 * @brief CRC-16
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
*******************************************************************************
* # Experimental Quality
* This code has not been formally tested and is provided as-is. It is not
* suitable for production environments. In addition, this code will not be
* maintained and there may be no bug maintenance planned for these resources.
* Silicon Labs may update projects from time to time.
******************************************************************************/
#include <stdint.h>
#include <stddef.h>
/**************************************************************************//**
 * E2E CRC calculation
 *****************************************************************************/
uint16_t sli_bt_cgm_crc16(uint16_t crc, uint8_t *data, size_t len)
{
  if (!data){
      return crc;
  }
  while (len--) {
    crc ^= *data++;
    for (int i=0; i<8; i++) {
      if (crc & 1){
        crc = (crc >> 1) ^ 0x8408;
      }
      else{
        crc = (crc >> 1);
      }
    }
  }
  return crc;
}
