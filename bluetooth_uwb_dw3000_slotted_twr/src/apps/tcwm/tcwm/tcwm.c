/**
 * @file      tcwm.c
 *
 * @brief     Process to test continuous wave mode
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 */

#include "tcwm.h"

#include "app.h"
#include "port.h"
#include "deca_device_api.h"

/* IMPLEMETATION */

/*
 * @brief     init function initialises all run-time environment allocated by
 *   the process
 *             it will be executed once
 * */
void tcwm_process_init(void)
{
  tcXm_configure_test_mode();

  dwt_configcwmode();
}

/*
 * @brief     run function implements continuous process functionality
 * */
void tcwm_process_run(void)
{
  /*do nothing*/
}

/*
 * @brief     stop function implements stop functionality if any
 *             which will be executed on reception of Stop command
 * */
void tcwm_process_terminate(void)
{
  port_stop_all_UWB();
}

/*
 * @brief     configure channel parameters and tx spectrum parameters
 * */
void tcXm_configure_test_mode(void)
{
  int             result;

  result = dwt_initialise(DWT_DW_INIT);

  if (DWT_SUCCESS != result) {
    error_handler(1, _ERR_INIT);
  }

  dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);    /* For debug - to see
                                                          *   TX LED light up
                                                          *   when in this mode
                                                          */
  dwt_setlnapamode(DWT_PA_ENABLE);                       /* For debug - to see
                                                          *   TX state output
                                                          *   and be able to
                                                          *   measure the packet
                                                          *   length
                                                          */

  if (dwt_configure(&app.pConfig->dwt_config)) {   /**< Configure the Physical
                                                    *   Channel parameters
                                                    *   (PLEN, PRF, etc)
                                                    */
    error_handler(1, _ERR_INIT);
  }

  /* configure power */
  dwt_configuretxrf(&app.pConfig->s.txConfig);

  /*
   * The dwt_initialize will read the default XTAL TRIM from the OTP or use the
   *   DEFAULT_XTAL_TRIM.
   * In this case we would apply the user-configured value.
   *
   * The bit 0x80 can be used to overwrite the OTP settings if any.
   * */
  if ((dwt_getxtaltrim() == DEFAULT_XTAL_TRIM)
      || (app.pConfig->s.xtalTrim & ~XTAL_TRIM_BIT_MASK)) {
    dwt_setxtaltrim(app.pConfig->s.xtalTrim & XTAL_TRIM_BIT_MASK);
  }

  /* set antenna delays */
  dwt_setrxantennadelay(app.pConfig->s.antRx_a);
  dwt_settxantennadelay(app.pConfig->s.antTx_a);

  dwt_setrxaftertxdelay(0);      /**< no any delays set by default : part of
                                  *   config of receiver on Tx sending */
  dwt_setrxtimeout(0);           /**< no any delays set by default : part of
                                  *   config of receiver on Tx sending */
}
