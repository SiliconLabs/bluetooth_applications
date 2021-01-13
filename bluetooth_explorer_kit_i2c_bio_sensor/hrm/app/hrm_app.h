/**************************************************************************//**
 * @file hrm_app.h
 * @brief Header file of hrm_app.c
 * @version 1.0.0
*******************************************************************************
* # License
* <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
#ifndef HRM_APP_H__
#define HRM_APP_H__

#define HRM_DEMO_NAME "si117xHRM Demo"
#define HRM_DEMO_VERSION "1.0.0"

/*  External Events  */
#define MAXM86161_IRQ_EVENT     0x1
#define BTN0_IRQ_EVENT          0x2

typedef enum HRMSpO2State
{
   HRM_STATE_IDLE,
   HRM_STATE_NOSIGNAL,
   HRM_STATE_ACQUIRING,
   HRM_STATE_ACTIVE,
   HRM_STATE_INVALID
}hrm_spo2_state_t;

/**************************************************************************//**
 * @brief Initialize the HRM application.
 *****************************************************************************/
void hrm_init_app(void);

void hrm_process_event(uint32_t event_flags);

void hrm_loop(void);
/**************************************************************************//**
 * @brief This function returns the current heart rate.
 *****************************************************************************/
int16_t hrm_get_heart_rate(void);

/**************************************************************************//**
 * @brief This function returns the current finger contact status.
 *****************************************************************************/
bool hrm_get_status(void);

int16_t hrm_get_spo2(void);

#endif    //HRM_APP_H__
