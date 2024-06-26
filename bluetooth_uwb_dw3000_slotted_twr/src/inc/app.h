/**
 * @file      app.h
 *
 * @brief     Decawave Application Layer header contains all macros and
 *   structures related apllicaitions
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef APP_H_
#define APP_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "default_config.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "error.h"
#include "common.h"
#include "uwb_frames.h"

/* SPI/USART/USB buffers */
#define UART_RX_BUF_SIZE       0x800    // 0x100 Read buffer for UART
                                        //   reception, shall be 1<<X
#define USB_RX_BUF_SIZE        0x100    // Read buffer for USB reception,
                                        //   shall be 1<<X
#define COM_RX_BUF_SIZE        UART_RX_BUF_SIZE // USB_RX_BUF_SIZE
                                                // Communication RX buffer size
#define MAX_CMD_LENGTH         0x100
#if (COM_RX_BUF_SIZE < CDC_DATA_FS_MAX_PACKET_SIZE)
# error "COM_RX_BUF_SIZE should be longer than CDC_DATA_FS_MAX_PACKET_SIZE"
#endif

/* events to start/stop tasks : event group */
enum EventTask {
  Ev_Node_Task        = 0x10,
  Ev_Trilat_N_Task    = 0x20,
  Ev_Tag_Task         = 0x40,
  Ev_Anchor_Task      = 0x80,
  Ev_Tcfm_Task        = 0x100,
  Ev_Tcwm_Task        = 0x200,
  Ev_Usb2spi_Task     = 0x400,
  Ev_Listener_Task    = 0x800,
  // User-available max is 0x800000
  Ev_Stop_All         = 0x800000,
};

/**
 * @brief  GPIO Bit SET and Bit RESET enumeration
 */
typedef enum
{
  GPIO_PIN_RESET = 0,
  GPIO_PIN_SET
}GPIO_PinState;

/* Application tasks handles & corresponded signals structure */
struct task_signal_s
{
  osThreadId_t Handle;       /* Task's handler */
  osMutexId_t  MutexId;      /* Task's mutex */
  int32_t      Signal;       /* Task's signal */
  void         *arg;          /* additional parameters for the Task */
};

typedef struct task_signal_s    task_signal_t;

/* System mode of operation. used to
 *
 * 1. indicate in which mode of operation system is running
 * 2. configure the access rights to command handler in control mode
 * */
typedef enum {
  mANY = 0,     /**< Used only for Commands: indicates the command can be
                 *   executed in any modes below */
  mIDLE,        /**< IDLE mode */
  mPNODE,       /**< PDOA Slotted TWR (Node active) mode */
  mPTAG,        /**< PDoA Slotted TWR (Tag active ) mode */
  mTRILAT_N,    /**< Node running the Trilateration mode */
  mUSB2SPI,     /**< USB2SPI mode : special binary mode */
  mTCWM,        /**< Transmit Continuous Wave Mode mode */
  mTCFM,        /**< Transmit Continuous Frame Mode mode */
  mLwipPerf,
  mLISTENER,    /**< Listener mode*/
  mMASK      = 0xFFFF
}mode_e;

struct eth_circ_buf_s {
  uint8_t     buf[ETH_REPORT_BUFSIZE];
  uint16_t    head;
  uint16_t    tail;
} __attribute__((__packed__));

typedef struct eth_circ_buf_s eth_circ_buf_t;

struct data_circ_buf_s
{
  uint16_t    head;
  uint16_t    tail;
  uint8_t        buf[UART_RX_BUF_SIZE];
};

typedef struct data_circ_buf_s data_circ_buf_t;

struct tcfm_info_s
{
  int nframes;
  int period_ms;
  int bytes;
};

typedef struct tcfm_info_s tcfm_info_t;

/* Application's global parameters structure */
struct app_s
{
  param_block_t   *pConfig;         /**< Current configuration */
  volatile mode_e mode;             /**< Information: handle the current "mode"
                                     *   of operation */
  int             lastErrorCode;    /**< Saves the error code in the
                                     *   error_handler() function */
  int             maxMsgLen;        /**< See the longest string size to optimize
                                     *   the MAX_STR_SIZE */

  /* USB / CTRL */
  enum {
    USB_DISCONNECTED,
    USB_PLUGGED,
    USB_CONNECTED,
    USB_CONFIGURED,
    USB_UNPLUGGED
  }

  usbState;                                          /**< USB connect state */

  enum {
    USB_OFF,
    USB_LINE_CODING_RECEIVED,
    USB_LINE_CODING_REQUESTED,
    USB_LINE_SET_CTRL_LINE_STATE
  }

