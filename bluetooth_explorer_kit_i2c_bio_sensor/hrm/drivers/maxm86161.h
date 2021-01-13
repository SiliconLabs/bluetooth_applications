/***************************************************************************//**
* @file maxm86161.h
* @brief Header file of maxm86161 biometric sensor driver
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
* This code has been minimally tested to ensure that it builds with the specified
* dependency versions and is suitable as a demonstration for evaluation purposes only.
* This code will be maintained at the sole discretion of Silicon Labs.
*
******************************************************************************/

#ifndef MAXM86161_H_
#define MAXM86161_H_

/***************************************************************************//**
 * @addtogroup Drivers
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup Maxm86161
 * @{
 ******************************************************************************/

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 *******************************   TYPES   ***********************************
 ******************************************************************************/
typedef void* HANDLE;

#include "maxm86161_i2c.h"
#include "sl_status.h"
#include "sl_udelay.h"


/*******************************************************************************
 ***************   Functions found in maxm86161_sys_out.c     ********************
 ******************************************************************************/
int32_t maxm86161_i2c_write_to_register( uint8_t address, uint8_t value);
int32_t maxm86161_i2c_read_from_register(uint8_t address);
int32_t maxm86161_i2c_block_write(uint8_t address, uint8_t length, uint8_t const *values);
int32_t maxm86161_i2c_block_read(uint8_t address, uint16_t length, uint8_t* values);

/*******************************************************************************
 *******    max86161 Register and Parameter Bit Definitions  *********************
 ******************************************************************************/

#define MAXM86161_FIFO_CFG_2_FLUSH_FIFO                 0x10
#define MAXM86161_FIFO_CFG_2_FIFO_READ_DATA_CLR         0x08
#define MAXM86161_FIFO_CFG_2_FIFO_STAT_DATA_CLR         0x00
#define MAXM86161_FIFO_CFG_2_FULL_TYPE_RPT              0x00
#define MAXM86161_FIFO_CFG_2_FULL_TYPE_ONCE             0x04
#define MAXM86161_FIFO_CFG_2_FIFO_ROLL_OVER             0x02
#define MAXM86161_FIFO_CFG_2_FIFO_STOP                  0x00

#define MAXM86161_INT_ENABLE                             0x01
#define MAXM86161_INT_DISABLE                            0x00

#define MAXM86161_PPG_CFG_ALC_EN                         0x01
#define MAXM86161_PPG_CFG_ALC_DS                         0x00

#define MAXM86161_PPG_CFG_OFFSET_ADD                     0x01
#define MAXM86161_PPG_CFG_OFFSET_NO                      0x00

enum maxm86161_ppg_cfg_tint{
  MAXM86161_PPG_CFG_TINT_14p8_US  = 0x00,
  MAXM86161_PPG_CFG_TINT_29p4_US  = 0x01,
  MAXM86161_PPG_CFG_TINT_58p7_US  = 0x02,
  MAXM86161_PPG_CFG_TINT_117p3_US = 0x03,
};

enum maxm86161_ppg_cfg_led_range{
  MAXM86161_PPG_CFG_LED_RANGE_4k  = 0x00,
  MAXM86161_PPG_CFG_LED_RANGE_8k  = 0x01,
  MAXM86161_PPG_CFG_LED_RANGE_16k = 0x02,
  MAXM86161_PPG_CFG_LED_RANGE_32k = 0x03,
};

