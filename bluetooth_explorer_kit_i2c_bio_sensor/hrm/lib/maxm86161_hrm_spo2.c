/***************************************************************************//**
* @file maxm86161_hrm_spo2.c
* @brief Maxm86161 HRM/SPO2 algorithm
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

#include "hrm_helper.h"
#include "maxm86161_hrm_spo2.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "em_device.h"

/**************************************************************************//**
 * Static Function Prototypes
 *****************************************************************************/
static void maxm86161_hrm_adjust_led_current(maxm_hrm_handle_t *handle, int32_t channel, int32_t direction);
static void maxm86161_hrm_perform_agc(maxm_hrm_handle_t *handle);
static void maxm86161_hrm_perform_dc_sensing(maxm_hrm_handle_t *handle, maxm86161_hrm_irq_sample_t *ppg);
static int32_t maxm86161_hrm_initialize_buffers(maxm_hrm_handle_t *handle);
static int32_t maxm86161_hrm_bpf_filtering(maxm_hrm_handle_t *handle, int32_t PS_input, int16_t *pPS_output, maxm86161_bpf_t *pBPF);
static int32_t maxm86161_hrm_frame_process(maxm_hrm_handle_t *handle, int16_t *heart_rate, int32_t *HeartRateInvalidation, mamx86161_hrm_data_t *hrm_data);
static int32_t maxm86161_hrm_init_measurement_parameters(maxm_hrm_handle_t *handle, int16_t measurement_rate);
static int32_t maxm86161_hrm_get_sample(maxm86161_hrm_irq_sample_t *samples);
static int32_t maxm86161_hrm_sample_process(maxm_hrm_handle_t *handle, uint32_t hrm_ps, uint32_t SpO2_PS_RED, uint32_t SpO2_PS_IR, mamx86161_hrm_data_t *hrm_data);
static int32_t maxm86161_hrm_spo2_frame_process(maxm_hrm_handle_t *handle, int16_t *SpO2, int32_t *HeartRateInvalidation, mamx86161_hrm_data_t *hrm_data);
static int32_t maxm86161_hrm_identify_part(int16_t *part_id);

/**************************************************************************//**
 * Global Variables and Constants
 *****************************************************************************/
static maxm86161_device_config_t default_maxim_config = {
    15,//interrupt level
    {
#if (PROX_SELECTION & PROX_USE_IR)
        0x02,//LED2 - IR
        0x01,//LED1 - green
        0x03,//LED3 - RED
#elif (PROX_SELECTION & PROX_USE_RED)
        0x03,//LED3 - RED
        0x02,//LED2 - IR
        0x01,//LED1 - green
#else // default use GREEN
        0x01,//LED1 - green
        0x02,//LED2 - IR
        0x03,//LED3 - RED
#endif
        0x00,
        0x00,
        0x00,
    },
    {
        0x05,// green
        0x05,// IR
        0x05,// LED
    },
    {
        MAXM86161_PPG_CFG_ALC_DS,
        MAXM86161_PPG_CFG_OFFSET_NO,
        MAXM86161_PPG_CFG_TINT_117p3_US,
        MAXM86161_PPG_CFG_LED_RANGE_16k,
        MAXM86161_PPG_CFG_SMP_RATE_P1_24sps,
        MAXM86161_PPG_CFG_SMP_AVG_1
    },
    {
        MAXM86161_INT_ENABLE,//full_fifo
        MAXM86161_INT_DISABLE,//data_rdy
        MAXM86161_INT_DISABLE,//alc_ovf
#ifdef PROXIMITY
        MAXM86161_INT_ENABLE,//proximity
#else
        MAXM86161_INT_DISABLE,
#endif
        MAXM86161_INT_DISABLE,//led_compliant
        MAXM86161_INT_DISABLE,//die_temp
        MAXM86161_INT_DISABLE,//pwr_rdy
        MAXM86161_INT_DISABLE//sha
    }
};

static const int16_t hrm_interpolator_coefs_r4[]={  // In Q15. R=4, L=4, Alpha=0.5
         //0x0000, 0x0000, 0x0000, 0x7FFF, 0x0000, 0x0000, 0x0000, 0x0000,
         0xFF56, 0x03FE, 0xF061, 0x6F86, 0x253F, 0xF4C6, 0x034D, 0xFF6B,
         0xFF22, 0x050D, 0xEDBE, 0x4E0F, 0x4E0F, 0xEDBE, 0x050D, 0xFF22,
         0xFF6B, 0x034D, 0xF4C6, 0x253F, 0x6F86, 0xF061, 0x03FE, 0xFF56,
};//hrm_interpolator_coefs_r4

#if (MAXM86161_HRM_ENABLE_MEASUREMENT_RATE_25Hz == 1)
static int32_t bpf_25hz[]=  { //Q16
              0x00002000, 0x00000000, 0xFFFFE000, 0x0000FFFF, 0xFFFEF5F9, 0x00002F38,
              0x00008000, 0x000098B7, 0x00007FFF, 0x0000FFFF, 0xFFFF954A, 0x000075C0,
              0x0001FFFE, 0xFFFC0239, 0x0001FFFE, 0x0000FFFF, 0xFFFE2787, 0x0000DFCA
            };
// This was used in the finger-tip algorithm -
static const short bpf_output_scaler_25hz=0x6F6F; //Q15
// static const sihrmFloat_t bpf_output_scaler_float_25Hz=0.870593092509911;
#endif

#if (MAXM86161_HRM_ENABLE_MEASUREMENT_RATE_60Hz == 1)
static int32_t bpf_60hz[]={ //Q16. fs=79; [BPF_b,BPF_a] = cheby2(3,35, [18, 500]/60/fs*2); 0dB at 45BMP. -1dB at 160BPM, -6dB at 200BPM. No overflow.
                0x00001000, 0x00000000, 0xFFFFF000, 0x0000FFFF, 0xFFFE43CF, 0x0000BFDC,
                0x00004000, 0xFFFFA265, 0x00004000, 0x0000FFFF, 0xFFFE4604, 0x0000CF60,
                0x0000FFFF, 0xFFFE001E, 0x0000FFFF, 0x0000FFFF, 0xFFFE0A1A, 0x0000F69F
              };
static const short bpf_output_scaler_60hz=0x7ebc; //Q15
#endif

#if (MAXM86161_HRM_ENABLE_MEASUREMENT_RATE_95Hz == 1)
static int32_t bpf_95hz[]={ //Q16
                0x00001000, 0x00000000, 0xFFFFF000, 0x0000FFFF, 0xFFFE37E6, 0x0000CA9D,
                0x00004000, 0xFFFF97C4, 0x00004000, 0x0000FFFF, 0xFFFE36EF, 0x0000D793,
                0x0000FFFF, 0xFFFE0015, 0x0000FFFF, 0x0000FFFF, 0xFFFE0832, 0x0000F84B
              };
static const short bpf_output_scaler_95hz=0x679c; //Q15 bpf_output_scaler_95hz
#endif

#if (MAXM86161_HRM_ENABLE_MEASUREMENT_RATE_185Hz == 1)
static int32_t bpf_185hz[]={  //Q16
                0x00000CCD, 0x00000000, 0xFFFFF333, 0x0000FFFF, 0xFFFE1D35, 0x0000E37D,
                0x00003333, 0xFFFF9ED3, 0x00003333, 0x0000FFFF, 0xFFFE1961, 0x0000EA8B,
                0x0000CCCC, 0xFFFE666C, 0x0000CCCC, 0x0000FFFF, 0xFFFE041C, 0x0000FC06
               };
static const short bpf_output_scaler_185hz=0x6aac; //Q15
#endif

#if (MAXM86161_HRM_ENABLE_MEASUREMENT_RATE_229Hz == 1)
static int32_t bpf_229hz[]={   //Q16
                0x00000CCD, 0x00000000, 0xFFFFF333, 0x0000FFFF, 0xFFFE17B5, 0x0000E8C1,
                0x00003333, 0xFFFF9D08, 0x00003333, 0x0000FFFF, 0xFFFE140B, 0x0000EE89,
                0x0000CCCC, 0xFFFE666B, 0x0000CCCC, 0x0000FFFF, 0xFFFE034E, 0x0000FCC8
              };
static const short bpf_output_scaler_229hz=0x5727; //Q15
#endif

#if (MAXM86161_HRM_ENABLE_MEASUREMENT_RATE_430Hz == 1)
static int32_t bpf_430hz[]={  //Q16
                0x00000AAB, 0x00000000, 0xFFFFF555, 0x0000FFFF, 0xFFFE0CC9, 0x0000F359,
                0x00002AAB, 0xFFFFAB7D, 0x00002AAA, 0x0000FFFF, 0xFFFE0A36, 0x0000F689,
                0x0000AAAA, 0xFFFEAAAD, 0x0000AAAA, 0x0000FFFF, 0xFFFE01C1, 0x0000FE47
              };
static const short bpf_output_scaler_430hz=0x52ab; //Q15
#endif

static const int16_t num_of_lowest_hr_cycles_min=300;//num_of_lowest_hr_cycles_min    // 3.00*60/45BPM=4.0s, Min(Initial) #(x100) of lowest-HR cycle. The longer the frame scale is, more accurate the heart rate is.
static const int16_t num_of_lowest_hr_cycles_max=500;// num_of_lowest_hr_cycles_max    // 5.00*60/45BPM=6.7s, Max(Final) #(x100) of lowest-HR cycle. The longer the frame scale is, more accurate the heart rate is.
static const int16_t f_low=45;              // (bpm), min heart rate.
static const int16_t f_high=200;            // (bpm), max heart rate.
static const uint16_t hrm_raw_min_thresh=6000; //hrm_raw_min_thresh  // HRM PS threshold for validation (this depends on the LED active current: 16000 for current=0xF(359mA))

static const int16_t spo2_crest_factor_thresh=12; //spo2_crest_factor_thresh      // Crest factor (C^2) threshold
static const uint16_t spo2_dc_to_acpp_ratio_low_thresh=20; //spo2_dc_to_acpp_ratio_low_thresh
static const uint16_t spo2_dc_to_acpp_ratio_high_thresh=400; //spo2_dc_to_acpp_ratio_high_thresh   // 1/60=1.67%, 1/100=1%. DC to ACpp ratio threshold for validation. Normally, <250 for finger. <700 for wrist.
#if SPO2_REFLECTIVE_MODE
    static const uint16_t spo2_dc_min_thresh=4000; //spo2_dc_min_thresh // Red or IR PS threshold for validation.
