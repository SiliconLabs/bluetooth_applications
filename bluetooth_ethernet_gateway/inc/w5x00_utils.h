/***************************************************************************//**
 * @file w5x00_utils.h
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
#ifndef W5x00_UTILS_H_
#define W5x00_UTILS_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @defgroup Utils IP Address Utilities
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup Utils
 * @brief  IP address utilities.
 * @details
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *    This is the aligned version of ip4_addr_t,
 *    used as local variable, on the stack, etc.
 ******************************************************************************/
struct w5x00_ip4_addr {
  uint32_t addr;
};

typedef struct w5x00_ip4_addr w5x00_ip4_addr_t;

/***************************************************************************//**
 * @brief
 *    IP=255.255.255.255
 ******************************************************************************/
#define WIZNET_IPADDR_NONE         ((uint32_t)0xffffffffUL)

/***************************************************************************//**
 * @brief
 *    IP=127.0.0.1
 ******************************************************************************/
#define WIZNET_IPADDR_LOOPBACK     ((uint32_t)0x7f000001UL)

/***************************************************************************//**
 * @brief
 *    IP=0.0.0.0
 ******************************************************************************/
#define WIZNET_IPADDR_ANY          ((uint32_t)0x00000000UL)

/***************************************************************************//**
 * @brief
 *    IP=255.255.255.255
 ******************************************************************************/
#define WIZNET_IPADDR_BROADCAST    ((uint32_t)0xffffffffUL)

/***************************************************************************//**
 * @brief
 *    Set an IP address in u32 format
 ******************************************************************************/
#define WIZNET_IP4_DATA(a, b, c, d)                 \
  htonl((uint32_t)(((uint32_t)((a) & 0xff) << 24)   \
                   | ((uint32_t)((b) & 0xff) << 16) \
                   | ((uint32_t)((c) & 0xff) << 8)  \
                   | (uint32_t)((d) & 0xff)))

/***************************************************************************//**
 * @brief
 *    Set an IP address given by the four byte-parts
 ******************************************************************************/
#define WIZNET_IP4_ADDR(ipaddr, a, b, c, d) \
    (ipaddr)->addr = WIZNET_IP4_DATA(a, b, c, d)

/***************************************************************************//**
 * @brief
 *    Get one byte from the 4-byte address
 ******************************************************************************/
#define w5x00_ip4_addr_get_byte(ipaddr, idx) \
  (((const uint8_t*)(&(ipaddr)->addr))[idx])

/***************************************************************************//**
 * @brief
 *    Set one byte from the 4-byte address
 ******************************************************************************/
#define w5x00_ip4_addr_set_byte(ipaddr, idx, val) \
  ((uint8_t*)(&(ipaddr)->addr))[idx] = val;

/***************************************************************************//**
 * @brief
 *   Convert u16 to TCP/IP network byte order (which is big-endian).
 ******************************************************************************/
#define htons(x) ((((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF))

/***************************************************************************//**
 * @brief
 *    Convert u16 in TCP/IP network byte order to little-endian u32
 ******************************************************************************/
#define ntohs(x) htons(x)

/***************************************************************************//**
 * @brief
     Convert u32 to TCP/IP network byte order (which is big-endian).
 ******************************************************************************/
#define htonl(x) (((x) << 24 & 0xFF000000UL) | \
                  ((x) << 8 & 0x00FF0000UL) |  \
                  ((x) >> 8 & 0x0000FF00UL) |  \
                  ((x) >> 24 & 0x000000FFUL))

/***************************************************************************//**
 * @brief
 *    Convert u32 in TCP/IP network byte order to little-endian u32
 ******************************************************************************/
#define ntohl(x) htonl(x)

/***************************************************************************//**
 * @brief
 *    Check whether "cp" is a valid ascii representation
 *    of an Internet address and convert to a binary address.
 *    Returns 1 if the address is valid, 0 if not.
 *    This replaces inet_addr, the return value from which
 *    cannot distinguish between failure and a local broadcast address.
 *
 * @param cp
 *    IP address in ascii representation (e.g. "127.0.0.1")
 * @param addr
 *    Pointer to which to save the ip address in network order
 * @return true
 *    If cp could be converted to addr, 0 on failure
 ******************************************************************************/
bool w5x00_ip4addr_aton(const char *cp, w5x00_ip4_addr_t *addr);

/** @} (end group Utils) */
#ifdef __cplusplus
}
#endif
#endif // W5x00_UTILS_H_
