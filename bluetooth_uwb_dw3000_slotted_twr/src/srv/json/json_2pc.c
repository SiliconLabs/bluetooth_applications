/**
 * @file     json_2pc.c
 * @brief    collection of JSON formatted functions which used
 *           to report from Node application to the PC
 *
 * @author Decawave
 *
 * @attention Copyright 2017 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */
#include <string.h>
#include <stdio.h>
#include "cmd_fn.h"
#include "node.h"
#include "usb_uart_tx.h"

/*
 * @brief function to report to PC a new tag was discovered
 *
 * 'JSxxxx{"NewTag":
 *             <string>//address64, string
 *        }'
 *
 * */
void signal_to_pc_new_tag_discovered(uint64_t addr64)
{
  char *str = CMD_MALLOC(MAX_STR_SIZE);

  if (str) {
    int  hlen;

    hlen = sprintf(str, "JS%04X", 0x5A5A);       // reserve space for length of
                                                 //   JS object

    sprintf(&str[strlen(str)], "{\"NewTag\":\"%08lX%08lX\"}",
            (uint32_t)(addr64 >> 32), (uint32_t)addr64);

    sprintf(&str[2], "%04X", strlen(str) - hlen);// add formatted 4X of length,
                                                 //   this will kill first '{'
    str[hlen] = '{';                              // restore the start bracket

    sprintf(&str[strlen(str)], "\r\n");
    port_tx_msg((uint8_t *)str, strlen(str));

    CMD_FREE(str);
  }
}

/*
 * @brief function to report to PC the Listener data received
 *
 * 'JSxxxx{"LSTN":[RxBytes_hex,..,],"TS":"0xTimeStamp32_Hex","O":Offset_dec}'
 *
 * This fn() uses pseudo-JSON format, it appends "+" to the end of
 *
 * */
#define MAX_PRINT_FAST_LISTENER (6)
error_e send_to_pc_listener_info(uint8_t *data,
                                 uint8_t size,
                                 uint8_t *ts,
                                 int16_t cfo)
{
  error_e  ret = _ERR_Cannot_Alloc_Memory;

  uint32_t cnt, flag_plus = 0;
  uint16_t hlen;
  int      cfo_pphm;
  char    *str;

  if (app.listener_mode == 0) {// speed is a priority
    if (size > MAX_PRINT_FAST_LISTENER) {
      flag_plus = 1;
      size = MAX_PRINT_FAST_LISTENER;
    }

    str = CMD_MALLOC(MAX_STR_SIZE);
  } else {
    str = CMD_MALLOC(MAX_STR_SIZE + MAX_STR_SIZE);
  }

  if (str) {
    cfo_pphm = (int)((float)cfo * (CLOCK_OFFSET_PPM_TO_RATIO * 1e6 * 100));

    hlen = sprintf(str, "JS%04X", 0x5A5A);       // reserve space for length of
                                                 //   JS object
    sprintf(&str[strlen(str)], "{\"LSTN\":[");

    // Loop over the received data
    for (cnt = 0; cnt < size; cnt++)
    {
      sprintf(&str[strlen(str)], "%02X,", data[cnt]);    // Add the byte and the
                                                         //   delimiter - "XX,"
    }

    if (flag_plus) {
      sprintf(&str[strlen(str)], "+,");
    }

    sprintf(&str[strlen(str) - 1],
            "],\"TS\":\"0x%02X%02X%02X%02X\",",
            ts[3],
            ts[2],
            ts[1],
            ts[0]);
    sprintf(&str[strlen(str)], "\"O\":%d", cfo_pphm);

    sprintf(&str[strlen(str)], "%s", "}\r\n");
    sprintf(&str[2], "%04X", strlen(str) - hlen); // add formatted 4X of length,
                                                  // this will erase first '{'
    str[hlen] = '{';                              // restore the start bracket
    ret = port_tx_msg((uint8_t *)str, strlen(str));

    CMD_FREE(str);
  }

  return (ret);
}

