/**
 * @file      cmd_fn.c
 *
 * @brief     Collection of executables functions from defined known_commands[]
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#include "cmd_fn.h"
#include "translate.h"
#include "version.h"
#include "config.h"
#include "task_flush.h"
#include "deca_dbg.h"
#include "port_common.h"
#include "usb_uart_tx.h"
#include "cmd.h"
#include "deca_device_api.h"
#include "em_gpio.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"

#define CMD_COLUMN_WIDTH     10
#define CMD_COLUMN_MAX       4
// -----------------------------------------------------------------------------
const char CMD_FN_RET_OK[] = "ok\r\n";

/***************************************************************************//*
 *
 *                          f_xx "command" FUNCTIONS
 *
 * REG_FN(f_node) macro will create a function
 *
 * const char *f_node(char *text, param_block_t *pbss, int val)
 *
 * */

// -----------------------------------------------------------------------------
// Operation Mode change section

/** @defgroup Application_Selection
 * @brief    Commands to start the particular application
 * @{
 */

// communication to the user application
void command_stop_received(void)
{
  xEventGroupSetBits(app.xStartTaskEvent, Ev_Stop_All);
}

/**
 * @brief    defaultTask will start PDoA NODE user application
 * app.mode will be mNODE
 * */
REG_FN(f_node)
{
  xEventGroupSetBits(app.xStartTaskEvent, Ev_Node_Task);
  return (CMD_FN_RET_OK);
}

/**
 * @brief    defaultTask will start TAG user application
 * app.mode will be mTAG
 * */
REG_FN(f_tag)
{
  xEventGroupSetBits(app.xStartTaskEvent, Ev_Tag_Task);
  return (CMD_FN_RET_OK);
}

/**
 * @brief   defaultTask will start USB2SPI user application
 * app.mode will be mUspi
 * */
REG_FN(f_uspi)
{
  xEventGroupSetBits(app.xStartTaskEvent, Ev_Usb2spi_Task);
  return(CMD_FN_RET_OK);
}

/**
 * @brief   defaultTask will start TCWM user application
 * app.mode will be mTcwm
 *
 * */
REG_FN(f_tcwm)
{
  xEventGroupSetBits(app.xStartTaskEvent, Ev_Tcwm_Task);
  return(CMD_FN_RET_OK);
}

/**
 * @brief   defaultTask will start TCFM user application
 * app.mode will be mTcfm
 * */
REG_FN(f_tcfm)
{
  char    cmd[12];
  int     n, nframes = 0, period = 0, nbytes = 0;

  n = sscanf(text, "%9s %d %d %d", cmd, &nframes, &period, &nbytes);

  switch (n)
  {
    case 1:
      nframes = 1000000;
      period = 1;
      nbytes = 20;
      break;
    case 2:
      period = 50;   // 50ms
      nbytes = 20;   // 20 bytes packet
      break;
    case 3:
      nbytes = 20;   // 20 bytes packet
      break;
    default:
      break;
  }

  app.tcfm_info.period_ms = period;
  app.tcfm_info.bytes = nbytes;
  app.tcfm_info.nframes = nframes;

  xEventGroupSetBits(app.xStartTaskEvent, Ev_Tcfm_Task);

  return(CMD_FN_RET_OK);
}

/**
 * @brief   defaultTask will start listener user application
 *
 * */
REG_FN(f_listen)
{
  char    cmd[12];
  int     mode = 0;

  sscanf(text, "%9s %d", cmd, &mode);

  app.listener_mode = mode;

  xEventGroupSetBits(app.xStartTaskEvent, Ev_Listener_Task);
  return(CMD_FN_RET_OK);
}

/**
 * @brief   defaultTask will start the Trilateration task for the Node
 * app.mode will be mTRILAT_N
 *
 * */
REG_FN(f_trilat_n)
{
  xEventGroupSetBits(app.xStartTaskEvent, Ev_Trilat_N_Task);
  return(CMD_FN_RET_OK);
}

/**
 * @brief    defaultTask will stop all working threads
 * app.mode will be mIDLE
 * */
REG_FN(f_stop)
{
  FlushTask_reset();
  port_tx_msg((uint8_t *)"\r\n", 2);
  xEventGroupSetBits(app.xStartTaskEvent, Ev_Stop_All);
  return (CMD_FN_RET_OK);
}

/**
 * @}
 */

/** @defgroup Application_Parameters
 * @brief Parameters change section : allowed only in app.mode = mIdle
 * @{
 */

/**
 * @brief set the 16bit (short) address
 * */
REG_FN(f_addr)
{
  pbss->s.addr = (uint16_t)(val);
  return (CMD_FN_RET_OK);
}
REG_FN(f_panid)
{
  pbss->s.panID = (uint16_t)(val);
  return (CMD_FN_RET_OK);
}
REG_FN(f_numSlots)
{
  pbss->s.sfConfig.numSlots = (uint16_t)(val);
  return (CMD_FN_RET_OK);
}
REG_FN(f_slotPeriod)
{
  pbss->s.sfConfig.slotPeriod = (uint16_t)(val);
  return (CMD_FN_RET_OK);
}
REG_FN(f_sfPeriod)
{
  pbss->s.sfConfig.sfPeriod_ms = (uint16_t)(val);
  return (CMD_FN_RET_OK);
}
REG_FN(f_tag_replyDly_us)
{
  pbss->s.sfConfig.tag_replyDly_us = (uint16_t)(val);
  return (CMD_FN_RET_OK);
}
REG_FN(f_tag_pollTxFinalTx_us)
{
  pbss->s.sfConfig.tag_pollTxFinalTx_us = (uint16_t)(val);
  return (CMD_FN_RET_OK);
}
REG_FN(f_ant_tx_a)
{
  pbss->s.antTx_a = (uint16_t)(val);
  return (CMD_FN_RET_OK);
}
REG_FN(f_ant_rx_a)
{
  pbss->s.antRx_a = (uint16_t)(val);
  return (CMD_FN_RET_OK);
}
REG_FN(f_ant_rx_b)
{
  pbss->s.antRx_b = (uint16_t)(val);
  return (CMD_FN_RET_OK);
}
REG_FN(f_pdoa_offset)
{
  pbss->s.pdoaOffset_deg = (int16_t)val;
  return (CMD_FN_RET_OK);
}
REG_FN(f_rng_offset)
{
  pbss->s.rngOffset_mm = (int16_t)val;
  return (CMD_FN_RET_OK);
}
REG_FN(f_phase_corr_enable)
{
  pbss->s.phaseCorrEn = (uint8_t)val;
  return (CMD_FN_RET_OK);
}
REG_FN(f_pdoa_temp_coeff)
{
  pbss->s.pdoa_temp_coeff_mrad = (int16_t)val;
  return (CMD_FN_RET_OK);
}
REG_FN(f_acc)
{
  pbss->s.accEn = (uint8_t)val;
  return (CMD_FN_RET_OK);
}
REG_FN(f_dbg)
{
  pbss->s.debugEn = (uint8_t)(val);
  return (CMD_FN_RET_OK);
}
REG_FN(f_diag)
{
  pbss->s.diagEn = (uint8_t)val;
  return (CMD_FN_RET_OK);
}
REG_FN(f_uart)
{
  if (pbss->s.uartEn && !val) {
    deca_uart_close();
  } else if (!pbss->s.uartEn && val) {
    deca_uart_init();
  }
  pbss->s.uartEn = (uint8_t)(val);
  return (CMD_FN_RET_OK);
}
REG_FN(f_twr_report)
{
  pbss->s.reportLevel = (uint8_t)val;
  return (CMD_FN_RET_OK);
}
REG_FN(f_rc_delay)
{
  pbss->s.rcDelay_us = (uint32_t)val;
  return (CMD_FN_RET_OK);
}
REG_FN(f_x)
{
  pbss->s.fixed_pos_x_mm = (int16_t)val;
  return (CMD_FN_RET_OK);
}
REG_FN(f_y)
{
  pbss->s.fixed_pos_y_mm = (int16_t)val;
  return (CMD_FN_RET_OK);
}
REG_FN(f_z)
{
  pbss->s.fixed_pos_z_mm = (int16_t)val;
  return (CMD_FN_RET_OK);
}
REG_FN(f_restore)
{
  CMD_ENTER_CRITICAL();
  restore_bssConfig();
  CMD_EXIT_CRITICAL();
  return (CMD_FN_RET_OK);
}