#else
    static const uint16_t spo2_dc_min_thresh=9000;  // Red or IR PS threshold for validation.
#endif


/*****************************************************************************
*  Error Checking Macros
******************************************************************************/

#ifndef checkParamRange
#define checkParamRange(param_value, min, max, param_number)  if((param_value < min) || (param_value > max)) \
                              { error = MAXM86161_HRM_ERROR_PARAM1_OUT_OF_RANGE - param_number - 1; \
                                goto Error; \
                              }
#endif

/**************************************************************************//*
 * @brief
 *  Start the device's autonomous measurement operation.
 *  The device must be configured before calling this function.
 *
 * @param[in] _handle
 *  maxm86161hrm handle
 *
 * @param[in] reset
 *  Reset the internal parameters of the HRM algorithm.  If set to false the
 *  HRM algorithm will begin running using parameter values from the last time
 *  it was run.  If the users heart rate has changed significantly since the
 *  last time the algorithm has run and reset is set to false, it could take
 *  longer to converge on the correct heart rate.  It is recommended to set
 *  reset to true if the HRM algorithm has been stopped for greater than 15s.
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_run(maxm_hrm_handle_t *handle)
{
  int32_t error = MAXM86161_HRM_SUCCESS;
  int16_t part_id;

  if (!maxm86161_hrm_identify_part(&part_id))
  {
    error = MAXM86161_HRM_ERROR_INVALID_PART_ID;
    goto Error;
  }

  maxm86161_init_device(*handle->device_config);
  maxm86161_hrm_init_measurement_parameters (handle, handle->measurement_rate);
  maxm86161_shutdown_device(false);

Error:
  return error;
}

/**************************************************************************//**
 * @brief
 *  Pause the device's autonomous measurement operation.
 *  HRM must be running before calling this function.
 *
 * @param[in] _handle
 *  maxm86161hrm handle
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_pause(void)
{
  int32_t error = MAXM86161_HRM_SUCCESS;
  int16_t part_id = 0;

  if (!maxm86161_hrm_identify_part(&part_id))
  {
    error = MAXM86161_HRM_ERROR_INVALID_PART_ID;
    goto Error;
  }

  maxm86161_shutdown_device(true);

Error:
  return error;
}


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
int32_t maxm86161_hrm_setup_debug(maxm_hrm_handle_t *handle, int32_t enable)
{
  int32_t error = MAXM86161_HRM_SUCCESS;
  //error = helper_setup_debug(enable, debug);

  if (enable & MAXM86161_HRM_DEBUG_CHANNEL_ENABLE)
    handle->algorithm_status_control_flags |= SIHRM_ALGORITHM_STATUS_CONTROL_FLAG_DEBUG_ENABLED;
  else
    handle->algorithm_status_control_flags &= ~SIHRM_ALGORITHM_STATUS_CONTROL_FLAG_DEBUG_ENABLED;

  return error;
}

/**************************************************************************//**
 * @brief
 *  Configure device and algorithm
 *
 * @param[in] _handle
 *  maxm86161hrm handle
 *
 * @param[in] configuration
 *  Pointer to a configuration structure of type maxm86161hrmConfiguration_t
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_configure(maxm_hrm_handle_t *handle, maxm86161_device_config_t *device_config, bool enable_debug)
{
  int16_t retval= MAXM86161_HRM_SUCCESS;
  handle->hrm_ps_select = 0;
  handle->spo2_ir_ps_select = 1;
  handle->spo2_red_ps_select = 2;
  if(device_config == NULL)
    device_config = &default_maxim_config;
  handle->device_config = device_config;
  handle->measurement_rate = FS_25HZ;
  handle->timestamp_clock_freq = 8192;
  if(enable_debug)
    handle->algorithm_status_control_flags = SIHRM_ALGORITHM_STATUS_CONTROL_FLAG_DEBUG_ENABLED;
  maxm86161_hrm_init_measurement_parameters(handle, handle->measurement_rate);  // 25Hz
  return retval;
}

/**************************************************************************//**
 * @brief
 *  Process maxm86161 samples and compute HRM/SpO2 results
 *
 * @param[in] _handle
 *  maxm86161hrm handle
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
 *  Optional pointer to a maxm86161hrmData_t structure where this function will return
 *  auxiliary data useful for the application.  If the application is not
 *  interested in this data it may pass NULL to this parameter.
 *
 * @param[in] samples
 *  maxm86161 samples
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_process_external_sample(maxm_hrm_handle_t *handle, int16_t *heart_rate, int16_t *spo2, int32_t *hrm_status, mamx86161_hrm_data_t *hrm_data, maxm86161_hrm_irq_sample_t *samples)
{
  int32_t error = MAXM86161_HRM_SUCCESS;

  uint32_t *ppg_ptr[3];  // array of pointers used to select the ppg value used for the measurements
  ppg_ptr[0] = &samples->ppg[0];
  ppg_ptr[1] = &samples->ppg[1];
  ppg_ptr[2] = &samples->ppg[2];


  //checkErr(maxm86161_hrm_sample_process(handle, *ppg_ptr[handle->hrm_ps_select], *ppg_ptr[handle->spo2_red_ps_select], *ppg_ptr[handle->spo2_ir_ps_select], hrm_data));
  error = maxm86161_hrm_sample_process(handle, *ppg_ptr[handle->hrm_ps_select], *ppg_ptr[handle->spo2_red_ps_select], *ppg_ptr[handle->spo2_ir_ps_select], hrm_data);
  if(error != MAXM86161_HRM_SUCCESS)
    goto Error;
  //checkErr(maxm86161_hrm_frame_process(handle, heart_rate, hrm_status, hrm_data));
  error = maxm86161_hrm_frame_process(handle, heart_rate, hrm_status, hrm_data);
  if(error != MAXM86161_HRM_SUCCESS)
    goto Error;
  if (handle->spo2 != NULL)  // Call spo2 Frame process
    //checkErr(maxm86161_hrm_spo2_frame_process(handle, spo2, hrm_status, hrm_data));
    error = maxm86161_hrm_spo2_frame_process(handle, spo2, hrm_status, hrm_data);
  if(error != MAXM86161_HRM_SUCCESS)
    goto Error;

  Error:
  return error;
}

/**************************************************************************//**
 * @brief
 *  HRM process engine.  This function should be called at least once per sample
 *
 * @param[in] _handle
 *  maxm86161hrm handle
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
 *  Optional pointer to a maxm86161hrmData_t structure where this function will return
 *  auxiliary data useful for the application.  If the application is not
 *  interested in this data it may pass NULL to this parameter.
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_process(maxm_hrm_handle_t *handle, int16_t *heart_rate, int16_t *SpO2, int16_t numSamples, int16_t *numSamplesProcessed, int32_t *hrm_status, mamx86161_hrm_data_t *hrm_data)
{
  int32_t error = MAXM86161_HRM_SUCCESS;
  maxm86161_hrm_irq_sample_t samples;
  int32_t i, channel;

  for(i=0; i<numSamples; i++)
  {
    error = maxm86161_hrm_get_sample(&samples);
    if(error != MAXM86161_HRM_SUCCESS)
      goto Error;

#if (UART_DEBUG & PPG_LEVEL)
    printf("\n%lu,%lu,%lu,", samples.ppg[0], samples.ppg[1], samples.ppg[2]);
#endif

    if(handle->hrm_dc_sensing_flag == HRM_DC_SENSING_START || handle->hrm_dc_sensing_flag == HRM_DC_SENSING_RESTART)
    {
      if(handle->hrm_dc_change_int_level == 0){
          // change the interrupt level to gain the most accuracy of led current of dc working threshold
          maxm86161_set_int_level(3);
          handle->hrm_dc_change_int_level = 1;
      }

      *hrm_status |= MAXM86161_HRM_STATUS_FINGER_OFF;
      maxm86161_hrm_perform_dc_sensing(handle, &samples);
    }
    else if(handle->hrm_dc_sensing_flag == HRM_DC_SENSING_CHANGE_PARAMETERS)
    {
        handle->hrm_dc_sensing_flag = HRM_DC_SENSING_SENSE_FINISH;
        maxm86161_set_int_level(15);
    }
    else if(handle->hrm_dc_sensing_flag == HRM_DC_SENSING_SENSE_FINISH)
    {
  #if HRM_PS_AGC
      if ((handle->heart_rate_invalidation_previous & MAXM86161_HRM_STATUS_HRM_MASK)==MAXM86161_HRM_STATUS_SUCCESS)
      {
        for(channel = 0; channel < 3; channel++){
            handle->hrm_agc[channel].raw_ppg_sum += samples.ppg[channel];   //sum the raw data for later averaging.
            handle->hrm_agc[channel].raw_ppg_count++;
        }
      }
      maxm86161_hrm_perform_agc(handle);
  #endif
      error = maxm86161_hrm_process_external_sample(handle, heart_rate, SpO2, hrm_status, hrm_data, &samples);
    }
  }

Error:
  *numSamplesProcessed = i;
  return error;
}

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
 *  Pointer to maxm86161hrm handle
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_initialize(maxm86161_data_storage_t *data, maxm_hrm_handle_t **handle)
{
  int32_t error = MAXM86161_HRM_SUCCESS;

  maxm_hrm_handle_t *_handle;

#if (SIHRM_USE_DYNAMIC_DATA_STRUCTURE == 0)
  _handle = (maxm_hrm_handle_t*)(data->hrm);
  _handle->spo2 = (maxm86161_spo2_handle_t*)(data->spo2);
#else
  _handle = (maxm_hrm_handle_t *)malloc(sizeof(maxm_hrm_handle_t));
  _handle->spo2 = (maxm86161_spo2_handle_t *)malloc(sizeof(maxm86161_spo2_handle_t));
#endif

  maxm86161_hrm_helper_initialize();

  (*handle) = (maxm_hrm_handle_t *)_handle;

  return error;
}

/**************************************************************************//**
 * @brief
 *  Close the maxm86161 device
 *
 * @param[in] handle
 *  Pointer to maxm86161hrm handle
 *
 * @return
 *  Returns error status
 *****************************************************************************/
