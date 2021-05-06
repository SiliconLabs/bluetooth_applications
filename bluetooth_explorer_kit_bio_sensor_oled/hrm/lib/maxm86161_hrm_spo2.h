/***************************************************************************//**
* @file maxm86161_hrm_spo2.h
* @brief Header file of Maxm86161 HRM/SPO2 algorithm
* @version 1.1.0
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
* This code has not been formally tested and is provided as-is.
* It is not suitable for production environments.
* This code will not be maintained.
*
******************************************************************************/

#ifndef MAXM86161_HRM_SPO2_H__

#define MAXM86161_HRM_SPO2_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <maxm86161.h>

/***************************************************************************//**
 **************      HRM/SpO2 Algorithm Defines    *****************************
 ******************************************************************************/
#define FS_25HZ                                         800
#define MAXM86161_HRM_SUCCESS                           0
#define MAXM86161_HRM_ERROR_RESERVED                    -1
#define MAXM86161_HRM_ERROR_INVALID_MEASUREMENT_RATE    -2
#define MAXM86161_HRM_ERROR_SAMPLE_QUEUE_EMPTY          -3
#define MAXM86161_HRM_ERROR_INVALID_PART_ID             -4
#define MAXM86161_HRM_ERROR_FUNCTION_NOT_SUPPORTED      -5
#define MAXM86161_HRM_ERROR_BAD_POINTER                 -6
#define MAXM86161_HRM_ERROR_DEBUG_DISABLED              -7
#define MAXM86161_HRM_ERROR_NON_SUPPORTED_PART_ID       -8  //The feature is not support by the part
#define MAXM86161_HRM_ERROR_INVALID_SAMPLE_DATA         -9
#define MAXM86161_HRM_ERROR_PARAM1_OUT_OF_RANGE         -10
#define MAXM86161_HRM_ERROR_PARAM2_OUT_OF_RANGE         (MAXM86161_HRM_ERROR_PARAM0_OUT_OF_RANGE - 1)
#define MAXM86161_HRM_ERROR_PARAM3_OUT_OF_RANGE         (MAXM86161_HRM_ERROR_PARAM0_OUT_OF_RANGE - 2)
#define MAXM86161_HRM_ERROR_PARAM4_OUT_OF_RANGE         (MAXM86161_HRM_ERROR_PARAM0_OUT_OF_RANGE - 3)
#define MAXM86161_HRM_ERROR_PARAM5_OUT_OF_RANGE         (MAXM86161_HRM_ERROR_PARAM0_OUT_OF_RANGE - 4)
#define MAXM86161_HRM_ERROR_PARAM6_OUT_OF_RANGE         (MAXM86161_HRM_ERROR_PARAM0_OUT_OF_RANGE - 5)
#define MAXM86161_HRM_ERROR_PARAM7_OUT_OF_RANGE         (MAXM86161_HRM_ERROR_PARAM0_OUT_OF_RANGE - 6)
#define MAXM86161_HRM_ERROR_PARAM8_OUT_OF_RANGE         (MAXM86161_HRM_ERROR_PARAM0_OUT_OF_RANGE - 7)
#define MAXM86161_HRM_ERROR_PARAM9_OUT_OF_RANGE         (MAXM86161_HRM_ERROR_PARAM0_OUT_OF_RANGE - 8)
#define MAXM86161_HRM_ERROR_PARAM10_OUT_OF_RANGE        (MAXM86161_HRM_ERROR_PARAM0_OUT_OF_RANGE - 9)

// The HRM status values are a bit field. More than one status can be 'on' at any given time.
#define MAXM86161_HRM_STATUS_SUCCESS                    0
#define MAXM86161_HRM_STATUS_FINGER_OFF                 (1<<0)
#define MAXM86161_HRM_STATUS_FINGER_ON                  (1<<1)
#define MAXM86161_HRM_STATUS_ZERO_CROSSING_INVALID      (1<<2)
#define MAXM86161_HRM_STATUS_BPF_PS_VPP_OFF_RANGE       (1<<3)
#define MAXM86161_HRM_STATUS_AUTO_CORR_TOO_LOW          (1<<4)
#define MAXM86161_HRM_STATUS_CREST_FACTOR_TOO_HIGH      (1<<5)
#define MAXM86161_HRM_STATUS_FRAME_PROCESSED            (1<<6)
#define MAXM86161_HRM_STATUS_AUTO_CORR_MAX_INVALID      (1<<7)
#define MAXM86161_HRM_STATUS_HRM_MASK                   0x00ff  // Include all HRM bits
#define SPO2_STATUS_PROCESS_SPO2_FRAME                  1   // Used to inform to process the SpO2 frame.
#define MAXM86161_HRM_STATUS_SPO2_FINGER_OFF            (1<<8)
#define MAXM86161_HRM_STATUS_SPO2_FINGER_ON             (1<<9)
#define MAXM86161_HRM_STATUS_SPO2_CREST_FACTOR_OFF      (1<<10)
#define MAXM86161_HRM_STATUS_SPO2_TOO_LOW_AC            (1<<11)
#define MAXM86161_HRM_STATUS_SPO2_TOO_HIGH_AC           (1<<12)
#define MAXM86161_HRM_STATUS_SPO2_EXCEPTION             (1<<13)
#define MAXM86161_HRM_STATUS_SPO2_MASK                  0xff00  // Include all HRM bits