/**
 * @}
 */

/** @defgroup Various_commands
 * @brief    This commands to control the PDoA Node application;
 *           test commands;
 * @{
 */

/**
 * @brief add all discovered tags to knownTagList
 *        this is automatic function for manual adding:
 *        it assign low u16 from addr64 as a new addr16 for every tag
 *        uses tag_Mode, tag_mSlow, tag_mFast as default parameters
 */
REG_FN(f_add_all_to_list)
{
  const char  *ret = "All tags were added.";

  char *str = CMD_MALLOC(MAX_STR_SIZE);

  if (str) {
    tag_addr_slot_t    *tag;
    uint64_t        *pAddr64;
    uint16_t        size = getDList_size();

    /* below can be removed if the d2k command, i.e. "add all discovered tags to
     *   known list automatically", is not used */
    uint16_t    tag_mFast = 1;                  /**< Used to pass a "moving"
                                                 *   refresh rate to all tags in
                                                 *   the "d2k" command */
    uint16_t    tag_mSlow = 2;                  /**< Used to pass a "stationary"
                                                 *   refresh rate to all tags in
                                                 *   the "d2k" command */
    uint16_t    tag_Mode = 1;                   /**< Used to pass a common
                                                 *   "mode" parameter to all
                                                 *   tags in the "d2k" command
                                                 */

    pAddr64 = getDList();

    while ((*pAddr64 != 0) && (size > 0))
    {
      tag = add_tag_to_knownTagList(*pAddr64,
                                    (uint16_t)(*pAddr64),
                                    tag_mFast,
                                    tag_mSlow,
                                    tag_Mode);

      if (!tag) {
        sprintf(str, "Cannot add: list is full.");
        sprintf(&str[strlen(str)], "\r\n");
        port_tx_msg((uint8_t *)str, strlen(str));

        ret = (NULL);            // cannot add new tag, the knownTagList is
                                 //   full.
        break;
      } else {
        tag->reqUpdatePending = 1;            // enable update of Tag's
                                              //   configuration on its next
                                              //   Poll
      }

      size--;
      pAddr64++;
    }
    CMD_FREE(str);
  }
  initDList();      // clear Discovered Tag List

  return (const char *)(ret);
}

REG_FN(f_decaid)
{
  diag_printf("Decawave device ID = 0x%8lx\r\n", dwt_readdevid());
  diag_printf("Decawave lotID = 0x%8lx, partID = 0x%8lx\r\n",
              dwt_getlotid(),
              dwt_getpartid());
  return (CMD_FN_RET_OK);
}

REG_FN(f_led_glow)
{
  return(CMD_FN_RET_OK);
}

REG_FN(f_user_button)
{
#if !defined(HAL_IO_PORT_BUTTON) || !defined(HAL_IO_PIN_BUTTON)
  diag_printf("Button is not available!\r\n");
#else
  uint32_t timer = 0;
  start_timer(&timer);
  diag_printf("Press the user button within 10 seconds\r\n");
  while (1)
  {
    if (GPIO_PinInGet(HAL_IO_PORT_BUTTON,
                      HAL_IO_PIN_BUTTON) == HAL_IO_BUTTON_ACTIVE_STATE) {
      diag_printf("User Button pressed\r\n");
      break;
    }
    if (check_timer(timer, 15000)) {
      diag_printf("User Button not pressed\r\n");
      break;
    }
  }
#endif
  return(CMD_FN_RET_OK);
}

REG_FN(f_uart_test)
{
  const char test_string[] = "Hello testing UART";
  uint16_t str_length = strlen(test_string), i = 0;    // strlen(test_string);
  diag_printf("UART Test, check the UART terminal\r\n");

  // if (uart_enabled)
  // {
  //    return(CMD_FN_RET_ERR);
  // }
  while (str_length > 0)
  {
    app_uart_put(test_string[i]);
    str_length--;
    i++;
  }
  return(CMD_FN_RET_OK);
}

REG_FN(f_get_version)
{
  const char version[] = RTLS_APP_VERSION;
  diag_printf("VERSION:%s\r\n", version);
  return(CMD_FN_RET_OK);
}

