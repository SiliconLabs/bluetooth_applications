/***************************************************************************//**
* @file hrm_helper.h
* @brief Helper function to reducing burden on algorithm code
* @version 1.0
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
*
* EXPERIMENTAL QUALITY
* This code has not been formally tested and is provided as-is.  It is not suitable for production environments.
* This code will not be maintained.
*
******************************************************************************/

#ifndef MAXM86161_HELPER_H_
#define MAXM86161_HELPER_H_

#include <maxm86161.h>
#include <maxm86161_hrm_config.h>
#include <maxm86161_hrm_spo2.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *  max86161_hrm_helper_handle_t;

void maxm86161_hrm_helper_process_irq();
int32_t maxm86161_hrm_helper_sample_queue_numentries();
int32_t maxm86161_hrm_helper_sample_queue_get(maxm86161_hrm_irq_sample_t *samples);
void maxm86161_hrm_helper_sample_queue_clear();
int32_t maxm86161_hrm_helper_initialize();
int32_t maxm86161_hrm_helper_close();
#ifdef PROXIMITY
bool maxm86161_get_prox_mode(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MAXM86161_HELPER_H_ */
