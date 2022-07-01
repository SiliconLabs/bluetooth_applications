/***************************************************************************//**
 * @file sl_sdc_sd_card.c
 * @brief Storage Device Controls SD Card
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 ********************************************************************************
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
 * # Evaluation Quality
 * This code has been minimally tested to ensure that it builds and is suitable
 * as a demonstration for evaluation purposes only. This code will be maintained
 * at the sole discretion of Silicon Labs.
 ******************************************************************************/
#include "sl_sdc_sd_card.h"

// Definitions for MMC/SDC command
#define CMD0      (0)         // GO_IDLE_STATE
#define CMD1      (1)         // SEND_OP_COND (MMC)
#define ACMD41    (0x80 + 41) // SEND_OP_COND (SDC)
#define CMD8      (8)         // SEND_IF_COND
#define CMD9      (9)         // SEND_CSD
#define CMD10     (10)        // SEND_CID
#define CMD12     (12)        // STOP_TRANSMISSION
#define ACMD13    (0x80 + 13) // SD_STATUS (SDC)
#define CMD16     (16)        // SET_BLOCKLEN
#define CMD17     (17)        // READ_SINGLE_BLOCK
#define CMD18     (18)        // READ_MULTIPLE_BLOCK
#define CMD23     (23)        // SET_BLOCK_COUNT (MMC)
#define ACMD23    (0x80 + 23) // SET_WR_BLK_ERASE_COUNT (SDC)
#define CMD24     (24)        // WRITE_BLOCK
#define CMD25     (25)        // WRITE_MULTIPLE_BLOCK
#define CMD32     (32)        // ERASE_ER_BLK_START
#define CMD33     (33)        // ERASE_ER_BLK_END
#define CMD38     (38)        // ERASE
#define CMD48     (48)        // READ_EXTR_SINGLE
#define CMD49     (49)        // WRITE_EXTR_SINGLE
#define CMD55     (55)        // APP_CMD
#define CMD58     (58)        // READ_OCR

static volatile DSTATUS sd_card_status = STA_NOINIT; // Disk status
static BYTE sd_card_type; // Card type flags
static volatile UINT sd_card_timer_1, sd_card_timer_2; // 1kHz decrement timer

static bool wait_ready(UINT wt);
static void deselect (void);
static bool select(void);
static bool rcvr_datablock(BYTE *buff, UINT btr);
#if FF_FS_READONLY == 0
static bool xmit_datablock(const BYTE *buff, BYTE token);
#endif
static BYTE send_cmd(BYTE cmd, DWORD arg);

/***************************************************************************//**
 * @brief
 *   Wait for card ready.
 *
 * @param[in] wt
 *   Timeout [ms]
 *
 * @return 1:Ready, 0:Timeout
 ******************************************************************************/
static bool wait_ready(UINT wt)
{
  BYTE data;

  sd_card_timer_2 = wt;
  do {
    sdc_xchg_spi(0xff, &data);
    // This loop takes a time. Insert rot_rdq() here for multitask envilonment.
  } while (data != 0xff && sd_card_timer_2);  // Wait for card goes ready or timeout

  return (data == 0xff) ? 1 : 0;
}

/***************************************************************************//**
 * @brief
 *   Deselect card and release SPI.
 *
 ******************************************************************************/
static void deselect(void)
{
  BYTE data;

  CS_HIGH();
  sdc_xchg_spi(0xff, &data); // Dummy clock (force DO hi-z for multiple slave SPI)
}

/***************************************************************************//**
 * @brief
 *   Select card and wait for ready.
 *
 * @return 1:OK, 0:Timeout
 ******************************************************************************/
static bool select(void)
{
  BYTE data;

  CS_LOW();
  // Dummy clock (force DO enabled)
  sdc_xchg_spi(0xff, &data);

  if (wait_ready(500)) {
    return 1;  // Wait for card ready
  }

  deselect();
  return 0; // Timeout
}

/***************************************************************************//**
 * @brief
 *   Receive a data packet from MMC.
 *
 * @param[in] buff
 *   Pointer to Data buffer to store received data
 *
 * @param[in] btr
 *   Byte count (must be multiple of 4)
 *
 * @return 1:OK, 0:Failed
 ******************************************************************************/
static bool rcvr_datablock(BYTE *buff, UINT btr)
{
  BYTE token;

  sd_card_timer_1 = 100;
  do { // Wait for data packet in timeout of 100m
    sdc_xchg_spi(0xff, &token);
  } while ((token == 0xff) && sd_card_timer_1);

  // If not valid data token, return with error
  if(token != 0xfe) {
    return 0;
  }

  sdc_rcvr_spi_multi(buff, btr); // Receive the data block into buffer
  // Discard 2 byte-CRC.
  // Refer to http://elm-chan.org/docs/mmc/mmc_e.html#dataxfer for details"
  sdc_xchg_spi(0xff, &token);
  sdc_xchg_spi(0xff, &token);

  return 1;
}