#define SIHRM_USE_DYNAMIC_DATA_STRUCTURE  0

#define SIHRM_ALGORITHM_STATUS_CONTROL_FLAG_VALID_PART      0x1
#define SIHRM_ALGORITHM_STATUS_CONTROL_FLAG_DEBUG_ENABLED   0x2
#define SIHRM_ALGORITHM_STATUS_CONTROL_FLAG_FIFO_TEST_MODE  0x4

#define MAXM86161_HRM_VERSION "1.0.0"   //this should never be more than 32 characters per the API reference guide

#define DC_SENSING_DECISION_CNT     255
#define DC_SENSING_AGC_START_CNT    DC_SENSING_DECISION_CNT+10

#define MAXM86161_HRM_TRUE          1
#define MAXM86161_HRM_FALSE         0

#define QF_SCALER                   8   // Q-format for Normalization_Scaler
#define FS_ESTIMATE_DURATION        60  // (sample)
#define FS_UPDATE_RATE              30  // (%)

//#define ZR_BIAS_SCALE             (+1/4)  // %butter BPF, BPF(PS) biases to positive side
#define ZR_BIAS_SCALE               (-1/4)  // %cheby2 BPF, BPF(PS) biases to negative side
#define ZC_HR_TOLERANCE             25

#define HRM_PS_DC_REFERENCE               15000 // Used for the auto HRM PS AC scaler based on the 1s-averaged PS DC.
#define HRM_PS_VPP_MAX_GREEN_FINGERTIP    HRM_PS_DC_REFERENCE*30/300  // PI=20.0% if positive and negative AC amplitudes are same.

#define HRM_CREST_FACTOR_THRESH_FINGERTIP 15

/* if PS raw value is greater than this threshold the algorithm believes that
 * skin contact is detected */
#define HRM_PS_RAW_SKIN_CONTACT_THRESHOLD   300
#define HRM_DC_SENSING_SKIN_CONTACT_THRESH  HRM_PS_RAW_SKIN_CONTACT_THRESHOLD*10/10
#define HRM_DC_SENSING_WORK_LEVEL           25000
#define HRM_PI_MIN_THRESHOLD                20  // 20 means PI=0.2%. If PI is below the threshold,
                                                //the HRM AGC increases the LED current by 1 notch, up to 0x0f.

#define HRM_AMBIENT_BLOCKED_THRESHOLD     500

#define HRM_DC_SENSING_START              0x00  // DC sensing scheme is looking for the next DC sensing.
#define HRM_DC_SENSING_RESTART            0x01  // Restart DC sensing.
#define HRM_DC_SENSING_CHANGE_PARAMETERS  0x02  // Stop the PPG measurement and change the configuration. Restart the PPG measurement.
#define HRM_DC_SENSING_SENSE_FINISH       0x04  // Finish the DC measurement and select the proper current for the HRM operation.
#define HRM_DC_SENSING_SEEK_NO_CONTACT    0x10  // Monitor to see if PS is below the finger-off threshold. Restore Si117x current and set HRM_DC_Sensing_Flag=HRM_DC_SENSING_START for the next DC sensing.
#define HRM_DC_SENSING_IN_PROGRESS        0x08

#define INTERPOLATOR_L                      4

#define HRM_BPF_ACTIVE                    0x01  //BPF_Active_Flag bit
#define SpO2_RED_BPF_ACTIVE               0x02  //BPF_Active_Flag bit
#define SpO2_IR_BPF_ACTIVE                0x04  //BPF_Active_Flag bit