enum maxm86161_ppg_cfg_sample_rate {
  MAXM86161_PPG_CFG_SMP_RATE_P1_24sps   = 0x00,
  MAXM86161_PPG_CFG_SMP_RATE_P1_50sps   = 0x01,
  MAXM86161_PPG_CFG_SMP_RATE_P1_84sps   = 0x02,
  MAXM86161_PPG_CFG_SMP_RATE_P1_99sps   = 0x03,
  MAXM86161_PPG_CFG_SMP_RATE_P1_199sps  = 0x04,
  MAXM86161_PPG_CFG_SMP_RATE_P1_399sps  = 0x05,
  MAXM86161_PPG_CFG_SMP_RATE_P2_24sps   = 0x06,
  MAXM86161_PPG_CFG_SMP_RATE_P2_50sps   = 0x07,
  MAXM86161_PPG_CFG_SMP_RATE_P2_84sps   = 0x08,
  MAXM86161_PPG_CFG_SMP_RATE_P2_99sps   = 0x09,
  MAXM86161_PPG_CFG_SMP_RATE_P1_8sps    = 0x0A,
  MAXM86161_PPG_CFG_SMP_RATE_P1_16sps   = 0x0B,
  MAXM86161_PPG_CFG_SMP_RATE_P1_32sps   = 0x0C,
  MAXM86161_PPG_CFG_SMP_RATE_P1_64sps   = 0x0D,
  MAXM86161_PPG_CFG_SMP_RATE_P1_128sps  = 0x0E,
  MAXM86161_PPG_CFG_SMP_RATE_P1_256sps  = 0x0F,
  MAXM86161_PPG_CFG_SMP_RATE_P1_512sps  = 0x10,
  MAXM86161_PPG_CFG_SMP_RATE_P1_1024sps = 0x11,
  MAXM86161_PPG_CFG_SMP_RATE_P1_2048sps = 0x12,
  MAXM86161_PPG_CFG_SMP_RATE_P1_4096sps = 0x13
};

enum maxm86161_ppg_cfg_sample_avarage {
  MAXM86161_PPG_CFG_SMP_AVG_1   = 0x00,
  MAXM86161_PPG_CFG_SMP_AVG_2   = 0x01,
  MAXM86161_PPG_CFG_SMP_AVG_4   = 0x02,
  MAXM86161_PPG_CFG_SMP_AVG_8   = 0x03,
  MAXM86161_PPG_CFG_SMP_AVG_16  = 0x04,
  MAXM86161_PPG_CFG_SMP_AVG_32  = 0x05,
  MAXM86161_PPG_CFG_SMP_AVG_64  = 0x06,
  MAXM86161_PPG_CFG_SMP_AVG_128 = 0x07,
};

enum maxm86161_led_current {
  MAXM86161_LED_RANGE_CURRENT_31_MA  = 0x00,
  MAXM86161_LED_RANGE_CURRENT_62_MA  = 0x01,
  MAXM86161_LED_RANGE_CURRENT_93_MA  = 0x02,
  MAXM86161_LED_RANGE_CURRENT_124_MA = 0x03,
};

enum heart_rate_led {
  HEARTRATE2_LED_1 = 0,
  HEARTRATE2_LED_2 = 1,
  HEARTRATE2_LED_3 = 2,
};

enum maxm86161_ledsq_setting {
  MAXM86161_LEDSQ_OFF            = 0x0,
  MAXM86161_LEDSQ_GREEN          = 0x1,
  MAXM86161_LEDSQ_IR             = 0x2,
  MAXM86161_LEDSQ_RED            = 0x3,
  MAXM86161_LEDSQ_PILOT_LED1     = 0x8,
  MAXM86161_LEDSQ_DIRECT_AMBIENT = 0x9,
};

#define MAXM86161_PIN_HIGH                               1
#define MAXM86161_PIN_LOW                                0

//Cfg shift data
#define MAXM86161_PPG_CFG_ALC                                  7
#define MAXM86161_PPG_CFG_OFFSET                               6
#define MAXM86161_PPG_CFG_ADC_RANGE                            2
#define MAXM86161_PPG_CFG_TINT                                 0
#define MAXM86161_PPG_CFG_SMP_RATE                             3
#define MAXM86161_PPG_CFG_SMP_AVG                              0

//Led range shift data
#define MAXM86161_LED_RANGE_SHIFT_GREEN                        0
#define MAXM86161_LED_RANGE_SHIFT_IR                           2
#define MAXM86161_LED_RANGE_SHIFT_RED                          4

//Led sequence shift
#define MAXM86161_LEDSQ_SHIFT                                  4

