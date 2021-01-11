/***********************************************************************************************//**
 * \file   app_timer.h
 * \brief  Application timer header file
 ***************************************************************************************************
 * <b> (C) Copyright 2015 Silicon Labs, http://www.silabs.com</b>
 ***************************************************************************************************
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 **************************************************************************************************/

#ifndef APP_TIMER_H
#define APP_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * @addtogroup Application
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app
 * @{
 **************************************************************************************************/

/***************************************************************************************************
   Public Macros and Definitions
***************************************************************************************************/

/** Application timer enumeration. */
typedef enum {

 /** Timer event that is reserved for the HRM application.  Note that
  *  some HRM examples do not use this timer.
  */
  HEART_RATE_TIMER,
  PULSE_OXIMETER_TIMER,
 /** Display Polarity Inversion Timer
  * Timer for toggling the the EXTCOMIN signal, which prevents building up a DC bias
     within the Sharp memory LCD panel */
  DISP_POL_INV_TIMER
} app_timer_t;

/** @} (end addtogroup app) */
/** @} (end addtogroup Application) */

#endif /* APP_TIMER_H */
