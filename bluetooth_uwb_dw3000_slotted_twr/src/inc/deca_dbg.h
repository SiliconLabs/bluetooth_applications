/**
 * @file      deca_dbg.h
 *
 * @brief     Debug macros for debug prints
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */
#ifndef DECA_DBG_H_
#define DECA_DBG_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "usb_uart_tx.h"
#include <stdio.h>
#include <string.h>

#if 0
#define DBG_PRINTF                  diag_printf

#define DBG_TRACE_EVENTS            if (!TX_EVENT_RECEIVED) printf( \
    "MsgPtrs [ %d - %d %d %d - %3d - 0x%X] \n",                     \
    rxData[                                                         \
      phyEvent[app.eventIdx].rxDataPtr].rxDataLen,                  \
    phyCfg.eventIdx,                                                \
    app.eventIdx,                                                   \
    phyEvent[app.eventIdx].type,                                    \
    rxData[                                                         \
      phyEvent[app.eventIdx].rxDataPtr].msg.stdMsg.seqNum,          \
    rxData[                                                         \
      phyEvent[app.eventIdx].rxDataPtr].msg.stdMsg.messageData[0]);

#define DBG_TRACE_CLOCK_SYNC_TX     printf(         \
    "CS Sent at .. @ .. 0x%02x%02x%02x%02x%02x \n", \
    phyEvent[app.eventIdx].timeStamp[4],            \
    phyEvent[app.eventIdx].timeStamp[3],            \
    phyEvent[app.eventIdx].timeStamp[2],            \
    phyEvent[app.eventIdx].timeStamp[1],            \
    phyEvent[app.eventIdx].timeStamp[0]);           \
  printf("C:%d\n", app.rtls_info.ccpSeqNum);

#define DBG_TRACE_CLOCK_SYNC_RX     printf(          \
    "CS Received .. @ .. 0x%02x%02x%02x%02x%02x \n", \
    phyEvent[app.eventIdx].timeStamp[4],             \
    phyEvent[app.eventIdx].timeStamp[3],             \
    phyEvent[app.eventIdx].timeStamp[2],             \
    phyEvent[app.eventIdx].timeStamp[1],             \
    phyEvent[app.eventIdx].timeStamp[0]);            \
  printf("C:%d\n", rxMsg.msg.bcastMsg.seqNum);

#define DBG_TRACE_TAG_BLINK         printf(  \
    "Tag Blink : Seq : %d : Time : %lld \n", \
    rxMsg.msg.blinkMsg.seqNum,               \
    phyEvent[app.eventIdx].timeStamp);       \
  // printf("S:%d\n", rxMsg.msg.blinkMsg.seqNum);

#define DBG_TRACE_NETWORK_MSG       printf(      \
    "Received Network Configuration : %s %d \n", \
    (app.rtls_info.master?"MASTER":"SLAVE"),     \
    app.rtls_info.id);

#define DBG_TRACE_CONFIGURATION     printf(         \
    "Configuration Message %x %x : Time : %lld \n", \
    rxData[phyEvent[app.eventIdx].                  \
           rxDataPtr].msg.stdMsg.messageData[0],    \
    rxData[phyEvent[app.eventIdx].                  \
           rxDataPtr].msg.stdMsg.messageData[1],    \
    phyEvent[app.eventIdx].timeStamp);

#else

#define DIAG_BUF_LEN                0x8
#define DIAG_STR_LEN                64
// note, (DIAG_BUF_LEN*(DIAG_STR_LEN+4)) shall be < HTTP_PAGE_MALLOC_SIZE-16
// <br>=4
typedef struct
{
  uint8_t buf[DIAG_BUF_LEN][DIAG_STR_LEN];
  int      head;
}gDiagPrintFStr_t;

extern gDiagPrintFStr_t  gDiagPrintFStr;

#define diag_printf(...) do{                                               \
    snprintf((char *)(&gDiagPrintFStr.buf[gDiagPrintFStr.head][0]),        \
             DIAG_STR_LEN,                                                 \
             __VA_ARGS__);                                                 \
    port_tx_msg(&gDiagPrintFStr.buf[gDiagPrintFStr.head][0],               \
                strlen((char *)(&gDiagPrintFStr.buf[gDiagPrintFStr.head][0 \
                                ])));                                      \
    gDiagPrintFStr.head = (gDiagPrintFStr.head + 1) & (DIAG_BUF_LEN - 1);  \
}while (0)

#define DBG_PRINTF                diag_printf
#define DBG_TRACE_EVENTS
#define DBG_TRACE_CLOCK_SYNC_TX
#define DBG_TRACE_CLOCK_SYNC_RX
#define DBG_TRACE_TAG_BLINK
#define DBG_TRACE_NETWORK_MSG
#define DBG_TRACE_CONFIGURATION
#endif

#define DBG_TRACE(x)    { uint8_t buf; buf = x; dwt_writetodevice(0xb, \
                                                                  0,   \
                                                                  1,   \
                                                                  &buf); }

/* TRACE_DEBUG and TRACE_INFO used in drivers when -DDEBUG in Makefile defined
 */
#ifdef DEBUG
#define TRACE_DEBUG     diag_printf
#define TRACE_INFO      diag_printf
#else
#define TRACE_DEBUG
#define TRACE_INFO      diag_printf
#endif

#ifdef __cplusplus
}
#endif

#endif /* DECA_DBG_H_ */
