/***************************************************************************//**
 * @file air_quality_app.h
 * @brief Define driver structures and APIs for the air_quality_app.c
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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/
#ifndef AIR_QUALITY_APP_H
#define AIR_QUALITY_APP_H

#include "sl_bt_api.h"

/*  External Events  */
#define AIR_QUALITY_MONITOR_EVENT           (1)
#define AIR_QUALITY_MONITOR_BUTTON_EVENT    (2)

#define NOTIFICATION_ATT_LENGTH             (1)
#define BUZZER_ATT_LENGTH                   (1)
#define CO2_ATT_LENGTH                      (2)
#define TVOC_ATT_LENGTH                     (2)
#define UPDATE_PERIOD_ATT_LENGTH            (1)

#define IS_NOTIFICATION_ACTIVE_MIN          (0)
#define IS_NOTIFICATION_ACTIVE_MAX          (1)
#define IS_NOTIFICATION_ACTIVE_DEFAULT      (1)

#define UPDATE_PERIOD_IN_SECOND_MIN         (1)
#define UPDATE_PERIOD_IN_SECOND_MAX         (30)
#define UPDATE_PERIOD_IN_SECOND_DEFAULT     (10)

#define BUZZER_VOLUME_MIN                   (1)
#define BUZZER_VOLUME_MAX                   (10)
#define BUZZER_VOLUME_DEFAULT               (6)

#define THRESHOLD_CO2_PPM_MIN               (400)
#define THRESHOLD_CO2_PPM_MAX               (8192)
#define THRESHOLD_CO2_PPM_DEFAULT           (1000)

#define THRESHOLD_TVOC_PPB_MIN              (1)
#define THRESHOLD_TVOC_PPB_MAX              (1187)
#define THRESHOLD_TVOC_PPB_DEFAULT          (100)

/***************************************************************************//**
 * @addtogroup air_quality_app
 * @brief air_quality_app interface.
 * @{
 ******************************************************************************/
/***************************************************************************//**
 * @brief
 *    Typedef for holding application configuration parameters.
 ******************************************************************************/
typedef struct {
  uint8_t notification_data;       ///<  Notification status (0: disabled, 1:
                                   ///<   enabled)
  uint8_t buzzer_data;             ///<  Buzzer volume (0-10)
  uint8_t measurement_period_data; ///<  Measurement update period in s (1-30)
  uint16_t threshold_co2_ppm;      ///<  co2 threshold value in ppm.
  uint16_t threshold_tvoc_ppb;     ///<  tvoc threshold value in ppb.
} air_quality_data_t;

/***************************************************************************//**
 * @brief
 *    Return codes for the status of air quality index function.
 ******************************************************************************/
typedef enum air_quality_status {
  EXCELLENT   = 1, ///< The air inside is as fresh as the air outside.
  FINE        = 2, ///< The air quality inside remains at harmless levels.
  MODERATE    = 3, ///< The air quality inside has reached conspicuous levels.
  POOR        = 4, ///< The air quality inside has reached precarious levels.
  VERY_POOR   = 5, ///< The air quality inside has reached unacceptable levels.
  SEVERE      = 6  ///< The air quality inside has exceeded maximum workplace
                   ///<   concentration values.
} air_quality_status_t;

/*********************************************************************&*****//**
 * @brief
 *   Initialize the AIR QUALITY application.
 *
 * @return
 *   @ref SL_STATUS_OK on success or @ref SL_STATUS_FAIL on failure
 ********************************************************************&*********/
sl_status_t air_quality_app_init(void);

/*******************************************************************&*******//**
 * @brief
 *   Process Bluetooth external events.
 *
 * @param[in] event_flags
 *   Event Handler
 *
 * @return
 *   None
 *******************************************************************&**********/
void air_quality_process_event(uint32_t event_flags);

/*******************************************************************************
 * @brief
 *   Function to handle write data.
 *
 * @param[in] evt
 *   Gecko event
 *
 * @return
 *   None
 ******************************************************************************/
void air_quality_user_write_callback(sl_bt_msg_t *evt);

/*******************************************************************************
 * @brief
 *   Function to handle disconnect event.
 *
 * @param[in] evt
 *   Gecko event
 *
 * @return
 *   None
 ******************************************************************************/
void air_quality_user_read_callback(sl_bt_msg_t *evt);

/** @} */

#endif //AIR_QUALITY_APP_H