/***************************************************************************//**
 * @brief
 *   Send a data packet to MMC.
 *
 * @param[in] buff
 *   Pointer to 512 byte data block to be transmitted
 *
 * @param[in] token
 *   Data token
 *
 * @return 1:OK, 0:Failed
 ******************************************************************************/
#if FF_FS_READONLY == 0
static bool xmit_datablock(const BYTE *buff, BYTE token)
{
  BYTE data;

  if (!wait_ready(500)) {
    return 0;
  }

  sdc_xchg_spi(token, &data);      // Xmit a token
  if (token != 0xfd) {             // Not StopTran token
    sdc_xmit_spi_multi(buff, 512); // Xmit the data block to the MMC
    // Discard 2 byte-CRC.
    // Refer to http://elm-chan.org/docs/mmc/mmc_e.html#dataxfer for details"
    sdc_xchg_spi(0xff, &data);
    sdc_xchg_spi(0xff, &data);
    sdc_xchg_spi(0xff, &data);     // Receive a data response
    // If not accepted, return with error
    if ((data & 0x1F) != 0x05) {
      return 0;
    }
  }

  return 1;
}
#endif

/***************************************************************************//**
 * @brief
 *   Send a command packet to MMC.
 *
 * @param[in] cmd
 *   Command index
 *
 * @param[in] arg
 *   Argument
 *
 * @return Status of Disk Functions
 ******************************************************************************/
static BYTE send_cmd(BYTE cmd, DWORD arg)
{
  BYTE n, data;

  // ACMD<n> is the command sequense of CMD55-CMD<n>
  if (cmd & 0x80) {
    cmd &= 0x7f;
    data = send_cmd(CMD55, 0);
    if (data > 1) {
      return data;
    }
  }

  // Select the card and wait for ready except to stop multiple block read
  if (cmd != CMD12) {
    deselect();
    if (!select()) {
      return 0xff;
    }
  }

  // Send command packet
  sdc_xchg_spi(0x40 | cmd, &data);          // Start + Command index
  sdc_xchg_spi(((BYTE)(arg >> 24)), &data); // Argument[31..24]
  sdc_xchg_spi(((BYTE)(arg >> 16)), &data); // Argument[23..16]
  sdc_xchg_spi(((BYTE)(arg >> 8)), &data);  // Argument[15..8]
  sdc_xchg_spi((BYTE)(arg), &data);         // Argument[7..0]

  n = 0x01;           // Dummy CRC + Stop
  if (cmd == CMD0) {
    n = 0x95;         // Valid CRC for CMD0(0) + Stop
  }
  if (cmd == CMD8) {
    n = 0x87;         // Valid CRC for CMD8(0x1AA) + Stop
  }
  sdc_xchg_spi(n, &data);

  // Receive command response
  if (cmd == CMD12) {
    sdc_xchg_spi(0xff, &data); // Skip a stuff byte on stop to read
  }
  n = 10;             // Wait for a valid response in timeout of 10 attempts
  do {
    sdc_xchg_spi(0xff, &data);
  } while ((data & 0x80) && --n);

  return data;     // Return with the response value
}

/***************************************************************************//**
 * Inidialize an SD Card.
 ******************************************************************************/
DSTATUS sd_card_disk_initialize(void)
{
  BYTE n, cmd, ty, ocr[4], data;

  if(sdc_platform_spi_init() != SL_STATUS_OK) {
    return STA_NOINIT;
  }

  for (sd_card_timer_1 = 10; sd_card_timer_1;);   // Wait for 10ms

  if (sd_card_status & STA_NODISK) {
    return sd_card_status; // Is card existing in the soket?
  }

  FCLK_SLOW();
  for (n = 10; n; n--) {
    sdc_xchg_spi(0xff, &data);  // Send 80 dummy clocks
  }

  ty = 0;
  if (send_cmd(CMD0, 0) == 1) {       // Put the card SPI mode
    sd_card_timer_1 = 1000;           // Initialization timeout = 1 sec
    if (send_cmd(CMD8, 0x1aa) == 1) { // Is the card SDv2?
      for (n = 0; n < 4; n++) {
        sdc_xchg_spi(0xff, &ocr[n]);          // Get 32 bit return value of R7 resp
      }
      if (ocr[2] == 0x01 && ocr[3] == 0xaa) { // Is the card supports vcc of 2.7-3.6V?
        while (sd_card_timer_1 && send_cmd(ACMD41, 1UL << 30)) ; // Wait for end of initialization with ACMD41(HCS)
        if (sd_card_timer_1 && send_cmd(CMD58, 0) == 0) { // Check CCS bit in the OCR
          for (n = 0; n < 4; n++) {
            sdc_xchg_spi(0xff, &ocr[n]);
          }
          ty = (ocr[0] & 0x40) ? CT_SDC2 | CT_BLOCK : CT_SDC2;  // Card id SDv2
        }
      }
    } else {  // Not SDv2 card
      if (send_cmd(ACMD41, 0) <= 1) {   // SDv1 or MMC?
        ty = CT_SDC1;
        cmd = ACMD41;     // SDv1 (ACMD41(0))
      } else {
        ty = CT_MMC3;
        cmd = CMD1;       // MMCv3 (CMD1(0))
      }
      while (sd_card_timer_1 && send_cmd(cmd, 0)) ;    // Wait for end of initialization
      if ((!sd_card_timer_1) || (send_cmd(CMD16, 512) != 0)) { // Set block length: 512
        ty = 0;
      }
    }
  }
  sd_card_type = ty;   // Card type
  deselect();

  if (ty) {        // OK
    FCLK_FAST();   // Set fast clock
    sd_card_status &= ~STA_NOINIT;  // Clear STA_NOINIT flag
  } else {         // Failed
    sd_card_status = STA_NOINIT;
  }

  return sd_card_status;
}