REG_FN(f_lstat)
{
  char *str = CMD_MALLOC(MAX_STR_SIZE);

  if (str) {
    CMD_ENTER_CRITICAL();
    int  hlen;

    /** Listener RX Event Counts object */
    hlen = sprintf(str, "JS%04X", 0x5A5A);       // reserve space for length of
                                                 //   JS object
    sprintf(&str[strlen(str)], "{\"RX Events\":{\r\n");
    sprintf(&str[strlen(str)], "\"CRCG\":%d,\r\n", (int)app.event_counts.CRCG);
    sprintf(&str[strlen(str)], "\"CRCB\":%d,\r\n", (int)app.event_counts.CRCB);
    sprintf(&str[strlen(str)], "\"ARFE\":%d,\r\n", (int)app.event_counts.ARFE);
    sprintf(&str[strlen(str)], "\"PHE\":%d,\r\n", (int)app.event_counts.PHE);
    sprintf(&str[strlen(str)], "\"RSL\":%d,\r\n", (int)app.event_counts.RSL);
    sprintf(&str[strlen(str)], "\"SFDTO\":%d,\r\n",
            (int)app.event_counts.SFDTO);
    sprintf(&str[strlen(str)], "\"PTO\":%d,\r\n", (int)app.event_counts.PTO);
    sprintf(&str[strlen(str)], "\"FTO\":%d,\r\n", (int)app.event_counts.RTO);
    sprintf(&str[strlen(str)], "\"STSE\":%d,\r\n",
            (int)app.event_counts_sts_bad);
    sprintf(&str[strlen(str)],
            "\"STSG\":%d,\r\n",
            (int)app.event_counts_sts_good);
    sprintf(&str[strlen(str)], "\"SFDD\":%d}}",
            (int)app.event_counts_sfd_detect);
    sprintf(&str[2], "%04X", strlen(str) - hlen);// add formatted 4X of length,
                                                 //   this will erase first '{'
    str[hlen] = '{';                              // restore the start bracket
    sprintf(&str[strlen(str)], "\r\n");
    port_tx_msg((uint8_t *)str, strlen(str));

    CMD_FREE(str);

    memset((uint8_t *)&app.event_counts, 0, sizeof(app.event_counts));
    app.event_counts_sts_bad = 0;
    app.event_counts_sts_good = 0;
    app.event_counts_sfd_detect = 0;

    dwt_configeventcounters(1);     // we need and can clear and enable counters
                                    //   in the chip

    CMD_EXIT_CRITICAL();
  }
  return(CMD_FN_RET_OK);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// JSON format for TagList section

/** JSON reporting:
 *
 * Root elements:
 * "NewTag": string (addr64)           | on discovering of new Tag
 * "DList" : array of strings (addr64) | reply to getDlist
 * "KList" : array of tag objects      | reply to getKlist
 * "TagAdded" : tag object             | reply to Add2List
 * "TagDeleted" : string (addr64)      | reply to delTag
 * "TWR" : twr object                  | on calculation of new range
 * "SN"  : service object(accel)       | on receiving data from IMU sensor
 *
 *
 * */

/** @brief Discovered List
 *
 * 'JSxxxx{"DList": ["addr64_1","addr64_2","addr64_3"]}'
 *
 * */
REG_FN(f_get_discovered_list)
{
  uint64_t    *pAddr64;
  uint16_t     size;
  int          jlen, tmp;

  pAddr64 = getDList();
  size = getDList_size();

  char *str = CMD_MALLOC(MAX_STR_SIZE);

  if (str) {
    CMD_ENTER_CRITICAL();

    /**  11+2 - minimum JSON length
     *  tmp=16+2 bytes per discovered tag: address64 plus quotes
     *  ',\r\n'=3 bytes per separation, if more than 1 elements
     *  we need to pre-calculate the length of json string for DList & KList :
     *   in order to keep malloc() small
     */
    jlen = (11 + 2);
    if (size > 0) {
      tmp = 0;
      tmp = strlen("\"1122334455667788\"");          // 16+2 for every addr64
      jlen += (tmp + 3) * size - 3;
    }

    sprintf(str, "JS%04X", jlen);                      // print pre-calculated
                                                       //   length of JS object
    sprintf(&str[strlen(str)], "{\"DList\":[ ");       // +11 to json.
    port_tx_msg((uint8_t *)str, strlen(str));

    // DList cannot be with gaps
    // if changed, will need to change the calculation of jlen
    while (size > 0)
    {
      sprintf(str, "\"%08lX%08lX\"",
              (uint32_t)(*pAddr64 >> 32),
              (uint32_t)(*pAddr64));                    // +18 to json

      if (size > 1) {
        sprintf(&str[strlen(str)], ",\r\n");            // +3 to json
      }

      port_tx_msg((uint8_t *)str, strlen(str));

      pAddr64++;
      size--;
    }

    port_tx_msg((uint8_t *)"]}\r\n", 4);                // +2 to json

    CMD_EXIT_CRITICAL();

    initDList();                                        // clear the Discovered
                                                        //   list

    CMD_FREE(str);
  }

  return (CMD_FN_RET_OK);
}

/** @brief This function shall return fixed length tag object staring
 *         66 characters : do not change : see f_get_known_list
 * */
static void fill_json_tag(char *str, tag_addr_slot_t *tag)
{
  sprintf(str,
          "{\"slot\":\"%04X\",\"a64\":\"%08lX%08lX\",\"a16\":\"%04X\","
          "\"F\":\"%04X\",\"S\":\"%04X\",\"M\":\"%04X\"}",
          tag->slot,
          (uint32_t)(tag->addr64 >> 32),
          (uint32_t)(tag->addr64),
          tag->addr16,
          tag->multFast,
          tag->multSlow,
          tag->mode);
}

/** @brief Known List
 *
 *  'JSxxxx{"KList":
 *  [
 *  {
 *   "slot":<string>,   //hex
 *   "a64":<string>,    //address64, string
 *   "a16":<string>,    //address16, string
 *   "F":<string>,      //multFast, hex
 *   "S":<string>,      //multSlow, hex
 *   "M":<string>       //mode, hex
 *  },
 *  {
 *   "slot":<string>,   //hex
 *   "a64":<string>,    //address64, string
 *   "a16":<string>,    //address16, string
 *   "F":<string>,      //multFast, hex
 *   "S":<string>,      //multSlow, hex
 *   "M":<string>       //mode, hex
 *  }
 *  ]}'
 *
 */
REG_FN(f_get_known_list)
{
  tag_addr_slot_t    *tag;
  int                jlen, size, tmp;

  char *str = CMD_MALLOC(MAX_STR_SIZE);

  if (str) {
    CMD_ENTER_CRITICAL();

    /**  16 bytes overhead for JSON
     *  66 bytes per known tag
     *  3 bytes per separation
     *  we need to pre-calculate the length of json string for DList & KList :
     *   in order to keep malloc() small
     */
    tag = get_knownTagList();
    size = get_knownTagList_size();

    jlen = (10 + 2);

    if (size > 0) {
      tmp = 0;
      fill_json_tag(str, tag);
      tmp = strlen(str);                                  // +NN to json for
                                                          //   every known tag
      jlen += (tmp + 3) * size - 3;
    }

    sprintf(str, "JS%04X", jlen);                           // 6 print
                                                            //   pre-calculated
                                                            //   length of JS
                                                            //   object
    sprintf(&str[strlen(str)], "{\"KList\":[");             // 10
    port_tx_msg((uint8_t *)str, strlen(str));

    // KList can be with gaps, so need to scan it whole
    for (int i = 0; i < MAX_KNOWN_TAG_LIST_SIZE; i++)
    {
      if (tag->slot != (uint16_t)(0)) {
        fill_json_tag(str, tag);                            // NN

        if (size > 1) {         // last element should not have ',\r\n'
          sprintf(&str[strlen(str)], ",\r\n");              // 3
        }

        size--;

        port_tx_msg((uint8_t *)str, strlen(str));
      }

      tag++;
    }

    port_tx_msg((uint8_t *)"]}\r\n", 4);

    CMD_EXIT_CRITICAL();

    CMD_FREE(str);
  }

  return (CMD_FN_RET_OK);
}

/**
 * add tag from incoming command to knownTagList
 *
 * report:
 * 'JSxxxx{"TagAdded":
 *  {
 *   "slot":1,
 *   "a64":<string>,//address64, string
 *   "a16":<string>,//address16, string
 *   "F":<int>,        //multFast, int
 *   "S":<int>,        //multSlow, int
 *   "M":<int>        //mode, int
 *  }
 * }'
 *
 * Note: sscanf needs at least 212 bytes from stack
 *          and it uses malloc but not RTOS' malloc
 *
 * */
REG_FN(f_add_tag_to_list)
{
  const char *ret = NULL;

  char *str = CMD_MALLOC(MAX_STR_SIZE);

  if (str) {
    tag_addr_slot_t    *tag;
    uint64_t           addr64 = 0;
    unsigned int       addr1 = 0, addr2 = 0;
    unsigned int       addr16 = 0, multFast = 0, multSlow = 0, mode = 0, n = 1,
                       hlen;
    char               tmp[10];

    /** "addtag 11AABB4455FF7788 10AA 1 2 1" */
    n = sscanf(text, "%8s %08x%08x %x %x %x %x",
               tmp, &addr1, &addr2, &addr16, &multFast, &multSlow, &mode);

    if (!(multFast == 0) && !(multSlow == 0) && (n == 7)) {
      addr64 = (uint64_t)((((uint64_t)addr1) << 32) | addr2);

      tag = add_tag_to_knownTagList(addr64, (uint16_t)addr16, 1, 1, 1);

      if (tag) {
        tag->multFast = (uint16_t)multFast;
        tag->multSlow = (uint16_t)multSlow;
        tag->mode = (uint16_t)mode;
        tag->reqUpdatePending = 1;                        // update Tag's
                                                          //   configuration on
                                                          //   its next Poll

        hlen = sprintf(str, "JS%04X", 0x5A5A);           // reserve space for
                                                         //   length of JS
                                                         //   object
        sprintf(&str[strlen(str)], "{\"TagAdded\": ");

        fill_json_tag(&str[strlen(str)], tag);

        sprintf(&str[strlen(str)], "}");        // \r\n

        sprintf(&str[2], "%04X", strlen(str) - hlen);    // add formatted 4X of
                                                         //   length, this will
                                                         //   kill first '{'
        str[hlen] = '{';                                  // restore the start
                                                          //   bracket

        sprintf(&str[strlen(str)], "\r\n");
        port_tx_msg((uint8_t *)str, strlen(str));

        ret = CMD_FN_RET_OK;
      }
    }

    CMD_FREE(str);
  }

  return (ret);
}

/**
 * @brief delete the tag addr64 from knownTagList
 *          the function will always report the tag was deleted.
 * report:
 * 'JSxxxx{"TagDeleted":
 *             <string> //address64, string
 *        }'
 *
 * */
REG_FN(f_del_tag_from_list)
{
  const char *ret = NULL;

  char *str = CMD_MALLOC(MAX_STR_SIZE);

  if (str) {
    uint64_t        addr64 = 0;
    unsigned int    addr1 = 0, addr2 = 0, hlen;
    char            tmp[10];

    /** "delTag 11AABB4455FF7788" */
    sscanf(text, "%8s %08x%08x", tmp, &addr1, &addr2);

    addr64 = (uint64_t)((((uint64_t)addr1) << 32) | (uint64_t)addr2);
    if (addr64 > 0xFFFF) {
      del_tag64_from_knownTagList(addr64);
    } else {
      del_tag16_from_knownTagList((uint16_t)addr64);
    }

    hlen = sprintf(str, "JS%04X", 0x5A5A);       // reserve space for length of
                                                 //   JS object
    sprintf(&str[strlen(str)], "{\"TagDeleted\": \"%08x%08x\"}", addr1, addr2);

    sprintf(&str[2], "%04X", strlen(str) - hlen);// add formatted 4X of length,
                                                 //   this will erase first '{'
    str[hlen] = '{';                              // restore the start bracket
    sprintf(&str[strlen(str)], "\r\n");
    port_tx_msg((uint8_t *)str, strlen(str));

    ret = CMD_FN_RET_OK;

    CMD_FREE(str);
  }

  return (const char *)(ret);
}

/** @brief
 * */
REG_FN(f_decaJuniper)
{
  const char *ret = NULL;
  const char ver[] = FULL_VERSION;

  char *str = CMD_MALLOC(MAX_STR_SIZE);

  if (str) {
    int  hlen;

    hlen = sprintf(str, "JS%04X", 0x5A5A);       // reserve space for length of
                                                 //   JS object

    sprintf(&str[strlen(str)], "{\"Info\":{\r\n");
    sprintf(&str[strlen(str)], "\"Device\":\"Juniper ");
    sprintf(&str[strlen(str)], "%s\",\r\n",
            (app.mode == mIDLE)?("STOP")
            :(app.mode == mPNODE)?("PDoA NODE")
            :(app.mode == mPTAG)?("PDoA TAG")
            :(app.mode == mTCWM)?("TCWM")
            :(app.mode == mTCFM)?("TCFM")
            :(app.mode == mUSB2SPI)?("USB2SPI")
            :(app.mode == mLISTENER)?(LISTENER_FUNC)
            :("unknown"));
    sprintf(&str[strlen(str)], "\"Version\":\"%s\",\r\n", ver);
    sprintf(&str[strlen(str)], "\"Build\":\"%s %s\",\r\n", __DATE__, __TIME__);

    sprintf(&str[strlen(str)],
            "\"Apps\":[\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"],\r\n",
            NODE_FUNC,
            TAG_FUNC,
            TRILAT_FUNC,
            USPI_FUNC,
            TCWM_FUNC,
            TCFM_FUNC,
            LISTENER_FUNC);

    sprintf(&str[strlen(str)], "\"Driver\":\"%s\"}}", dwt_version_string());

    sprintf(&str[2], "%04X", strlen(str) - hlen);   // add formatted 4X of
                                                    //   length, this will erase
                                                    //   first '{'
    str[hlen] = '{';                                // restore the start bracket
    sprintf(&str[strlen(str)], "\r\n");
    port_tx_msg((uint8_t *)str, strlen(str));

    CMD_FREE(str);
    ret = CMD_FN_RET_OK;
  }

  return (ret);
}

