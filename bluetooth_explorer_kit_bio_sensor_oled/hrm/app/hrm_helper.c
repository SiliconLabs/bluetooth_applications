/***************************************************************************//**
* @file hrm_helper.c
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

#include <hrm_helper.h>
#include <maxm86161.h>
#include <maxm86161_hrm_config.h>
#include <sl_app_log.h>

maxm86161_fifo_queue_t ppg_queue;
maxm86161_ppg_sample_t maxm86161_irq_queue[APP_QUEUE_SIZE];
static bool maxm86161_prox_mode = false;

void maxm86161_helper_sample_queue_clear()
{
  maxm86161_clear_queue(&ppg_queue);
}

int32_t maxm86161_hrm_helper_sample_queue_numentries()
{
  int16_t count=0;

  count = maxm86161_num_samples_in_queue(&ppg_queue);
  return count;
}

int32_t maxm86161_hrm_helper_sample_queue_get(maxm86161_hrm_irq_sample_t *samples)
{

  int ret = MAXM86161_HRM_SUCCESS;
  maxm86161_ppg_sample_t s;

  ret = maxm86161_dequeue_ppg_sample_data(&ppg_queue, &s);
  if (ret == 0) {
      samples->ppg[0] = s.ppg1;
      samples->ppg[1] = s.ppg2;
      samples->ppg[2] = s.ppg3;
  }
  else {
      ret = MAXM86161_HRM_ERROR_SAMPLE_QUEUE_EMPTY;
            goto Error;
  }

Error:
  return ret;
}

/**************************************************************************//**
 * @brief
 *****************************************************************************/
int32_t maxm86161_hrm_helper_initialize()
{
  int16_t error = 0;
  maxm86161_allocate_ppg_data_queue(&ppg_queue, maxm86161_irq_queue, APP_QUEUE_SIZE*MAXM86161DRV_PPG_SAMPLE_SIZE_BYTES);
  maxm86161_helper_sample_queue_clear();
  return error;
}


#ifdef PROXIMITY
void maxm86161_hrm_helper_process_irq(void)
{
  uint8_t reg_status;
  uint8_t ppg_sr_status;

  maxm86161_i2c_read_from_register(MAXM86161_REG_IRQ_STATUS1, &reg_status);
  if (reg_status & MAXM86161_INT_1_FULL) {
      maxm86161_read_samples_in_fifo(&ppg_queue);
  }

  if (reg_status & MAXM86161_INT_1_PROXIMITY_INT) {
    maxm86161_i2c_read_from_register(MAXM86161_REG_PPG_CONFIG2, &ppg_sr_status);
    if ((ppg_sr_status >> 3) == 0x0A) {
      maxm86161_prox_mode = true;
    }
    else {
      maxm86161_prox_mode = false;
    }
  }
}
#else
void maxm86161_hrm_helper_process_irq(void)
{
  uint8_t reg_status;

  maxm86161_i2c_read_from_register(MAXM86161_REG_IRQ_STATUS1, &reg_status);
  if (reg_status & MAXM86161_INT_1_FULL) {
    maxm86161_read_samples_in_fifo(&ppg_queue);
  }
}
#endif

/**************************************************************************//**
 * @brief Use to check maxm86161 in proximity mode or normal mode
 *****************************************************************************/
#ifdef PROXIMITY
bool maxm86161_get_prox_mode(void)
{
  return maxm86161_prox_mode;
}
#endif

/**************************************************************************//**
 * @brief Prints heart rate and spo2 to USB debug interface
 *****************************************************************************/
void hrm_helper_output_debug_message(int16_t heart_rate, int16_t spo2)
{
  sl_app_log("Heart rate = %hdbpm, SpO2 = %hd%%\n", heart_rate, spo2);
}

/**************************************************************************//**
 * @brief Prints samples to USB debug interface
 *****************************************************************************/
void hrm_helper_output_raw_sample_debug_message(maxm86161_hrm_irq_sample_t *sample)
{
  sl_app_log("\n%lu,%lu,%lu,", sample->ppg[0], sample->ppg[1], sample->ppg[2]);
}