int32_t maxm86161_hrm_close(maxm_hrm_handle_t *handle)
{
  int32_t error = MAXM86161_HRM_SUCCESS;

  if (handle != 0)
  {
    if (handle->flag_samples == 0)  // If we are passing samples then do not attempt to write to registers
    {
      //sihrmUser_Close(handle->maxm86161_handle);
    }
#if (SIHRM_USE_DYNAMIC_DATA_STRUCTURE != 0)
    free(handle);
#endif
  }

  return error;
}

/**************************************************************************//**
 * @brief
 *  Returns algorithm version
 *
 * @param[out] revision
 *  String representing the maxm86161hrm library version.  The version string has
 *  a maximum size of 32 bytes.
 *
 * @return
 *  Returns error status.
 *****************************************************************************/
int32_t maxm86161_hrm_query_software_revision(int8_t *revision)
{
  int32_t error = MAXM86161_HRM_SUCCESS;

  strcpy((char *)revision, MAXM86161_HRM_VERSION);

  return error;
}

/**************************************************************************//**
 * @brief Adjust the LED current setting based on AGC result
 *****************************************************************************/
static void maxm86161_hrm_adjust_led_current(maxm_hrm_handle_t *handle, int32_t channel, int32_t direction)
{
  if(direction == 1)
  {
    if(handle->dc_sensing_led[channel] != 255)
      handle->dc_sensing_led[channel] = handle->dc_sensing_led[channel] + 1;
  }
  else
  {
    if(handle->dc_sensing_led[channel] != 0)
      handle->dc_sensing_led[channel] = handle->dc_sensing_led[channel] - 1;
  }
  handle->hrm_agc[channel].led_current_value = handle->dc_sensing_led[channel];
  maxm86161_led_pa_config_specific(channel, handle->dc_sensing_led[channel]);
}

/**************************************************************************//**
 * @brief Perform the AGC (automatic gain control)
 *****************************************************************************/
static void maxm86161_hrm_perform_agc(maxm_hrm_handle_t *handle)
{
  uint32_t average_ppg;
  uint16_t agc_threshold_percent;
  int32_t direction = 0;
  int16_t channel;

  for(channel = 0; channel < 3; channel++)
  {
    if((handle->hrm_agc[channel].raw_ppg_count >= HRM_NUM_SAMPLES_IN_FRAME))
    {
      average_ppg = handle->hrm_agc[channel].raw_ppg_sum/handle->hrm_agc[channel].raw_ppg_count;  // Average the raw PS value for the frame
      handle->hrm_agc[channel].raw_ppg_sum=0;
      handle->hrm_agc[channel].raw_ppg_count=0;

      // Ideally the relative threshold should be set according to the integration time and sensitivity
      // Set agc_threshold_percent (%). When PPG is out of the range, AGC increase/decrease the LED current by 1.
      agc_threshold_percent = 20;

      if ((average_ppg<(uint32_t)HRM_DC_WORKING_LEVEL*(100-agc_threshold_percent)/100) || (average_ppg>(uint32_t)HRM_DC_WORKING_LEVEL*(100+agc_threshold_percent)/100))
      { // Increase or decrease the LED current by 1
        if(average_ppg<(uint32_t)HRM_DC_WORKING_LEVEL*(100-agc_threshold_percent)/100)
          direction = 1;
        else
          direction = 0;

        maxm86161_hrm_adjust_led_current(handle, channel, direction);

        handle->hrm_agc[channel].agc_flag = 1;  // Signal that the AGC made a change to the current or gain.  This flag is cleared in sample process.
      }
    }
  }
}

/**************************************************************************//**
 * @brief Perform the DC Sensing
 *****************************************************************************/
static void maxm86161_hrm_perform_dc_sensing(maxm_hrm_handle_t *handle, maxm86161_hrm_irq_sample_t *sample)
{
  uint8_t channel;

  for(channel = 0; channel < 3; channel++)
  {
    if(handle->ppg_led_local>=5 && handle->ppg_led_local<255)
    {
      maxm86161_led_pa_config_specific(channel, handle->ppg_led_local);
      if(sample->ppg[channel] < HRM_PS_RAW_SKIN_CONTACT_THRESHOLD)
      {
        handle->hrm_dc_sensing_flag = HRM_DC_SENSING_SEEK_NO_CONTACT;
        handle->ppg_led_local=0;   // Restart DC sensing for another try
        break;
      }
      else
      {
        if(handle->dc_sensing_finish[channel] == false)
        {
          if(sample->ppg[channel] > HRM_DC_WORKING_LEVEL)
          {
              handle->dc_sensing_finish[channel] = true;
              handle->dc_sensing_led[channel] = handle->ppg_led_local;
          }
        }
      }
    }
  }

  if ((handle->dc_sensing_finish[0] == true) && (handle->dc_sensing_finish[1] == true) && (handle->dc_sensing_finish[2] == true))  // If we Found the working level for all led
  {
    handle->hrm_dc_sensing_flag = HRM_DC_SENSING_CHANGE_PARAMETERS;
    // DC Sensing successful.  Set the working LED currents based on DC-sensing results.
    for(channel = 0; channel < 3; channel++)
    {
      maxm86161_led_pa_config_specific(channel, handle->dc_sensing_led[channel]);
    }
  }
  else if(handle->ppg_led_local == 255) // If we have tried all the LED currents
  {
    handle->hrm_dc_sensing_flag = HRM_DC_SENSING_CHANGE_PARAMETERS;
    for(channel = 0; channel < 3; channel++)
    {
      if(handle->dc_sensing_finish[channel] == false) // check led not reach HRM_DC_WORKING_LEVEL
      {
          handle->dc_sensing_led[channel] = 255;  // Set it to the max level
      }
      maxm86161_led_pa_config_specific(channel, handle->dc_sensing_led[channel]);
    }
  }

  if(handle->hrm_dc_sensing_flag == HRM_DC_SENSING_SEEK_NO_CONTACT)
  {
    handle->ppg_led_local=0;   // Restart DC sensing for another try
    handle->hrm_dc_sensing_flag = HRM_DC_SENSING_RESTART;
    for(channel = 0; channel < 3; channel++)
      handle->dc_sensing_finish[channel] = false;
  }
  else
    handle->ppg_led_local = handle->ppg_led_local >= 255 ? 256 :  handle->ppg_led_local+1;
}

/**************************************************************************//**
 * @brief Initialize HRM/SpO2 buffers and other variables
 *****************************************************************************/
static int32_t maxm86161_hrm_initialize_buffers(maxm_hrm_handle_t *handle)
{
  int32_t i, j;
  int32_t error = MAXM86161_HRM_SUCCESS;

  // Buffer initialization
  for(i=0; i<MAX_FRAME_SAMPLES; i++)
  {
  handle->hrm_sample_buffer[i]=0;

    if (handle->spo2 != NULL)
    {
      handle->spo2->spo2_red_ac_sample_buffer[i]=0;
      handle->spo2->spo2_red_dc_sample_buffer[i]=0;
      handle->spo2->spo2_ir_ac_sample_buffer[i]=0;
      handle->spo2->spo2_ir_dc_sample_buffer[i]=0;
    }
  }

  for(i=0; i<handle->bpf_biquads; i++)  // zero-out the input and output buffer
  {
    for(j=0; j<3; j++)
    {
      handle->hrm_bpf.x[i][j]=0;
      handle->hrm_bpf.yi[i][j]=0;
      handle->hrm_bpf.yf[i][j]=0;
      if (handle->spo2 != NULL)  //SpO2 BPF buffer initialization
      {
        handle->spo2->spo2_red_bpf.x[i][j]=0;
        handle->spo2->spo2_red_bpf.yi[i][j]=0;
        handle->spo2->spo2_red_bpf.yf[i][j]=0;

        handle->spo2->spo2_ir_bpf.x[i][j]=0;
        handle->spo2->spo2_ir_bpf.yi[i][j]=0;
        handle->spo2->spo2_ir_bpf.yf[i][j]=0;
      }
    }
  }

  for (i=0; i<INTERPOLATOR_L*2; i++) handle->hrm_interpolator_buf[i]=0;  // Initialized the whole HRM interpolator buffer to 0.

  handle->sample_count=0;
  handle->hrm_ps_raw_level_count=0;
  handle->hrm_buffer_in_index=0;

  if (handle->spo2 != NULL)  // Variable Initialization
  {
    for (i=0; i<INTERPOLATOR_L*2; i++)
    {
      handle->spo2->spo2_red_interpolator_buf[i]=0;  // Initialized the whole SpO2 Red interpolator buffer to 0.
      handle->spo2->spo2_ir_interpolator_buf[i]=0;   // Initialized the whole SpO2 Red interpolator buffer to 0.
    }
    handle->spo2->spo2_buffer_in_index=0;
    handle->spo2->spo2_raw_level_count=0;
    handle->spo2->spo2_red_bpf_ripple_count=0;
    handle->spo2->spo2_red_ps_input_old=0;
    handle->spo2->spo2_ir_bpf_ripple_count=0;
    handle->spo2->spo2_ir_ps_input_old=0;
    handle->spo2->time_since_finger_off=0;
  }

  handle->hrm_dc_sensing_count=0;
  handle->hrm_raw_ps_old=0;
  handle->hrm_dc_sensing_flag=HRM_DC_SENSING_START;
  for(i=0; i<3; i++)
  {
      handle->dc_sensing_finish[i] = false;
  }
  handle->hrm_dc_change_int_level = 0;
  handle->bpf_active_flag=(0xff-HRM_BPF_ACTIVE-SpO2_RED_BPF_ACTIVE-SpO2_IR_BPF_ACTIVE);  // BPF-ripple-reduction flags. Clear these bits.
  handle->hrm_bpf_ripple_count=0;
  handle->hrm_ps_input_old=0;
  handle->heart_rate_invalidation_previous=MAXM86161_HRM_STATUS_FINGER_OFF;
  handle->hrm_ps_dc=0;

  // DC sensing initialization
  handle->ppg_led_local = 0;

  // AGC initialization
  for(i=0; i<3; i++)
  {
    handle->hrm_agc[i].raw_ppg_sum = 0;
    handle->hrm_agc[i].raw_ppg_count = 0;
    handle->hrm_agc[i].led_current_value = 0;
    handle->hrm_agc[i].agc_flag = 0;
    handle->hrm_agc[i].saved_ppg = 0;
  }
  return error;
}

