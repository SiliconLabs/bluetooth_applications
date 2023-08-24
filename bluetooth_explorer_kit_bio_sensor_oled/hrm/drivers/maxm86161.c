/***************************************************************************//**
 * @file maxm86161.c
 * @brief Platform independent driver for maxm86161 biometric sensor
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
 *
 * EVALUATION QUALITY
 * This code has been minimally tested to ensure that it builds with
 * the specified dependency versions and is suitable as a demonstration
 * for evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 *
 ******************************************************************************/

#include <maxm86161.h>
#include <maxm86161_hrm_config.h>
#include <sl_udelay.h>

// --------------------- PRIVATE FUNCTION DECLARATIONS -----------------------

// Function for checking data if it's 1 or 0
static sl_status_t maxm86161_bool_check(uint8_t value);

// Function for tint data validation check
static sl_status_t maxm86161_ppg_tint_check(uint8_t value);

// Function for led range data validation check
static sl_status_t maxm86161_led_range_check(uint8_t value);

// Function for sample rate data validation check
static sl_status_t maxm86161_smp_rate_check(uint8_t value);

// Function for smp avg freq data validation check
static sl_status_t maxm86161_smo_freq_check(uint8_t value);

// Function for led range current data validation check
static sl_status_t maxm86161_led_range_curr_check(uint8_t value);

// Function for squence data validation check
static sl_status_t maxm86161_sequence_check(uint8_t value);

// Function for delay after reset bit is set to 1
static void maxm86161_soft_reset_delay(void);

// Function for letting I2C wait after status check
// static void maxm86161_dev_i2c_delays(void);

// --------------------- Device configuration functions ------------------------

/***************************************************************************//**
 * @brief
 *    Initialize the Maxim86161 with the device configuration
 *
 * @param[in] global_cfg
 * device configuration structure
 *
 * @return
 *    sl_status_t error code
 ******************************************************************************/
sl_status_t maxm86161_init_device(maxm86161_device_config_t global_cfg)
{
  sl_status_t ret = SL_STATUS_OK;

  maxm86161_software_reset();
  ret |= maxm86161_ppg_config(&global_cfg.ppg_cfg);
  ret |= maxm86161_led_sequence_config(&global_cfg.ledsq_cfg);
  ret |= maxm86161_interupt_control(&global_cfg.int_cfg);
  maxm86161_led_pa_config(&global_cfg.ledpa_cfg);
  // interrupt level setup should be happened after above configuration
  maxm86161_set_int_level(global_cfg.int_level);
  maxm86161_i2c_write_to_register(MAXM86161_REG_FIFO_CONFIG2,
                                  (MAXM86161_FIFO_CFG_2_FULL_TYPE_RPT
                                   | MAXM86161_FIFO_CFG_2_FIFO_READ_DATA_CLR));
  // clear FIFO, don't know if it is necessary in this case
  maxm86161_flush_fifo();
#ifdef PROXIMITY
  maxm86161_i2c_write_to_register(MAXM86161_REG_LED_PILOT_PA, 0x05);
  // threshold = 1*2048
  maxm86161_i2c_write_to_register(MAXM86161_REG_PROX_INT_THRESHOLD, 0x01);
#endif

  return ret;
}

/***************************************************************************//**
 * @brief
 *    Turn on/off shutdown mode
 *
 *    All interrupts are cleared.
 *    In this mode, the oscillator is shutdown and the part draws minimum
 *   current
 *    If this bit is asserted during an active conversion,
 *    then the conversion is aborted.
 *
 * @param[in] turn_off
 * bool value for turn on/off option
 *
 * @return
 *    None
 ******************************************************************************/
void maxm86161_shutdown_device(bool turn_off)
{
  uint8_t value = 0;

  maxm86161_i2c_read_from_register(MAXM86161_REG_SYSTEM_CONTROL, &value);
  if (turn_off) {
    value |= MAXM86161_SYS_CTRL_SHUT_DOWN;
  } else {
    value &= ~MAXM86161_SYS_CTRL_SHUT_DOWN;
  }
  maxm86161_i2c_write_to_register(MAXM86161_REG_SYSTEM_CONTROL, value);
}

