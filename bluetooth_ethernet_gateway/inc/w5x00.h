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

#ifndef W5X00_H_
#define W5X00_H_

#include <stdbool.h>
#include "w5x00_config.h"
#include "w5x00_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @defgroup w5x00 W5x00 Ethernet driver
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup w5x00
 * @brief  W5x00 Ethernet driver..
 * @details
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @def W5x00_MAX_SOCK_NUM
 * @brief
 *    Maximum socket number
 * @details
 ******************************************************************************/
#define W5x00_MAX_SOCK_NUM 8

/***************************************************************************//**
 * @def W5x00_CH_SIZE
 * @brief
 *    Socket channel size
 ******************************************************************************/
#define W5x00_CH_SIZE      0x0100

/***************************************************************************//**
 * @brief
 *    Socket mode
 ******************************************************************************/
enum SnMR {
  SnMR_CLOSE  = 0x00, ///< SnMR_CLOSE
  SnMR_TCP    = 0x21, ///< SnMR_TCP
  SnMR_UDP    = 0x02, ///< SnMR_UDP
  SnMR_IPRAW  = 0x03, ///< SnMR_IPRAW
  SnMR_MACRAW = 0x04, ///< SnMR_MACRAW
  SnMR_PPPOE  = 0x05, ///< SnMR_PPPOE
  SnMR_ND     = 0x20, ///< SnMR_ND
  SnMR_MULTI  = 0x80  ///< SnMR_MULTI
};

/***************************************************************************//**
 * @brief Socket command
 ******************************************************************************/
enum SockCMD {
  Sock_OPEN      = 0x01,  ///< Sock_OPEN
  Sock_LISTEN    = 0x02,  ///< Sock_LISTEN
  Sock_CONNECT   = 0x04,  ///< Sock_CONNECT
  Sock_DISCON    = 0x08,  ///< Sock_DISCON
  Sock_CLOSE     = 0x10,  ///< Sock_CLOSE
  Sock_SEND      = 0x20,  ///< Sock_SEND
  Sock_SEND_MAC  = 0x21,  ///< Sock_SEND_MAC
  Sock_SEND_KEEP = 0x22,  ///< Sock_SEND_KEEP
  Sock_RECV      = 0x40   ///< Sock_RECV
};

/***************************************************************************//**
 * @brief
 *    Socket interrupt
 ******************************************************************************/
enum SnIR {
  SnIR_SEND_OK = 0x10,  ///< This is issued when SEND command is completed.
  SnIR_TIMEOUT = 0x08,  ///< This is issued when ARPTO or TCPTO occurs.
  SnIR_RECV    = 0x04,  ///< This is issued whenever data
                        ///< is received from a peer.
  SnIR_DISCON  = 0x02,  ///< This is issued when FIN or FIN/ACK
                        ///< packet is received from a peer.
  SnIR_CON     = 0x01,  ///< This is issued one time when
                        ///< the connection with peer is successful and then
                        ///< #SnSR changed to @ref SOCK_ESTABLISHED.
};

typedef uint8_t w5x00_socket_t;

/***************************************************************************//**
 * @brief Socket status
 ******************************************************************************/
enum SnSR {
  SnSR_CLOSED      = 0x00,  ///< Socket closed
  SnSR_INIT        = 0x13,  ///< Socket init
  SnSR_LISTEN      = 0x14,  ///< Socket listen
  SnSR_SYNSENT     = 0x15,  ///< Socket SYNSENT
  SnSR_SYNRECV     = 0x16,  ///< Socket SYNRECV
  SnSR_ESTABLISHED = 0x17,  ///< Socket ESTABLISHED
  SnSR_FIN_WAIT    = 0x18,  ///< Socket FIN_WAIT
  SnSR_CLOSING     = 0x1A,  ///< Socket CLOSING
  SnSR_TIME_WAIT   = 0x1B,  ///< Socket TIME_WAIT
  SnSR_CLOSE_WAIT  = 0x1C,  ///< Socket CLOSE_WAIT
  SnSR_LAST_ACK    = 0x1D,  ///< Socket LAST_ACK
  SnSR_UDP         = 0x22,  ///< Socket UDP
  SnSR_IPRAW       = 0x32,  ///< Socket IPRAW
  SnSR_MACRAW      = 0x42,  ///< Socket MACRAW
  SnSR_PPPOE       = 0x5F   ///< Socket PPPOE
};