//Int shift data and int mask
#define MAXM86161_INT_SHIFT_FULL                               7
#define MAXM86161_INT_SHIFT_DATA_RDY                           6
#define MAXM86161_INT_SHIFT_ALC_OVF                            5
#define MAXM86161_INT_SHIFT_PROXY                              4
#define MAXM86161_INT_SHIFT_LED_COMPLIANT                      3
#define MAXM86161_INT_SHIFT_DIE_TEMEP                          2
#define MAXM86161_INT_SHIFT_PWR_RDY                            0
#define MAXM86161_INT_SHIFT_SHA                                0
#define MAXM86161_INT_MASK                                     0x01


#define MAXM86161_SYS_CTRL_SW_RESET                      0x01
#define MAXM86161_SYS_CTRL_SHUT_DOWN                     0x02
#define MAXM86161_SYS_CTRL_POWER_ON                      0x00
#define MAXM86161_SYS_CTRL_LOW_PWR_MODE                  0x04
#define MAXM86161_SYS_CTRL_SINGLE_PPG                    0x08
#define MAXM86161_SYS_CTRL_DUAL_PPG                      0x00

#define MAXM86161_INT_1_FULL                            0x80
#define MAXM86161_INT_1_NOT_FULL                        0x00
#define MAXM86161_INT_1_DATA_RDY                        0x40
#define MAXM86161_INT_1_DATA_NOT_RDY                    0x00
#define MAXM86161_INT_1_ALC_OVERFLOW                    0x20
#define MAXM86161_INT_1_ALC_NORMAL                      0x00
#define MAXM86161_INT_1_PROXIMITY_INT                   0x10
#define MAXM86161_INT_1_PROXIMITY_NORMAL                0x00
#define MAXM86161_INT_1_LED_COMPLIANT                   0x08
#define MAXM86161_INT_1_LED_NOT_COMPLIANT               0x00
#define MAXM86161_INT_1_DIE_TEMP_RDY                    0x04
#define MAXM86161_INT_1_DIE_TEMP_NOT_RDY                0x00
#define MAXM86161_INT_1_PWR_RDY                         0x01
#define MAXM86161_INT_1_PWR_NOT_RDY                     0x00


/**
 * @brief Structure for reading fifo data
 */
typedef struct
{
  uint32_t data_val; // PPG value - 19 bits
  uint8_t  tag; // 5-bits to determine which LED it belong
} maxm86161_fifo_data_t;

/// @endcond
/// Raw data FIFO queue typedef
typedef struct {
  volatile uint16_t head;        ///< Index of next byte to get.
  volatile uint16_t tail;        ///< Index of where to enqueue next byte.
  volatile uint16_t used;        ///< Number of bytes queued.
  uint16_t size;                 ///< Size of FIFO.
  uint8_t located;
  int8_t *fifo;                  ///< Pointer to FIFO of queue data (allocated by user)
} maxm86161_fifo_queue_t;

typedef struct {
  uint32_t ppg1;        ///< ppg1 sample data
  uint32_t ppg2;        ///< ppg2 sample data
  uint32_t ppg3;        ///< ppg3 sample data
} maxm86161_ppg_sample_t;

typedef struct {
  maxm86161_fifo_queue_t queue;
  uint8_t sampleSize;
} maxm86161_fifo_queue_config_t;

#define MAXM86161DRV_PPG_SAMPLE_SIZE_BYTES 12

/**
 * @brief Structure configurating ppg
 */
typedef struct
{
  uint8_t alc;        ///< Enable/disable ALC
  uint8_t offset;     ///< Enable/disable dark current measurement
  uint8_t ppg_tint;   ///< Set the pulse width of the LED drivers and the integration time of PPG ADC
  uint8_t adc_range;  ///< Set the ADC range of the sensor
  uint8_t smp_rate;   ///< Set the number of exposure per sample
  uint8_t smp_freq;   ///< Set the effective sampling rate of the PPG sensor
} maxm86161_ppg_cfg_t;

/**
 * @brief Structure for interrupt status
 */