// -----------------------------------------------------------------------------

/**
 * @brief   show current mode of operation,
 *          version, and the configuration in JSON format
 *          Should be executed from STOP as
 *
 * */
REG_FN(f_jstat)
{
  const char *ret = NULL;

  char *str = CMD_MALLOC(MAX_STR_SIZE);

  if (str) {
    CMD_ENTER_CRITICAL();

    int  hlen;

    /** System Config object */
    hlen = sprintf(str, "JS%04X", 0x5A5A);       // reserve space for length of
                                                 //   JS object
    sprintf(&str[strlen(str)], "{\"System Config\":{\r\n");
    sprintf(&str[strlen(str)],
            "\"ADDR\":\"%04X\",\r\n",
            (unsigned int)pbss->s.addr);
    sprintf(&str[strlen(str)],
            "\"PANID\":\"%04X\",\r\n",
            (unsigned int)pbss->s.panID);
    sprintf(&str[strlen(str)],
            "\"NUMSLOT\":%d,\r\n",
            (unsigned int)pbss->s.sfConfig.numSlots);
    sprintf(&str[strlen(str)],
            "\"SLOTPER\":%d,\r\n",
            (unsigned int)pbss->s.sfConfig.slotPeriod);
    sprintf(&str[strlen(str)],
            "\"SFPER\":%d,\r\n",
            (unsigned int)pbss->s.sfConfig.sfPeriod_ms);
    sprintf(&str[strlen(str)],
            "\"REPDEL\":%d,\r\n",
            (unsigned int)pbss->s.sfConfig.tag_replyDly_us);
    sprintf(&str[strlen(str)],
            "\"P2FDEL\":%d,\r\n",
            (unsigned int)pbss->s.sfConfig.tag_pollTxFinalTx_us);
    sprintf(&str[strlen(str)],
            "\"RCDEL\":%d}}",
            (unsigned int)pbss->s.rcDelay_us);

    sprintf(&str[2], "%04X", strlen(str) - hlen);// add formatted 4X of length,
                                                 //   this will erase first '{'
    str[hlen] = '{';                              // restore the start bracket
    sprintf(&str[strlen(str)], "\r\n");
    port_tx_msg((uint8_t *)str, strlen(str));

    /** Run Time object */
    hlen = sprintf(str, "JS%04X", 0x5A5A);       // reserve space for length of
                                                 //   JS object
    sprintf(&str[strlen(str)], "{\"Run Time\":{\r\n");
    sprintf(&str[strlen(str)], "\"UART\":%d,\r\n", pbss->s.uartEn);
    sprintf(&str[strlen(str)], "\"PCREP\":%d}}", pbss->s.reportLevel);
    sprintf(&str[2], "%04X", strlen(str) - hlen);// add formatted 4X of length,
                                                 //   this will erase first '{'
    str[hlen] = '{';                              // restore the start bracket
    sprintf(&str[strlen(str)], "\r\n");
    port_tx_msg((uint8_t *)str, strlen(str));

    /** Calibration object */
    hlen = sprintf(str, "JS%04X", 0x5A5A);       // reserve space for length of
                                                 //   JS object
    sprintf(&str[strlen(str)], "{\"Calibration\":{\r\n");
    sprintf(&str[strlen(str)], "\"X_MM\":%d,\r\n", (int)pbss->s.fixed_pos_x_mm);
    sprintf(&str[strlen(str)], "\"Y_MM\":%d,\r\n", (int)pbss->s.fixed_pos_y_mm);
    sprintf(&str[strlen(str)], "\"Z_MM\":%d,\r\n", (int)pbss->s.fixed_pos_z_mm);
    sprintf(&str[strlen(str)], "\"ANTTXA\":%d,\r\n", (int)pbss->s.antTx_a);
    sprintf(&str[strlen(str)], "\"ANTRXA\":%d,\r\n", (int)pbss->s.antRx_a);
    sprintf(&str[strlen(str)], "\"ANTRXB\":%d,\r\n", (int)pbss->s.antRx_b);
    sprintf(&str[strlen(str)],
            "\"PDOAOFF\":%d,\r\n",
            (int)pbss->s.pdoaOffset_deg);
    sprintf(&str[strlen(str)], "\"RNGOFF\":%d}}", (int)pbss->s.rngOffset_mm);
    sprintf(&str[2], "%04X", strlen(str) - hlen);// add formatted 4X of length,
                                                 //   this will erase first '{'
    str[hlen] = '{';                              // restore the start bracket
    sprintf(&str[strlen(str)], "\r\n");
    port_tx_msg((uint8_t *)str, strlen(str));

    CMD_FREE(str);

    CMD_EXIT_CRITICAL();

    ret = CMD_FN_RET_OK;
  }
  return (ret);
}

/**
 * @brief set or show current Key & IV parameters in JSON format
 * @param no param - show current Key & IV
 *        correct scanned string - set the params and then show them
 *        incorrect scanned string - error
 *
 * */
REG_FN(f_power)
{
  const char *ret = CMD_FN_RET_OK;

  char *str = CMD_MALLOC(MAX_STR_SIZE);

  if (str) {
    /* Display the Key Config */
    int  n, hlen;

    unsigned int pwr, pgDly;
    int pgCnt;

    n = sscanf(text, "%9s 0X%08x 0X%08x 0X%08x", str, &pwr, &pgDly, &pgCnt);

    if (n == 4) {
      pbss->s.txConfig.power = pwr;
      pbss->s.txConfig.PGdly = pgDly;
      pbss->s.txConfig.PGcount = pgCnt;
    } else if (n != 1) {
      ret = NULL;
    }

    hlen = sprintf(str, "JS%04X", 0x5A5A);       // reserve space for length of
                                                 //   JS object
    sprintf(&str[strlen(str)], "{\"TX POWER\":{\r\n");

    sprintf(&str[strlen(str)],
            "\"PWR\":\"0x%08X\",\r\n",
            (unsigned int)pbss->s.txConfig.power);
    sprintf(&str[strlen(str)],
            "\"PGDLY\":\"0x%08X\",\r\n",
            (unsigned int)pbss->s.txConfig.PGdly);
    sprintf(&str[strlen(str)],
            "\"PGCOUNT\":\"0x%08X\"}}",
            (unsigned int)pbss->s.txConfig.PGcount);

    sprintf(&str[2], "%04X", strlen(str) - hlen);   // add formatted 4X of
                                                    //   length, this will erase
                                                    //   first '{'
    str[hlen] = '{';                                // restore the start bracket
    sprintf(&str[strlen(str)], "\r\n");
    port_tx_msg((uint8_t *)str, strlen(str));

    CMD_FREE(str);
  }
  return (ret);
}

