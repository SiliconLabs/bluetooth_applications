/**
 * @file       usb2spi.c
 *
 * @brief      Process to run USB2SPI Mode
 *
 *             can be used in bare-metal or RTOS systems
 *             1. select the CHIP to drive: setup_usb2spi_process(CHIP);
 *             2. execute once init_usb2spi_process();
 *             3. continuously execute the run_usb2spi_process() on reception of
 *   data;
 *
 * @author     Decawave
 *
 * @attention  Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *             All rights reserved.
 *
 */
#include "usb2spi.h"

#include "port.h"
#include "port_common.h"
#include "usb_uart_rx.h"
#include "usb_uart_tx.h"
#include "deca_dbg.h"
// -----------------------------------------------------------------------------

/* Definitions for compatibility */
#define USB2SPI_ENTER_CRITICAL()    taskENTER_CRITICAL()
#define USB2SPI_EXIT_CRITICAL()     taskEXIT_CRITICAL()

#define USB2SPI_MALLOC              pvPortMalloc
#define USB2SPI_FREE                vPortFree

#undef USB2SPI_USE_DYNAMIC_BUF
// -----------------------------------------------------------------------------

#define SOFTWARE_VER_STRINGUSB      "EVB3000 USB2SPI 2.0"

#define SPI_ACC_BUF_SIZE            0x3100 /**< Maximum size of buffer to read
                                            *   whole accumulator in the the
                                            *   USB2SPI */

/* Constants */
const char deca[] = "deca?";
const char dwSTOP[] = "stop";
const char dwRESET[] = "reset";

extern void command_stop_received(void);

// -----------------------------------------------------------------------------
// local data
typedef struct
{
  uint8_t     rx_buf[USB_RX_BUF_SIZE];
  uint8_t     tx_buf[SPI_ACC_BUF_SIZE];   /**< need a long buffer for
                                           *   accumulator read */
  size_t      tx_buf_length;
}usb2spi_t;

#ifndef USB2SPI_USE_DYNAMIC_BUF
static usb2spi_t  usb2spi_static;
#endif

static usb2spi_t *pUsb2Spi;
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Implementation

/*
 * @brief    setup the speed for usb2spi:
 *             0 for slow spi speed and 1 for high
 **/
// static
void configSPIspeed(int rate)
{
  static int localSPIspeed = -1;

  if (localSPIspeed != rate) {
    localSPIspeed = rate;
    if (localSPIspeed == 1) {
      set_dw_spi_fast_rate();
    } else {
      set_dw_spi_slow_rate();
    }
  }
}

/*
 * @brief     Decawave usb2spi standard protocol
 * */
static usb_data_e usb2spi(uint8_t *pBuf, uint16_t len)
{
  usb_data_e result = NO_DATA;
  uint32_t   msglength, datalength;
  // first byte specifies the SPI speed and SPI read/write operation
  // bit 0 = 1 for write, 0 for read
  // bit 1 = 1 for high speed, 0 for low speed

  // <STX>   <ETX>
  //
  // to read from the device (e.g. 4 bytes), total length is = 1 + 1 +
  //   length_of_command (2) + length_of_data_to_read (2) + header length (1/2)
  //   + 1
  //
  // to write to the device (e.g. 4 bytes),  total length is = 1 + 1 +
  //   length_of_command (2) + length_of_data_to_write (2) + header length (1/2)
  //   + data length + 1
  //
  // LBS comes first:   0x2, 0x2, 0x7, 0x0, 0x04, 0x00, 0x3

  if (len > 0) {
    // 0x2 = STX - start of SPI transaction data
    if ((len >= 7) && (pBuf[0] == Usb_Msg_Header)) {
      msglength = ((uint32_t)pBuf[3]) << 8 | (uint32_t)pBuf[2];
      if (len < msglength) {
        return(result);
      }
      datalength = ((uint32_t)pBuf[5]) << 8 | (uint32_t)pBuf[4];

      configSPIspeed((pBuf[1] >> 1) & 0x1);        // configure SPI speed

      if ((pBuf[1] & 0x1) == 0) {       // SPI read
        if ((datalength + 3) > (sizeof(pUsb2Spi->tx_buf) - 1)) {
          return(result);
        }

        pUsb2Spi->tx_buf[0] = Usb_Msg_Header;
        pUsb2Spi->tx_buf[1] = 0x0;         // no error
        pUsb2Spi->tx_buf[datalength + 2] = Usb_Msg_Footer;

        // max data we can read in a single SPI transaction is BUFFLEN as the
        //   USB/VCP tx buffer is only 4096 bytes long
        if (pBuf[msglength - 1] != Usb_Msg_Footer) {
          pUsb2Spi->tx_buf[1] = 0x1;           // if no ETX (0x3) indicate error
          pUsb2Spi->tx_buf[2] = Usb_Msg_Footer;
          datalength = 0;
        } else {
          // do the read from the SPI
          readfromspi(msglength - 7,
                      &pBuf[6],
                      datalength,
                      &pUsb2Spi->tx_buf[2]); // result is stored in the tx_buf
        }

        pUsb2Spi->tx_buf_length = datalength + 3;
        result = DATA_SEND;
      } else
      if ((pBuf[1] & 0x1) == 1) {    // SPI write
        if (datalength > msglength - 7) {
          return(result);
        }
        int headerlength = msglength - 7 - datalength;

        pUsb2Spi->tx_buf[0] = Usb_Msg_Header;
        pUsb2Spi->tx_buf[1] = 0x0;                 // no error
        pUsb2Spi->tx_buf[2] = Usb_Msg_Footer;

        if ((pBuf[msglength - 1] != Usb_Msg_Footer)
            || (headerlength < 0)) {
          pUsb2Spi->tx_buf[1] = 0x1;           // if no ETX (0x3) indicate error
        } else {
          // do the write to the SPI
          writetospi(headerlength,
                     &pBuf[6],
                     datalength,
                     &pBuf[6 + headerlength]); // result is stored in the buffer
        }

        pUsb2Spi->tx_buf_length = 3;
        result = DATA_SEND;
      } else {
      }
    } else
    if ((len >= 5) && (memcmp(pBuf, deca, 5) == 0)) {
      // send a reply 'y'
      pUsb2Spi->tx_buf[0] = 'y';
      strcpy((char *)&pUsb2Spi->tx_buf[1], SOFTWARE_VER_STRINGUSB);
      pUsb2Spi->tx_buf_length = strlen(SOFTWARE_VER_STRINGUSB) + 2;

      result = DATA_SEND;
    } else
    if ((len >= 5) && (memcmp(pBuf, dwRESET, 5) == 0)) {// Reset DW
      reset_DW3000();
    } else
    if ((len >= 4) && (memcmp(pBuf, dwSTOP, 4) == 0)) {
      /* this should execute the fn_stop to exit the current process */
      command_stop_received();          // communication to the user application
      result = NO_DATA;
    } else {
      // usb2spi_protocol_check take care of correct input
    }
  }

  return result;
}