/***************************************************************************//**
 * Get SD Card Status.
 ******************************************************************************/
DSTATUS sd_card_disk_status(void)
{
  return sd_card_status;
}

/***************************************************************************//**
 * Read Sector(s) from SD Card.
 ******************************************************************************/
dresult_t sd_card_disk_read(BYTE *buff, LBA_t sector, UINT count)
{
  DWORD sect = (DWORD)sector;

  // Check parameter
  if (!count) {
    return RES_PARERR;
  }

  // Check if drive is ready
  if (sd_card_status & STA_NOINIT) {
    return RES_NOTRDY;
  }

  // LBA ot BA conversion (byte addressing cards)
  if (!(sd_card_type & CT_BLOCK)) {
    sect *= 512;
  }

  // Single block read
  if (count == 1) {
    if ((send_cmd(CMD17, sect) == 0)  // READ_SINGLE_BLOCK
        && rcvr_datablock(buff, 512)) {
      count = 0;
    }
  } else { // Multiple block read
    if (send_cmd(CMD18, sect) == 0) { // READ_MULTIPLE_BLOCK
      do {
        if (!rcvr_datablock(buff, 512)) {
          break;
        }
        buff += 512;
      } while (--count);
      send_cmd(CMD12, 0); // STOP_TRANSMISSION
    }
  }
  deselect();

  return count ? RES_ERROR : RES_OK;
}

/***************************************************************************//**
 * Write Sector(s) to SD Card.
 ******************************************************************************/
#if FF_FS_READONLY == 0
dresult_t sd_card_disk_write(const BYTE *buff, LBA_t sector, UINT count)
{
  DWORD sect = (DWORD)sector;

  if (sd_card_status & STA_NOINIT) { // Check drive status
    return RES_NOTRDY;
  }
  if (sd_card_status & STA_PROTECT) { // Check write protect
    return RES_WRPRT;
  }

  if (!(sd_card_type & CT_BLOCK)) { // LBA ==> BA conversion (byte addressing cards)
    sect *= 512;
  }

  if (count == 1) { // Single sector write
    if ((send_cmd(CMD24, sect) == 0)  // WRITE_BLOCK
      && xmit_datablock(buff, 0xFE)) {
      count = 0;
    }
  } else { // Multiple sector write
    if (sd_card_type & CT_SDC) send_cmd(ACMD23, count); // Predefine number of sectors
    if (send_cmd(CMD25, sect) == 0) { // WRITE_MULTIPLE_BLOCK
      do {
        if (!xmit_datablock(buff, 0xFC)) {
          break;
        }
        buff += 512;
      } while (--count);

      if (!xmit_datablock(0, 0xFD)) { // STOP_TRAN token
        count = 1;
      }
    }
  }
  deselect();

  return count ? RES_ERROR : RES_OK;
}
#endif

/***************************************************************************//**
 * This function is called to control device specific features 
 * and miscellaneous functions other than generic read/write.
 ******************************************************************************/