/**
 * @brief set or show current Key & IV parameters in JSON format
 * @param no param - show current Key & IV
 *        correct scanned string - set the params and then show them
 *        incorrect scanned string - error
 *
 * */
REG_FN(f_stskeyiv)
{
  const char *ret = CMD_FN_RET_OK;

  char *str = CMD_MALLOC(MAX_STR_SIZE);

  if (str) {
    /* Display the Key Config */
    int  hlen, n;

    unsigned int key0, key1, key2, key3;
    unsigned int iv0, iv1, iv2, iv3;
    unsigned int sMode;

    n = sscanf(text,
               "%9s 0X%08x%08x%08x%08x 0X%08x%08x%08x%08x %d",
               str,
               &key3,
               &key2,
               &key1,
               &key0,
               &iv3,
               &iv2,
               &iv1,
               &iv0,
               &sMode);

    if ((n == 9) || (n == 10)) {
      pbss->s.stsStatic = sMode;

      pbss->s.stsIv.iv0 = iv0;
      pbss->s.stsIv.iv1 = iv1;
      pbss->s.stsIv.iv2 = iv2;
      pbss->s.stsIv.iv3 = iv3;

      pbss->s.stsKey.key0 = key0;
      pbss->s.stsKey.key1 = key1;
      pbss->s.stsKey.key2 = key2;
      pbss->s.stsKey.key3 = key3;
    } else if (n != 1) {
      ret = NULL;
    }

    hlen = sprintf(str, "JS%04X", 0x5A5A);       // reserve space for length of
                                                 //   JS object
    sprintf(&str[strlen(str)], "{\"STS KEY_IV\":{\r\n");

    sprintf(&str[strlen(str)], "\"STS KEY\":\"0x%08X%08X%08X%08X\",\r\n",
            (unsigned int)pbss->s.stsKey.key3,
            (unsigned int)pbss->s.stsKey.key2,
            (unsigned int)pbss->s.stsKey.key1,
            (unsigned int)pbss->s.stsKey.key0);
    sprintf(&str[strlen(str)], "\"STS IV\":\"0x%08X%08X%08X%08X\",\r\n",
            (unsigned int)pbss->s.stsIv.iv3,
            (unsigned int)pbss->s.stsIv.iv2,
            (unsigned int)pbss->s.stsIv.iv1,
            (unsigned int)pbss->s.stsIv.iv0);
    sprintf(&str[strlen(str)], "\"STS_STATIC\":\"%d\"}}",
            (int)pbss->s.stsStatic);

    sprintf(&str[2], "%04X", strlen(str) - hlen);   // add formatted 4X of
                                                    //   length, this will erase
                                                    //   first '{'
    str[hlen] = '{';                                // restore the start bracket
    sprintf(&str[strlen(str)], "\r\n");
    port_tx_msg((uint8_t *)str, strlen(str));

    CMD_FREE(str);
  }
  return (ret);
}

/**
 * @brief set or show current XTAL TRIM in JSON format
 * @param no param - show current Xtal Trim code
 *        correct scanned string - set the Trim code
 *        incorrect scanned string - do not set XTAL TRIM
 *
 * */
REG_FN(f_xtal_trim)
{
  const char *ret = CMD_FN_RET_OK;

  char *str = CMD_MALLOC(MAX_STR_SIZE);

  int n, xtalTrim;

  if (str) {
    n = sscanf(text, "%9s 0X%02x", str, &xtalTrim);

    CMD_ENTER_CRITICAL();

    if (n == 2) {
      dwt_setxtaltrim((uint8_t)xtalTrim & 0x7F);
    } else {
      dwt_setxtaltrim(app.pConfig->s.xtalTrim & XTAL_TRIM_BIT_MASK);
      xtalTrim = dwt_getxtaltrim()
                 | (app.pConfig->s.xtalTrim & ~XTAL_TRIM_BIT_MASK);
    }

    app.pConfig->s.xtalTrim = xtalTrim;     // it can have the 0x80 bit set to
                                            //   be able overwrite OTP values
                                            //   during APP starts.

    CMD_EXIT_CRITICAL();

    /* Display the XTAL object */
    int  hlen;

    hlen = sprintf(str, "JS%04X", 0x5A5A);       // reserve space for length of
                                                 //   JS object
    sprintf(&str[strlen(str)], "{\"XTAL\":{\r\n");
    sprintf(&str[strlen(str)], "\"TEMP TRIM\":\"0x%02x\"}}", xtalTrim);

    sprintf(&str[2], "%04X", strlen(str) - hlen);// add formatted 4X of length,
                                                 //   this will erase first '{'
    str[hlen] = '{';                              // restore the start bracket
    sprintf(&str[strlen(str)], "\r\n");
    port_tx_msg((uint8_t *)str, strlen(str));

    CMD_FREE(str);
  }

  return (ret);
}

/**
 * @brief   set or show current UWB parameters in JSON format
 * @param if cmd "UWBCFG" has no params, then show the current UWB config
 *        if it has correctly scanned string - set the UWB config and then show
 *   them
 *        if it has incorrect scanned string - indicate an error (however no
 *   error check would
 *        be performed, so all incorrect parameters would be set to UWB
 *   settings)
 *
 * */
REG_FN(f_uwbcfg)
{
  const char *ret = CMD_FN_RET_OK;

  char *str = CMD_MALLOC(MAX_STR_SIZE);

  int n;
  int chan;         //!< Channel number (5 or 9)
  int txPreambLength;   //!< DWT_PLEN_64..DWT_PLEN_4096
  int rxPAC;        //!< Acquisition Chunk Size (Relates to RX preamble length)
  int txCode;       //!< TX preamble code (the code configures the PRF, e.g. 9
                    //!<   -> PRF of 64 MHz)
  int rxCode;       //!< RX preamble code (the code configures the PRF, e.g. 9
                    //!<   -> PRF of 64 MHz)
  int sfdType;      //!< SFD type (0 for short IEEE 8-bit standard, 1 for DW
                    //!<   8-bit, 2 for DW 16-bit, 3 for 4z BPRF)
  int dataRate;     //!< Data rate {DWT_BR_850K or DWT_BR_6M8}
  int phrMode;      //!< PHR mode {0x0 - standard DWT_PHRMODE_STD, 0x3 -
                    //!<   extended frames DWT_PHRMODE_EXT}
  int phrRate;      //!< PHR rate {0x0 - standard DWT_PHRRATE_STD, 0x1 - at
                    //!<   datarate DWT_PHRRATE_DTA}
  int sfdTO;        //!< SFD timeout value (in symbols)
  int stsMode;      //!< STS mode (no STS, STS before PHR or STS after data)
  int stsLength;    //!< STS length (the allowed values are listed in
                    //!<   dwt_sts_lengths_e
  int pdoaMode;     //!< PDOA mode

  if (str) {
    n = sscanf(text,
               "%9s %d %d %d %d %d %d %d %d %d %d %d %d %d",
               str,
               &chan,
               &txPreambLength,
               &rxPAC,
               &txCode,
               &rxCode,
               &sfdType,
               &dataRate,
               &phrMode,
               &phrRate,
               &sfdTO,
               &stsMode,
               &stsLength,
               &pdoaMode);
    if (n == 14) {// set parameters :: this is unsafe, TODO :: add a range check
      pbss->dwt_config.chan = chan_to_deca(chan);
      pbss->dwt_config.txPreambLength = plen_to_deca(txPreambLength);
      pbss->dwt_config.rxPAC = pac_to_deca(rxPAC);
      pbss->dwt_config.dataRate = bitrate_to_deca(dataRate);
      pbss->dwt_config.stsLength = sts_length_to_deca(stsLength);

      pbss->dwt_config.txCode = txCode;
      pbss->dwt_config.rxCode = rxCode;
      pbss->dwt_config.sfdType = sfdType;
      pbss->dwt_config.phrMode = phrMode;
      pbss->dwt_config.phrRate = phrRate;
      pbss->dwt_config.sfdTO = sfdTO;
      pbss->dwt_config.stsMode = stsMode;
      pbss->dwt_config.pdoaMode = pdoaMode;
    } else if (n != 1) {
      ret = NULL;       // produce an error
    }

    /* Display the UWB Config object */
    int  hlen;

    hlen = sprintf(str, "JS%04X", 0x5A5A);       // reserve space for length of
                                                 //   JS object
    sprintf(&str[strlen(str)], "{\"UWB PARAM\":{\r\n");

    sprintf(&str[strlen(str)], "\"CHAN\":%d,\r\n",
            deca_to_chan(pbss->dwt_config.chan));
    sprintf(&str[strlen(str)], "\"PLEN\":%d,\r\n",
            deca_to_plen(pbss->dwt_config.txPreambLength));
    sprintf(&str[strlen(str)], "\"PAC\":%d,\r\n",
            deca_to_pac(pbss->dwt_config.rxPAC));
    sprintf(&str[strlen(str)], "\"TXCODE\":%d,\r\n", pbss->dwt_config.txCode);
    sprintf(&str[strlen(str)], "\"RXCODE\":%d,\r\n", pbss->dwt_config.rxCode);
    sprintf(&str[strlen(str)], "\"SFDTYPE\":%d,\r\n", pbss->dwt_config.sfdType);
    sprintf(&str[strlen(str)], "\"DATARATE\":%d,\r\n",
            deca_to_bitrate(pbss->dwt_config.dataRate));
    sprintf(&str[strlen(str)], "\"PHRMODE\":%d,\r\n", pbss->dwt_config.phrMode);
    sprintf(&str[strlen(str)], "\"PHRRATE\":%d,\r\n", pbss->dwt_config.phrRate);
    sprintf(&str[strlen(str)], "\"SFDTO\":%d,\r\n", pbss->dwt_config.sfdTO);
    sprintf(&str[strlen(str)], "\"STSMODE\":%d,\r\n", pbss->dwt_config.stsMode);
    sprintf(&str[strlen(str)], "\"STSLEN\":%d,\r\n",
            deca_to_sts_length(pbss->dwt_config.stsLength));
    sprintf(&str[strlen(str)], "\"PDOAMODE\":%d}}", pbss->dwt_config.pdoaMode);

    sprintf(&str[2], "%04X", strlen(str) - hlen);// add formatted 4X of length,
                                                 //   this will erase first '{'
    str[hlen] = '{';                              // restore the start bracket
    sprintf(&str[strlen(str)], "\r\n");
    port_tx_msg((uint8_t *)str, strlen(str));

    CMD_FREE(str);
  }
  return (ret);
}