typedef struct
{
  uint8_t full_fifo;      ///< The FIFO buffer is full
  uint8_t data_rdy;       ///< New data in the FIFO
  uint8_t alc_ovf;        ///< The ambient light cancellation function of the photodiode
  uint8_t proxy;          ///< Proximity mode
  uint8_t led_compliant;  ///< LED is not compliant
  uint8_t die_temp;       ///< The TEMP ADC has finished its current conversion
  uint8_t pwr_rdy;        ///< VBATT went below the UVLO threshold
  uint8_t sha;            ///< SHA Authentication done
} maxm86161_int_t;

/**
 * @brief Structure led brightness
 */
typedef struct
{
  uint8_t green;  ///<set the nominal drive current of green LED
  uint8_t ir;     ///<set the nominal drive current of ir LED
  uint8_t red;    ///<set the nominal drive current of red LED
} maxm86161_ledpa_t;

/**
 * @brief Structure configurating led sequence
 */
typedef struct
{
  uint8_t ledsq1; ///<set the data type for LED Sequence 1 of the FIFO
  uint8_t ledsq2; ///<set the data type for LED Sequence 2 of the FIFO
  uint8_t ledsq3; ///<set the data type for LED Sequence 3 of the FIFO
  uint8_t ledsq4; ///<set the data type for LED Sequence 4 of the FIFO
  uint8_t ledsq5; ///<set the data type for LED Sequence 5 of the FIFO
  uint8_t ledsq6; ///<set the data type for LED Sequence 6 of the FIFO
} maxm86161_ledsq_cfg_t;

/**
 * @brief Structure setting led current range
 */
typedef struct
{
  uint8_t green;  ///<Range selection of the green LED current
  uint8_t ir;     ///<Range selection of the ir LED current
  uint8_t red;    ///<Range selection of the red LED current
} maxm86161_led_range_curr_t;

/**
 * @brief Structure setting led current range
 */
typedef struct maxm86161_device_config
{
  uint8_t int_level;
  maxm86161_ledsq_cfg_t ledsq_cfg;
  maxm86161_ledpa_t ledpa_cfg;
  maxm86161_ppg_cfg_t ppg_cfg;
  maxm86161_int_t int_cfg;
} maxm86161_device_config_t;


/*******************************************************************************
 ***************   Functions supplied by maxm86161drv.c   ******************
 ******************************************************************************/
void maxm86161_get_irq_status(maxm86161_int_t *int_status);
void maxm86161_flush_fifo();
void maxm86161_led_pa_config(maxm86161_ledpa_t *ledpa);
void maxm86161_set_int_level(uint8_t level);
void maxm86161_software_reset();
void maxm86161_shutdown_device(bool turn_off);
sl_status_t maxm86161_led_pa_config_specific(uint8_t ledx, uint8_t value);
sl_status_t maxm86161_led_range_config(maxm86161_led_range_curr_t *led_range);
sl_status_t maxm86161_led_sequence_config(maxm86161_ledsq_cfg_t *ledsq);
sl_status_t maxm86161_interupt_control(maxm86161_int_t *int_ctrl);
sl_status_t maxm86161_ppg_config(maxm86161_ppg_cfg_t *ppg_cfg);
sl_status_t maxm86161_set_prox_threshold();
sl_status_t maxm86161_init_device(maxm86161_device_config_t global_cfg);
int32_t maxm86161_read_fifo(maxm86161_fifo_queue_t *queue);

/*******************************************************************************
 ************************** Maxm86161 I2C Registers *******************************
 ******************************************************************************/
/// @cond DOXYGEN_SHOULD_SKIP_THIS

//Status group
#define MAXM86161_REG_IRQ_STATUS1		0x00
#define MAXM86161_REG_IRQ_STATUS2		0x01
#define MAXM86161_REG_IRQ_ENABLE1		0x02
#define MAXM86161_REG_IRQ_ENABLE2		0x03

