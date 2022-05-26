/***************************************************************************//**
 * @file vl53l1x.c
 * @brief VL53L1X Source file
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
 *
 * EVALUATION QUALITY
 * This code has been minimally tested to ensure that it builds with the
 * specified dependency versions and is suitable as a demonstration for
 * evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 *
 ******************************************************************************/

#include <stddef.h>
#include "vl53l1x_core.h"
#include "vl53l1x_calibration.h"
#include "vl53l1x_config.h"
#include "vl53l1_platform.h"
#include "vl53l1x.h"


#ifdef __cplusplus
extern "C" {
#endif

sl_status_t vl53l1x_init(uint16_t dev) {
	return VL53L1X_SensorInit(dev);
}

sl_status_t vl53l1x_set_i2cspm_instance(sl_i2cspm_t *i2cspm_instance) {
	if (NULL == i2cspm_instance) {
		return SL_STATUS_INVALID_PARAMETER;
	}

	vl53l1x_platform_set_i2cspm_instance(i2cspm_instance);

	return SL_STATUS_OK;
}

sl_status_t vl53l1x_start_ranging(uint16_t dev) {
	return VL53L1X_StartRanging(dev);
}

sl_status_t vl53l1x_stop_ranging(uint16_t dev) {
	return VL53L1X_StopRanging(dev);
}

sl_status_t vl53l1x_set_i2c_address(uint16_t dev, uint8_t new_address) {
	return VL53L1X_SetI2CAddress(dev, new_address);
}

sl_status_t vl53l1x_clear_interrupt(uint16_t dev) {
	return VL53L1X_ClearInterrupt(dev);
}

sl_status_t vl53l1x_set_interrupt_polarity(uint16_t dev, uint8_t int_pol) {
	return VL53L1X_SetInterruptPolarity(dev, int_pol);
}

sl_status_t vl53l1x_get_interrupt_polarity(uint16_t dev, uint8_t *int_pol) {
	if (NULL == int_pol) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetInterruptPolarity(dev, int_pol);
}

sl_status_t vl53l1x_check_for_data_ready(uint16_t dev, uint8_t *is_data_ready) {
	if (NULL == is_data_ready) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_CheckForDataReady(dev, is_data_ready);
}

sl_status_t vl53l1x_set_timing_budget_in_ms(uint16_t dev,
		uint16_t timing_budget_in_ms) {
	return VL53L1X_SetTimingBudgetInMs(dev, timing_budget_in_ms);
}

sl_status_t vl53l1x_get_timing_budget_in_ms(uint16_t dev,
		uint16_t *timing_budget_in_ms) {
	if (NULL == timing_budget_in_ms) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetTimingBudgetInMs(dev, timing_budget_in_ms);
}

sl_status_t vl53l1x_set_distance_mode(uint16_t dev, uint16_t distance_mode) {
	return VL53L1X_SetDistanceMode(dev, distance_mode);
}

sl_status_t vl53l1x_get_distance_mode(uint16_t dev, uint16_t *distance_mode) {
	if (NULL == distance_mode) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetDistanceMode(dev, distance_mode);
}

sl_status_t vl53l1x_set_inter_measurement_in_ms(uint16_t dev,
		uint32_t inter_measurement_in_ms) {
	return VL53L1X_SetInterMeasurementInMs(dev, inter_measurement_in_ms);
}

sl_status_t vl53l1x_get_inter_measurement_in_ms(uint16_t dev,
		uint16_t *inter_measurement_in_ms) {
	if (NULL == inter_measurement_in_ms) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetInterMeasurementInMs(dev, inter_measurement_in_ms);
}

sl_status_t vl53l1x_get_boot_state(uint16_t dev, uint8_t *state) {
	if (NULL == state) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_BootState(dev, state);
}

sl_status_t vl53l1x_get_sensor_id(uint16_t dev, uint16_t *id) {
	if (NULL == id) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetSensorId(dev, id);
}

sl_status_t vl53l1x_get_distance(uint16_t dev, uint16_t *distance) {
	if (NULL == distance) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetDistance(dev, distance);
}

sl_status_t vl53l1x_get_signal_per_spad(uint16_t dev, uint16_t *signal_per_spad) {
	if (NULL == signal_per_spad) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetSignalPerSpad(dev, signal_per_spad);
}

sl_status_t vl53l1x_get_ambient_per_spad(uint16_t dev, uint16_t *ambient) {
	if (NULL == ambient) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetAmbientPerSpad(dev, ambient);
}

sl_status_t vl53l1x_get_signal_rate(uint16_t dev, uint16_t *signal_rate) {
	if (NULL == signal_rate) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetSignalRate(dev, signal_rate);
}

sl_status_t vl53l1x_get_spads_enabled(uint16_t dev, uint16_t *spads_enabled) {
	if (NULL == spads_enabled) {
		return SL_STATUS_INVALID_PARAMETER;
	}

	return VL53L1X_GetSpadNb(dev, spads_enabled);
}

sl_status_t vl53l1x_get_ambient_rate(uint16_t dev, uint16_t *ambient_rate) {
	if (NULL == ambient_rate) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetAmbientRate(dev, ambient_rate);
}

sl_status_t vl53l1x_get_range_status(uint16_t dev, uint8_t *range_status) {
	if (NULL == range_status) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetRangeStatus(dev, range_status);
}

sl_status_t vl53l1x_get_result(uint16_t dev, vl53l1x_result_t *result) {
	VL53L1X_Result_t vl53_result;
	sl_status_t ret = SL_STATUS_OK;

	if (NULL == result) {
		return SL_STATUS_INVALID_PARAMETER;
	}

	ret = VL53L1X_GetResult(dev, &vl53_result);

	result->status = vl53_result.Status;
	result->ambient = vl53_result.Ambient;
	result->distance = vl53_result.Distance;
	result->number_per_spads = vl53_result.NumSPADs;
	result->signal_per_spad = vl53_result.SigPerSPAD;

	return ret;
}

sl_status_t vl53l1x_set_offset(uint16_t dev, int16_t offset_value) {
	return VL53L1X_SetOffset(dev, offset_value);
}

sl_status_t vl53l1x_get_offset(uint16_t dev, int16_t *offset) {
	if (NULL == offset) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetOffset(dev, offset);
}

sl_status_t vl53l1x_set_xtalk(uint16_t dev, uint16_t x_talk_value) {
	return VL53L1X_SetXtalk(dev, x_talk_value);
}

sl_status_t vl53l1x_get_xtalk(uint16_t dev, uint16_t *x_talk_value) {
	if (NULL == x_talk_value) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetXtalk(dev, x_talk_value);
}

sl_status_t vl53l1x_set_distance_threshold(uint16_t dev, uint16_t threshold_low,
		uint16_t threshold_high, uint8_t window) {
	return VL53L1X_SetDistanceThreshold(dev, threshold_low, threshold_high,
			window, 0);
}

sl_status_t vl53l1x_set_distance_threshold_window_mode(uint16_t dev,
		uint16_t *window) {
	if (NULL == window) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetDistanceMode(dev, window);
}

sl_status_t vl53l1x_get_distance_threshold_low(uint16_t dev, uint16_t *low) {
	if (NULL == low) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetDistanceThresholdLow(dev, low);
}

sl_status_t vl53l1x_get_distance_threshold_high(uint16_t dev, uint16_t *high) {
	if (NULL == high) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetDistanceThresholdHigh(dev, high);
}

sl_status_t vl53l1x_set_roi_xy(uint16_t dev, uint16_t x, uint16_t y) {
	return VL53L1X_SetROI(dev, x, y);
}

sl_status_t vl53l1x_get_roi_xy(uint16_t dev, uint16_t *x, uint16_t *y) {
	if (NULL == x && NULL == y) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetROI_XY(dev, x, y);
}

sl_status_t vl53l1x_set_roi_center(uint16_t dev, uint8_t center) {
	return VL53L1X_SetROICenter(dev, center);
}

sl_status_t vl53l1x_get_roi_center(uint16_t dev, uint8_t *center) {
	if (NULL == center) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetROICenter(dev, center);
}

sl_status_t vl53l1x_set_signal_threshold(uint16_t dev, uint16_t signal) {
	return VL53L1X_SetSignalThreshold(dev, signal);
}

sl_status_t vl53l1x_get_signal_threshold(uint16_t dev, uint16_t *signal) {
	if (NULL == signal) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetSignalThreshold(dev, signal);
}

sl_status_t vl53l1x_set_sigma_threshold(uint16_t dev, uint16_t sigma) {
	return VL53L1X_SetSigmaThreshold(dev, sigma);
}

sl_status_t vl53l1x_get_sigma_threshold(uint16_t dev, uint16_t *sigma) {
	if (NULL == sigma) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_GetSigmaThreshold(dev, sigma);
}

sl_status_t vl53l1x_start_temperature_update(uint16_t dev) {
	return VL53L1X_StartTemperatureUpdate(dev);
}

sl_status_t vl53l1x_get_core_sw_version(vl53l1x_core_version_t *version) {
	VL53L1X_Version_t vl53_version;
	sl_status_t ret = SL_STATUS_OK;

	if (NULL == version) {
		return SL_STATUS_INVALID_PARAMETER;
	}

	ret = VL53L1X_GetSWVersion(&vl53_version);

	version->build = vl53_version.build;
	version->major = vl53_version.major;
	version->minor = vl53_version.minor;
	version->revision = vl53_version.revision;

	return ret;
}

sl_status_t vl53l1x_calibrate_offset(uint16_t dev,
		uint16_t target_distance_in_mm, int16_t *offset) {
	if (NULL == offset) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_CalibrateOffset(dev, target_distance_in_mm, offset);
}

sl_status_t vl53l1x_calibrate_xtalk(uint16_t dev,
		uint16_t target_distance_in_mm, uint16_t *xtalk) {
	if (NULL == xtalk) {
		return SL_STATUS_INVALID_PARAMETER;
	}
	return VL53L1X_CalibrateXtalk(dev, target_distance_in_mm, xtalk);
}

#ifdef __cplusplus
}
#endif