/**
 * @brief show current mode of operation,
 *           version, and the configuration
 *
 * */
REG_FN(f_stat)
{
  const char *ret = CMD_FN_RET_OK;

  char *str = CMD_MALLOC(MAX_STR_SIZE);

  if (str) {
    sprintf(str, "MODE: %s\r\n"
                 "LAST ERR CODE: %d\r\n"
                 "MAX MSG LEN: %d\r\n",
            (app.mode == mIDLE)?("STOP")
            :(app.mode == mPNODE)?("PDoA NODE")
            :(app.mode == mPTAG)?("PDoA TAG")
            :(app.mode == mTCWM)?("TCWM")
            :(app.mode == mTCFM)?("TCFM")
            :(app.mode == mUSB2SPI)?("USB2SPI")
            :(app.mode == mLISTENER)?("LISTENER")
            :(app.mode == mTRILAT_N)?("TRILAT")
            :("UNKNOWN"),
            app.lastErrorCode,
            app.maxMsgLen);

    port_tx_msg((uint8_t *)str, strlen(str));

    CMD_FREE(str);
    app.lastErrorCode = 0;
    app.maxMsgLen = 0;

    f_decaJuniper(NULL, pbss, 0, NULL);
    f_jstat(NULL, pbss, 0, NULL);
    f_get_discovered_list(NULL, NULL, 0, NULL);
    f_get_known_list(NULL, NULL, 0, NULL);
  }

  ret = CMD_FN_RET_OK;
  return (ret);
}

/**
 * @brief Show all available commands
 *
 * */
REG_FN(f_help_std)
{
  int        indx = 0, cnt = 0;
  const char *ret = NULL;
  char *str = CMD_MALLOC(MAX_STR_SIZE);

  if (str) {
    CMD_ENTER_CRITICAL();

    while (known_commands[indx].cmnt != NULL)
    {
      uint32_t mode = known_commands[indx].mode;

      if (((mode & mMASK) == app.mode) || ((mode & mMASK) == mANY)
          || (mIDLE == app.mode)) {
        switch (mode & mCmdGrpMASK)
        {
          case mCmdGrp0:
            if (cnt > 0) {
              sprintf(&str[cnt], "\r\n");
              port_tx_msg((uint8_t *)str, strlen(str));
              cnt = 0;
            }

            /*print the Group name */
            sprintf(str, "---- %s---\r\n", known_commands[indx].cmnt);
            port_tx_msg((uint8_t *)str, strlen(str));
            break;

          case mCmdGrp1:
            /* print appropriate list of parameters for the current application
             */
            if (known_commands[indx].name) {
              sprintf(&str[cnt], "%-10s", known_commands[indx].name);
              cnt += CMD_COLUMN_WIDTH;
              if (cnt >= CMD_COLUMN_WIDTH * CMD_COLUMN_MAX) {
                sprintf(&str[cnt], "\r\n");
                port_tx_msg((uint8_t *)str, strlen(str));
                cnt = 0;
              }
            }
            break;

          case mCmdGrp2:
          case mCmdGrp3:
          case mCmdGrp4:
          case mCmdGrp5:
          case mCmdGrp6:
          case mCmdGrp7:
          case mCmdGrp8:
          case mCmdGrp9:
          case mCmdGrp10:
          case mCmdGrp11:
          case mCmdGrp12:
          case mCmdGrp13:
          case mCmdGrp14:
          /*reserved for the future*/
          default:
            break;
        }
      }

      indx++;
    }

    sprintf(&str[cnt], "\r\n");
    port_tx_msg((uint8_t *)str, strlen(str));

    CMD_EXIT_CRITICAL();

    CMD_FREE(str);
    ret = CMD_FN_RET_OK;
  }

  return (ret);
}

/*
 * @brief This fn() displays the help information to the function,
 *        i.e. comment field.
 *        usage: "help help", "help tag" etc
 *
 * */
REG_FN(f_help_help)
{
  int        indx = 0, n = 0;
  const char *ret = NULL;
  char *str = CMD_MALLOC(MAX_STR_SIZE);

  if (str) {
    CMD_ENTER_CRITICAL();

    while (known_commands[indx].cmnt != NULL)
    {
      uint32_t mode = known_commands[indx].mode;

      if (((mode & mMASK) == app.mode) || ((mode & mMASK) == mANY)
          || (mIDLE == app.mode)) {
        if (strcmp(known_commands[indx].name, text) == 0) {
          sprintf(str, "\r\n%s:\r\n", text);
          port_tx_msg((uint8_t *)str, strlen(str));

          const char *ptr = known_commands[indx].cmnt;

          while (ptr)
          {
            // CMD_COLUMN_WIDTH*CMD_COLUMN_MAX
            n = snprintf(str, 78, "%s", ptr);
            sprintf(&str[strlen(str)], "\r\n");
            port_tx_msg((uint8_t *)str, strlen(str));
            if (n < 78) {
              ptr = NULL;
            } else {
              ptr += 77;
            }
          }

          break;
        }
      }

      indx++;
    }

    sprintf(str, "\r\n");
    port_tx_msg((uint8_t *)str, strlen(str));

    CMD_EXIT_CRITICAL();

    CMD_FREE(str);
    ret = CMD_FN_RET_OK;
  }

  return ret;
}

/**
 * @brief Show all available commands
 *
 * */
REG_FN(f_help_app)
{
  char    help[12];
  char    cmd[12];
  int     n;
  const char *ret = NULL;

  n = sscanf(text, "%9s %10s", cmd, help);

  switch (n)
  {
    case 1:
      ret = f_help_std(cmd, pbss, val, params);
      break;
    case 2:
      if (help[0] == 0) {
        ret = f_help_help(&help[1], pbss, val, params);
      } else {
        ret = f_help_help(&help[0], pbss, val, params);
      }
      break;
    default:
      break;
  }

  return (ret);
}