/* @fn         usb2spi_frame_complete
 * @brief      Check if the data contains a complete USB2SPI frame
 * @return     DATA_READY if buffer contains a USB2SPI frame, DATA_ERROR
 *   otherwise
 * */
usb_data_e usb2spi_frame_complete(uint8_t *p, uint16_t len)
{
  usb_data_e ret = DATA_ERROR;

  if (len > 7) {
    if ((p[0] == Usb_Msg_Header) && ((p[2] + (p[3] << 8)) == len)
        && (p[len - 1] == Usb_Msg_Footer)) {
      ret = DATA_READY;
    }
  }

  return ret;
}

/* @fn         usb2spi_protocol_check
 * @brief      Check if the data contains a valid USB2SPI command
 * @return     DATA_READY if buffer contains a USB2SPI command, DATA_ERROR
 *   otherwise
 * */
usb_data_e usb2spi_command_check(uint8_t *p, uint16_t len)
{
  usb_data_e ret = DATA_ERROR;
  if (len < 4) {
    return ret;
  }
  if (((len >= sizeof(deca) - 1)
       && (memcmp(p, deca, sizeof(deca) - 1) == 0))          /* "deca?" */
      || ((len >= sizeof(dwSTOP) - 1)
          && (memcmp(p, dwSTOP, sizeof(dwSTOP) - 1) == 0))   /* "stop" */
      || ((len >= sizeof(dwRESET) - 1)
          && (memcmp(p, dwRESET, sizeof(dwRESET) - 1) == 0)) /* "reset" */
      ) {
    ret = DATA_READY;
  }

  return ret;
}

// -----------------------------------------------------------------------------
// IMPLEMETATION : interface to the process : bare-metal implementation

/*
 * @brief     exporting functions to setup the bare-metal process.
 *             fn_init - executed once.
 *             fn_run  - executed on reception of data.
 *            fn_stop - executed to terminate the process.
 * */
error_e usb2spi_process_init(void)
{
  error_e    ret = _NO_ERR;

  if (pUsb2Spi) {
    ret = _ERR_Usb2Spi_ptr_busy;
  } else {
#if USB2SPI_USE_DYNAMIC_BUF
    pUsb2Spi = USB2SPI_MALLOC(sizeof(usb2spi_t));
#else
    pUsb2Spi = &usb2spi_static;
#endif
    if (!pUsb2Spi) {
      ret = _ERR_Usb2Spi_ptr_alloc;
    } else {
      app.local_buff_length = 0;
      memset(&app.local_buff, 0, sizeof(app.local_buff));
    }
  }
  return (ret);
}

/*
 * @brief     the "run" function implements usb2spi process functionality:
 *             it parses the app.local_buff: app.local_buff_length
 *
 * */
void usb2spi_process_run(void)
{
  usb_data_e tmp;
  tmp = usb2spi(app.local_buff, app.local_buff_length);

  if (tmp != NO_DATA) {
    app.local_buff_length = 0;

    if (pUsb2Spi->tx_buf_length) {
      port_tx_msg(pUsb2Spi->tx_buf, pUsb2Spi->tx_buf_length);
    }
  }
}

/*
 * @brief     stop function implements the stop functionality if any suitable
 *   for current process
 *             which will be executed on reception of Stop command
 * */
void usb2spi_process_terminate(void)
{
  if (pUsb2Spi) {
    USB2SPI_ENTER_CRITICAL();

    port_stop_all_UWB();

    strcpy((char *)pUsb2Spi->tx_buf, "Ok\r\n");

    pUsb2Spi->tx_buf_length = strlen((char *)pUsb2Spi->tx_buf);

    port_tx_msg(pUsb2Spi->tx_buf, pUsb2Spi->tx_buf_length);

#if USB2SPI_USE_DYNAMIC_BUF
    USB2SPI_FREE(pUsb2Spi);
#endif
    pUsb2Spi = NULL;

    USB2SPI_EXIT_CRITICAL();
  }
}

// -----------------------------------------------------------------------------
