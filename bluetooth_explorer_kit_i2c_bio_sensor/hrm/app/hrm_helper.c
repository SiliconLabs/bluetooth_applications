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
  if(ret == 0)
    {
      samples->ppg[0] = s.ppg1;
      samples->ppg[1] = s.ppg2;
      samples->ppg[2] = s.ppg3;
    }
  else{
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
void maxm86161_hrm_helper_process_irq()
{
  uint8_t int_status;
  uint8_t ppg_sr_status;

  int_status =  maxm86161_i2c_read_from_register(MAXM86161_REG_IRQ_STATUS1);
  if(int_status & MAXM86161_INT_1_FULL)
  {
      maxm86161_read_samples_in_fifo(&ppg_queue);
      //GPIO_PinOutToggle(gpioPortA, 4);
  }

  if(int_status & MAXM86161_INT_1_PROXIMITY_INT)
  {
    ppg_sr_status = maxm86161_i2c_read_from_register(MAXM86161_REG_PPG_CONFIG2) >> 3;
    if(ppg_sr_status == 0x0A)
    {
      maxm86161_prox_mode = true;
    }
    else
    {
      maxm86161_prox_mode = false;
    }
  }
}
#else
void maxm86161_hrm_helper_process_irq()
{
  uint8_t intStatus;
  intStatus =  maxm86161_i2c_read_from_register(MAXM86161_REG_IRQ_STATUS1);
  if(intStatus & MAXM86161_INT_1_FULL)
  {
    //GPIO_PinOutToggle(gpioPortA, 4);
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
