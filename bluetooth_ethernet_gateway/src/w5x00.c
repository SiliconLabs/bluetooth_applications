/***************************************************************************//**
 * @file w5x00.h
 * @brief Wiznet W5x00 Ethernet driver.
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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "w5x00.h"

#ifdef W5x00_ETHERNET_LARGE_BUFFERS
static uint16_t SSIZE;
static uint16_t SMASK;
#else
static const uint16_t SSIZE = 2048;
static const uint16_t SMASK = 0x07FF;
#endif

#define SBASE(socknum)  \
  ((chip) == W5x00_W5100 ? ((socknum) * SSIZE + 0x4000) : ((socknum) * SSIZE + 0x8000))
#define RBASE(socknum)  \
  ((chip) == W5x00_W5100 ? ((socknum) * SSIZE + 0x6000) : ((socknum) * SSIZE + 0xC000))


static const char tag_w5100[] = "W5100";
static const char tag_w5200[] = "W5200";
static const char tag_w5500[] = "W5500";
static const char *tag = "wiz";

// W5100 controller instance
static uint8_t  CH_BASE_MSB;
static enum W5x00Chip  chip = W5x00_UNKNOWN;
static uint8_t w5x00_is_w5100(void);
static uint8_t w5x00_is_w5200(void);
static uint8_t w5x00_is_w5500(void);

static inline uint32_t w5x00_bus_writebyte(const uint8_t data)
{
  uint8_t wb = data;
  return w5x00_bus_write(&wb, 1);
}

static uint8_t w5x00_is_w5100(void)
{
  chip = W5x00_W5100;
  tag = tag_w5100;
  if (!w5x00_soft_reset()) {
    return 0;
  }
  w5x00_writeMR(0x10);
  if (w5x00_readMR() != 0x10) {
    return 0;
  }
  w5x00_writeMR(0x12);
  if (w5x00_readMR() != 0x12) {
    return 0;
  }
  w5x00_writeMR(0x00);
  if (w5x00_readMR() != 0x00) {
    return 0;
  }
  w5x00_log_info(tag, "Chip is W5100\r\n");
  return 1;
}

static uint8_t w5x00_is_w5200(void)
{
  chip = W5x00_W5200;
  tag = tag_w5200;
  if (!w5x00_soft_reset()) {
    return 0;
  }
  w5x00_writeMR(0x08);
  if (w5x00_readMR() != 0x08) {
    return 0;
  }
  w5x00_writeMR(0x10);
  if (w5x00_readMR() != 0x10) {
    return 0;
  }
  w5x00_writeMR(0x00);
  if (w5x00_readMR() != 0x00) {
    return 0;
  }
  int ver = w5x00_readVERSIONR_W5200();
  w5x00_log_info(tag, "Version = %d\r\n", ver);
  if (ver != 3) {
    return 0;
  }
  w5x00_log_info(tag, "Chip is W5200\r\n");
  return 1;
}

static uint8_t w5x00_is_w5500(void)
{
  chip = W5x00_W5500;
  tag = tag_w5500;
  if (!w5x00_soft_reset()) {
    return 0;
  }
  w5x00_writeMR(0x08);
  if (w5x00_readMR() != 0x08) {
    return 0;
  }
  w5x00_writeMR(0x10);
  if (w5x00_readMR() != 0x10) {
    return 0;
  }
  w5x00_writeMR(0x00);
  if (w5x00_readMR() != 0x00) {
    return 0;
  }
  int ver = w5x00_readVERSIONR_W5500();
  w5x00_log_info(tag, "Version = %d\r\n", ver);
  if (ver != 4) {
    return 0;
  }
  w5x00_log_info(tag, "Chip is W5500\r\n");
  return 1;
}

/***************************************************************************//**
 * W5x00 Init.
 ******************************************************************************/