//FIFO group
#define MAXM86161_REG_FIFO_WRITE_PTR	  0x04
#define MAXM86161_REG_FIFO_READ_PTR		  0x05
#define MAXM86161_REG_OVF_COUNTER			  0x06
#define MAXM86161_REG_FIFO_DATA_COUNTER	0x07
#define MAXM86161_REG_FIFO_DATA			    0x08
#define MAXM86161_REG_FIFO_CONFIG1		  0x09
#define MAXM86161_REG_FIFO_CONFIG2		  0x0A

//System
#define MAXM86161_REG_SYSTEM_CONTROL		0x0D

//PPG configuration
#define MAXM86161_REG_PPG_SYNC_CONTROL	  0x10
#define MAXM86161_REG_PPG_CONFIG1			    0x11
#define MAXM86161_REG_PPG_CONFIG2			    0x12
#define MAXM86161_REG_PPG_CONFIG3			    0x13
#define MAXM86161_REG_PROX_INT_THRESHOLD	0x14
#define MAXM86161_REG_PD_BIAS				      0x15

//PPG picket fence detect and replace
#define MAXM86161_REG_PICKET_FENCE	0x16

//LEd sequence control
#define MAXM86161_REG_LED_SEQ1		0x20
#define MAXM86161_REG_LED_SEQ2		0x21
#define MAXM86161_REG_LED_SEQ3		0x22

//LED pulse amplitude
#define MAXM86161_REG_LED1_PA			  0x23
#define MAXM86161_REG_LED2_PA			  0x24
#define MAXM86161_REG_LED3_PA			  0x25
#define MAXM86161_REG_LED_PILOT_PA	0x29
#define MAXM86161_REG_LED_RANGE1		0x2A

//PPG1_HI_RES_DAC
#define MAXM86161_REG_S1_HI_RES_DAC1	0x2C
#define MAXM86161_REG_S2_HI_RES_DAC1	0x2D
#define MAXM86161_REG_S3_HI_RES_DAC1	0x2E
#define MAXM86161_REG_S4_HI_RES_DAC1	0x2D
#define MAXM86161_REG_S5_HI_RES_DAC1	0x30
#define MAXM86161_REG_S6_HI_RES_DAC1	0x31

//Die temperature
#define MAXM86161_REG_DIE_TEMP_CONFIG		0x40
#define MAXM86161_REG_DIE_TEMP_INT		  0x41
#define MAXM86161_REG_DIE_TEMP_FRACTION	0x42

//DAC calibration
#define MAXM86161_REG_DAC_CALIB_ENABLE	0x50

//SHA256
#define MAXM86161_REG_SHA_CMD		  0xF0
#define MAXM86161_REG_SHA_CONFIG	0xF1

//Memory
#define MAXM86161_REG_MEMORY_CONTROL	0xF2
#define MAXM86161_REG_MEMORY_INDEX	  0xF3
#define MAXM86161_REG_MEMORY_DATA		  0xF4

// Part ID
#define MAXM86161_REG_REV_ID		0xFE
#define MAXM86161_REG_PART_ID		0xFF

#define MAXM86161_REG_FIFO_DATA_MASK  0x07FFFF
#define MAXM86161_REG_FIFO_RES        19
#define MAXM86161_REG_FIFO_TAG_MASK   0x1F

/***************************************************************************//**
 * Data Fifo Queue Functions
 ******************************************************************************/
void maxm86161_clear_queue(maxm86161_fifo_queue_t *queue);
uint16_t maxm86161_num_samples_in_queue(maxm86161_fifo_queue_t *queue);
sl_status_t maxm86161_enqueue_ppg_sample_data (maxm86161_fifo_queue_t *queue, maxm86161_ppg_sample_t *sample);
sl_status_t maxm86161_dequeue_ppg_sample_data (maxm86161_fifo_queue_t *queue, maxm86161_ppg_sample_t *sample);
sl_status_t maxm86161_allocate_ppg_data_queue( maxm86161_fifo_queue_t *queue, maxm86161_ppg_sample_t *queueBuffer, int16_t queueSizeInBytes);

/***************************************************************************//**
 * Fifo/Interrupt Processing functions
 ******************************************************************************/
void maxm86161_read_samples_in_fifo(maxm86161_fifo_queue_t *queue);

#endif