/**************************************************************************//**
 * @brief Perform band-pass filtering on PPG samples
 *****************************************************************************/
static int32_t maxm86161_hrm_bpf_filtering(maxm_hrm_handle_t *handle, int32_t ps_input, int16_t *p_ps_output, maxm86161_bpf_t *p_bpf)
{
  // Band-pass filter: 32-bit integer-point precision.
  const int16_t bpf_q15=(1<<15)-1;
  int32_t error = MAXM86161_HRM_SUCCESS;
  int16_t i,j;
  int32_t bpf_new_sample;
  int32_t *p_bpf_a, *p_bpf_b;  // Pointers for the BPF calculations
  int32_t new_sample_i1, new_sample_i2;  // May use __int64 for debug
  int32_t new_sample_i=0, new_sample_f=0;

  // BPF-Ripple Reduction: After finger-On is detected, check the PS DC after 1s and clear BPF data buffers with p_bpf->x[0][:]=current PS.
  uint8_t bpf_current_flag;
  uint16_t ps_normlized_thresh, ps_normalization_scaler, *p_bpf_ripple_count, *p_ps_input_old;

  if (p_bpf == &handle->hrm_bpf)
  {
    ps_normlized_thresh=hrm_raw_min_thresh;
    bpf_current_flag = HRM_BPF_ACTIVE;
    p_bpf_ripple_count = &handle->hrm_bpf_ripple_count;
    p_ps_input_old = &handle->hrm_ps_input_old;
    ps_normalization_scaler=handle->hrm_normalization_scaler;
  }
  // SpO2 BPF-ripple reduction
  else if ((handle->spo2 != NULL) && (p_bpf == &handle->spo2->spo2_red_bpf))
  {
    if (handle->spo2 != NULL)
    {
      ps_normlized_thresh=spo2_dc_min_thresh;
      bpf_current_flag = SpO2_RED_BPF_ACTIVE;
      p_bpf_ripple_count = &handle->spo2->spo2_red_bpf_ripple_count;
      p_ps_input_old = &handle->spo2->spo2_red_ps_input_old;
      ps_normalization_scaler=handle->red_normalization_scaler;
    }
  }
  else if ((handle->spo2 != NULL) && (p_bpf == &handle->spo2->spo2_ir_bpf))
  {
    if (handle->spo2 != NULL)
    {
      ps_normlized_thresh=spo2_dc_min_thresh;
      bpf_current_flag = SpO2_IR_BPF_ACTIVE;
      p_bpf_ripple_count = &handle->spo2->spo2_ir_bpf_ripple_count;
      p_ps_input_old = &handle->spo2->spo2_ir_ps_input_old;
      ps_normalization_scaler=handle->ir_normalization_scaler;
    }
  }
  else
  {
    error = MAXM86161_HRM_ERROR_BAD_POINTER;
    goto Error;
  }

  if (ps_input<ps_normlized_thresh)  // Look for the finger Off-to-On transition and then initialize the BPF variables
  {
    handle->bpf_active_flag &=(0xff-bpf_current_flag);  // Skip HRM BPF filter
    *p_bpf_ripple_count=0;
  }
  else
  {
    // When Finger On is detected, check diff(PS) and Ripple Count before initialize the BPF buffer.
    if ((handle->bpf_active_flag & bpf_current_flag)==0)
    {
      (*p_bpf_ripple_count)++;
      i = ps_input>*p_ps_input_old ? ps_input-*p_ps_input_old : *p_ps_input_old-ps_input;  // i=abs(PS-PS_old)
      if ((i<((10*ps_normalization_scaler)>>QF_SCALER)) && (*p_bpf_ripple_count>(10*handle->fs/100)))  // delay 1s
      {
        handle->bpf_active_flag |= bpf_current_flag;
        for(i=0; i<handle->bpf_biquads; i++)  // zero-out the input and output buffer
        {
          for(j=0; j<3; j++)
          {
            p_bpf->x[i][j] =0;
            p_bpf->yi[i][j]=0;
            p_bpf->yf[i][j]=0;
          }
        }
        // Use the current level to initialize these BPF variables to significantly reduce the BPF ripple.
        for(j=0; j<3; j++)
          p_bpf->x[0][j]=ps_input;  // Initialized to the HRM PS DC.
      }
    }
  }

  if ((handle->bpf_active_flag & bpf_current_flag)==0)
  {
    *p_ps_input_old=ps_input;
    *p_ps_output=0;
    return error;
  }

  p_bpf_b=handle->pbpf_b_0;      // Initialize to the start of b coefs.
  p_bpf_a=handle->pbpf_b_0+3;    // Initialize to the start of a coefs.
  bpf_new_sample=ps_input;          // Normalized input

  for(i=0; i<handle->bpf_biquads; i++)
  { // filter biquad process
    for(j=3-1; j>0; j--)  // Shift the BPF in/out data buffers and add the new input sample
    {
      p_bpf->x[i][j]  = p_bpf->x[i][j-1];
      p_bpf->yi[i][j] = p_bpf->yi[i][j-1];
      p_bpf->yf[i][j] = p_bpf->yf[i][j-1];
    }
    p_bpf->x[i][0]=(int32_t)bpf_new_sample;  // Add new sample for the current biquad
    new_sample_i1=0;          // bpf_new_sample is used for the next biquad
    new_sample_i2=0;
    for(j=0; j<3; j++)
    {
      if (abs(p_bpf->x[i][j])<(1<<14))  // If |x| is large, scale down first before the 32-bit-int32_t multiplication.
        new_sample_i1 += ((((p_bpf_b[j] * p_bpf->x[i][j]))>>(SCALE_I_SHIFT-1))+1)>>1;  // +1 is round-up
      else
        new_sample_i1 += ((((p_bpf_b[j] * (p_bpf->x[i][j]>>2)))>>(SCALE_I_SHIFT-2-1))+1)>>1;  // +1 is round-up
    }
    for(j=1; j<3; j++)  // Shift the BPF in/out data buffers and add the new input sample
    {
      if (abs(p_bpf->yi[i][j])<(1<<14))  // If |y| is large, scale down first before the 32-bit-int32_t multiplication.
        new_sample_i1 -= ((((p_bpf_a[j] * (int32_t)p_bpf->yi[i][j]))>>(SCALE_I_SHIFT-1))+1)>>1;  // +1 is round-up
      else
        new_sample_i1 -= ((((p_bpf_a[j] * (int32_t)(p_bpf->yi[i][j]>>2)))>>(SCALE_I_SHIFT-2-1))+1)>>1;  // +1 is round-up
      new_sample_i2 -= ((p_bpf_a[j] * p_bpf->yf[i][j]));
    }
    new_sample_i  = ((new_sample_i1>>(SOS_SCALE_SHIFT-SCALE_I_SHIFT-1))+1)>>1;  // +1 is round-up
    new_sample_i += ((new_sample_i2>>(SOS_SCALE_SHIFT+SCALE_F_SHIFT-1))+1)>>1;  // +1 is round-up

    new_sample_f  = ((new_sample_i1>>(SOS_SCALE_SHIFT-SCALE_I_SHIFT-SCALE_F_SHIFT-1))+1)>>1;  // +1 is round-up
    new_sample_f += ((new_sample_i2>>(SOS_SCALE_SHIFT-1))+1)>>1;  // +1 is round-up
    new_sample_f -= new_sample_i<<SCALE_F_SHIFT;

    bpf_new_sample= new_sample_i;

    p_bpf->yi[i][0]=new_sample_i;
    p_bpf->yf[i][0]=new_sample_f;

    // Update new input sample for next 2nd-order stage. point p_bpf_b/_a to the next 2nd-order stage
    p_bpf_b +=6;
    p_bpf_a +=6;
  }
  *p_ps_output=(int16_t)(new_sample_i*handle->bpf_output_scaler/bpf_q15);  // 0.5=roundup

Error:
  return error;
}

/**************************************************************************//**
 * @brief Heart rate frame process
 *****************************************************************************/