#define HRM_PS_AGC_DELAY_WINDOW           50      //(samples)

#define SOS_SCALE_SHIFT                   16
#define SCALE_I_SHIFT                     5
#define SCALE_F_SHIFT                     8

#define SQRT_SHIFT                        16
#define R_SCALER                          128

#define SPO2_REFLECTIVE_MODE              1 // 1--REFLECTIVE MODE, 0--TRANSMISSIVE MODE

// Do not run PS AGC
#define HRM_PS_AGC                        1   // 1--Increase LED current during valid HR if PS is below the work level or PI is below 0.2%(could occur on wrist).
#define HRM_DATA_LOG                      0 // Debugging: 1--Dump out test data, such as PS, the normalized PS1 and BPF(PS1).

#define HRM_PS_AGC_DISABLED               0x00
#define HRM_PS_AGC_ENABLED                0x01
#define HRM_PS_AGC_GAIN_CHANGED           0x02
#define HRM_PS_AGC_NO_TOUCH               0x04

#define HRM_DC_WORKING_LEVEL              25000 // thanhnd21
#define HRM_NUM_SAMPLES_IN_FRAME          256  //10 second frame - used by the AGC

// BPF parameters
#define BPF_BIQUAD_STAGE_MAX              3   // Coefficients of all BPFs below have 3 stages.

#define MAX_FRAME_SAMPLES                             7*95   // Max Fs=95Hz
#define MAXM86161_HRM_ENABLE_MEASUREMENT_RATE_25Hz    0
#define MAXM86161_HRM_ENABLE_MEASUREMENT_RATE_60Hz    0
#define MAXM86161_HRM_ENABLE_MEASUREMENT_RATE_95Hz    1
#define MAXM86161_HRM_ENABLE_MEASUREMENT_RATE_185Hz   0
#define MAXM86161_HRM_ENABLE_MEASUREMENT_RATE_229Hz   0
#define MAXM86161_HRM_ENABLE_MEASUREMENT_RATE_430Hz   0
#define MAXM86161_HRM_USE_DYNAMIC_DATA_STRUCTURE      0

#define MAXM86161_HRM_MAX_INTERPOLATION_FACTOR        9

#define MAXM86161_HRM_BUILD_GECKO_SPO2                1


/**************************************************************************//**
 ******************    HRM/SpO2 Algorithm Types     ***************************
 *****************************************************************************/
typedef float sihrmFloat_t;

typedef void *HANDLE;

/**
 * Data structure for BPF
 */
typedef struct maxm86161_bpf
{
  int32_t x[BPF_BIQUAD_STAGE_MAX][3]; // BPF data buffer
  int32_t yi[BPF_BIQUAD_STAGE_MAX][3];  // BPF data buffer
  int32_t yf[BPF_BIQUAD_STAGE_MAX][3];  // BPF data buffer
} maxm86161_bpf_t;

/**
 * Data structure for Si117x HRM AGC scheme
 */
typedef struct mamx86161_hrm_agc
{
  uint32_t raw_ppg_sum;
  uint32_t raw_ppg_count;
  uint16_t led_current_value;
  uint16_t agc_flag;
  uint16_t saved_ppg;
} mamx86161_hrm_agc_t;

/**
 * Data structure for Si117x SpO2 algorithm handler
 */
typedef struct maxm86161_spo2_handle
{
  maxm86161_bpf_t spo2_red_bpf, spo2_ir_bpf;  //SpO2_Red_BPF   SpO2_IR_BPF BPF struct for SpO2_Red and SpO2_IR

  int16_t spo2_red_ac_sample_buffer[MAX_FRAME_SAMPLES];// Data buffer for the framed samples that can be processed in a background job.
  int16_t spo2_ir_ac_sample_buffer[MAX_FRAME_SAMPLES]; // Data buffer for the framed samples that can be processed in a background job.
  uint16_t spo2_red_dc_sample_buffer[MAX_FRAME_SAMPLES]; // Data buffer for the framed samples that can be processed in a background job.
  uint16_t spo2_ir_dc_sample_buffer[MAX_FRAME_SAMPLES]; // Data buffer for the framed samples that can be processed in a background job.

  /* Use each of 4 SpO2 frame sample buffers as a circular buffer,
   * so that SpO2 sample process adds samples to it and SpO2 frame
   * process outputs samples from it. */
  int16_t spo2_buffer_in_index; // Input pointer to the frame sample circular buffer.
  int16_t spo2_percent;// SpO2(%) and the debug message
  int16_t spo2_raw_level_count;// Used for Finger On/Off validation
  uint16_t spo2_red_interpolator_buf[INTERPOLATOR_L*2], spo2_ir_interpolator_buf[INTERPOLATOR_L*2];//SpO2_Red_interpolator_buf SpO2_IR_interpolator_buf

  //These were local static vars in V0.6.2:
  uint16_t spo2_red_bpf_ripple_count;// SpO2 BPF-ripple reduction variable
  uint16_t spo2_red_ps_input_old;// SpO2 BPF-ripple reduction variable
  uint16_t spo2_ir_bpf_ripple_count;// SpO2 BPF-ripple reduction variable
  uint16_t spo2_ir_ps_input_old;// SpO2 BPF-ripple reduction variable
  uint16_t time_since_finger_off;// (sample)
} maxm86161_spo2_handle_t;