/***************************************************************************//**
 * @brief PHY link status
 ******************************************************************************/
enum W5x00Linkstatus {
  UNKNOWN,  ///< Status unknown
  LINK_ON,  ///< Link is on
  LINK_OFF  ///< Link is off
};

/***************************************************************************//**
 * @brief W5x00 CHIP
 ******************************************************************************/
enum W5x00Chip {
  W5x00_UNKNOWN=0,  ///< Chip is unknown
  W5x00_W5100=51,   ///< Chip is W5100
  W5x00_W5200=52,   ///< Chip is W5200
  W5x00_W5500=55    ///< Chip is W5300
};

/***************************************************************************//**
 * @brief
 *    Get socket channel base MSB address
 * @return
 *    Socket channel base MSB address
 ******************************************************************************/
uint16_t  w5x00_get_CH_BASE_MSB(void);

/***************************************************************************//**
 * @brief
 *    Get socket data pointer mask (read & write)
 * @return
 *    Socket data pointer mask (read & write)
 ******************************************************************************/
uint16_t w5x00_get_SMASK(void);

/***************************************************************************//**
 * @brief
 *    Get socket buffer size
 * @return
 ******************************************************************************/
uint16_t w5x00_get_SSIZE(void);

/***************************************************************************//**
 * @brief
 *    Get socket send buffer base address
 * @param[in] socknum
 *    Socket number
 * @return
 *    Socket send buffer base address
 ******************************************************************************/
uint16_t w5x00_get_SBASE(uint8_t socknum);

/***************************************************************************//**
 * @brief
 *    Get socket receive buffer base address
 * @param[in] socknum
 *    Socket number
 * @return
 *    Socket receive buffer base address
 ******************************************************************************/
uint16_t w5x00_get_RBASE(uint8_t socknum);

/***************************************************************************//**
 * @brief
 *    Write to w5x00 register
 * @param[in] addr
 *    Register address
 * @param[in] buf
 *    Pointer to data buffer
 * @param[in] len
 *    Pointer to data buffer which is to be written
 * @return
 *    Length of written data on success, 0 on failure
 ******************************************************************************/
uint16_t w5x00_write(uint16_t addr, const uint8_t *buf, uint16_t len);

/***************************************************************************//**
 * @brief
 *    Read from w5x00 register
 * @param[in] addr
 *    Register address
 * @param[in] buf
 *    Pointer to data buffer
 * @param[in] len
 *    Number of bytes of data to be read.
 * @return
 *    Length of read data on success, 0 on failure
 ******************************************************************************/
uint16_t w5x00_read(uint16_t addr, uint8_t *buf, uint16_t len);

/***************************************************************************//**
 * @brief
 *    Read socket register
 * @param[in] s
 *    Socket number
 * @param[in] addr
 *    Register address
 * @return
 *    1 byte of register data
 ******************************************************************************/
static inline uint8_t w5x00_readSn(w5x00_socket_t s, uint16_t addr)
{
  uint8_t br = 0;

  w5x00_read(w5x00_get_CH_BASE_MSB() + s * W5x00_CH_SIZE + addr, &br, 1);
  return br;
}

/***************************************************************************//**
 * @brief
 *    Write socket register
 * @param[in] s
 *    Socket number
 * @param[in] addr
 *    Register address
 * @param[in] addr
 *    Register data
 * @return
 *    1 on success, 0 on failure
 ******************************************************************************/
static inline uint16_t w5x00_writeSn(w5x00_socket_t s,
                                     uint16_t addr,
                                     uint8_t data)
{
  uint8_t bw = data;
  return w5x00_write(w5x00_get_CH_BASE_MSB() + s * W5x00_CH_SIZE + addr,
                     &bw,
                     1);
}

/***************************************************************************//**
 * @brief
 *    Read multi-bytes from socket register
 * @param[in] s
 *    Socket number
 * @param[in] addr
 *    Start address
 * @param[out] buf
 *    Pointer to data buffer to store the read data.
 * @param[in] len
 *    Number of bytes of data to be read.
 * @return
 *    Number of read bytes
 ******************************************************************************/
static inline uint16_t w5x00_readSn_buf(w5x00_socket_t s,
                                        uint16_t addr,
                                        uint8_t *buf,
                                        uint16_t len)
{
  return w5x00_read(w5x00_get_CH_BASE_MSB() + s * W5x00_CH_SIZE + addr,
                    buf,
                    len);
}