bool w5x00_init(void)
{
  static bool initialized = false;
  uint8_t i;

  if (initialized) {
    return true;
  }
  w5x00_bus_init();
  w5x00_reset();
  // Many Ethernet shields have a CAT811 or similar reset chip
  // connected to W5100 or W5200 chips.  The W5200 will not work at
  // all, and may even drive its MISO pin, until given an active low
  // reset pulse!  The CAT811 has a 240 ms typical pulse length, and
  // a 400 ms worst case maximum pulse length.  MAX811 has a worst
  // case maximum 560 ms pulse length.  This delay is meant to wait
  // until the reset pulse is ended.  If your hardware has a shorter
  // reset time, this can be edited or removed.
  w5x00_delay_ms(560);

  // Attempt W5200 detection first, because W5200 does not properly
  // reset its SPI state when CS goes high (inactive).  Communication
  // from detecting the other chips can leave the W5200 in a state
  // where it won't recover, unless given a reset pulse.
  if (w5x00_is_w5200()) {
    CH_BASE_MSB = 0x40;
#ifdef W5x00_ETHERNET_LARGE_BUFFERS
#if W5x00_MAX_SOCK_NUM <= 1
    SSIZE = 16384;
#elif W5x00_MAX_SOCK_NUM <= 2
    SSIZE = 8192;
#elif W5x00_MAX_SOCK_NUM <= 4
    SSIZE = 4096;
#else
    SSIZE = 2048;
#endif
    SMASK = SSIZE - 1;
#endif
    for (i = 0; i < W5x00_MAX_SOCK_NUM; i++) {
      w5x00_writeSnRX_SIZE(i, SSIZE >> 10);
      w5x00_writeSnTX_SIZE(i, SSIZE >> 10);
    }
    for (; i < 8; i++) {
      w5x00_writeSnRX_SIZE(i, 0);
      w5x00_writeSnTX_SIZE(i, 0);
    }
  // Try W5500 next.  Wiznet finally seems to have implemented
  // SPI well with this chip.  It appears to be very resilient,
  // so try it after the fragile W5200
  } else if (w5x00_is_w5500()) {
    CH_BASE_MSB = 0x10;
#ifdef W5x00_ETHERNET_LARGE_BUFFERS
#if W5x00_MAX_SOCK_NUM <= 1
    SSIZE = 16384;
#elif W5x00_MAX_SOCK_NUM <= 2
    SSIZE = 8192;
#elif W5x00_MAX_SOCK_NUM <= 4
    SSIZE = 4096;
#else
    SSIZE = 2048;
#endif
    SMASK = SSIZE - 1;
    for (i = 0; i < W5x00_MAX_SOCK_NUM; i++) {
      w5x00_writeSnRX_SIZE(i, SSIZE >> 10);
      w5x00_writeSnTX_SIZE(i, SSIZE >> 10);
    }
    for (; i < 8; i++) {
      w5x00_writeSnRX_SIZE(i, 0);
      w5x00_writeSnTX_SIZE(i, 0);
    }
#endif
  // Try W5100 last.  This simple chip uses fixed 4 byte frames
  // for every 8 bit access.  Terribly inefficient, but so simple
  // it recovers from "hearing" unsuccessful W5100 or W5200
  // communication.  W5100 is also the only chip without a VERSIONR
  // register for identification, so we check this last.
  } else if (w5x00_is_w5100()) {
    CH_BASE_MSB = 0x04;
#ifdef W5x00_ETHERNET_LARGE_BUFFERS
#if W5x00_MAX_SOCK_NUM <= 1
    SSIZE = 8192;
    w5x00_writeTMSR(0x03);
    w5x00_writeRMSR(0x03);
#elif W5x00_MAX_SOCK_NUM <= 2
    SSIZE = 4096;
    w5x00_writeTMSR(0x0A);
    w5x00_writeRMSR(0x0A);
#else
    SSIZE = 2048;
    w5x00_writeTMSR(0x55);
    w5x00_writeRMSR(0x55);
#endif
    SMASK = SSIZE - 1;
#else
    w5x00_writeTMSR(0x55);
    w5x00_writeRMSR(0x55);
#endif
    // No hardware seems to be present.  Or it could be a W5200
    // that's heard other SPI communication if its chip select
    // pin wasn't high when a SD card or other SPI chip was used.
  } else {
    chip = W5x00_UNKNOWN;
    return false; // no known chip is responding :-(
  }
  initialized = true;
  return true; // successful init
}

/***************************************************************************//**
 * Soft reset.
 ******************************************************************************/
uint8_t w5x00_soft_reset(void)
{
  uint16_t count = 0;

  // write to reset bit
  w5x00_writeMR(0x80);
  // then wait for soft reset to complete
  do {
    uint8_t mr = w5x00_readMR();

    // Serial.print("mr=");
    // w5x00_log_info(tag, mr, HEX);
    if (mr == 0) {
      return 1;
    }
    w5x00_delay_ms(1);
  } while (++count < 20);
  return 0;
}