/**
 * Data structure for maxim86161 HRM algorithm handler
 */
typedef struct maxm_hrm_handle
{
  maxm86161_spo2_handle_t *spo2;
  maxm86161_device_config_t *device_config;

  uint8_t hrm_interpolator_factor; //HRM_Interpolator_Factor
  uint16_t hrm_interpolator_buf[INTERPOLATOR_L*2];
  int16_t *phrm_interpolator_coefs;
  int32_t flag_samples;

  int16_t hrm_sample_buffer[MAX_FRAME_SAMPLES];// Data buffer for the framed samples that can be processed in a background j
  int16_t hrm_buffer_in_index;// Input index to the frame sample circular buffer.
  int16_t fs;// (Hz)PS sampling rate
  int16_t bpf_biquads, bpf_output_scaler;//BPF_biquads, BPF_output_scaler
  int32_t *pbpf_b_0;
  maxm86161_bpf_t hrm_bpf;// BPF struct for HRM
  int16_t hr_update_interval;// HRM_Inits set to 1(s)*Fs
  int16_t t_low0, t_high0;// (sample), times for min and max heart rates.
  int16_t hr_iframe;// (sample), Frame length.
  int16_t num_of_lowest_hr_cycles;// # of lowest-HR cycle. The longer the frame scale is, more accurate the heart rate is.

  int16_t measurement_rate;
  uint16_t hrm_normalization_scaler;// Normalization scaler for HRM PS based on ADC gain, LED current and etc.
  uint16_t red_normalization_scaler;// Normalization scaler for SpO2 Red PS based on ADC gain, LED current and etc.
  uint16_t ir_normalization_scaler;// Normalization scaler for SpO2 IR PS based on ADC gain, LED current and etc.
  int16_t sample_count;
  int16_t hrm_ps_raw_level_count;
  uint16_t hrm_raw_ps;
  uint16_t normalized_hrm_ps;
  uint8_t hrm_ps_select;// 0=PPG1, 1=PPG2, 2=PPG3
  uint8_t spo2_ir_ps_select;// 0=PPG1, 1=PPG2, 2=PPG3
  uint8_t spo2_red_ps_select;// 0=PPG1, 1=PPG2, 2=PPG3
  int16_t hrm_ps_vpp_max;
  int16_t hrm_ps_vpp_min;
  int16_t hrm_ps_crestfactor_thresh;

  uint16_t hrm_dc_sensing_count;// Used in HRM DC sensing
  uint16_t hrm_raw_ps_old;
  uint16_t hrm_dc_sensing_flag;// Used in HRM DC sensing
  uint8_t hrm_dc_change_int_level;
  uint16_t ppg_led_local;// Used in HRM DC sensing
  uint8_t dc_sensing_finish[3];
  //uint32_t dc_sensing_level[3][256];// DC-sensing levels for PS(n) for n=1:2:63 LED currents with configured ADCgain(n), n=1,2,3,4.
  uint16_t dc_sensing_led[3];// Used in HRM DC sensing
  mamx86161_hrm_agc_t hrm_agc[3];// AGC struct for HRM
  uint8_t hrm_interpolator_factor_saved;// Used for Fs=10 & 25Hz HRM & SpO2
  uint8_t bpf_active_flag;// BPF-ripple-reduction flags
  uint16_t hrm_bpf_ripple_count;// HRM BPF-ripple reduction variable
  int16_t heart_rate_invalidation_previous;
  uint32_t hrm_ps_dc;
  uint16_t hrm_perfusion_index;// 20 means PI=20/10000=0.2%
  uint16_t hrm_ps_input_old;// HRM BPF-ripple reduction variable
  uint16_t algorithm_status_control_flags;// bit 0: 1=valid part found; 0=valid part not found
  uint8_t hrm_active_dc;
  uint16_t timestamp_clock_freq;// Actual Fs. (10000 or 8192)
} maxm_hrm_handle_t;