/***************************************************************************//**
 * @brief
 *    Write multi-bytes to socket register
 * @param[in] s
 *    Socket number
 * @param[in] addr
 *    Start address
 * @param[in] buf
 *    Pointer to data buffer which is to be written
 * @param[in] len
 *    Number of bytes of data to write.
 * @return
 *    Number of written bytes
 ******************************************************************************/
static inline uint16_t w5x00_writeSn_buf(w5x00_socket_t s,
                                         uint16_t addr,
                                         uint8_t *buf,
                                         uint16_t len)
{
  return w5x00_write(w5x00_get_CH_BASE_MSB() + s * W5x00_CH_SIZE + addr,
                     buf,
                     len);
}

#define w5x00_get_socket_tx_max_size(sn) \
    (((uint16_t)w5x00_readSnTX_SIZE(sn)) << 10)

/***************************************************************************//**
 * @brief
 *    Check if chip has address mapping
 * @return
 *    true or false
 ******************************************************************************/
bool w5x00_has_offset_address_mapping(void);

/***************************************************************************//**
 * @brief
 *    Get chip type
 * @return
 *    chip type #W5x00Chip
 ******************************************************************************/
enum W5x00Chip w5x00_get_chip(void);

/***************************************************************************//**
 * @brief
 *    Get PHY link status
 * @return
 *    returns a #W5x00Linkstatus value enum
 ******************************************************************************/
enum W5x00Linkstatus w5x00_get_link_status(void);

/***************************************************************************//**
 * @brief
 *    Set device ip address
 * @param[in] addr
 *    Pointer to IPv4 address buffer
 ******************************************************************************/
void w5x00_set_ip_address(const uint8_t *addr);

/***************************************************************************//**
 * @brief
 *    Get current device ip address
 * @param[out] addr
 *    Pointer to IPv4 address buffer
 ******************************************************************************/
void w5x00_get_ip_address(uint8_t *addr);

/***************************************************************************//**
 * @brief
 *    Set gateway ip
 * @param[in] addr
 *    Pointer to IPv4 address buffer
 ******************************************************************************/
void w5x00_set_gateway_ip(const uint8_t *addr);

/***************************************************************************//**
 * @brief
 *    Get gateway ip
 * @param[out] addr
 *    Pointer to IPv4 address buffer
 ******************************************************************************/
void w5x00_get_gateway_ip(uint8_t *addr);

/***************************************************************************//**
 * @brief
 *    Set subnet mask
 * @param[in] addr
 *    Pointer to data buffer (4 bytes)
 ******************************************************************************/
void w5x00_set_subnet_mask(const uint8_t *addr);

/***************************************************************************//**
 * @brief
 *    Get subnet mask
 * @param[in] addr
 *    Pointer to data buffer (4 bytes)
 ******************************************************************************/
void w5x00_get_subnet_mask(uint8_t *addr);

/***************************************************************************//**
 * @brief
 *    Set MAC address
 * @param[in] addr
 *    Pointer to data buffer (6 bytes)
 ******************************************************************************/
void w5x00_set_mac_address(const uint8_t *addr);

/***************************************************************************//**
 * @brief
 *    Get MAC address
 * @param[in] addr
 *    Pointer to data buffer (6 bytes)
 ******************************************************************************/
void w5x00_get_mac_address(uint8_t *addr);

/***************************************************************************//**
 * @brief
 *    Set TCP retransmission time
 * @param[in] timeout
 *    Timeout in millisecond
 ******************************************************************************/
void w5x00_set_retransmission_time(uint16_t timeout);

/***************************************************************************//**
 * @brief
 *    Set TCP retransmission time count
 * @param retry
 *    Maximum retry
 ******************************************************************************/
void w5x00_set_retransmission_count(uint8_t retry);

/***************************************************************************//**
 * @brief
 *    Send command to socket
 * @param[in] s
 *    Socket number
 * @param[in] cmd
 *    command in #SockCMD
 ******************************************************************************/
void w5x00_exec_cmd_socket(w5x00_socket_t s, uint8_t cmd);

/***************************************************************************//**
 * @brief
 *    Init w5x00 chip
 * @return
 *    true on success, false on failure
 ******************************************************************************/
bool w5x00_init(void);

/***************************************************************************//**
 * @brief
 *    Soft reset the Wiznet chip, by writing to its MR register reset bit
 * @return
 *    1 on success, 0 on failure
 ******************************************************************************/
uint8_t w5x00_soft_reset(void);