dresult_t sd_card_disk_ioctl(BYTE cmd, void *buff)
{
  dresult_t res;
  BYTE n;
  BYTE csd[16];
  BYTE *ptr = buff;
  BYTE data;
  DWORD csize;
#ifdef FF_USE_TRIM
  LBA_t *range;
  DWORD st;
  DWORD ed;
#endif

  if (sd_card_status & STA_NOINIT) {
    return RES_NOTRDY;
  }

  res = RES_ERROR;
  switch (cmd) {
    // Make sure that no pending write process.
    // Do not remove this or written sector might not left updated.
    case CTRL_SYNC:
      if (select()) {
        res = RES_OK;
      }
      deselect();
      break;

    // Get number of sectors on the disk (DWORD)
    case GET_SECTOR_COUNT:
      if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
        if ((csd[0] >> 6) == 1) { // SDC ver 2.00
          csize = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
          *(LBA_t*)buff = csize << 10;
        } else {          // SDC ver 1.XX or MMC
          n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
          csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
          *(LBA_t*)buff = csize << (n - 9);
        }
        res = RES_OK;
      }
      deselect();
      break;

    // Get erase block size in unit of sector (DWORD)
    case GET_BLOCK_SIZE:
      if (sd_card_type & CT_SDC2) {         // SDv2?
        if (send_cmd(ACMD13, 0) == 0) { // Read SD status
          sdc_xchg_spi(0xff, &data);
          if (rcvr_datablock(csd, 16)) {// Read partial block
            for (n = 64 - 16; n; n--) {
              sdc_xchg_spi(0xff, &data);    // Purge trailing data
            }
            *(DWORD*)buff = 16UL << (csd[10] >> 4);
            res = RES_OK;
          }
        }
      } else {                          // SDv1 or MMCv3
        if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) { // Read CSD
          if (sd_card_type & CT_SDC1) { // SDv1
            *(DWORD*)buff = (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
          } else {          // MMCv3
            *(DWORD*)buff = ((WORD)((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
          }
          res = RES_OK;
        }
      }
      deselect();
      break;

#ifdef FF_USE_TRIM
    // Erase a block of sectors (used when _USE_TRIM in ffconf.h is 1)
    case CTRL_TRIM:
      // Check if the card is SDC
      if (!(sd_card_type & CT_SDC)) {
        break;
      }
      // Get CSD
      if (sd_card_disk_ioctl(MMC_GET_CSD, csd)) {
        break;
      }
      // Check if ERASE_BLK_EN = 1
      if (!(csd[10] & 0x40)) {
        break;
      }
      range = buff;
      st = (DWORD)range[0];
      ed = (DWORD)range[1]; // Load sector block
      if (!(sd_card_type & CT_BLOCK)) {
        st *= 512;
        ed *= 512;
      }
      // Erase sector block
      if ((send_cmd(CMD32, st) == 0)
          && (send_cmd(CMD33, ed) == 0)
          && (send_cmd(CMD38, 0) == 0)
          && wait_ready(60000)) {
        res = RES_OK; // FatFs does not check result of this command
      }
      break;
#endif

    // Following commands are never used by FatFs module
    // Get card type flags (1 byte)
    case MMC_GET_TYPE:
      *ptr = sd_card_type;
      res = RES_OK;
      break;

    // Receive CSD as a data block (16 bytes)
    case MMC_GET_CSD:
      // READ_CSD
      if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(ptr, 16)) {
        res = RES_OK;
      }
      deselect();
      break;

    // Receive CID as a data block (16 bytes)
    case MMC_GET_CID:
      // READ_CID
      if ((send_cmd(CMD10, 0) == 0) && rcvr_datablock(ptr, 16)) {
        res = RES_OK;
      }
      deselect();
      break;

    // Receive OCR as an R3 resp (4 bytes)
    case MMC_GET_OCR:
      // READ_OCR
      if (send_cmd(CMD58, 0) == 0) {
        for (n = 4; n; n--) {
          *ptr++ = sdc_xchg_spi(0xff, &data);
        }
        res = RES_OK;
      }
      deselect();
      break;

    // Receive SD status as a data block (64 bytes)
    case MMC_GET_SDSTAT:
      // SD_STATUS
      if (send_cmd(ACMD13, 0) == 0) {
        sdc_xchg_spi(0xff, &data);
        if (rcvr_datablock(ptr, 64)) {
          res = RES_OK;
        }
      }
      deselect();
      break;

    default:
      res = RES_PARERR;
      break;
  }

  return res;
}

/***************************************************************************//**
 * Device timer function.
 * This function must be called from timer interrupt routine in period
 * of 1 ms to generate card control timing.
 ******************************************************************************/
void disk_timerproc(void)
{
  WORD n;
  BYTE s;

  n = sd_card_timer_1; // 1kHz decrement timer stopped at 0
  if (n) {
    sd_card_timer_1 = --n;
  }

  n = sd_card_timer_2;
  if (n) {
    sd_card_timer_2 = --n;
  }

  s = sd_card_status;

  if (MMC_CD) {      // Card is in socket
    s &= ~STA_NODISK;
  } else {           // Socket empty
    s |= (STA_NODISK | STA_NOINIT);
  }
  sd_card_status = s;
}