/***************************************************************************//**
 * Get chip type.
 ******************************************************************************/
enum W5x00Chip w5x00_get_chip(void)
{
  return chip;
}

/***************************************************************************//**
 * Get link status.
 ******************************************************************************/
enum W5x00Linkstatus w5x00_get_link_status(void)
{
  uint8_t phystatus;

  if (!w5x00_init()) {
    return UNKNOWN;
  }
  switch (chip) {
    case W5x00_W5200:
      phystatus = w5x00_readPSTATUS_W5200();
    if (phystatus & 0x20) {
      return LINK_ON;
    }
      return LINK_OFF;
    case W5x00_W5500:
      phystatus = w5x00_readPHYCFGR_W5500();
    if (phystatus & 0x01) {
      return LINK_ON;
    }
      return LINK_OFF;
    default:
      return UNKNOWN;
  }
}

/***************************************************************************//**
 * Write Register.
 ******************************************************************************/
uint16_t w5x00_write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
  uint8_t cmd[8];
  uint32_t ret = 0;

  if (chip == W5x00_W5100) {
    for (uint16_t i = 0; i < len; i++) {
      w5x00_bus_select();
      ret += w5x00_bus_writebyte(0xF0);
      ret += w5x00_bus_writebyte(addr >> 8);
      ret += w5x00_bus_writebyte(addr & 0xFF);
      addr++;
      ret += w5x00_bus_writebyte(buf[i]);
      w5x00_bus_deselect();
    }
  } else if (chip == W5x00_W5200) {
    w5x00_bus_select();
    cmd[0] = addr >> 8;
    cmd[1] = addr & 0xFF;
    cmd[2] = ((len >> 8) & 0x7F) | 0x80;
    cmd[3] = len & 0xFF;
    ret += w5x00_bus_write(cmd, 4);
#ifdef SPI_HAS_TRANSFER_BUF
    ret += w5x00_bus_write(buf, NULL, len);
#else
    // TODO: copy 8 bytes at a time to cmd[] and block transfer
    for (uint16_t i = 0; i < len; i++) {
      ret += w5x00_bus_writebyte(buf[i]);
    }
#endif
    w5x00_bus_deselect();
  } else { // chip == W5x00_W5500
    w5x00_bus_select();
    if (addr < 0x100) {
      // common registers 00nn
      cmd[0] = 0;
      cmd[1] = addr & 0xFF;
      cmd[2] = 0x04;
    } else if (addr < 0x8000) {
      // socket registers  10nn, 11nn, 12nn, 13nn, etc
      cmd[0] = 0;
      cmd[1] = addr & 0xFF;
      cmd[2] = ((addr >> 3) & 0xE0) | 0x0C;
    } else if (addr < 0xC000) {
      // transmit buffers  8000-87FF, 8800-8FFF, 9000-97FF, etc
      //  10## #nnn nnnn nnnn
      cmd[0] = addr >> 8;
      cmd[1] = addr & 0xFF;
#if defined(W5x00_ETHERNET_LARGE_BUFFERS) && W5x00_MAX_SOCK_NUM <= 1
      cmd[2] = 0x14;                       // 16K buffers
#elif defined(W5x00_ETHERNET_LARGE_BUFFERS) && W5x00_MAX_SOCK_NUM <= 2
      cmd[2] = ((addr >> 8) & 0x20) | 0x14; // 8K buffers
#elif defined(W5x00_ETHERNET_LARGE_BUFFERS) && W5x00_MAX_SOCK_NUM <= 4
      cmd[2] = ((addr >> 7) & 0x60) | 0x14; // 4K buffers
#else
      cmd[2] = ((addr >> 6) & 0xE0) | 0x14; // 2K buffers
#endif
    } else {
      // receive buffers
      cmd[0] = addr >> 8;
      cmd[1] = addr & 0xFF;
#if defined(W5x00_ETHERNET_LARGE_BUFFERS) && W5x00_MAX_SOCK_NUM <= 1
      cmd[2] = 0x1C;                       // 16K buffers
#elif defined(W5x00_ETHERNET_LARGE_BUFFERS) && W5x00_MAX_SOCK_NUM <= 2
      cmd[2] = ((addr >> 8) & 0x20) | 0x1C; // 8K buffers
#elif defined(W5x00_ETHERNET_LARGE_BUFFERS) && W5x00_MAX_SOCK_NUM <= 4
      cmd[2] = ((addr >> 7) & 0x60) | 0x1C; // 4K buffers
#else
      cmd[2] = ((addr >> 6) & 0xE0) | 0x1C; // 2K buffers
#endif
    }
    if (len <= 5) {
      for (uint8_t i = 0; i < len; i++) {
        cmd[i + 3] = buf[i];
      }
      ret += w5x00_bus_write(cmd, len + 3);
    } else {
      ret += w5x00_bus_write(cmd, 3);
#if 1
      ret += w5x00_bus_write(buf, len);
#else
      // TODO: copy 8 bytes at a time to cmd[] and block transfer
      for (uint16_t i = 0; i < len; i++) {
        ret += w5x00_bus_writebyte(buf[i]);
      }
#endif
    }
    w5x00_bus_deselect();
  }
  if (ret) {
    return 0;
  }
  return len;
}