/** @cond hidden */
// -----------------------------------------------------------------------------
// Common Registers IOMAP
#define __GP_REGISTER8(name, address)                       \
  static inline uint16_t w5x00_write##name(uint8_t data) {  \
    uint8_t bw = data;                                      \
    return w5x00_write(address, &bw, 1);                    \
  }                                                         \
  static inline uint8_t w5x00_read##name() {                \
    uint8_t br;                                             \
    w5x00_read(address, &br, 1);                            \
    return br;                                              \
  }
#define __GP_REGISTER16(name, address)                      \
  static inline void w5x00_write##name(uint16_t _data) {    \
    uint8_t buf[2];                                         \
    buf[0] = _data >> 8;                                    \
    buf[1] = _data & 0xFF;                                  \
    w5x00_write(address, buf, 2);                           \
  }                                                         \
  static inline uint16_t w5x00_read##name() {               \
    uint8_t buf[2];                                         \
    w5x00_read(address, buf, 2);                            \
    return (buf[0] << 8) | buf[1];                          \
  }
#define __GP_REGISTER_N(name, address, size)                          \
  static inline uint16_t w5x00_write##name(const uint8_t *_buff) {    \
    return w5x00_write(address, _buff, size);                         \
  }                                                                   \
  static inline uint16_t w5x00_read##name(uint8_t *_buff) {           \
    return w5x00_read(address, _buff, size);                          \
  }

__GP_REGISTER8(MR, 0x0000);             // Mode
__GP_REGISTER_N(GAR, 0x0001, 4);        // Gateway IP address
__GP_REGISTER_N(SUBR, 0x0005, 4);       // Subnet mask address
__GP_REGISTER_N(SHAR, 0x0009, 6);       // Source MAC address
__GP_REGISTER_N(SIPR, 0x000F, 4);       // Source IP address
__GP_REGISTER16(INTLEVEL, 0x0013);      // Interrupt Low Level Timer
__GP_REGISTER8(IR, 0x0015);             // Interrupt
__GP_REGISTER8(IMR, 0x0016);            // Interrupt Mask
__GP_REGISTER8(SIR, 0x0017);            // Socket Interrupt (W5500 only)
__GP_REGISTER16(RTR_W5100, 0x0017);     // Timeout address
__GP_REGISTER8(SIMR, 0x0018);           // Socket Interrupt Mask
__GP_REGISTER16(RTR_W5500, 0x0019);     // Timeout address (W5500 only)
__GP_REGISTER8(RCR_W5100, 0x0019);      // Retry count
__GP_REGISTER8(RCR_W5500, 0x001B);      // Retry count (W5500 only)
__GP_REGISTER8(RMSR, 0x001A);           // Receive memory size (W5100 only)
__GP_REGISTER8(TMSR, 0x001B);           // Transmit memory size (W5100 only)
__GP_REGISTER8(PATR, 0x001C);           // Authentication type address in PPPoE mode
__GP_REGISTER8(PTIMER_W5100, 0x0028);   // PPP LCP Request Timer
__GP_REGISTER8(PTIMER_W5500, 0x001C);   // PPP LCP Request Timer (W5500 only)
__GP_REGISTER8(PMAGIC_W5100, 0x0029);   // PPP LCP Magic Number
__GP_REGISTER8(PMAGIC_W5500, 0x001D);   // PPP LCP Magic Number (W5500 only)
__GP_REGISTER_N(UIPR_W5100, 0x002A, 4); // Unreachable IP address in UDP mode (W5100 only)
__GP_REGISTER_N(UIPR_W5500, 0x0028, 4); // Unreachable IP address in UDP mode (W5500 only)
__GP_REGISTER16(UPORT_W5100, 0x002E);   // Unreachable Port address in UDP mode (W5100 only)
__GP_REGISTER16(UPORT_W5500, 0x002C);   // Unreachable Port address in UDP mode (W5500 only)
__GP_REGISTER8(VERSIONR_W5200, 0x001F); // Chip Version Register (W5200 only)
__GP_REGISTER8(VERSIONR_W5500, 0x0039); // Chip Version Register (W5500 only)
__GP_REGISTER8(PSTATUS_W5200, 0x0035);  // PHY Status
__GP_REGISTER8(PHYCFGR_W5500, 0x002E);  // PHY Configuration register, default: 10111xxx
__GP_REGISTER_N(PHAR, 0x001E, 6);       // PPP Destination MAC address
__GP_REGISTER16(PSID, 0x0024);          // PPP Session ID
__GP_REGISTER16(PMRU, 0x0026);          // PPP Maximum Segment Size

