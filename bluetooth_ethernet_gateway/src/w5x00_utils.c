/***************************************************************************//**
 * @file w5x00_utils.c
 * @brief Wiznet IP Address utilities.
 * @version 0.0.1
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided \'as-is\', without any express or implied
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
 *
 * EVALUATION QUALITY
 * This code has been minimally tested to ensure that it builds with the
 * specified dependency versions and is suitable as a demonstration for
 * evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 *
 ******************************************************************************/

#include <stdint.h>
#include <ctype.h>

#include "w5x00_utils.h"

/***************************************************************************//**
 * Parse IPv4 from String.
 ******************************************************************************/
bool w5x00_ip4addr_aton(const char *cp, w5x00_ip4_addr_t *addr)
{
  uint32_t val;
  uint8_t base;
  char c;
  uint32_t parts[4];
  uint32_t *pp = parts;

  c = *cp;
  for (;;) {
    /*
     * Collect number up to ``.''.
     * Values are specified as for C:
     * 0x=hex, 0=octal, 1-9=decimal.
     */
    if (!isdigit(c)) {
      return false;
    }
    val = 0;
    base = 10;
    if (c == '0') {
      c = *++cp;
      if ((c == 'x') || (c == 'X')) {
        base = 16;
        c = *++cp;
      } else {
        base = 8;
      }
    }
    for (;;) {
      if (isdigit(c)) {
        val = (val * base) + (uint32_t)(c - '0');
        c = *++cp;
      } else if ((base == 16) && isxdigit(c)) {
        val = (val << 4) | (uint32_t)(c + 10 - (islower(c) ? 'a' : 'A'));
        c = *++cp;
      } else {
        break;
      }
    }
    if (c == '.') {
      /*
       * Internet format:
       *  a.b.c.d
       *  a.b.c   (with c treated as 16 bits)
       *  a.b (with b treated as 24 bits)
       */
      if (pp >= parts + 3) {
        return false;
      }
      *pp++ = val;
      c = *++cp;
    } else {
      break;
    }
  }
  // Check for trailing characters.
  if ((c != '\0') && !isspace(c)) {
    return false;
  }
  /*
   * Concoct the address according to
   * the number of parts specified.
   */
  switch (pp - parts + 1) {
    case 0:
      return false;     /* initial nondigit */

    case 1:             /* a -- 32 bits */
      break;

    case 2:             /* a.b -- 8.24 bits */
      if (val > 0xffffffUL) {
        return false;
      }
      if (parts[0] > 0xff) {
        return false;
      }
      val |= parts[0] << 24;
      break;

    case 3:             /* a.b.c -- 8.8.16 bits */
      if (val > 0xffff) {
        return false;
      }
      if ((parts[0] > 0xff) || (parts[1] > 0xff)) {
        return false;
      }
      val |= (parts[0] << 24) | (parts[1] << 16);
      break;

    case 4:             /* a.b.c.d -- 8.8.8.8 bits */
      if (val > 0xff) {
        return false;
      }
      if ((parts[0] > 0xff) || (parts[1] > 0xff) || (parts[2] > 0xff)) {
        return false;
      }
      val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
      break;
    default:
      break;
  }
  if (addr) {
    addr->addr = htonl(val);
  }
  return true;
}