/*
 * @brief This is a report of twr to pc
 *
 * There are two modes of operation: JSON(long output) or plain(short output)
 * JSON (default):
 *  'JSxxxx{"TWR":
 *    {     "a16":%04X, //addr16
 *          "R":%d,//range num
 *          "T":%d,//sys timestamp of Final WRTO Node's SuperFrame start, us
 *          "D":%f,//distance
 *          "P":%f,//raw pdoa
 *          "Xcm":%f,//X, cm
 *          "Ycm":%f,//Y, cm
 *          "O":%f,//clock offset in hundreds part of ppm
 *          "V":%d //service message data from the tag: (stationary, etc)
 *          "X":%d //service message data from the tag: (stationary, etc)
 *          "Y":%d //service message data from the tag: (stationary, etc)
 *          "Z":%d //service message data from the tag: (stationary, etc)
 *    }
 *   }'
 *
 * Plain:
 * used if any from below is true:
 * diag, acc,
 * */
void send_to_pc_twr(result_t *pRes)
{
  char *str = CMD_MALLOC(MAX_STR_SIZE);
  int  hlen;

  if (str) {
    if ((app.pConfig->s.accEn == 1)     \
        || (app.pConfig->s.diagEn == 1) \
        || (app.pConfig->s.reportLevel > 1)) {
      if (app.pConfig->s.reportLevel == 3) {
        /* shortest "AR" output: 18 chars per location: ~640 locations per
         *   second
         * */
        sprintf(str, "AR%04X%04X%08lX%08lX",
                (uint16_t)(pRes->addr16),
                (uint16_t) (pRes->rangeNum),
                (long int)(pRes->x_cm),
                (long int)(pRes->y_cm));
      } else {
        /* optimum "RA" output: 58 chars per location: ~200 locations per second
         * */
        sprintf(str, "RA%04X %04X %08lX %08lX %08lX %1X X:%04X Y:%04X Z:%04X",
                (uint16_t)(pRes->addr16),
                (uint16_t)(pRes->rangeNum),
                (long int)(pRes->x_cm),
                (long int)(pRes->y_cm),
                (long int)(pRes->clockOffset_pphm),
                (uint8_t) (pRes->flag),
                (uint16_t)(pRes->acc_x),
                (uint16_t)(pRes->acc_y),
                (uint16_t)(pRes->acc_z));
      }
      sprintf(&str[strlen(str)], "\r\n");
      port_tx_msg((uint8_t *)str, strlen(str));
    } else if (app.pConfig->s.reportLevel == 1) {
      /* use JSON type of output during a normal operation
       *
       * This is not very efficient, as one TWR location is ~110 chars,
       * as per format below
       * JS  xx{"TWR": {"a16":"2E5C","R":3,"T":8605,"D":343,"P":1695,"Xcm":165,
       *        "Ycm":165,"O":14,"V":1,"X":53015,"Y":60972,"Z":10797}}
       *
       * For pure UART, with limit of 115200b/s,
       * the channel can handle ~100 locations per second,
       * i.e. 10 tags ranging on maximum rate of 10 times a second.
       * For higher throughput cut the JSON TWR object
       * or use plain output instead.
       *
       */

      /* Floating point values are standard for JSON objects,
       * however the floating point printing
       * is not used in current application.
       * If the floating point printing required,
       * will need to add "-u _printf_float" to the
       * linker string, to include "floating printf"
       * to the nano.spec of the stdlib.
       * This will increase the size of application by ~6kBytes
       * and the floating printing also requires
       * much more stack space.
       * Use this with caution,
       * as this might result unpredictable stack overflow / HardFault.
       */
      hlen = sprintf(str, "JS%04X", 0x5A5A);         // reserve space for length
                                                     //   of JS object

      sprintf(&str[strlen(str)], "{\"TWR\": ");

      sprintf(&str[strlen(str)],
              "{\"a16\":\"%04X\","
              "\"R\":%d,"           // range number
              "\"T\":%d,",          // sys timestamp of Final WRTO Node's
                                    //   SuperFrame start, us
              (int)(pRes->addr16),
              (int)(pRes->rangeNum),
              (int)(pRes->resTime_us));

      if (app.pConfig->s.debugEn) {
        sprintf(&str[strlen(str)],
                "\"Tm\":%d,",        // Master's temperature, in degree
                                     //   centigrade
                (int)(pRes->tMaster_C));
      }

      sprintf(&str[strlen(str)],
              "\"D\":%d,"           // distance as int, in cm
              "\"P\":%d,"           // pdoa  as int in milli-radians
              "\"P'\":%d,"          // pdoa from Poll message as int in
                                    //   milli-radians
              "\"Xcm\":%d,"         // X distance wrt Node in cm
              "\"Ycm\":%d,"         // Y distance wrt Node in cm
              "\"Pdiffnm\":%d,",
              (int)(pRes->dist_cm),
              (int)(pRes->pdoa_raw_deg),
              (int)(pRes->pdoa_raw_degP),
              (int)(pRes->x_cm),
              (int)(pRes->y_cm),
              (int)(pRes->path_diff));

      sprintf(&str[strlen(str)],
              "\"O\":%d,"       // clock offset as int
              "\"V\":%d,"       // service message data from the tag: (bitmask:
                                //   bit0 = stationary, bit15 = zeroed
                                //   pdoaOffset used; bit14 = zeroed rngOffset
                                //   used)
              "\"X\":%d,"       // Normalized accel data X from the Tag, mg
              "\"Y\":%d,"       // Normalized accel data Y from the Tag, mg
              "\"Z\":%d"        // Normalized accel data Z from the Tag, mg
              "}",
              (int)(pRes->clockOffset_pphm),
              (int)(pRes->flag),
              (int)(pRes->acc_x),
              (int)(pRes->acc_y),
              (int)(pRes->acc_z));

      sprintf(&str[strlen(str)], "}");

      sprintf(&str[2], "%04X", strlen(str) - hlen);     // add formatted 4X of
                                                        //   length, this will
                                                        //   kill first '{'
      str[hlen] = '{';                                  // restore the start
                                                        //   bracket

      sprintf(&str[strlen(str)], "\r\n");
      port_tx_msg((uint8_t *)str, strlen(str));

      if (0) {/* TWR PDoA mini Diag: SDTP-50 */
        hlen = sprintf(str, "JS%04X", 0x5A5A);       // reserve space for length
                                                     //   of JS object

        sprintf(&str[strlen(str)], "{\"TWR_DIAG\": ");

        sprintf(&str[strlen(str)],
                "{\"a16\":\"%04X\","
                "\"R\":%d,"         // range number
                "\"T\":%d,"         // sys timestamp of Final WRTO Node's
                                    //   SuperFrame start, us
                ,
                (int)(pRes->addr16),
                (int)(pRes->rangeNum),
                (int)(pRes->resTime_us));

        sprintf(&str[strlen(str)],
                "\"pDTUNE5\":\"0x%02X\","
                "\"pCIA_TDOA_0\":\"0x%08X\","
                "\"pCIA_TDOA_1_PDOA\":\"0x%08X\","
                "\"pIP_DIAG_10\":\"0x%04X\","
                "\"pCY0_DIAG_10\":\"0x%04X\","
                "\"pCY0_TOA_HI\":\"0x%04X\","
                "\"pCY1_TOA_HI\":\"0x%04X\","
                ,
                (unsigned int)pRes->finalPDOA.mDiag.DTUNE5,
                (unsigned int)pRes->finalPDOA.mDiag.CIA_TDOA_0,
                (unsigned int)pRes->finalPDOA.mDiag.CIA_TDOA_1_PDOA,
                (unsigned int)pRes->finalPDOA.mDiag.IP_DIAG_10,
                (unsigned int)pRes->finalPDOA.mDiag.CY0_DIAG_10,
                (unsigned int)pRes->finalPDOA.mDiag.CY0_TOA_HI,
                (unsigned int)pRes->finalPDOA.mDiag.CY1_TOA_HI);

        sprintf(&str[strlen(str)],
                "\"fDTUNE5\":\"0x%02X\","
                "\"fCIA_TDOA_0\":\"0x%08X\","
                "\"fCIA_TDOA_1_PDOA\":\"0x%08X\","
                "\"fIP_DIAG_10\":\"0x%04X\","
                "\"fCY0_DIAG_10\":\"0x%04X\","
                "\"fCY0_TOA_HI\":\"0x%04X\","
                "\"fCY1_TOA_HI\":\"0x%04X\""
                "}",
                (unsigned int)pRes->finalPDOA.mDiag.DTUNE5,
                (unsigned int)pRes->finalPDOA.mDiag.CIA_TDOA_0,
                (unsigned int)pRes->finalPDOA.mDiag.CIA_TDOA_1_PDOA,
                (unsigned int)pRes->finalPDOA.mDiag.IP_DIAG_10,
                (unsigned int)pRes->finalPDOA.mDiag.CY0_DIAG_10,
                (unsigned int)pRes->finalPDOA.mDiag.CY0_TOA_HI,
                (unsigned int)pRes->finalPDOA.mDiag.CY1_TOA_HI);

        sprintf(&str[strlen(str)], "}");

        sprintf(&str[2], "%04X", strlen(str) - hlen);   // add formatted 4X of
                                                        //   length, this will
                                                        //   kill first '{'
        str[hlen] = '{';                                // restore the start
                                                        //   bracket

        sprintf(&str[strlen(str)], "\r\n");
        port_tx_msg((uint8_t *)str, strlen(str));
      }
    } else {
      // no output
    }

    CMD_FREE(str);
  }
}