#undef __GP_REGISTER8
#undef __GP_REGISTER16
#undef __GP_REGISTER_N

// -----------------------------------------------------------------------------
// Socket Registers IOMAP

#define __SOCKET_REGISTER8(name, address)                                   \
  static inline void w5x00_write##name(w5x00_socket_t _s, uint8_t _data) {  \
    w5x00_writeSn(_s, address, _data);                                      \
  }                                                                         \
  static inline uint8_t w5x00_read##name(w5x00_socket_t _s) {               \
    return w5x00_readSn(_s, address);                                       \
  }
#define __SOCKET_REGISTER16(name, address)                                  \
  static inline void w5x00_write##name(w5x00_socket_t _s, uint16_t _data) { \
    uint8_t buf[2];                                                         \
    buf[0] = _data >> 8;                                                    \
    buf[1] = _data & 0xFF;                                                  \
    w5x00_writeSn_buf(_s, address, buf, 2);                                 \
  }                                                                         \
  static inline uint16_t w5x00_read##name(w5x00_socket_t _s) {              \
    uint8_t buf[2];                                                         \
    w5x00_readSn_buf(_s, address, buf, 2);                                  \
    return (buf[0] << 8) | buf[1];                                          \
  }
#define __SOCKET_REGISTER_N(name, address, size)                            \
  static inline uint16_t w5x00_write##name(w5x00_socket_t _s, uint8_t *_buff) { \
    return w5x00_writeSn_buf(_s, address, _buff, size);                     \
  }                                                                         \
  static inline uint16_t w5x00_read##name(w5x00_socket_t _s, uint8_t *_buff) {  \
    return w5x00_readSn_buf(_s, address, _buff, size);                      \
  }

__SOCKET_REGISTER8(SnMR, 0x0000)               // Mode
__SOCKET_REGISTER8(SnCR, 0x0001)               // Command
__SOCKET_REGISTER8(SnIR, 0x0002)               // Interrupt
__SOCKET_REGISTER8(SnSR, 0x0003)               // Status
__SOCKET_REGISTER16(SnPORT, 0x0004)            // Source Port
__SOCKET_REGISTER_N(SnDHAR, 0x0006, 6)         // Destination Hardw Addr
__SOCKET_REGISTER_N(SnDIPR, 0x000C, 4)         // Destination IP Addr
__SOCKET_REGISTER16(SnDPORT, 0x0010)           // Destination Port
__SOCKET_REGISTER16(SnMSSR, 0x0012)            // Max Segment Size
__SOCKET_REGISTER8(SnPROTO, 0x0014)            // Protocol in IP RAW Mode
__SOCKET_REGISTER8(SnTOS, 0x0015)              // IP TOS
__SOCKET_REGISTER8(SnTTL, 0x0016)              // IP TTL
__SOCKET_REGISTER8(SnRX_SIZE, 0x001E)          // RX Memory Size (W5200 only)
__SOCKET_REGISTER8(SnTX_SIZE, 0x001F)          // RX Memory Size (W5200 only)
__SOCKET_REGISTER16(SnTX_FSR, 0x0020)          // TX Free Size
__SOCKET_REGISTER16(SnTX_RD, 0x0022)           // TX Read Pointer
__SOCKET_REGISTER16(SnTX_WR, 0x0024)           // TX Write Pointer
__SOCKET_REGISTER16(SnRX_RSR, 0x0026)          // RX Free Size
__SOCKET_REGISTER16(SnRX_RD, 0x0028)           // RX Read Pointer
__SOCKET_REGISTER16(SnRX_WR, 0x002A)           // RX Write Pointer (supported?)
__SOCKET_REGISTER8(Sn_IMR, 0x002C)             // Interrupt Mask (W5500 only)
__SOCKET_REGISTER16(SnFRAG, 0x002D)            // Fragment Offset in IP header (W5500 only)
__SOCKET_REGISTER8(Sn_KPALVTR, 0x002F)         // Keep alive timer (W5500 only)

#undef __SOCKET_REGISTER8
#undef __SOCKET_REGISTER16
#undef __SOCKET_REGISTER_N

/** @endcond */

/** @} (end group w5x00) */
#ifdef __cplusplus
}
#endif
#endif // W5X00_H_