static int32_t maxm86161_hrm_frame_process(maxm_hrm_handle_t *handle, int16_t *heart_rate, int32_t *p_heartrate_invalidation, mamx86161_hrm_data_t *hrm_data)
{
  int32_t error = MAXM86161_HRM_SUCCESS;
  int16_t i, j, k, m;
  int16_t zc_count, zc_hr;              // Zero-crossing count and the heart rate estimated from zc_count
  int16_t hrm_ps_vpp_low=0, hrm_ps_vpp_high=0;      // Variables for BPF(PS) min and max. Initializations are needed to avoid the usage before set.
  int16_t t_low, t_high;                // (sample), times for min and max heart rates based on Zero-crossing count.
  int32_t cf_abs_max=0, cf_abs_energy=1000;     // Initializations are needed to avoid the usage before set.

  int32_t hrm_ps_auto_corr_max, hrm_ps_auto_corr_current, hrm_ps_auto_corr_max_index;  // Auto correlation variables
  const int32_t hrm_ps_auto_corr_max_threshold=100000/2880/4;  // Auto correlation threshold for validation (may need for the wrist-band as the pulse is usually weaker than that on fingertip))
  int16_t heart_rate_x10;
  int16_t hrm_buffer_out_index;       // Output index to the circular buffer of the frame sample buffer.

  //restart heart rate value to invalid
  //*heart_rate = 0;
  t_low=handle->t_low0;
  t_high=handle->t_high0;         // (sample), times for min and max heart rates based on Zero-crossing count.

  handle->hrm_ps_dc += handle->normalized_hrm_ps*handle->hrm_interpolator_factor; // hrm_ps_dc is the PS DC accumulator. PS is the normalized PS and also input to BPF.

  // Frame process (Background job)
  if (handle->sample_count>=handle->hr_update_interval)
  {
  // Received new hr_update_interval samples
    handle->sample_count -= handle->hr_update_interval;   // Adjust the count for the next block of samples
    *p_heartrate_invalidation = MAXM86161_HRM_STATUS_SUCCESS;    // Clears all HRM and SpO2 status bits.

    if (handle->spo2 != NULL)
    {
      handle->spo2->spo2_percent=SPO2_STATUS_PROCESS_SPO2_FRAME;  // Notify SpO2 to process the SpO2 frame.
    }

    // Re-initialize hr_iframe to the min length when "Finger-off", so to speed up the next HR reporting.
    if ((handle->heart_rate_invalidation_previous & (MAXM86161_HRM_STATUS_FINGER_OFF|MAXM86161_HRM_STATUS_FINGER_ON|MAXM86161_HRM_STATUS_BPF_PS_VPP_OFF_RANGE|MAXM86161_HRM_STATUS_AUTO_CORR_MAX_INVALID)))
    {
      handle->num_of_lowest_hr_cycles=num_of_lowest_hr_cycles_min;
      handle->hr_iframe=handle->t_low0*handle->num_of_lowest_hr_cycles/100;  // (sample)
    }
    // After the first valid HR is detected, hr_iframe is increased by hr_update_interval up to the max(final) length.
    if (((handle->heart_rate_invalidation_previous & MAXM86161_HRM_STATUS_HRM_MASK)==MAXM86161_HRM_STATUS_SUCCESS) &&
        (handle->num_of_lowest_hr_cycles < num_of_lowest_hr_cycles_max) && (handle->hr_iframe+handle->hr_update_interval)<MAX_FRAME_SAMPLES)  // Make sure hr_iframe<MAX_FRAME_SAMPLES.
    {
      handle->hr_iframe +=handle->hr_update_interval;  // (sample)
      handle->num_of_lowest_hr_cycles += handle->hr_update_interval*100/handle->t_low0;  // cycles(x100)
    }

    hrm_buffer_out_index=handle->hrm_buffer_in_index-handle->hr_iframe;     // Set the output pointer based on the Input pointer.
    if (hrm_buffer_out_index<0) hrm_buffer_out_index +=MAX_FRAME_SAMPLES;  // Wrap around

    // Validation: Invalidate if any sample in the current frame is below the level threshold
    if (handle->hrm_ps_raw_level_count < handle->hr_iframe)
    {
      if(handle->hrm_ps_raw_level_count<=handle->hr_update_interval)
        *p_heartrate_invalidation |= MAXM86161_HRM_STATUS_FINGER_OFF;
      else
        *p_heartrate_invalidation |= MAXM86161_HRM_STATUS_FINGER_ON;
    }
    else if ((handle->heart_rate_invalidation_previous & MAXM86161_HRM_STATUS_HRM_MASK) == MAXM86161_HRM_STATUS_FINGER_ON)
    {
      // Validation: Still Finger-On mode if the last frame is Finger-On and the leading zeros in the current frame are 5% or more of iFrame samples.
      i=hrm_buffer_out_index;
      for(k=0; k<handle->hr_iframe*5/100; k++)
      {
        if (handle->hrm_sample_buffer[i]!=0) break;  // Break if non-zero
        if (++i==MAX_FRAME_SAMPLES) i=0;  // Wrap around
      }
      if (k==handle->hr_iframe*5/100) *p_heartrate_invalidation|=MAXM86161_HRM_STATUS_FINGER_ON;
    }

    if (*p_heartrate_invalidation==0)  // Skip below if the frame is invalid for heart-rate detection
    {
      // a) Calculate the max/min values and zero-crossing counts
      zc_count=0;
      hrm_ps_vpp_high=-20000;
      hrm_ps_vpp_low =+20000;
      i=hrm_buffer_out_index;
      handle->hrm_ps_dc /= handle->hr_update_interval;  // Now, hrm_ps_dc is the averaged normalized PS over hr_update_interval sample (1s)
      for(k=0; k<handle->hr_iframe; k++)  // Scaled HRM PS AC, max/min values
      {
        // Scaled HRM PS AC based on HRM PS DC with reference of HRM_PS_DC_REFERENCE
        m=(handle->hrm_sample_buffer[i]*HRM_PS_DC_REFERENCE+(uint16_t)handle->hrm_ps_dc/2)/(uint16_t)handle->hrm_ps_dc;  // Round-up
        handle->hrm_sample_buffer[i]=m;  // Scaled HRM_PS_AC
        if (hrm_ps_vpp_high<m) hrm_ps_vpp_high=m;
        if (hrm_ps_vpp_low >m) hrm_ps_vpp_low =m;
        if (++i==MAX_FRAME_SAMPLES) i=0;  // Wrap around
      }
      // Validation: Invalidate if the HRM_PS_Vpp is out of the range
      if (hrm_ps_vpp_low<handle->hrm_ps_vpp_min || hrm_ps_vpp_high>handle->hrm_ps_vpp_max)
        *p_heartrate_invalidation |= MAXM86161_HRM_STATUS_BPF_PS_VPP_OFF_RANGE;

      i=hrm_buffer_out_index;
      for(k=0; k<handle->hr_iframe-1; k++)  // zero-crossing counts
      { // Find zero-crossing counts for BPF(PS)-hrm_ps_vpp_high/ZR_BIAS_SCALE as BPF(PS) is asymmetric.
        m=i+1;
        if (m==MAX_FRAME_SAMPLES) m=0;  // Wrap around
        if (((int32_t)(handle->hrm_sample_buffer[i]-hrm_ps_vpp_high*(ZR_BIAS_SCALE))*2+1)*((int32_t)(handle->hrm_sample_buffer[m]-hrm_ps_vpp_high*(ZR_BIAS_SCALE))*2+1)<0)
          zc_count++;
        if (++i==MAX_FRAME_SAMPLES) i=0;  // Wrap around
      }

      zc_hr=zc_count*60*(handle->fs+5)/10/handle->hr_iframe/2;
      // Validation: Invalidate if the zero-crossing HR is out of the HR range
      if (zc_hr<f_low || zc_hr>f_high)
        *p_heartrate_invalidation |= MAXM86161_HRM_STATUS_ZERO_CROSSING_INVALID;
      else
      {
        // Modify t_high and t_low based on rough heart-rate estimate that is from the zero-cross count.
        t_low  = zc_hr*(100-ZC_HR_TOLERANCE)< f_low*100  ? handle->t_low0  : 60*(handle->fs+5)/10*100/(zc_hr*(100-ZC_HR_TOLERANCE));
        t_high = zc_hr*(100+ZC_HR_TOLERANCE)> f_high*100 ? handle->t_high0 : 60*(handle->fs+5)/10*100/(zc_hr*(100+ZC_HR_TOLERANCE));

        // b) Compute Crest factor of PS
        cf_abs_max= hrm_ps_vpp_high > abs(hrm_ps_vpp_low) ? hrm_ps_vpp_high : abs(hrm_ps_vpp_low);
        cf_abs_energy=0;
        i=hrm_buffer_out_index;
        for(k=0; k<handle->hr_iframe; k++)  // zero-crossing counts
        { // Find zero-crossing counts for BPF(PS)-hrm_ps_vpp_high/ZR_BIAS_SCALE as BPF(PS) is asymmetric.
          if (abs(handle->hrm_sample_buffer[i]) < handle->hrm_ps_vpp_max )
          {
            cf_abs_energy += handle->hrm_sample_buffer[i]*handle->hrm_sample_buffer[i];
          }
          else
          {
            cf_abs_energy += handle->hrm_ps_vpp_max*handle->hrm_ps_vpp_max;
          }
          if (++i==MAX_FRAME_SAMPLES) i=0;  // Wrap around
        }
      }
      // Validation 1: Invalidate the frame if CF > CF_threshold
      if (cf_abs_max*cf_abs_max > handle->hrm_ps_crestfactor_thresh*(cf_abs_energy/handle->hr_iframe))
        *p_heartrate_invalidation |= MAXM86161_HRM_STATUS_CREST_FACTOR_TOO_HIGH;

      if (*p_heartrate_invalidation!=0)
      { //Scaled back HRM PS AC, so that HRM sample buffer is same as before
        i=hrm_buffer_out_index;
        for(k=0; k<handle->hr_iframe; k++)
        {
          handle->hrm_sample_buffer[i]=(handle->hrm_sample_buffer[i]*(uint16_t)handle->hrm_ps_dc+HRM_PS_DC_REFERENCE/2)/HRM_PS_DC_REFERENCE; //Round up
          if (++i==MAX_FRAME_SAMPLES) i=0;  // Wrap around
        }
      }
    }  // p_heartrate_invalidation

    if (*p_heartrate_invalidation==0) //Skip if the frame is invalid for heart-rate detection
    {
      // Compute the auto correlation on BPF(PS)
      hrm_ps_auto_corr_max=-100000000;     // A large negative number
      hrm_ps_auto_corr_max_index=t_high;
      for(i=t_high; i<=t_low; i++)
      {
        hrm_ps_auto_corr_current=0;
        j=hrm_buffer_out_index;
        for(k=0; k<handle->hr_iframe-i; k++)
        {
          m=j+i;
          if (m>=MAX_FRAME_SAMPLES) m -= MAX_FRAME_SAMPLES;  // Wrap around
          hrm_ps_auto_corr_current += handle->hrm_sample_buffer[j]*handle->hrm_sample_buffer[m];
          if (++j==MAX_FRAME_SAMPLES) j=0;  // Wrap around
        }
        // Find the max
        if (hrm_ps_auto_corr_current > hrm_ps_auto_corr_max)
        {
          hrm_ps_auto_corr_max = hrm_ps_auto_corr_current;
          hrm_ps_auto_corr_max_index = i;
        }
      }

      // Validation: Invalidate if the auto-correlation max is too small.
      if (hrm_ps_auto_corr_max < hrm_ps_auto_corr_max_threshold*handle->hr_iframe)
      {
        *p_heartrate_invalidation |= MAXM86161_HRM_STATUS_AUTO_CORR_TOO_LOW;
      }
      else if (hrm_ps_auto_corr_max_index == t_low || hrm_ps_auto_corr_max_index == t_high)
      {
      // Validation: Invalidate if the auto-correlation max occurs at the search bounday of [t_low, t_high].
        *p_heartrate_invalidation |=MAXM86161_HRM_STATUS_AUTO_CORR_MAX_INVALID;
      }
      else
      {
        heart_rate_x10=(600*(handle->fs+5)/10*2/hrm_ps_auto_corr_max_index+1)/2;  // +1 for Roundup
        *heart_rate = (heart_rate_x10/5+1)/2;
      }

      // Scaled back HRM PS AC, so that HRM sample buffer is same as before
      i=hrm_buffer_out_index;
      for(k=0; k<handle->hr_iframe; k++)
      {
        handle->hrm_sample_buffer[i]=(handle->hrm_sample_buffer[i]*(uint16_t)handle->hrm_ps_dc+HRM_PS_DC_REFERENCE/2)/HRM_PS_DC_REFERENCE;  // Round up
        if (++i==MAX_FRAME_SAMPLES) i=0;  // Wrap around
      }
    }
    handle->heart_rate_invalidation_previous=*p_heartrate_invalidation & MAXM86161_HRM_STATUS_HRM_MASK;  // Copy all HRM bits.
    *p_heartrate_invalidation |=MAXM86161_HRM_STATUS_FRAME_PROCESSED;
    if(hrm_data != 0)  // Copy these variables to hrm_data for display.
    {
      hrm_data->hrm_ps_vpp_low  = (hrm_ps_vpp_low*(uint16_t)handle->hrm_ps_dc+HRM_PS_DC_REFERENCE/2)/HRM_PS_DC_REFERENCE;   // Round up
      hrm_data->hrm_ps_vpp_high = (hrm_ps_vpp_high*(uint16_t)handle->hrm_ps_dc+HRM_PS_DC_REFERENCE/2)/HRM_PS_DC_REFERENCE;  // Round up
      if ((cf_abs_energy/handle->hr_iframe)==0)
        hrm_data->hrm_crest_factor=-1;  // Invalid Crest factor
      else
        hrm_data->hrm_crest_factor=(cf_abs_max*cf_abs_max)/(cf_abs_energy/handle->hr_iframe);
      if (hrm_data->hrm_ps!=0)
        handle->hrm_perfusion_index=10000*(hrm_data->hrm_ps_vpp_high - hrm_data->hrm_ps_vpp_low)/hrm_data->hrm_ps;
      else
        handle->hrm_perfusion_index=0;
      hrm_data->hrm_perfusion_index=handle->hrm_perfusion_index;
      for(i=0; i<3; i++) hrm_data->dc_sensing_led[i]=handle->dc_sensing_led[i];
    }
    handle->hrm_ps_dc=0;  // Reset HRM PS DC accumulator
  }  // Frame process, sample_count==hr_update_interval

  return error;
}