/* @brief input "str" must be a null-terminated string with enough space in it.
 *        this is slow output of accumulator from the chip, starting with
 *   ACC_OFFSET value.
 *        To be used solely for debug purposes.
 * */
static void send_acc(char       *str,
                     uint16_t   maxLen,
                     uint8_t    *pAcc)
{
  int     n;
  int16_t cmplex_m[2];

  cmplex_m[0] = 0;
  cmplex_m[1] = 0;

  n = strlen(str);

  for (int i = 1; i <= (FULL_ACC_LEN * 4); i += 4)
  {
    if (n >= (maxLen - 4)) {
      while (port_tx_msg((uint8_t *)str, n) != _NO_ERR)
      {
        osThreadYield();        // force switch content
        osDelay(5);             // wait 5ms for Flush thread freed the buffer
      }
      n = 0;
    }

    if (i > (ACC_OFFSET * 4)) {
      memcpy(&cmplex_m[0], &pAcc[i], 4);
    }

    n +=
      sprintf(&str[n], "%04X%04X", (cmplex_m[0] & 0xFFFF),
              (cmplex_m[1] & 0xFFFF));
  }

  n += sprintf(&str[n], "\r\n");

  while (port_tx_msg((uint8_t *)str, n) != _NO_ERR)
  {
    osThreadYield();        // force switch content
    osDelay(5);             // wait 5ms for Flush thread freed the buffer
  }
}