// -----------------------------------------------------------------------------
// Communication change section

/**
 * @brief save configuration
 *
 * */
REG_FN(f_save)
{
  error_e    err_code;

  CMD_ENTER_CRITICAL();

  switch (app.mode)
  {
    case mPNODE:
      app.pConfig->s.default_event = Ev_Node_Task;
      break;
    case mPTAG:
      app.pConfig->s.default_event = Ev_Tag_Task;
      break;
    case mTRILAT_N:
      app.pConfig->s.default_event = Ev_Trilat_N_Task;
      break;
    default:
      app.pConfig->s.default_event = 0;   // default app is defined in the
                                          //   main();
      break;
  }

  err_code = save_bssConfig(pbss);

  CMD_EXIT_CRITICAL();

  if (err_code != _NO_ERR) {
    error_handler(0, err_code);        // not a fatal error
    return (NULL);
  }

  return (CMD_FN_RET_OK);
}

/**
 * @}
 */

// -----------------------------------------------------------------------------

/** end f_xx command functions */

const char COMMENT_PDOA_NODE_OPT[] = { "PDoA Node Options ----" };
const char COMMENT_PDOA_NODE_CMD[] = { "PDoA Node commands ---" };
const char COMMENT_PDOA_TAG_OPT[] = { "PDoA Tag Options -----" };
const char COMMENT_LISTENER_OPT[] = { "LISTENER Options -----" };
const char COMMENT_ANYTIME_OPTIONS[] = { "Anytime commands -----" };
const char COMMENT_APPSELECTION[] = { "Application selection " };
const char COMMENT_SERVICE[] = { "Service commands -----" };

const char STD_CMD_COMMENT[] =
{ "This command described in the documentation" };

const char COMMENT_RCDEL[] =
{
  "Time, \'us\', between end of Tag's Blink_TX and Tag start Rx of Ranging Config message. From Node's view this is a delay between end of reception of Blink's data and start of transmission of preamble of the Ranging Config message. This time should be set to the same value on the Tag and on the Node."
};
const char COMMENT_REPDEL[] =
{
  "Time, \'us\', when Tag shall be ready to receive the Node's Response after end of transmission of the Poll (wait4response). This parameter is defined by the Node and sent to the Tag in the Ranging Config message."
};
const char COMMENT_P2FDEL[] =
{
  "Time, \'us\',  when Tag shall transmit Final's RMARKER, calculated from Tag's Poll's RMARKER. This parameter is defined by the Node and sent to the Tag in the Ranging Config message."
};

const char COMMENT_STOP[] = { "Stops running any top-level applications" };
const char COMMENT_STAT[] = { "Displays the Status information" };
const char COMMENT_SAVE[] = { "Saves the configuration to the NVM" };
const char COMMENT_DECAJUNIPER[] =
{ "This command reports the running application and the version information" };
const char COMMENT_HELP[] =
{
  "This command displays the help information. Usage: \"help\" or \"help <CMD>\", <CMD> is the command from the list, i.e. \"help tag\"."
};

const char COMMENT_NODE[] = { "Slotted DS-TWR PDoA Node application." };
const char COMMENT_TAG[] = { "Slotted DS-TWR Tag application." };
const char COMMENT_TRILAT[] =
{
  "Trilateration application runs on top of the Node application. It requires at least 3 Tags with configured and placed to X-Y-Z corners of the room to demonstarte the Trilateration Location Solver."
};
const char COMMENT_USPI[] =
{
  "Starts the USB2SPI mode. This is a special binary protocol to read/write registers of DW chip using external application. DecaRanging application prior to 3.24 require this mode to be started manually."
};
const char COMMENT_TCWM[] =
{
  "Test Continuous Wave mode generates the base frequency of the corresponded channel on the RF port."
};
const char COMMENT_TCFM[] =
{
  "Test Continuous Frame mode is to transmit packets for test purposes. Usage: \"tcfm <NUM> <PAUSE> <LEN>\" <NUM>>: number of packets to transmit. <PAUSE>: pause in between packets in ms. <LEN>: length of the transmit payload, bytes\r\n"
};
const char COMMENT_LISTENER[] =
{
  "Listen for the UWB packjets using the UWB configuration.\r\nUsage: \"Listener <PARM>\" <PARM>: if present or 0, this is priority of speed. In this mode Listener will output maximum six first bytes from the input string. With <PARM> set to 1, the priority is on data and listener will output maximum 127 bytes of a payload."
};

const char COMMENT_LSTAT[] =
{ "Displays the statistics inside the Listener application." };

const char COMMENT_X[] =
{
  "X-coordinate of this Tag in the room, mm. Used only in the tag, which is a part of a Trilateration example running on the fift unit."
};
const char COMMENT_Y[] =
{
  "Y-coordinate of this Tag in the room, mm. Used only in the tag, which is a part of a Trilateration example running on the fift unit."
};
const char COMMENT_Z[] =
{
  "Z-coordinate of this Tag in the room, mm. Used only in the tag, which is a part of a Trilateration example running on the fift unit."
};

const char COMMENT_ADDR[] =
{ "Sets the 16-bit address to the Node.\r\nUsage: \" \"" };
const char COMMENT_PANID[] =
{
  "Sets the 16-bit PanID for the Node.Node will use this PANID to setup the Tag. \r\nUsage: \" \""
};
const char COMMENT_NUMSLOT[] =
{
  "Sets the number of Slots used for Ranging. \r\nUsage: \"numslot 20\". Cannot be smaller than MAX_KNOWN_TAG_LIST_SIZE"
};
const char COMMENT_SLOTPER[] =
{ "Sets the slot period in ms.\r\nUsage: \"slotper 5\"" };
const char COMMENT_SFPER[] =
{
  "Sets the superframe period in ms.\r\nUsage: \"sfper 100\". Cannot be smaller than NUM_SLOTS*SLOT_PERIOD"
};

const char COMMENT_JSTAT[] = { "Status in JSON format" };
const char COMMENT_GETDLIST[] =
{ "Discovered Tags's <UI64_HEX> list in JSON format." };
const char COMMENT_GETKLIST[] = { "Known Tags list in JSON format." };
const char COMMENT_ADDTAG[] =
{
  "Add the tag <UI64_HEX> to Known list using specified parameters.\r\nUsage: \"addtag <UI64_HEX> <NEW_UI16_HEX> <SLOW> <FAST> <MODE>\""
};
const char COMMENT_DELTAG[] =
{
  "Delete tag <UI16/64_HEX> from Known Tags list\r\nUsage: \"deltag <UI16/64_HEX>\""
};
const char COMMENT_PDOAOFF[] =
{ "Phase Difference offset for this Node\r\nUsage: \" \"" };
const char COMMENT_RNGOFF[] = { "Range offset for this node \r\nUsage: \" \"" };
const char COMMENT_PDOATEMP[] = { "TBD" };
const char COMMENT_PHCORREN[] = { "TBD" };

const char COMMENT_UART[] =
{
  "Switch the interface between UART and USB CDC.\r\rUsage: \"uart <ENABLE_UART>\"\r\n ENABLE_UART: 1 to use UART, 0 to use USB CDC"
};
const char COMMENT_ANTTXA[] = { "Antenna TX delay \r\nUsage: \"anttx <DEC>\"" };
const char COMMENT_ANTRXA[] =
{ "Antenna RX delay \r\nUsage: \"antrxa <DEC>\"" };
const char COMMENT_ANTRXB[] = { "TBD" };

const char COMMENT_RESTORE[] =
{ "Restores the default configuration, both UWB and System." };
const char COMMENT_PCREP[] = { "Report level to the COM port" };
const char COMMENT_D2K[] =
{ "Service command to add all Tag from the DList to the KList" };
const char COMMENT_DIAG[] = { "TBD" };
const char COMMENT_ACCUM[] = { "TBD" };

