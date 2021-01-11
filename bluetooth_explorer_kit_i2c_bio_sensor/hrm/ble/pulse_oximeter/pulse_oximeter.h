/**
 * The Bluetooth Developer Studio auto-generated code
 *
 ***************************************************************************************************
 * <b> (C) Copyright 2016 Silicon Labs, http://www.silabs.com </b>
 ***************************************************************************************************
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 */

#ifndef PULSE_OXIMETER_H_
#define PULSE_OXIMETER_H_

/*******************************************************************************
 *******************************   INCLUDES   **********************************
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
//#include "native_gecko.h"
#include "sl_bt_types.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/*! Pulse Oximeter Service - SpO2PR */
typedef struct plxSpO2PR_tag
{
  uint16_t      SpO2;
  uint16_t      PR;
}plxSpO2PR_t;

/*! Pulse Oximeter Service - Daytime */
typedef struct ctsDateTime_tage
{
  uint16_t                    Year;                  /* M */
  uint8_t                     Month;                 /* M */
  uint8_t                     Day;                   /* M */
  uint8_t                     Hours;                 /* M */
  uint8_t                     Minutes;               /* M */
  uint8_t                     Seconds;               /* M */
}ctsDateTime_t;

/*! Pulse Oximeter Service - Spotcheck Measurement */
typedef struct plxSpotcheckMeasurement_tag
{
  ctsDateTime_t               timestamp;              /* C1 */
  plxSpO2PR_t                 SpO2PRSpotcheck;        /* M */
  uint32_t                    deviceAndSensorStatus;  /* C3 */
  uint16_t                    measurementStatus;      /* C2 */
  uint16_t                    pulseAmplitudeIndex;    /* C4 */
  uint8_t                     flags;                  /* M */
}plxSpotcheckMeasurement_t;

/*! Pulse Oximeter Service - Continuous Measurement */
typedef struct plxContinuousMeasurement_tag
{
  plxSpO2PR_t                 SpO2PRNormal;           /* M */
  plxSpO2PR_t                 SpO2PRFast;             /* C1 */
  plxSpO2PR_t                 SpO2PRSlow;             /* C2 */
  uint32_t                    deviceAndSensorStatus;  /* C4 */
  uint16_t                    measurementStatus;      /* C3 */
  uint16_t                    pulseAmplitudeIndex;    /* C5 */
  uint8_t                     flags;                  /* M */
}plxContinuousMeasurement_t;

/*! Pulse Oximeter Service - PLX Features */
typedef struct plxFeatures_tag
{
  uint16_t    supportedFeatures;              /* M */
  uint16_t    measurementStatusSupport;       /* C1 */
  uint32_t    deviceAndSensorStatusSupport;   /* C2 */
}plxFeatures_t;

/*! Pulse Oximeter Service - Op Code */
typedef uint8_t plxOpCode_t;

/*! Pulse Oximeter Service - Operator */
typedef uint8_t plxOperator_t;

/*! Pulse Oximeter Service - Response Code */
typedef uint8_t plxRspCodeValue_t;

/*! Pulse Oximeter Service - Response Code */
typedef struct plxResponseCode_tag
{
  plxOpCode_t         reqOpCode;
  plxRspCodeValue_t   rspCodeValue;
}plxResponseCode_t;

/*! Pulse Oximeter Service - RACP Procedure */
typedef struct plxProcedure_tag
{
  plxOpCode_t         opCode;                 /* M */
  plxOperator_t       plxOperator;            /* M */
  union {
    uint16_t          numberOfRecords;
    plxResponseCode_t responseCode;
  } operand;                                  /* M */
} plxProcedure_t;

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/*******************************************************************************
 * @brief
 *   Service Pulse Oximeter initialization
 * @return
 *   None
 ******************************************************************************/
void pulse_oximeter_init(void);

/*******************************************************************************
 * @brief
 *   Function to handle read data
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void pulse_oximeter_read_callback(sl_bt_msg_t *evt);

/*******************************************************************************
 * @brief
 *   Function to handle write data
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void pulse_oximeter_write_callback(sl_bt_msg_t *evt);

/*******************************************************************************
 * @brief
 *   Function to handle disconnect event
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void pulse_oximeter_disconnect_event(sl_bt_msg_t *evt);

/*******************************************************************************
 * @brief
 *   Function to handle gecko_evt_gatt_server_characteristic_status_id event
 * @param[in] evt
 *   Gecko event
 * @return
 *   None
 ******************************************************************************/
void pulse_oximeter_characteristic_status(sl_bt_msg_t *evt);

/*******************************************************************************
 * @brief
 *   Function to update SpO2 data
 * @return
 *   None
 ******************************************************************************/
void pulse_oximeter_send_new_data(uint8_t connect);


#endif /* PULSE_OXIMETER_H_ */