/***************************************************************************//**
 * @brief
 *  All configuration, threshold and data registers including distributed
 *  registers are reset to their power-on-state
 *
 ******************************************************************************/
void maxm86161_software_reset(void)
{
  uint8_t value = 0;

  maxm86161_i2c_read_from_register(MAXM86161_REG_SYSTEM_CONTROL, &value);
  value |= MAXM86161_SYS_CTRL_SW_RESET;
  maxm86161_i2c_write_to_register(MAXM86161_REG_SYSTEM_CONTROL, value);
  maxm86161_soft_reset_delay();
}

/***************************************************************************//**
 * @brief
 *  the FIFO gets flushed, FIFO_DATA_COUNT becomes 0.
 *  The contents of the FIFO are lost.
 *
 ******************************************************************************/
void maxm86161_flush_fifo(void)
{
  uint8_t value = 0;

  maxm86161_i2c_read_from_register(MAXM86161_REG_FIFO_CONFIG2, &value);
  value |= MAXM86161_FIFO_CFG_2_FLUSH_FIFO;
  maxm86161_i2c_write_to_register(MAXM86161_REG_FIFO_CONFIG2, value);
}

/***************************************************************************//**
 * @brief
 *    Set the number of sample the fifo for maxim to fire an FULL interrupt
 *
 * @param[in] level
 *    Number of sample
 *
 * @return
 *    None
 *
 ******************************************************************************/
void maxm86161_set_int_level(uint8_t level)
{
  uint8_t value = 0;
  value = 128 - level;
  maxm86161_i2c_write_to_register(MAXM86161_REG_FIFO_CONFIG1, value);
}

/***************************************************************************//**
 * @brief
 *    Configure PPG (such as adc range, sample rate, ...)
 *
 * @param[in] ppg_cfg
 *    Pointer to the ppg configuration struct
 *
 * @return
 *    sl_status_t error code
 *
 ******************************************************************************/