/***************************************************************************//**
 * Read Register.
 ******************************************************************************/
uint16_t w5x00_read(uint16_t addr, uint8_t *buf, uint16_t len)
{
  uint8_t cmd[4];
  uint32_t ret = 0;

  if (chip == W5x00_W5100) {
    for (uint16_t i = 0; i < len; i++) {
      w5x00_bus_select();
      ret += w5x00_bus_writebyte(0x0F);
      ret += w5x00_bus_writebyte(addr >> 8);
      ret += w5x00_bus_writebyte(addr & 0xFF);
      addr++;
      ret += w5x00_bus_read(&buf[i], 1);
      w5x00_bus_deselect();
    }
  } else if (chip == W5x00_W5200) {
    w5x00_bus_select();
    cmd[0] = addr >> 8;
    cmd[1] = addr & 0xFF;
    cmd[2] = (len >> 8) & 0x7F;
    cmd[3] = len & 0xFF;
    ret += w5x00_bus_write(cmd, 4);
    memset(buf, 0, len);
    ret += w5x00_bus_read(buf, len);
    w5x00_bus_deselect();
  } else { // chip == W5x00_W5500
    w5x00_bus_select();
    if (addr < 0x100) {
      // common registers 00nn
      cmd[0] = 0;
      cmd[1] = addr & 0xFF;
      cmd[2] = 0x00;
    } else if (addr < 0x8000) {
      // socket registers  10nn, 11nn, 12nn, 13nn, etc
      cmd[0] = 0;
      cmd[1] = addr & 0xFF;
      cmd[2] = ((addr >> 3) & 0xE0) | 0x08;
    } else if (addr < 0xC000) {
      // transmit buffers  8000-87FF, 8800-8FFF, 9000-97FF, etc
      //  10## #nnn nnnn nnnn
      cmd[0] = addr >> 8;
      cmd[1] = addr & 0xFF;
#if defined(W5x00_ETHERNET_LARGE_BUFFERS) && W5x00_MAX_SOCK_NUM <= 1
      cmd[2] = 0x10;                       // 16K buffers
#elif defined(W5x00_ETHERNET_LARGE_BUFFERS) && W5x00_MAX_SOCK_NUM <= 2
      cmd[2] = ((addr >> 8) & 0x20) | 0x10; // 8K buffers
#elif defined(W5x00_ETHERNET_LARGE_BUFFERS) && W5x00_MAX_SOCK_NUM <= 4
      cmd[2] = ((addr >> 7) & 0x60) | 0x10; // 4K buffers
#else
      cmd[2] = ((addr >> 6) & 0xE0) | 0x10; // 2K buffers
#endif
    } else {
      // receive buffers
      cmd[0] = addr >> 8;
      cmd[1] = addr & 0xFF;
#if defined(W5x00_ETHERNET_LARGE_BUFFERS) && W5x00_MAX_SOCK_NUM <= 1
      cmd[2] = 0x18;                       // 16K buffers
#elif defined(W5x00_ETHERNET_LARGE_BUFFERS) && W5x00_MAX_SOCK_NUM <= 2
      cmd[2] = ((addr >> 8) & 0x20) | 0x18; // 8K buffers
#elif defined(W5x00_ETHERNET_LARGE_BUFFERS) && W5x00_MAX_SOCK_NUM <= 4
      cmd[2] = ((addr >> 7) & 0x60) | 0x18; // 4K buffers
#else
      cmd[2] = ((addr >> 6) & 0xE0) | 0x18; // 2K buffers
#endif
    }
    ret += w5x00_bus_write(cmd, 3);
    memset(buf, 0, len);
    ret += w5x00_bus_read(buf, len);
    w5x00_bus_deselect();
  }
  if (ret) {
    return 0;
  }
  return len;
}