/**************************************************************************//**
 * @brief SpO2 frame process
 *****************************************************************************/
static int32_t maxm86161_hrm_spo2_frame_process(maxm_hrm_handle_t *handle, int16_t *SpO2, int32_t *p_heartrate_invalidation, mamx86161_hrm_data_t *hrm_data)
{
  int32_t error = MAXM86161_HRM_SUCCESS;
  int32_t spo2_red_dc, spo2_ir_dc=1;
  int32_t spo2_red_ac2, spo2_ir_ac2;
  int16_t spo2_red_ac_min, spo2_red_ac_max, spo2_ir_ac_min=0, spo2_ir_ac_max=0;
  int32_t spo2_r;
  uint16_t spo2_r_root, spo2_r_root_shift, spo2_r_root_tmp;
  uint16_t spo2_ac2_scaler;
  int16_t spo2_red_crest_factor, spo2_ir_crest_factor;
  int16_t spo2_buffer_out_index;  // Output index to the frame sample circular buffer.
  uint16_t spo2_dc_to_ac_ratio=0;   // Ratio of DC to ACpp. For debug reporting
  int16_t spo2_crest_factor=0;    // Crest factor (C^2)
  int16_t i, k;

  if (handle->spo2 == NULL)
    return MAXM86161_HRM_STATUS_SPO2_EXCEPTION;

  // Frame process (Background job)
  if (handle->spo2->spo2_percent==SPO2_STATUS_PROCESS_SPO2_FRAME)  // HRM_frame_process sets the flag value when a new frame of samples is available.
  {
    handle->spo2->spo2_percent ^= SPO2_STATUS_PROCESS_SPO2_FRAME; // Clear the flag. spo2_percent=0;
    // Count how many samples are Finger-On since the last Finger-Off.
    i=handle->spo2->spo2_buffer_in_index-handle->hr_update_interval;  // Set the output buffer index based on the Input index.
    if (i<0) i +=MAX_FRAME_SAMPLES;  // Wrap around
    for(k=0; k<handle->hr_update_interval; k++)  // Append the new block of samples
    {
      handle->spo2->spo2_raw_level_count = handle->spo2->spo2_raw_level_count>10000 ? handle->spo2->spo2_raw_level_count:handle->spo2->spo2_raw_level_count+1;  // limit the count to 10000
      if ((handle->spo2->spo2_red_dc_sample_buffer[i]<spo2_dc_min_thresh) || (handle->spo2->spo2_ir_dc_sample_buffer[i]<spo2_dc_min_thresh))
        handle->spo2->spo2_raw_level_count=0;   // Reset the count to 0
      if (++i==MAX_FRAME_SAMPLES) i=0;  // Wrap around
    }

    spo2_buffer_out_index=handle->spo2->spo2_buffer_in_index-handle->hr_iframe;  // Set the output buffer index based on the Input index.
    if (spo2_buffer_out_index<0) spo2_buffer_out_index +=MAX_FRAME_SAMPLES;  // Wrap around

    handle->spo2->time_since_finger_off = handle->spo2->time_since_finger_off <60000 ? handle->spo2->time_since_finger_off+1*handle->fs/10 : handle->spo2->time_since_finger_off;  // Limit to 60000

    if (handle->spo2->spo2_raw_level_count<handle->hr_iframe)
    {
      // Validation 1: Finger Off or On
      if (handle->spo2->spo2_raw_level_count < handle->hr_update_interval)
      {
        *p_heartrate_invalidation |= MAXM86161_HRM_STATUS_SPO2_FINGER_OFF;
        handle->spo2->time_since_finger_off=0;   // Reset if Finger off
      }
      else{
          *p_heartrate_invalidation |= MAXM86161_HRM_STATUS_SPO2_FINGER_ON;
      }
    }
    else
    {
      spo2_red_dc=0;
      spo2_ir_dc=0;
      spo2_red_ac2=0;   // sum(Red AC^2)
      spo2_ir_ac2=0;    // sum(IR AC^2)
      spo2_red_ac_min=0x7fff;
      spo2_ir_ac_min=0x7fff;
      spo2_red_ac_max=0x8000;
      spo2_ir_ac_max=0x8000;
      spo2_ac2_scaler=1;
      i=spo2_buffer_out_index;
      for(k=0; k<handle->hr_iframe; k++)
      {
        spo2_red_dc += handle->spo2->spo2_red_dc_sample_buffer[i];
        spo2_ir_dc  += handle->spo2->spo2_ir_dc_sample_buffer[i];
        if (spo2_red_ac2>0x20000000 || spo2_ir_ac2>0x20000000)  // AC_sample_buffer[i]<0x7fff
        {
          spo2_ac2_scaler++; spo2_red_ac2 >>=1; spo2_ir_ac2 >>=1;  // Change the scaler shift and scale sum(AC^2).
        }
        spo2_red_ac2 +=(handle->spo2->spo2_red_ac_sample_buffer[i]*handle->spo2->spo2_red_ac_sample_buffer[i])>>spo2_ac2_scaler;  // This is sum(AC^2)
        spo2_ir_ac2  +=(handle->spo2->spo2_ir_ac_sample_buffer[i] *handle->spo2->spo2_ir_ac_sample_buffer[i] )>>spo2_ac2_scaler;  // This is sum(AC^2)

        // Find the AC min and max.
        spo2_red_ac_min=handle->spo2->spo2_red_ac_sample_buffer[i]<spo2_red_ac_min ? handle->spo2->spo2_red_ac_sample_buffer[i]:spo2_red_ac_min;
        spo2_ir_ac_min =handle->spo2->spo2_ir_ac_sample_buffer[i] <spo2_ir_ac_min  ? handle->spo2->spo2_ir_ac_sample_buffer[i] :spo2_ir_ac_min;
        spo2_red_ac_max=handle->spo2->spo2_red_ac_sample_buffer[i]>spo2_red_ac_max ? handle->spo2->spo2_red_ac_sample_buffer[i]:spo2_red_ac_max;
        spo2_ir_ac_max =handle->spo2->spo2_ir_ac_sample_buffer[i] >spo2_ir_ac_max  ? handle->spo2->spo2_ir_ac_sample_buffer[i] :spo2_ir_ac_max;

        if (++i==MAX_FRAME_SAMPLES)
          i=0;  // Wrap around
      }

      // Calculate Crest factor: C^2=|Vpeak|^2/Vrms^2.
      i= -spo2_red_ac_min > spo2_red_ac_max ?  -spo2_red_ac_min : spo2_red_ac_max;  // i=|Vpeak|
      spo2_red_crest_factor = spo2_red_ac2>0 ? ((((i*i)>>spo2_ac2_scaler)*handle->hr_iframe)/spo2_red_ac2) : 9999;

      i= -spo2_ir_ac_min  > spo2_ir_ac_max  ?  -spo2_ir_ac_min  : spo2_ir_ac_max;   // i=|Vpeak|
      spo2_ir_crest_factor  = spo2_ir_ac2 >0 ? ((((i*i)>>spo2_ac2_scaler)*handle->hr_iframe)/spo2_ir_ac2) : 9999;

      spo2_crest_factor= spo2_red_crest_factor>spo2_ir_crest_factor ? spo2_red_crest_factor : spo2_ir_crest_factor;  // Take the larger one.

      // Validation 2: Check RED/IR AC with reference to its DC (to exclude the case on a non-finger surface or BPF transition).
      spo2_dc_to_ac_ratio=9999;
      if ((spo2_red_ac_max-spo2_red_ac_min)>0 && (spo2_ir_ac_max-spo2_ir_ac_min)>0)
      {
        spo2_dc_to_ac_ratio= (spo2_red_dc/handle->hr_iframe)/(spo2_red_ac_max-spo2_red_ac_min);
        spo2_dc_to_ac_ratio = spo2_dc_to_ac_ratio>((spo2_ir_dc/handle->hr_iframe)/(spo2_ir_ac_max-spo2_ir_ac_min)) ? spo2_dc_to_ac_ratio:((spo2_ir_dc/handle->hr_iframe)/(spo2_ir_ac_max-spo2_ir_ac_min));
      }

      if (((spo2_red_dc/handle->hr_iframe))>((spo2_red_ac_max-spo2_red_ac_min)*spo2_dc_to_acpp_ratio_high_thresh) || \
         ((spo2_ir_dc/handle->hr_iframe ))>((spo2_ir_ac_max -spo2_ir_ac_min )*spo2_dc_to_acpp_ratio_high_thresh))
      {
        *p_heartrate_invalidation |= MAXM86161_HRM_STATUS_SPO2_TOO_LOW_AC;   // Exclude the case on a non-finger surface
      }
      else if (((spo2_red_dc/handle->hr_iframe))<((spo2_red_ac_max-spo2_red_ac_min)*spo2_dc_to_acpp_ratio_low_thresh) || \
              ((spo2_ir_dc/handle->hr_iframe ))<((spo2_ir_ac_max -spo2_ir_ac_min )*spo2_dc_to_acpp_ratio_low_thresh))
      {
        *p_heartrate_invalidation |= MAXM86161_HRM_STATUS_SPO2_TOO_HIGH_AC;  // Exclude possible BPF transition.
      }
      else if (spo2_crest_factor>spo2_crest_factor_thresh)
      {
      // Validation 3: Crest factor check to exclude the finger moving and BPF transition.
        *p_heartrate_invalidation |= MAXM86161_HRM_STATUS_SPO2_CREST_FACTOR_OFF;
      }
      else
      {
        // Average DC and AC
        spo2_red_dc /= handle->hr_iframe;  // Averaged RED DC
        spo2_ir_dc  /= handle->hr_iframe;  // Averaged IR DC
        spo2_red_ac2/= handle->hr_iframe;  // Averaged RED AC^2
        spo2_ir_ac2 /= handle->hr_iframe;  // Averaged IR AC^2

        if (spo2_red_ac2>0 && spo2_ir_ac2>0)
        {
          // SpO2(%)= Coeff_A - Coeff_B*R, where R=(ACred/DCred)/(ACir/DCir)
          // 1. Calculate RMS(R)^2 as AC2=RMS^2.
          spo2_r=(R_SCALER*R_SCALER*((spo2_ir_dc*spo2_ir_dc)/spo2_red_dc))/spo2_red_dc;  // (R_SCALER*IR_DC/RED_DC)^2
          spo2_r=(spo2_r*spo2_red_ac2)/spo2_ir_ac2;

          // 2. Calculate 32-bit square root(The input is 32-bit (max<=0x3fffffff). The output is the square root in 16-bit spo2_r_root)
          spo2_r_root=1<<(SQRT_SHIFT-1);
          spo2_r_root_shift=1<<(SQRT_SHIFT-1);
          spo2_r_root_tmp=0;
          for (i=0; i<SQRT_SHIFT; i++)
          {
            if (spo2_r>=(spo2_r_root*spo2_r_root))
            {
              spo2_r_root_tmp=spo2_r_root;
            }
            spo2_r_root_shift >>= 1;  // Divided by 2
            spo2_r_root=spo2_r_root_shift+spo2_r_root_tmp;
          }

          // 3. SpO2(%)=114 - 32*R (Coeff_A and Coeff_B require calibration on different hardware platforms)
          handle->spo2->spo2_percent=(int16_t)(112-(35*spo2_r_root+R_SCALER/2)/R_SCALER);  // R_SCALER/2 due to the round-off.
          // 4. Limit SpO2(%) to 75~99%
          handle->spo2->spo2_percent= handle->spo2->spo2_percent>99 ? 99 : handle->spo2->spo2_percent;  // Upper limit=99%
          if (handle->spo2->spo2_percent<75)
            {
            *p_heartrate_invalidation |= MAXM86161_HRM_STATUS_SPO2_EXCEPTION;  // If SpO2<75%, SpO2 is invalid.
            }

          else if (handle->spo2->time_since_finger_off < 51*handle->fs/10/10) // Delay 5.1(s) to report SpO2 after the last Finger off.
            {
              *p_heartrate_invalidation |= MAXM86161_HRM_STATUS_SPO2_EXCEPTION;
            }
          else // update spo2 when it is really changed and valid
            *SpO2=handle->spo2->spo2_percent;
        }
        else
        {
          // Exception: Can occur if the finger is a little distance above the sensors without contact.
          *p_heartrate_invalidation |= MAXM86161_HRM_STATUS_SPO2_EXCEPTION;
        }
      }
    }
    // misunderstand in here, should not update spo2 value
    //*SpO2=handle->spo2->spo2_percent;
    if(hrm_data != 0)  // Copy these varibles to hrm_data for display.
    {
      i=handle->spo2->spo2_buffer_in_index-1;  // i points to the newest sample.
      if (i<0) i += MAX_FRAME_SAMPLES;  // Wrap around
      hrm_data->spo2_red_dc_sample=handle->spo2->spo2_red_dc_sample_buffer[i];
      hrm_data->spo2_ir_dc_sample=handle->spo2->spo2_ir_dc_sample_buffer[i];
      hrm_data->spo2_dc_to_ac_ratio=spo2_dc_to_ac_ratio;
      hrm_data->spo2_crest_factor=spo2_crest_factor;
      hrm_data->spo2_ir_perfusion_index=10000*(spo2_ir_ac_max-spo2_ir_ac_min)/spo2_ir_dc;
      for(i=0; i<3; i++) hrm_data->dc_sensing_led[i]=handle->dc_sensing_led[i];
    }
  }

  return error;
}