/**
 * Data structure for HRM Data returned from the algorithm
 */
typedef struct mamx86161_hrm_data
{
  int16_t fs;
  uint16_t hrm_raw_ppg;
  uint16_t hrm_raw_ppg2,hrm_raw_ppg3,hrm_raw_ppg4;
  int16_t hr_update_interval;
  uint16_t hrm_hrq;
  int16_t hr_iframe;
  uint16_t hrm_raw_ps;
  uint16_t hrm_ps;
  uint16_t hrm_interpolated_ps[MAXM86161_HRM_MAX_INTERPOLATION_FACTOR];// This is useful for displaying the waveform
  int16_t hrm_ps_vpp_low;
  int16_t hrm_ps_vpp_high;
  int16_t hrm_crest_factor;
  uint16_t hrm_adc_gain_current;// HRM AGC: ADCgain<<12+current[2]<<8+current[1]<<4+current[0]
  uint8_t hrm_num_samples;// each Process() may result in more than one sample due to interpolation
  int16_t hrm_samples[MAXM86161_HRM_MAX_INTERPOLATION_FACTOR];// samples after interpolation, filtering and normalization.  These are the sample used for HRM calculation
  uint16_t hrm_perfusion_index;
  uint16_t spo2_red_dc_sample;
  uint16_t spo2_ir_dc_sample;
  uint16_t spo2_dc_to_ac_ratio; // Peak To Peak Ratio
  int16_t spo2_crest_factor;
  uint16_t spo2_ir_perfusion_index;
  uint16_t dc_sensing_led[3];
} mamx86161_hrm_data_t;

/**
 * Maxim86161 interrupt sample structure
 */
typedef struct maxm86161_hrm_irq_sample
{
  uint32_t ppg[3];           // PPG Sample
} maxm86161_hrm_irq_sample_t;

/**
 * Data structure for auxiliary debug data
 */
#define MAXM86161_HRM_NUM_AUX_DEBUG_DATA_VALUES     4
#define MAXM86161_HRM_DEBUG_CHANNEL_ENABLE          0x1
#define MAXM86161_HRM_DEBUG_FIFO_TEST_MODE_ENABLE   0x2

typedef struct maxm86161_hrm_sample_aux_debug
{
  uint32_t data[MAXM86161_HRM_NUM_AUX_DEBUG_DATA_VALUES];
} maxm86161_hrm_sample_aux_debug_t;

/**
 * Number of bytes needed in RAM for HRM
 */
#define MAXM86161_HRM_HRM_DATA_SIZE 1700 //4688//4064

/**
 * Struct for passing allocated RAM locations to HRM library
 */
typedef struct mamx86161_hrm_data_storage
{
  uint8_t hrm[MAXM86161_HRM_HRM_DATA_SIZE];  /**< HRM_DATA_SIZE bytes allocated */

} mamx86161_hrm_data_storage_t;

/**
 * Bytes needed in RAM for SPO2
 */
#define MAXM86161_HRM_SPO2_DATA_SIZE 5600 //5584

/**
 * Struct for SpO2 RAM storage
 */
typedef struct maxm86161_spo2_data_storage
{
  uint8_t data[MAXM86161_HRM_SPO2_DATA_SIZE]; /**< SPO2_DATA_SIZE bytes allocated */
} maxm86161_spo2_data_storage_t;

/**
 * Struct for passing allocated RAM locations to HRM library
 */
typedef struct maxm86161_data_storage
{
  maxm86161_spo2_data_storage_t *spo2;  /**< Pointer to SpO2 RAM */
  mamx86161_hrm_data_storage_t *hrm;    /**< Pointer to HRM RAM */
} maxm86161_data_storage_t;


/**************************************************************************//**
 * @brief
 *  Initialize the optical sensor device and the HRM algorithm
 *
 * @param[in] portName
 *  Platform specific data to specify the i2c port information.
 *
 * @param[in] options
 *  Initialization options flags.
 *
 * @param[in] data
 *  Pointer to data storage structure
 *
 * @param[in] handle
 *  Pointer to maxm86161 handle
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_initialize(maxm86161_data_storage_t *data, maxm_hrm_handle_t **handle);

/**************************************************************************//**
 * @brief
 *  Close the optical sensor device and algorithm
 *
 * @param[in] handle
 *  Pointer to maxm86161 handle
 *
 * @return
 *  Returns error status
 *****************************************************************************/