const char COMMENT_UWBCFG[] =
{
  "UWB configuration\r\nUsage: To see UWB parameters \"uwbcfg\". To set the UWB config, list the parameters as a string argument \"uwbcfg <List of parameters>\""
};
const char COMMENT_STSKEYIV[] =
{
  "Sets STS Key, IV and their behavior mode.\r\nUsage: To see STS KEY, IV and Mode \"stskeyiv\". To set\"stskeyiv 0x<STS_KEY_HEX_16> 0x<IV_HEX_16> <MODE_DEC>\".\r\n<MODE_DEC>: 1 use fixed STS (Default), 0 use dynamic STS"
};
const char COMMENT_XTALTRIM[] =
{
  "Xtal trimming value.\r\nUsage: To see Crystal Trim value \"xtaltrim\". To set the Crystal trim value [0..7F] \"uwbcfg 0x<HEX>\""
};
const char COMMENT_TXPOWER[] =
{
  "Tx Power settings.\r\nUsage: To see Tx power \"txpower\". To set the Tx power \"txpower 0x<POWER_HEX> 0x<PGDLY_HEX> 0x<PGCAL_HEX>\""
};

const char COMMENT_DECAID[] = { "TBD" };
const char COMMENT_VERSION[] = { "Shows version of the SW" };

const char COMMENT_DEBUGLED[] = { "TBD" };

// -----------------------------------------------------------------------------

/** list of known commands:
 * NAME, allowed_MODE,     REG_FN(fn_name)
 * */
const struct command_s known_commands[] = {
  /** CMDNAME   MODE   fn  comment */
  /** Anytime commands */
  { NULL, mCmdGrp0 | mANY, NULL, COMMENT_ANYTIME_OPTIONS },
  { "STOP", mCmdGrp1 | mANY, f_stop, COMMENT_STOP },
  { "STAT", mCmdGrp1 | mANY, f_stat, COMMENT_STAT },
  { "SAVE", mCmdGrp1 | mANY, f_save, COMMENT_SAVE },
  { "DECA$", mCmdGrp1 | mANY, f_decaJuniper, COMMENT_DECAJUNIPER },
  { "HELP", mCmdGrp1 | mANY, f_help_app, COMMENT_HELP },
  { "?", mCmdGrp1 | mANY, f_help_app, COMMENT_HELP },

  /** 3. app start commands */
  { NULL, mCmdGrp0 | mIDLE, NULL, COMMENT_APPSELECTION },
  { NODE_FUNC, mCmdGrp1 | mIDLE, f_node, COMMENT_NODE },
  { TAG_FUNC, mCmdGrp1 | mIDLE, f_tag, COMMENT_TAG },
  { TRILAT_FUNC, mCmdGrp1 | mIDLE, f_trilat_n, COMMENT_TRILAT },
  { USPI_FUNC, mCmdGrp1 | mIDLE, f_uspi, COMMENT_USPI },
  { TCWM_FUNC, mCmdGrp1 | mIDLE, f_tcwm, COMMENT_TCWM },
  { TCFM_FUNC, mCmdGrp1 | mIDLE, f_tcfm, COMMENT_TCFM },
  { LISTENER_FUNC, mCmdGrp1 | mIDLE, f_listen, COMMENT_LISTENER },

#if 1 // (LISTENER == 1)
  { NULL, mCmdGrp0 | mLISTENER, NULL, COMMENT_LISTENER_OPT },
  { "LSTAT", mCmdGrp1 | mLISTENER, f_lstat, COMMENT_LSTAT  },
#endif

#if 1 // (PDOA_TAG == 1)
  { NULL, mCmdGrp0 | mPTAG, NULL, COMMENT_PDOA_TAG_OPT },
  { "RCDEL", mCmdGrp1 | mPTAG, f_rc_delay, COMMENT_RCDEL  },
  { "X_MM", mCmdGrp1 | mPTAG, f_x, COMMENT_X },
  { "Y_MM", mCmdGrp1 | mPTAG, f_y, COMMENT_Y },
  { "Z_MM", mCmdGrp1 | mPTAG, f_z, COMMENT_Z },
#endif

#if 1 // (PDOA_NODE == 1)

  /** 2. commands to set system config, run-time and calibration variables */
  { NULL, mCmdGrp0 | mPNODE, NULL, COMMENT_PDOA_NODE_OPT },
  { "ADDR", mCmdGrp1 | mPNODE, f_addr, COMMENT_ADDR },
  { "PANID", mCmdGrp1 | mPNODE, f_panid, COMMENT_PANID },
  { "NUMSLOT", mCmdGrp1 | mPNODE, f_numSlots, COMMENT_NUMSLOT },
  { "SLOTPER", mCmdGrp1 | mPNODE, f_slotPeriod, COMMENT_SLOTPER },
  { "SFPER", mCmdGrp1 | mPNODE, f_sfPeriod, COMMENT_SFPER },
  { "REPDEL", mCmdGrp1 | mPNODE, f_tag_replyDly_us, COMMENT_REPDEL },
  { "P2FDEL", mCmdGrp1 | mPNODE, f_tag_pollTxFinalTx_us, COMMENT_P2FDEL },
  { "RCDEL", mCmdGrp1 | mPNODE, f_rc_delay, COMMENT_RCDEL  },

  /** 4. node application commands */
  { NULL, mCmdGrp0 | mPNODE, NULL, COMMENT_PDOA_NODE_CMD },
  { "JSTAT", mCmdGrp1 | mPNODE, f_jstat, COMMENT_JSTAT },
  { "GETDLIST", mCmdGrp1 | mPNODE, f_get_discovered_list, COMMENT_GETDLIST },
  { "GETKLIST", mCmdGrp1 | mPNODE, f_get_known_list, COMMENT_GETKLIST },
  { "ADDTAG", mCmdGrp1 | mPNODE, f_add_tag_to_list, COMMENT_ADDTAG },
  { "DELTAG", mCmdGrp1 | mPNODE, f_del_tag_from_list, COMMENT_DELTAG },
  { "PDOAOFF", mCmdGrp1 | mPNODE, f_pdoa_offset, COMMENT_PDOAOFF },
  { "RNGOFF", mCmdGrp1 | mPNODE, f_rng_offset, COMMENT_RNGOFF },
  { "PDOATEMP", mCmdGrp1 | mPNODE, f_pdoa_temp_coeff, COMMENT_PDOATEMP },
  { "PHCORREN", mCmdGrp1 | mPNODE, f_phase_corr_enable, COMMENT_PHCORREN },
#endif

  { "UART", mCmdGrp1 | mIDLE, f_uart, COMMENT_UART },
  { "ANTTXA", mCmdGrp1 | mIDLE, f_ant_tx_a, COMMENT_ANTTXA },
  { "ANTRXA", mCmdGrp1 | mIDLE, f_ant_rx_a, COMMENT_ANTRXA },
  { "ANTRXB", mCmdGrp1 | mIDLE, f_ant_rx_b, COMMENT_ANTRXB },

  /** 5. service commands */
  { NULL, mCmdGrp0 | mIDLE, NULL, COMMENT_SERVICE },
  { "RESTORE", mCmdGrp1 | mIDLE, f_restore, COMMENT_RESTORE },
  { "PCREP", mCmdGrp1 | mIDLE, f_twr_report, COMMENT_PCREP },
  { "D2K", mCmdGrp1 | mPNODE, f_add_all_to_list, COMMENT_D2K },
  { "DIAG", mCmdGrp1 | mIDLE, f_diag, COMMENT_DIAG },
  { "ACCUM", mCmdGrp1 | mIDLE, f_acc, COMMENT_ACCUM },

  { "UWBCFG", mCmdGrp1 | mIDLE, f_uwbcfg, COMMENT_UWBCFG },
  { "STSKEYIV", mCmdGrp1 | mIDLE, f_stskeyiv, COMMENT_STSKEYIV },
  { "XTALTRIM", mCmdGrp1 | mIDLE, f_xtal_trim, COMMENT_XTALTRIM },
  { "TXPOWER", mCmdGrp1 | mIDLE, f_power, COMMENT_TXPOWER },
  { "DECAID", mCmdGrp1 | mIDLE, f_decaid, COMMENT_DECAID },
  { "VERSION", mCmdGrp1 | mIDLE, f_get_version, COMMENT_VERSION },

  { "LEDGLOW", mCmdGrp1 | mIDLE, f_led_glow, STD_CMD_COMMENT },
  { "BUTTON", mCmdGrp1 | mIDLE, f_user_button, STD_CMD_COMMENT },
  { "UARTTEST", mCmdGrp1 | mIDLE, f_uart_test, STD_CMD_COMMENT },
  { "DEBUGLED", mCmdGrp1 | mIDLE, f_dbg, COMMENT_DEBUGLED },

  { NULL, mANY, NULL, NULL }
};