/**************************************************************************//**
 * @brief Initialize HRM/SpO2 measurement parameters
 *****************************************************************************/
static int32_t maxm86161_hrm_init_measurement_parameters (maxm_hrm_handle_t *handle, int16_t measurement_rate)
{
  int32_t error = MAXM86161_HRM_SUCCESS;

  // fs=21~25Hz. Interpolate by a factor of 4 to fs=84~100Hz
  handle->hrm_interpolator_factor=4;
  handle->phrm_interpolator_coefs=(int16_t *)hrm_interpolator_coefs_r4;


  // Set fs and the BPF coefficients per measurementRate
  switch(measurement_rate)
  {
    case FS_25HZ:  // 25Hz
      handle->fs=1000;  // (Hz*10)
      handle->bpf_biquads=sizeof(bpf_95hz)/6/4;
      handle->pbpf_b_0=bpf_95hz;
      handle->bpf_output_scaler=bpf_output_scaler_95hz;
      break;
    default:
      error = MAXM86161_HRM_ERROR_INVALID_MEASUREMENT_RATE;
      goto Error;
      // break not needed because of goto statement above
      // break;
  }

  handle->hr_update_interval=1*(handle->fs+5)/10;   // (sample) in 1s

  handle->t_low0 =60*(handle->fs+5)/10/f_low-1;   // (sample), -1 for the auto-corr max validation.
  handle->t_high0=60*(handle->fs+5)/10/f_high+1;  // (sample), +1 for the auto-corr max validation.
  handle->num_of_lowest_hr_cycles=num_of_lowest_hr_cycles_min;
  handle->hr_iframe=handle->t_low0*handle->num_of_lowest_hr_cycles/100;  // (sample)

  maxm86161_hrm_initialize_buffers(handle);

  // Normalize HRM PS to around 20000 for the HRM algorithm
  handle->hrm_normalization_scaler=(1<<QF_SCALER);   // Scaler=1.0 in Qf
  handle->red_normalization_scaler=(1<<QF_SCALER);  // Scaler=1.0 in Qf
  handle->ir_normalization_scaler=(1<<QF_SCALER);     // Scaler=1.0 in Qf

  // Seems we can use same values for 3 cases.
  handle->hrm_ps_vpp_max=HRM_PS_VPP_MAX_GREEN_FINGERTIP;
  handle->hrm_ps_crestfactor_thresh=HRM_CREST_FACTOR_THRESH_FINGERTIP;
  handle->hrm_ps_vpp_min=handle->hrm_ps_vpp_max * (-1);

Error:
  return error;
}

/**************************************************************************//**
 * @brief Get maxm86161 sample from the sample queue
 *****************************************************************************/
static int32_t maxm86161_hrm_get_sample(maxm86161_hrm_irq_sample_t *samples)
{
  int32_t error = MAXM86161_HRM_SUCCESS;
  error = maxm86161_hrm_helper_sample_queue_get(samples);
  return error;
}

/**************************************************************************//**
 * @brief Identify maxm86161 parts
 *****************************************************************************/
static int32_t maxm86161_hrm_identify_part(int16_t *part_id)
{
  int32_t valid_part = 0;

  *part_id = maxm86161_i2c_read_from_register(MAXM86161_REG_PART_ID);

  switch(*part_id)
  { // Static HRM/SpO2 supports all maxm86161 parts
    case 0x36:
      valid_part = 1;
      break;
    default:
      valid_part = 0;
      break;
  }
  return valid_part;
}

/**************************************************************************//**
 * @brief Maxm86161 HRM/SpO2 sample process
 *****************************************************************************/