/***************************************************************************//**
 * Execute socket command.
 ******************************************************************************/
void w5x00_exec_cmd_socket(w5x00_socket_t s, uint8_t cmd)
{
  // Send command to socket
  w5x00_writeSnCR(s, cmd);
  // Wait for command to complete
  while (w5x00_readSnCR(s)) {}
}

/***************************************************************************//**
 * Check if device has offset address mapping.
 ******************************************************************************/
bool w5x00_has_offset_address_mapping(void)
{
  if (chip == W5x00_W5500) {
    return true;
  }
  return false;
}

/***************************************************************************//**
 * Get socket channel base address.
 ******************************************************************************/
uint16_t  w5x00_get_CH_BASE_MSB(void)
{
  return CH_BASE_MSB << 8;
}

/***************************************************************************//**
 * Get socket send buffer base address.
 ******************************************************************************/
uint16_t w5x00_get_SBASE(uint8_t socknum)
{
  return SBASE(socknum);
}

/***************************************************************************//**
 * Get socket receive buffer base address.
 ******************************************************************************/
uint16_t w5x00_get_RBASE(uint8_t socknum)
{
  return RBASE(socknum);
}

/***************************************************************************//**
 * Get socket data pointer mask.
 ******************************************************************************/
uint16_t w5x00_get_SMASK(void)
{
  return SMASK;
}

/***************************************************************************//**
 * Get socket buffer size.
 ******************************************************************************/
uint16_t w5x00_get_SSIZE(void)
{
  return SSIZE;
}

/***************************************************************************//**
 * Set Gateway IP.
 ******************************************************************************/
void w5x00_set_gateway_ip(const uint8_t * addr)
{
  w5x00_writeGAR(addr);
}

/***************************************************************************//**
 * Get Gateway IP.
 ******************************************************************************/
void w5x00_get_gateway_ip(uint8_t * addr)
{
  w5x00_readGAR(addr);
}

/***************************************************************************//**
 * Set Subnet Mask.
 ******************************************************************************/
void w5x00_set_subnet_mask(const uint8_t * addr)
{
  w5x00_writeSUBR(addr);
}

/***************************************************************************//**
 * Get Subnet Mask.
 ******************************************************************************/
void w5x00_get_subnet_mask(uint8_t * addr)
{
  w5x00_readSUBR(addr);
}

/***************************************************************************//**
 * Set MAC Address.
 ******************************************************************************/
void w5x00_set_mac_address(const uint8_t * addr)
{
  w5x00_writeSHAR(addr);
}

/***************************************************************************//**
 * Get MAC Address.
 ******************************************************************************/
void w5x00_get_mac_address(uint8_t * addr)
{
  w5x00_readSHAR(addr);
}

/***************************************************************************//**
 * Set Device IP.
 ******************************************************************************/
void w5x00_set_ip_address(const uint8_t * addr)
{
  w5x00_writeSIPR(addr);
}

/***************************************************************************//**
 * Get Device IP.
 ******************************************************************************/
void w5x00_get_ip_address(uint8_t * addr)
{
  w5x00_readSIPR(addr);
}

/***************************************************************************//**
 * Set TCP Retransmission Time.
 ******************************************************************************/
void w5x00_set_retransmission_time(uint16_t timeout)
{
  if (chip == W5x00_W5500) {
    w5x00_writeRTR_W5500(timeout);
  } else {
    w5x00_writeRTR_W5100(timeout);
  }
}

/***************************************************************************//**
 * Set TCP Retransmission Time Count.
 ******************************************************************************/
void w5x00_set_retransmission_count(uint8_t retry)
{
  if (chip == W5x00_W5500) {
    w5x00_writeRCR_W5500(retry);
  } else {
    w5x00_writeRCR_W5100(retry);
  }
}