sl_status_t maxm86161_ppg_config(maxm86161_ppg_cfg_t *ppg_cfg)
{
  if ((maxm86161_bool_check(ppg_cfg->alc)
       & maxm86161_bool_check(ppg_cfg->offset)
       & maxm86161_ppg_tint_check(ppg_cfg->ppg_tint)
       & maxm86161_led_range_check(ppg_cfg->adc_range)
       & maxm86161_smp_rate_check(ppg_cfg->smp_rate)
       & maxm86161_smo_freq_check(ppg_cfg->smp_freq)) != SL_STATUS_OK) {
    return SL_STATUS_FAIL;
  }

  maxm86161_i2c_write_to_register(MAXM86161_REG_PPG_CONFIG1,
                                  ((ppg_cfg->alc << MAXM86161_PPG_CFG_ALC)
                                   | (ppg_cfg->offset <<
                                      MAXM86161_PPG_CFG_OFFSET)
                                   | (ppg_cfg->adc_range <<
                                      MAXM86161_PPG_CFG_ADC_RANGE)
                                   | (ppg_cfg->ppg_tint <<
                                      MAXM86161_PPG_CFG_TINT)));

  maxm86161_i2c_write_to_register(MAXM86161_REG_PPG_CONFIG2,
                                  ((ppg_cfg->smp_rate <<
                                    MAXM86161_PPG_CFG_SMP_RATE)
                                   | (ppg_cfg->smp_freq <<
                                      MAXM86161_PPG_CFG_SMP_AVG)));
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *    Configure LED current for a specific LED
 *
 * @param[in] ledx
 *    Number of led that need to change current
 *
 * @param[in] value
 *    Current of the LED
 *
 * @return
 *    sl_status_t error code
 *
 ******************************************************************************/
sl_status_t maxm86161_led_pa_config_specific(uint8_t ledx, uint8_t value)
{
  switch (ledx) {
    case HEARTRATE2_LED_1:
      maxm86161_i2c_write_to_register(MAXM86161_REG_LED1_PA, value);
      break;
    case HEARTRATE2_LED_2:
      maxm86161_i2c_write_to_register(MAXM86161_REG_LED2_PA, value);
      break;
    case HEARTRATE2_LED_3:
      maxm86161_i2c_write_to_register(MAXM86161_REG_LED3_PA, value);
      break;
    default:
      return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *    Configure LED current for all the LED at the initial stage
 *
 * @param[in] ledpa
 *    Pointer to the ledpa struct
 *
 * @return
 *    None
 *
 ******************************************************************************/
void maxm86161_led_pa_config(maxm86161_ledpa_t *ledpa)
{
  maxm86161_i2c_write_to_register(MAXM86161_REG_LED1_PA, ledpa->green);
  maxm86161_i2c_write_to_register(MAXM86161_REG_LED2_PA, ledpa->ir);
  maxm86161_i2c_write_to_register(MAXM86161_REG_LED3_PA, ledpa->red);
}

/***************************************************************************//**
 * @brief
 *    Configure range current for all the LED at the initial stage
 *
 * @param[in] led_range
 *    Pointer to the led_range struct
 *
 * @return
 *    sl_status_t error code
 *
 ******************************************************************************/
sl_status_t maxm86161_led_range_config(maxm86161_led_range_curr_t *led_range)
{
  if ((maxm86161_led_range_curr_check(led_range->green)
       & maxm86161_led_range_curr_check(led_range->ir)
       & maxm86161_led_range_curr_check(led_range->red)) != SL_STATUS_OK) {
    return SL_STATUS_FAIL;
  }

  maxm86161_i2c_write_to_register(MAXM86161_REG_LED_RANGE1,
                                  ((led_range->green <<              \
                                    MAXM86161_LED_RANGE_SHIFT_GREEN) \
                                   | (led_range->ir <<               \
                                      MAXM86161_LED_RANGE_SHIFT_IR)  \
                                   | (led_range->red <<              \
                                      MAXM86161_LED_RANGE_SHIFT_RED)));

  return SL_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *    Configure led sequence. The data format in the FIFO as well as
 *    the sequencing of exposures are controlled by the LED Sequence
 *    Registers using LEDC1 through LEDC6
 *
 * @param[in] ledsq
 *    Pointer to the led_range struct
 *
 * @return
 *    sl_status_t error code
 *
 ******************************************************************************/
sl_status_t maxm86161_led_sequence_config(maxm86161_ledsq_cfg_t *ledsq)
{
  if ((maxm86161_sequence_check(ledsq->ledsq1)
       & maxm86161_sequence_check(ledsq->ledsq2)
       & maxm86161_sequence_check(ledsq->ledsq3)
       & maxm86161_sequence_check(ledsq->ledsq4)
       & maxm86161_sequence_check(ledsq->ledsq5)
       & maxm86161_sequence_check(ledsq->ledsq6)) != SL_STATUS_OK) {
    return SL_STATUS_FAIL;
  }

  maxm86161_i2c_write_to_register(MAXM86161_REG_LED_SEQ1,
                                  ((ledsq->ledsq2 << MAXM86161_LEDSQ_SHIFT) \
                                   | (ledsq->ledsq1)));

  maxm86161_i2c_write_to_register(MAXM86161_REG_LED_SEQ2,
                                  ((ledsq->ledsq4 << MAXM86161_LEDSQ_SHIFT) \
                                   | (ledsq->ledsq3)));

  maxm86161_i2c_write_to_register(MAXM86161_REG_LED_SEQ3,
                                  ((ledsq->ledsq6 << MAXM86161_LEDSQ_SHIFT) \
                                   | (ledsq->ledsq5)));

  return SL_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *    Configure interrupt at the initial stage
 *
 * @param[in] int_ctrl
 *    Pointer to the interrupt control struct
 *
 * @return
 *    sl_status_t error code
 *
 ******************************************************************************/
sl_status_t maxm86161_interupt_control(maxm86161_int_t *int_ctrl)
{
  if ((maxm86161_bool_check(int_ctrl->sha)
       & maxm86161_bool_check(int_ctrl->proxy)
       & maxm86161_bool_check(int_ctrl->led_compliant)
       & maxm86161_bool_check(int_ctrl->full_fifo)
       & maxm86161_bool_check(int_ctrl->data_rdy)
       & maxm86161_bool_check(int_ctrl->alc_ovf)
       & maxm86161_bool_check(int_ctrl->die_temp)) != SL_STATUS_OK) {
    return SL_STATUS_FAIL;
  }

  maxm86161_i2c_write_to_register(MAXM86161_REG_IRQ_ENABLE1,
                                  ((int_ctrl->full_fifo <<
                                    MAXM86161_INT_SHIFT_FULL)
                                   | (int_ctrl->data_rdy <<
                                      MAXM86161_INT_SHIFT_DATA_RDY)
                                   | (int_ctrl->alc_ovf <<
                                      MAXM86161_INT_SHIFT_ALC_OVF)
                                   | (int_ctrl->proxy <<
                                      MAXM86161_INT_SHIFT_PROXY)
                                   | (int_ctrl->led_compliant <<
                                      MAXM86161_INT_SHIFT_LED_COMPLIANT)
                                   | (int_ctrl->die_temp <<
                                      MAXM86161_INT_SHIFT_DIE_TEMEP)));

  maxm86161_i2c_write_to_register(MAXM86161_REG_IRQ_ENABLE2,
                                  ((int_ctrl->sha << MAXM86161_INT_SHIFT_SHA)));

  return SL_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *    Get status of all Maxim's interrupt
 *
 * @param[in] int_status
 *    Pointer to queue where PPG sample is put
 *
 * @return
 *    None
 *
 ******************************************************************************/
void maxm86161_get_irq_status(maxm86161_int_t *int_status)
{
  uint8_t int_status_value;

  maxm86161_i2c_read_from_register(MAXM86161_REG_IRQ_STATUS1,
                                   &int_status_value);
  int_status->pwr_rdy = ((int_status_value >> MAXM86161_INT_SHIFT_PWR_RDY)
                         & MAXM86161_INT_MASK);
  int_status->die_temp = ((int_status_value >> MAXM86161_INT_SHIFT_DIE_TEMEP)
                          & MAXM86161_INT_MASK);
  int_status->led_compliant = ((int_status_value >>                \
                                MAXM86161_INT_SHIFT_LED_COMPLIANT) \
                               & MAXM86161_INT_MASK);
  int_status->proxy = ((int_status_value >> MAXM86161_INT_SHIFT_PROXY)
                       & MAXM86161_INT_MASK);
  int_status->alc_ovf = ((int_status_value >> MAXM86161_INT_SHIFT_ALC_OVF)
                         & MAXM86161_INT_MASK);
  int_status->data_rdy = ((int_status_value >> MAXM86161_INT_SHIFT_DATA_RDY)
                          & MAXM86161_INT_MASK);
  int_status->full_fifo = ((int_status_value >> MAXM86161_INT_SHIFT_FULL)
                           & MAXM86161_INT_MASK);

  maxm86161_i2c_read_from_register(MAXM86161_REG_IRQ_STATUS2,
                                   &int_status_value);
  int_status->sha = ((int_status_value >> MAXM86161_INT_SHIFT_SHA)
                     & MAXM86161_INT_MASK);
}

/***************************************************************************//**
 * @brief
 *    Process FULL interrupt to get PPG sample and put it into the queue
 *
 * @param[out] queue
 *    Pointer to queue where PPG sample is put
 *
 * @return
 *    None
 *
 ******************************************************************************/
void maxm86161_read_samples_in_fifo(maxm86161_fifo_queue_t *queue)
{
  uint32_t temp_data;
  uint8_t sample_cnt;
  uint8_t block_buf[3 * 128];
  int i = 0;
  maxm86161_fifo_data_t fifo;
  maxm86161_ppg_sample_t sample;
  // we only start to push to queue incase we meet perfect sample
  // ( means PPG1, PPG2, PPG3)
  bool task_started = false;
  bool task_completed = false;

  maxm86161_i2c_read_from_register(MAXM86161_REG_FIFO_DATA_COUNTER,
                                   &sample_cnt);

  // reading one time for all the sample in buffer to prevent
  // the case pushing speed to FIFO > reading speed
  maxm86161_i2c_block_read(MAXM86161_REG_FIFO_DATA, 3 * sample_cnt, block_buf);
  for (i = 0; i < sample_cnt; i++) {
    temp_data = 0x000000;
    temp_data = (block_buf[i * 3 + 0] << 16 | block_buf[i * 3 + 1] << 8
                 | block_buf[i * 3 + 2]);
    fifo.data_val = temp_data & MAXM86161_REG_FIFO_DATA_MASK;
    fifo.tag = (temp_data >> MAXM86161_REG_FIFO_RES)
               & MAXM86161_REG_FIFO_TAG_MASK;

#if (PROX_SELECTION & PROX_USE_IR)
    if (fifo.tag == 1) {
      task_started = true;
      sample.ppg2 = fifo.data_val >> PPG_2_SCALER;
    } else if (fifo.tag == 2) {
      sample.ppg1 = fifo.data_val >> PPG_1_SCALER;
    } else if (fifo.tag == 3) {
      sample.ppg3 = fifo.data_val >> PPG_3_SCALER;
      if (task_started) {
        task_completed = true;
      }
    }
#elif (PROX_SELECTION & PROX_USE_RED)
    if (fifo.tag == 1) {
      task_started = true;
      sample.ppg3 = fifo.data_val >> PPG_3_SCALER;
    } else if (fifo.tag == 2) {
      sample.ppg2 = fifo.data_val >> PPG_2_SCALER;
    } else if (fifo.tag == 3) {
      sample.ppg1 = fifo.data_val >> PPG_1_SCALER;
      if (task_started) {
        task_completed = true;
      }
    }
#else // default use green led for proximity
    if (fifo.tag == 1) {
      task_started = true;
      sample.ppg1 = fifo.data_val >> PPG_1_SCALER;
    } else if (fifo.tag == 2) {
      sample.ppg2 = fifo.data_val >> PPG_2_SCALER;
    } else if (fifo.tag == 3) {
      sample.ppg3 = fifo.data_val >> PPG_3_SCALER;
      if (task_started) {
        task_completed = true;
      }
    }
#endif

    if (task_completed && task_started && queue->located) {
      maxm86161_enqueue_ppg_sample_data(queue, &sample);
      task_completed = false;
      task_started = false;
    }
  }
}

// ---------------------- Queue related functions
//   ---------------------------------

/***************************************************************************//**
 * @brief
 *    Count the number of sample in queue
 *
 * @param[in] queue
 *    Pointer to queue
 *
 * @return
 *    Number of sample in queue
 *
 ******************************************************************************/
uint16_t maxm86161_num_samples_in_queue(maxm86161_fifo_queue_t *queue)
{
  uint16_t samples;
  samples = queue->used / MAXM86161DRV_PPG_SAMPLE_SIZE_BYTES;
  return samples;
}

/***************************************************************************//**
 * @brief
 *    Allocate a fifo queue for PPG maxim data
 *
 * @param[in] queue
 *    Pointer to queue
 *
 * @param[in] queueBuffer
 *    Pointer to buffer to use for fifo queue
 *
 * @param[in] queueSizeInBytes
 *    Queue buffer size in bytes
 *
 * @return
 *    sl_status_t error code
 *
 ******************************************************************************/
sl_status_t maxm86161_allocate_ppg_data_queue(maxm86161_fifo_queue_t *queue,
                                              maxm86161_ppg_sample_t *queueBuffer,
                                              int16_t queueSizeInBytes)
{
  sl_status_t ret = SL_STATUS_OK;
  queue->head = 0;
  queue->tail = 0;
  queue->used = 0;
  queue->located = 1;
  queue->size = queueSizeInBytes;
  queue->fifo = (int8_t *) queueBuffer;
  if (queueSizeInBytes % MAXM86161DRV_PPG_SAMPLE_SIZE_BYTES != 0) {
    ret = SL_STATUS_ALLOCATION_FAILED;
  }
  return ret;
}

/***************************************************************************//**
 * @brief
 *    Put ppg sample to the queue
 *
 * @param[in] queue
 *    Pointer to queue
 *
 * @param[in] sample
 *    Pointer to ppg sample
 *
 * @return
 *    sl_status_t error code
 *
 ******************************************************************************/
sl_status_t maxm86161_enqueue_ppg_sample_data(maxm86161_fifo_queue_t *queue,
                                              maxm86161_ppg_sample_t *sample)
{
  sl_status_t ret = SL_STATUS_OK;
  int8_t buffer[MAXM86161DRV_PPG_SAMPLE_SIZE_BYTES];
  int8_t *buf = buffer;
  int size = MAXM86161DRV_PPG_SAMPLE_SIZE_BYTES;
  int i = 0;

  *(uint32_t *) buf = sample->ppg1;
  buf += 4;
  *(uint32_t *) buf = sample->ppg2;
  buf += 4;
  *(uint32_t *) buf = sample->ppg3;
  buf += 4;
  for (i = 0; i < size; i++) {
    queue->fifo[queue->head] = buffer[i];
    queue->head++;
    if (queue->head == queue->size) {
      queue->head = 0;
    }
  }
  queue->used += size;
  i = queue->tail;
  if (i == queue->head) {
    queue->tail += size;
    queue->used -= size;
    if (queue->tail >= queue->size) {
      queue->tail = queue->tail - queue->size;
    }
    ret = SL_STATUS_FULL;
  }

  return ret;
}

/***************************************************************************//**
 * @brief
 *    Pop a sample from queue
 *
 * @param[in] queue
 *    Pointer to queue
 *
 * @param[in] sample
 *    Pointer to ppg sample
 *
 * @return
 *    sl_status_t error code
 *
 ******************************************************************************/
sl_status_t maxm86161_dequeue_ppg_sample_data(maxm86161_fifo_queue_t *queue,
                                              maxm86161_ppg_sample_t *sample)
{
  int8_t buffer[MAXM86161DRV_PPG_SAMPLE_SIZE_BYTES];
  int8_t *buf = buffer;

  if (queue->used < MAXM86161DRV_PPG_SAMPLE_SIZE_BYTES) {
    buf = 0;
    return SL_STATUS_EMPTY;
  }
  for (int i = 0; i < MAXM86161DRV_PPG_SAMPLE_SIZE_BYTES; i++) {
    buf[i] = queue->fifo[queue->tail];
    queue->tail++;
    if (queue->tail == queue->size) {
      queue->tail = 0;
    }
  }
  queue->used -= MAXM86161DRV_PPG_SAMPLE_SIZE_BYTES;
  sample->ppg1 = *(uint32_t *) buf;
  buf += 4;
  sample->ppg2 = *(uint32_t *) buf;
  buf += 4;
  sample->ppg3 = *(uint32_t *) buf;
  buf += 4;

  return SL_STATUS_OK;
}

/***************************************************************************//**
 * @brief
 *    Clear the Maxm86161 queue
 *
 * @param[in] queue
 *    Pointer to queue
 *
 * @return
 *    None
 *
 ******************************************************************************/
void maxm86161_clear_queue(maxm86161_fifo_queue_t *queue)
{
  queue->head = 0;
  queue->tail = 0;
  queue->used = 0;
}

// --------------------- PRIVATE FUNCTION DEFINITONS -----------------------

// Need to delay to wait device ready after reset
static void maxm86161_soft_reset_delay(void)
{
  sl_udelay_wait(1000);// microsecond
}

static sl_status_t maxm86161_bool_check(uint8_t value)
{
  switch (value) {
    case 0x00:
    case 0x01:
      return SL_STATUS_OK;
    default:
      return SL_STATUS_FAIL;
  }
}

// check if ppg int value is valid
static sl_status_t maxm86161_ppg_tint_check(uint8_t value)
{
  switch (value) {
    case MAXM86161_PPG_CFG_TINT_14p8_US:
    case MAXM86161_PPG_CFG_TINT_29p4_US:
    case MAXM86161_PPG_CFG_TINT_58p7_US:
    case MAXM86161_PPG_CFG_TINT_117p3_US:
      return SL_STATUS_OK;
    default:
      return SL_STATUS_FAIL;
  }
}

// check if led range setting value is valid
static sl_status_t maxm86161_led_range_check(uint8_t value)
{
  switch (value) {
    case MAXM86161_PPG_CFG_LED_RANGE_4k:
    case MAXM86161_PPG_CFG_LED_RANGE_8k:
    case MAXM86161_PPG_CFG_LED_RANGE_16k:
    case MAXM86161_PPG_CFG_LED_RANGE_32k:
      return SL_STATUS_OK;
    default:
      return SL_STATUS_FAIL;
  }
}

// check if sample rate setting value is valid
static sl_status_t maxm86161_smp_rate_check(uint8_t value)
{
  switch (value) {
    case MAXM86161_PPG_CFG_SMP_RATE_P1_24sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P1_50sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P1_84sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P1_99sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P1_199sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P1_399sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P2_24sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P2_50sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P2_84sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P2_99sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P1_8sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P1_16sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P1_32sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P1_64sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P1_128sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P1_256sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P1_512sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P1_1024sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P1_2048sps:
    case MAXM86161_PPG_CFG_SMP_RATE_P1_4096sps:
      return SL_STATUS_OK;
    default:
      return SL_STATUS_FAIL;
  }
}

// check if led range setting value is valid
static sl_status_t maxm86161_smo_freq_check(uint8_t value)
{
  switch (value) {
    case MAXM86161_PPG_CFG_SMP_AVG_1:
    case MAXM86161_PPG_CFG_SMP_AVG_2:
    case MAXM86161_PPG_CFG_SMP_AVG_4:
    case MAXM86161_PPG_CFG_SMP_AVG_8:
    case MAXM86161_PPG_CFG_SMP_AVG_16:
    case MAXM86161_PPG_CFG_SMP_AVG_32:
    case MAXM86161_PPG_CFG_SMP_AVG_64:
    case MAXM86161_PPG_CFG_SMP_AVG_128:
      return SL_STATUS_OK;
    default:
      return SL_STATUS_FAIL;
  }
}

// check if led range current setting value is valid
static sl_status_t maxm86161_led_range_curr_check(uint8_t value)
{
  switch (value) {
    case MAXM86161_LED_RANGE_CURRENT_31_MA:
    case MAXM86161_LED_RANGE_CURRENT_62_MA:
    case MAXM86161_LED_RANGE_CURRENT_93_MA:
    case MAXM86161_LED_RANGE_CURRENT_124_MA:
      return SL_STATUS_OK;
    default:
      return SL_STATUS_FAIL;
  }
}

// check if sequence value is valid
static sl_status_t maxm86161_sequence_check(uint8_t value)
{
  switch (value) {
    case MAXM86161_LEDSQ_GREEN:
    case MAXM86161_LEDSQ_IR:
    case MAXM86161_LEDSQ_RED:
    case MAXM86161_LEDSQ_PILOT_LED1:
    case MAXM86161_LEDSQ_DIRECT_AMBIENT:
    case MAXM86161_LEDSQ_OFF:
      return SL_STATUS_OK;
    default:
      return SL_STATUS_FAIL;
  }
}