/* @brief input "str" must be a null-terminated string with enough space in it.
 *        To be used solely for debug purposes.
 * */
static void send_diag(char    *str,
                      uint16_t maxLen,
                      uint8_t *pDiag,
                      int16_t  pdoa)
{
  if ((strlen(str) + 2 * sizeof(dwt_rxdiag_t) + 1 + 10 + 6) < maxLen) {
    for (size_t i = 0; i < sizeof(dwt_rxdiag_t); i++)
    {
      sprintf(&str[strlen(str)], "%02X", pDiag[i]);
    }

    sprintf(&str[strlen(str)], " ");

    sprintf(&str[strlen(str)], "%04X", pdoa);

    while (port_tx_msg((uint8_t *)str, strlen(str)) != _NO_ERR)
    {
      osThreadYield();          // force switch content
      osDelay(5);               // wait 5ms for Flush thread freed the buffer
    }
  }
}

/* @brief send acc & diagnostics information
 *           these are blocking operations
 *
 * */
void send_to_pc_diag_acc(rx_mail_t *pRxMailPckt)
{
  static int logNum = 0;

  char *str = CMD_MALLOC(MAX_STR_SIZE);
  uint8_t *p;

  if (str) {
    // send the Accumulator information from the pRxMailPckt
    if (app.pConfig->s.accEn == 1) {
      /* "master chip" */
      p = (uint8_t *)&pRxMailPckt->acc;

      sprintf(str,
              "\r\nAM%04X %02X CLKOFF: %d\r\n",
              logNum,
              pRxMailPckt->res.rangeNum,
              (int)(pRxMailPckt->res.clockOffset_pphm));

      send_acc(str, MAX_STR_SIZE, p);
    }

    // send the Diagnostics information from the pRxMailPckt
    if (app.pConfig->s.diagEn == 1) {
      sprintf(str, "DM%04X %02X ", logNum, pRxMailPckt->res.rangeNum);
      send_diag(str, MAX_STR_SIZE,
                (uint8_t *)&pRxMailPckt->diag_dw3000,
                pRxMailPckt->res.pdoa_raw_deg);      // PDOA from the Final
    }

    CMD_FREE(str);
  }

  logNum++;
}