static int32_t maxm86161_hrm_sample_process(maxm_hrm_handle_t *handle, uint32_t hrm_ps, uint32_t spo2_ps_red, uint32_t spo2_ps_ir, mamx86161_hrm_data_t *hrm_data)
{
  int32_t error = MAXM86161_HRM_SUCCESS;
  uint16_t ps_local;
  uint16_t raw_ps_local;
  uint16_t i, j, i_interpolator;
  uint16_t spo2_ps_red_scaled, spo2_ps_ir_scaled;
  int32_t interpolator_ps;

  ps_local=hrm_ps;     // From maxm86161 EVB or the captured file
  raw_ps_local=hrm_ps;   // Use for display purpose

#if HRM_PS_AGC  // Change the HRM normalized scaler, so that the HRM BPF will not see the disturbance due to LED current change.
  if(handle->hrm_agc[0].agc_flag == 1)
  {
    handle->hrm_normalization_scaler = handle->hrm_normalization_scaler * handle->hrm_agc[0].saved_ppg / hrm_ps;  // Modify the normalization scaler based on the current and previous samples.
    for (i=0; i<(INTERPOLATOR_L*2); i++) handle->hrm_interpolator_buf[i] = handle->hrm_interpolator_buf[i]*hrm_ps / handle->hrm_agc[0].saved_ppg;  // Scale the HRM interpolator data buffer
    handle->hrm_agc[0].agc_flag = 0;
  }
  if(handle->hrm_agc[1].agc_flag == 1)
  {
    handle->ir_normalization_scaler = handle->ir_normalization_scaler * handle->hrm_agc[1].saved_ppg / spo2_ps_ir;  // Modify the normalization scaler based on the current and previous samples.
    for (i=0; i<(INTERPOLATOR_L*2); i++) handle->spo2->spo2_ir_interpolator_buf[i] = handle->spo2->spo2_ir_interpolator_buf[i]*spo2_ps_ir / handle->hrm_agc[1].saved_ppg;  // Scale the SpO2 interpolator data buffer
    handle->hrm_agc[1].agc_flag = 0;
  }
  if(handle->hrm_agc[2].agc_flag == 1)
  {
    handle->red_normalization_scaler = handle->red_normalization_scaler * handle->hrm_agc[2].saved_ppg / spo2_ps_red;  // Modify the normalization scaler based on the current and previous samples.
    for (i=0; i<(INTERPOLATOR_L*2); i++) handle->spo2->spo2_red_interpolator_buf[i] = handle->spo2->spo2_red_interpolator_buf[i]*spo2_ps_ir / handle->hrm_agc[2].saved_ppg;  // Scale the SpO2 interpolator data buffer
    handle->hrm_agc[2].agc_flag = 0;
  }
#endif

  handle->hrm_agc[0].saved_ppg = hrm_ps;       // ppg1
  handle->hrm_agc[1].saved_ppg = spo2_ps_ir;   // ppg2
  handle->hrm_agc[2].saved_ppg = spo2_ps_red;  // ppg3

  handle->hrm_raw_ps=raw_ps_local;    // Save the raw PS for next sample or frame process.

  // Interpolator causes a delay of INTERPOLATOR_L-1 samples due to the its filter.
  for (i=0; i<(INTERPOLATOR_L*2-1); i++) handle->hrm_interpolator_buf[i]=handle->hrm_interpolator_buf[i+1];  // Use memmove() instead for efficiency
  handle->hrm_interpolator_buf[i]=ps_local;  // Add the new HRM sample to the interpolator buffer

  if (handle->spo2 != NULL)  // SpO2 interpolator buffer shift and add new samples.
  {
    for (i=0; i<(INTERPOLATOR_L*2-1); i++)  // Shift the buffer
    {
      handle->spo2->spo2_red_interpolator_buf[i]=handle->spo2->spo2_red_interpolator_buf[i+1];    // Use memmove() instead for efficiency
      handle->spo2->spo2_ir_interpolator_buf[i] =handle->spo2->spo2_ir_interpolator_buf[i+1];   // Use memmove() instead for efficiency
    }
    handle->spo2->spo2_red_interpolator_buf[i]=spo2_ps_red;  // Add the new SpO2 Red sample to the interpolator buffer
    handle->spo2->spo2_ir_interpolator_buf[i] =spo2_ps_ir;   // Add the new SpO2 IR sample to the interpolator buffer
  }

  // finally it will push 4 samples into the sample queue instead of one sample
  for (i_interpolator=0; i_interpolator<handle->hrm_interpolator_factor; i_interpolator++)
  {
    if (i_interpolator==0)
    {
      if (handle->hrm_interpolator_factor >1)  // Don't touch hrm_ps and spo2_ps_red/IR if handle->hrm_interpolator_factor==1;
      {
        ps_local=handle->hrm_interpolator_buf[INTERPOLATOR_L-1];
        if (handle->spo2 != NULL)  // SpO2 interpolating
        {
          spo2_ps_red=handle->spo2->spo2_red_interpolator_buf[INTERPOLATOR_L-1];
          spo2_ps_ir =handle->spo2->spo2_ir_interpolator_buf[INTERPOLATOR_L-1];
        }
      }
    }
    else
    {
      interpolator_ps=0;
      for (j=0; j<INTERPOLATOR_L*2; j++)
        interpolator_ps += handle->hrm_interpolator_buf[j]*(handle->phrm_interpolator_coefs[(i_interpolator-1)*INTERPOLATOR_L*2+j]);  // -1 because of no 1st interpolator
      ps_local= interpolator_ps<0 ? 0: (interpolator_ps>>15); // Interpolator_Coefs are in Q15

      if (handle->spo2 != NULL)  // SpO2 interpolating
      {
        interpolator_ps=0;
        for (j=0; j<INTERPOLATOR_L*2; j++)
          interpolator_ps += handle->spo2->spo2_red_interpolator_buf[j]*(handle->phrm_interpolator_coefs[(i_interpolator-1)*INTERPOLATOR_L*2+j]);  // -1 because of no 1st interpolator
        spo2_ps_red= interpolator_ps<0 ? 0: (interpolator_ps>>15);  // Interpolator_Coefs are in Q15
        interpolator_ps=0;
        for (j=0; j<INTERPOLATOR_L*2; j++)
          interpolator_ps += handle->spo2->spo2_ir_interpolator_buf[j] *(handle->phrm_interpolator_coefs[(i_interpolator-1)*INTERPOLATOR_L*2+j]);  // -1 because of no 1st interpolator
        spo2_ps_ir = interpolator_ps<0 ? 0: (interpolator_ps>>15);  // Interpolator_Coefs are in Q15
      }
    }

    raw_ps_local=ps_local;  // raw_ps_local after interpolator if handle->hrm_interpolator_factor>1.

  // 0) Normalize PS
  ps_local=(uint16_t)(((int32_t)ps_local*(int32_t)handle->hrm_normalization_scaler)>>QF_SCALER);  // hrm_normalization_scaler in Q8

  maxm86161_hrm_bpf_filtering(handle, (int32_t)ps_local, &handle->hrm_sample_buffer[handle->hrm_buffer_in_index], &handle->hrm_bpf);  // Add the BPF(PS) in the framed sample circular buffer.

  if (handle->spo2 != NULL)  // SpO2 BPF process and compute Red_DC, Red_AC, IR_DC and IR_AC
  {
    // Normalize SpO2 PSs, removing the DC floor of 256
    spo2_ps_red_scaled=(uint16_t)(((int32_t)(spo2_ps_red-256)*(int32_t)handle->red_normalization_scaler)>>QF_SCALER);  // red_normalization_scaler in Q8
    spo2_ps_ir_scaled=(uint16_t)(((int32_t)(spo2_ps_ir-256)*(int32_t)handle->ir_normalization_scaler)>>QF_SCALER);   // ir_normalization_scaler in Q8

    maxm86161_hrm_bpf_filtering(handle, (int32_t)spo2_ps_red_scaled, &handle->spo2->spo2_red_ac_sample_buffer[handle->spo2->spo2_buffer_in_index], &handle->spo2->spo2_red_bpf);  // Add the BPF(PS2) in the framed sample circular buffer.
    maxm86161_hrm_bpf_filtering(handle, (int32_t)spo2_ps_ir_scaled,  &handle->spo2->spo2_ir_ac_sample_buffer[handle->spo2->spo2_buffer_in_index],  &handle->spo2->spo2_ir_bpf);   // Add the BPF(PS3) in the framed sample circular buffer.
    handle->spo2->spo2_red_dc_sample_buffer[handle->spo2->spo2_buffer_in_index]=(uint16_t)spo2_ps_red_scaled;
    handle->spo2->spo2_ir_dc_sample_buffer[handle->spo2->spo2_buffer_in_index] =(uint16_t)spo2_ps_ir_scaled;
    if (++handle->spo2->spo2_buffer_in_index==MAX_FRAME_SAMPLES) handle->spo2->spo2_buffer_in_index=0;  // Wrap around
  }

    // Count how many samples are valid since the last Finger-Off. Finger-On detection uses the raw PS and its detection threshold.
    //if (++handle->hrm_ps_raw_level_count>10000) handle->hrm_ps_raw_level_count--;   // limit the count
  if (hrm_ps < HRM_DC_SENSING_WORK_LEVEL * 0.5)
    handle->hrm_ps_raw_level_count = 0;
  else
    {
      if (++handle->hrm_ps_raw_level_count>MAX_FRAME_SAMPLES) handle->hrm_ps_raw_level_count--;   // limit the count
    }
    handle->sample_count++;

    if (hrm_data != 0)  // Copy these variables to hrm_data for display.
    {
      hrm_data->fs = handle->fs/handle->hrm_interpolator_factor;
      hrm_data->hr_iframe = handle->hr_iframe;
      hrm_data->hr_update_interval = handle->hr_update_interval/handle->hrm_interpolator_factor;
      hrm_data->hrm_raw_ps = raw_ps_local;
      hrm_data->hrm_ps = ps_local;
      hrm_data->hrm_num_samples = handle->hrm_interpolator_factor;
      hrm_data->hrm_interpolated_ps[i_interpolator] = raw_ps_local;
      hrm_data->hrm_samples[i_interpolator] = handle->hrm_sample_buffer[handle->hrm_buffer_in_index];
    }

    if (++handle->hrm_buffer_in_index==MAX_FRAME_SAMPLES) handle->hrm_buffer_in_index=0;  // Wrap around
  }
  handle->normalized_hrm_ps=ps_local;  // Save the HRM raw PS for next sample or frame process.

  return error;
}