int32_t maxm86161_hrm_close(maxm_hrm_handle_t *handle);

/**************************************************************************//**
 * @brief
 *  Configure maxm86161 debugging mode.
 *
 * @param[in] handle
 *  Pointer to maxm86161 handle
 *
 * @param[in] enable
 *  Enable or Disable debug
 *
 * @param[in] debug
 *  Pointer to debug status
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_setup_debug(maxm_hrm_handle_t *handle, int32_t enable);


/**************************************************************************//**
 * @brief
 *  Configure maxm86161hrm debugging mode.
 *
 * @param[in] handle
 *  Pointer to maxm86161hrm handle
 *
 * @param[in] enable
 *  Enable or Disable debug
 *
 * @param[in] debug
 *  Pointer to debug status
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_setup_debug(maxm_hrm_handle_t *handle, int32_t enable);

/**************************************************************************//**
 * @brief
 *  Configure device and algorithm
 *
 * @param[in] _handle
 *  maxm86161 handle
 *
 * @param[in] device_config
 *  Pointer to a configuration structure of type maxm86161_device_config_t
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_configure(maxm_hrm_handle_t *handle,
                                maxm86161_device_config_t *device_config,
                                bool enable_debug);

/**************************************************************************//**
 * @brief
 *  Process Maxm86161 samples and compute HRM/SpO2 results
 *
 * @param[in] _handle
 *  Maxm86161 handle
 *
 * @param[out] heart_rate
 *  Pointer to a location where this function will return the heart rate result
 *
 * @param[out] SpO2
 *  Pointer to a location where this function will return the SpO2 result
 *
 * @param[out] hrm_status
 *  Pointer to a integer where this function will report status flags
 *
 * @param[out] hrm_data
 *  Optional pointer to a si117xhrmData_t structure where this function will return
 *  auxiliary data useful for the application.  If the application is not
 *  interested in this data it may pass NULL to this parameter.
 *
 * @param[in] samples
 *  Maxm86161 samples
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_process_external_sample(maxm_hrm_handle_t *handle,
                                              int16_t *heart_rate,
                                              int16_t *SpO2,
                                              int32_t *hrm_status,
                                              mamx86161_hrm_data_t *hrm_data,
                                              maxm86161_hrm_irq_sample_t *samples);
/**************************************************************************//**
 * @brief
 *  HRM process engine. This function should be called at least once per sample
 *
 * @param[in] _handle
 *  Maxm86161 handle
 *
 * @param[out] heartRate
 *  Pointer to a location where this function will return the heart rate result
 *
 * @param[out] SpO2
 *  Pointer to a location where this function will return the SpO2 result
 *
 * @param[in] numSamples
 *
 * @param[out] numSamplesProcessed
 *
 * @param[out] hrmStatus
 *  Pointer to a integer where this function will report status flags
 *
 * @param[out] hrmData
 *  Optional pointer to a si117xhrmData_t structure where this function will return
 *  auxiliary data useful for the application.  If the application is not
 *  interested in this data it may pass NULL to this parameter.
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_process(maxm_hrm_handle_t *handle,
                              int16_t *heartRate,
                              int16_t *SpO2,
                              int16_t numSamples,
                              int16_t *numSamplesProcessed,
                              int32_t *hrmStatus,
                              mamx86161_hrm_data_t *hrmData);

/**************************************************************************//**
 * @brief
 *  Start the device's autonomous measurement operation.
 *  The device must be configured before calling this function.
 *
 * @param[in] _handle
 *  Maxm86161 handle
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_run(maxm_hrm_handle_t *handle);

/**************************************************************************//**
 * @brief
 *  Pause the device's autonomous measurement operation.
 *  HRM must be running before calling this function.
 *
 * @param[in] _handle
 *  Maxm86161 handle
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_pause(void);

/**************************************************************************//**
 * @brief
 *  Returns algorithm version
 *
 * @param[out] revision
 *  String representing the Maxm86161 library version.  The version string has
 *  a maximum size of 32 bytes.
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_query_software_revision(int8_t *revision);

#ifdef __cplusplus
}
#endif

#endif    //MAXM86161_HRM_SPO2_H__