  usbCtrl;

  data_circ_buf_t uartRx;   /**< circular buffer RX from USART */
  data_circ_buf_t usbRx;   /**< circular buffer RX from USB */

  uint16_t        local_buff_length;                /**< from usb_uart_rx parser
                                                     *   to application */
  uint8_t         local_buff[COM_RX_BUF_SIZE];      /**< for RX from USB/USART
                                                     */

  /* Tasks section */
  EventGroupHandle_t xStartTaskEvent;       /**< This event group will pass
                                             *   activation to tasks to start */
  // defaultTask is always running and is not accepting signals

  // Core tasks
  task_signal_t    ctrlTask;            /* usb/uart RX: Control task */
  task_signal_t    flushTask;           /* usb/uart TX: Flush task */

  // TDoA anchor branch
  task_signal_t    ctrlServer[NUM_BH_BUFFERS];   /* Set of Ctrl Servers 0 is
                                                  *   local;
                                                  *    [1..NUM_BH_BUFFERS-1] are
                                                  *   UWB_BH proxies */
  task_signal_t    rtlsTask;
  task_signal_t    rtlsTask_tx;
  task_signal_t    twrTask;
  task_signal_t    uwbBhTask;            /* UWB Backhaul Discovery process */
  task_signal_t    uwbBhTmr;             /* UWB Backhaul timer */
  task_signal_t    MdnsTask;             /* MDNS */

  // PDoA branch
  osMemoryPoolId_t rxPcktPool_q_id;      /**< Pool ID for PDoA Node processes:
                                          *   FreeRTOS does not free the
                                          *   resources */
  osMessageQueueId_t rxPcktQueue_q_id;   /**< Queue ID for PDoA Node processes:
                                          *   FreeRTOS does not free the
                                          *   resources */
  task_signal_t    rxTask;              /* PDoATag / PDoANode: Rx task */
  task_signal_t    listenerTask;        /* Listener task */
  task_signal_t    trilatTask;          /* PDoATag / PDoANode: trilateration */
  task_signal_t    calcTask;            /* PDoA Node only */
  task_signal_t    blinkTask;           /* PDoA Tag only */
  task_signal_t    pollTask;            /* PDoA Tag only */

  /* top-level tasks for special modes */
  task_signal_t    usb2spiTask;         /* USB2SPI top-level application */
  task_signal_t    tcfmTask;            /* TCFM top-level application */
  task_signal_t    tcwmTask;            /* TCWM top-level application */
  task_signal_t    commTask;            /* Communicaiton top-level applicaiton
                                         */

  task_signal_t    imuTask;             /* Tag/Node */

  /*Slow timer for TDoA RTLS*/
  void                (*timer2Isr)(void);  // *< DSR callback to execute on Slow
                                           //   APP Timer
  osTimerId_t         appTimerHandle;
  uint32_t            appTimer_MS;

  // Addressing And Channel
  uint8_t           eui64[ADDR_BYTE_SIZE_L];
  uint16_t          panId;

  uint8_t             lnaOn;
  // Logging
  uint8_t             debugLogs;
  uint32_t            logNum;

  struct
  {  // TODO: review the sleeping implementation on C0 chip
    enum
    {
      DW_CANNOT_SLEEP,
      DW_CAN_SLEEP,
      DW_CAN_SLEEP_APP             // DW can be put to sleep by the app, after
                                   //   events are processed (not immediately
                                   //   after IRQ).
    }               DwCanSleepInIRQ;

    enum
    {
      DW_NOT_SLEEPING,
      DW_IS_SLEEPING_RX,
      DW_IS_SLEEPING_IRQ,
      DW_IS_SLEEPING_FIRST
    }                DwEnterSleep;

    enum
    {
      DW_SPI_READY,
      DW_SPI_SLEEPING
    }                DwSpiReady;
  };

  void    (* HAL_RTCEx_WakeUpTimerEventCb)(void);

  int         listener_mode;

  struct
  {  // This is the Extended Tcfm parameters extension
    tcfm_info_t tcfm_info;

    dwt_deviceentcnts_t event_counts;
    uint32_t event_counts_sts_bad;       // counts the number of STS detections
                                         //   with bad STS quality
    uint32_t event_counts_sts_good;      // counts the number of STS detections
                                         //   with good STS quality
    uint32_t event_counts_sfd_detect;    // counts the number of SFD detections
                                         //   (RXFR has to be set also)
  };
};

typedef struct app_s app_t;

/* global application variables */
extern app_t app;

void error_handler(int block, error_e err);

#ifdef __cplusplus
}
#endif //APP_H_ 1

#endif //__DECA_APP_H
