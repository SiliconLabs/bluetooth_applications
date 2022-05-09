/***************************************************************************//**
 * @file vl53l1x.h
 * @brief VL53L1X Prototypes
 * @version 1.0.0
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

#ifndef VL53L1X_H_
#define VL53L1X_H_

#include "sl_status.h"
#include "sl_i2cspm.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup VL53L1X
 * @{
 *
 * @brief
 *  The implementation of a laser distance sensor using VL53L1X sensor.
 *
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *    Distance mode - SHORT (~1.3m range)
 ******************************************************************************/
#define VL53L1X_DISTANCE_MODE_SHORT 1

/***************************************************************************//**
 * @brief
 *    Distance mode - LONG (~4m range)
 ******************************************************************************/
#define VL53L1X_DISTANCE_MODE_LONG 2

/***************************************************************************//**
 * @brief
 *    Typedef for specifying the software version of the core driver.
 ******************************************************************************/

typedef struct {
	uint8_t major; /*!< major number */
	uint8_t minor; /*!< minor number */
	uint8_t build; /*!< build number */
	uint32_t revision; /*!< revision number */
} vl53l1x_core_version_t;

/***************************************************************************//**
 * @brief
 *    Typedef for specifying the packed result data.
 ******************************************************************************/
typedef struct {
	uint8_t status; /*!< Result status */
	uint16_t distance; /*!< Result distance */
	uint16_t ambient; /*!< Result ambient */
	uint16_t signal_per_spad;/*!< Result signal per SPAD */
	uint16_t number_per_spads; /*!< Result number per SPADs */
} vl53l1x_result_t;

/***************************************************************************//**
 * @brief
 *    This function loads the 135 bytes default values to initialize the sensor.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_init(uint16_t dev);

/***************************************************************************//**
 * @brief
 *    This function sets the IC2SPM instance used by platform functions.
 *
 * @param[in] i2cspm_instance
 *    I2CSPM instance, default: VL53L1X_CONFIG_I2C_INSTANCE
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if int_pol is invalid.
 ******************************************************************************/
sl_status_t vl53l1x_set_i2cspm_instance(sl_i2cspm_t *i2cspm_instance);

/***************************************************************************//**
 * @brief
 *    This function starts the ranging distance operation\n
 *    The ranging operation is continuous. The clear interrupt has to
 *    be done after each get data to allow the interrupt to raise
 *    when the next data is ready\n
 *    1=active high (default), 0=active low, use SetInterruptPolarity()
 *    to change the interrupt polarity if required.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_start_ranging(uint16_t dev);

/***************************************************************************//**
 * @brief
 *    This function stops the ranging.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_stop_ranging(uint16_t dev);

/***************************************************************************//**
 * @brief
 *    This function sets the I2C address of the device.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[in] new_address
 *    New address of the selected device.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_set_i2c_address(uint16_t dev, uint8_t new_address);

/***************************************************************************//**
 * @brief
 *    This function clears the interrupt, to be called after a ranging
 *    data reading to arm the interrupt for the next data ready event.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_clear_interrupt(uint16_t dev);

/***************************************************************************//**
 * @brief
 *    This function configures the interrupt polarity\n
 *    1=active high (default), 0=active low
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[in] int_pol
 *    Interrupt line polarity. Valid values: [0;1]
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_set_interrupt_polarity(uint16_t dev, uint8_t int_pol);

/***************************************************************************//**
 * @brief
 *    This function returns the interrupt polarity\n
 *    1=active high (default), 0=active low
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] int_pol
 *    Interrupt line polarity. Valid values: [0;1]
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if int_pol is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_interrupt_polarity(uint16_t dev, uint8_t *int_pol);

/***************************************************************************//**
 * @brief
 *    This function checks if the new ranging data is available
 *    by polling the dedicated register.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] is_data_ready
 *    Data ready. Valid values: [0=No;1=Yes]
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if int_pol is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_check_for_data_ready(uint16_t dev, uint8_t *is_data_ready);

/***************************************************************************//**
 * @brief
 *    This function programs the timing budget in ms.
 *    Predefined values = 15, 20, 33, 50, 100(default), 200, 500.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[in] timing_budget_in_ms
 *    Timing budget in ms. Valid values: 15, 20, 33, 50, 100(default), 200, 500.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if int_pol is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_set_timing_budget_in_ms(uint16_t dev,
		uint16_t timing_budget_in_ms);

/***************************************************************************//**
 * @brief
 *    This function returns the current timing budget in ms.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] timing_budget_in_ms
 *    Timing budget in ms. Valid values: 15, 20, 33, 50, 100(default), 200, 500.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if int_pol is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_timing_budget_in_ms(uint16_t dev,
		uint16_t *timing_budget_in_ms);

/***************************************************************************//**
 * @brief
 *    This function programs the distance mode (1=short, 2=long(default)).
 *    Short mode max distance is limited to 1.3 m but better ambient immunity.\n
 *    Long mode can range up to 4 m in the dark with 200 ms timing budget.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[in] distance_mode
 *    Selects distance mode. (1=short, 2=long(default))
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if int_pol is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_set_distance_mode(uint16_t dev, uint16_t distance_mode);

/***************************************************************************//**
 * @brief
 *    This function returns the current distance mode (1=short, 2=long).
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] distance_mode
 *    Returns distance mode. (1=short, 2=long(default))
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if int_pol is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_distance_mode(uint16_t dev, uint16_t *distance_mode);

/***************************************************************************//**
 * @brief
 *    This function programs the Intermeasurement period in ms\n
 *    Intermeasurement period must be >/= timing budget.
 *    This condition is not checked by the API, the customer has the
 *    duty to check the condition. Default = 100 ms.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[in] inter_measurement_in_ms
 *    Configures inter measurement time in ms.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_set_inter_measurement_in_ms(uint16_t dev,
		uint32_t inter_measurement_in_ms);

/***************************************************************************//**
 * @brief
 *    This function returns the Intermeasurement period in ms.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] inter_measurement_in_ms
 *    Returns inter measurement time in ms.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if inter_measurement_in_ms is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_inter_measurement_in_ms(uint16_t dev,
		uint16_t *inter_measurement_in_ms);

/***************************************************************************//**
 * @brief
 *    This function returns the boot state of the device (1:booted, 0:not booted).
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] state
 *    Returns the boot state of the device. (1:booted, 0:not booted)
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if state is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_boot_state(uint16_t dev, uint8_t *state);

/***************************************************************************//**
 * @brief
 *    This function returns the sensor id, sensor Id must be 0xEEAC.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] id
 *    Returns id of the device. (Must be 0xEEAC)
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if id is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_sensor_id(uint16_t dev, uint16_t *id);

/***************************************************************************//**
 * @brief
 *    This function returns the distance measured by the sensor in mm.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] distance
 *    Returns id of the device. (Must be 0xEEAC)
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if distance is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_distance(uint16_t dev, uint16_t *distance);

/***************************************************************************//**
 * @brief
 *    This function returns the returned signal per SPAD in kcps/SPAD.
 * With kcps stands for Kilo Count Per Second
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] signal_per_spad
 *    Returns signal per SPAD in kcps/SPAD.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if signal_per_spad is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_signal_per_spad(uint16_t dev, uint16_t *signal_per_spad);

/***************************************************************************//**
 * @brief
 *    This function returns the ambient per SPAD in kcps/SPAD.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] ambient
 *    Returns ambient per SPAD in kcps/SPAD.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if ambient is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_ambient_per_spad(uint16_t dev, uint16_t *ambient);

/***************************************************************************//**
 * @brief
 *    This function returns the returned signal in kcps.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] signal_rate
 *    Returns returned signal in kcps.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if signal_per_spad is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_signal_rate(uint16_t dev, uint16_t *signal_rate);

/***************************************************************************//**
 * @brief
 *    This function returns the current number of enabled SPADs
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] spads_enabled
 *    Returns the number of enabled SPADs.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if spads_enabled is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_spads_enabled(uint16_t dev, uint16_t *spads_enabled);

/***************************************************************************//**
 * @brief
 *    This function returns the ambient rate in kcps
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] ambient_rate
 *    Returns the ambient rate in kcps.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if ambient_rate is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_ambient_rate(uint16_t dev, uint16_t *ambient_rate);

/***************************************************************************//**
 * @brief
 *    This function returns the ranging status error \n
 *    (0:no error, 1:sigma failed, 2:signal failed, ..., 7:wrap-around)
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] range_status
 *    Returns the ranging status.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if range_status is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_range_status(uint16_t dev, uint8_t *range_status);

/***************************************************************************//**
 * @brief
 *    This function returns measurements and the
 *    range status in a single read access
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] result
 *    Returns the measurement and the range status in a single read access.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if result is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_result(uint16_t dev, vl53l1x_result_t *result);

/***************************************************************************//**
 * @brief
 *    This function programs the offset correction in mm.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[in] offset_value
 *    The offset correction value to program in mm.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_set_offset(uint16_t dev, int16_t offset_value);

/***************************************************************************//**
 * @brief
 *    This function returns the programmed offset correction value in mm.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] offset_value
 *    Return the offset correction value programmed in mm.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if offset is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_offset(uint16_t dev, int16_t *offset);

/***************************************************************************//**
 * @brief
 * This function programs the xtalk correction value in cps (Count Per Second).\n
 * This is the number of photons reflected back from the cover glass in cps.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[in] x_talk_value
 *    The Xtalk correction value to program in cps. (Count Per Second)
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_set_xtalk(uint16_t dev, uint16_t x_talk_value);

/***************************************************************************//**
 * @brief
 * This function returns the current programmed xtalk correction value in cps.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[out] x_talk_value
 *    The Xtalk correction value to program in cps. (Count Per Second)
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if x_talk_value is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_xtalk(uint16_t dev, uint16_t *x_talk_value);

/***************************************************************************//**
 * @brief
 * This function programs the threshold detection mode\n
 * Example:\n
 * vl53l1x_set_distance_threshold(dev,100,300,0): Below 100 \n
 * vl53l1x_set_distance_threshold(dev,100,300,1): Above 300 \n
 * vl53l1x_set_distance_threshold(dev,100,300,2): Out of window \n
 * vl53l1x_set_distance_threshold(dev,100,300,3): In window \n
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])

 * @param[in] threshold_low
 *    The threshold under which one
 *    the device raises an interrupt if Window = 0 (mm)
 * @param[in] threshold_high
 *    the threshold above which one
 *    the device raises an interrupt if Window = 1 (mm)
 * @param[in] window
 *    Window detection mode : 0=below, 1=above, 2=out, 3=in
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if x_talk_value is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_set_distance_threshold(uint16_t dev, uint16_t threshold_low,
		uint16_t threshold_high, uint8_t window);

/***************************************************************************//**
 * @brief
 * This function returns the window detection mode (0=below; 1=above; 2=out; 3=in)
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])

 * @param[out] window
 *    Window detection mode. (0=below; 1=above; 2=out; 3=in)
 *
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if window is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_set_distance_threshold_window_mode(uint16_t dev,
		uint16_t *window);

/***************************************************************************//**
 * @brief
 * This function returns the low threshold in mm.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])

 * @param[out] low
 *    Low threshold in mm.
 *
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if low is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_distance_threshold_low(uint16_t dev, uint16_t *low);

/***************************************************************************//**
 * @brief
 * This function returns the high threshold in mm.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])

 * @param[out] high
 *    High threshold in mm.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if high is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_distance_threshold_high(uint16_t dev, uint16_t *high);

/***************************************************************************//**
 * @brief
 * This function programs the ROI (Region of Interest)\n
 * The ROI position is centered, only the ROI size can be reprogrammed.\n
 * The smallest acceptable ROI size = 4\n
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])

 * @param[in] x
 *    ROI width.
 *
 * @param[in] y
 *    ROI height.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_set_roi_xy(uint16_t dev, uint16_t x, uint16_t y);

/***************************************************************************//**
 * @brief
 * This function returns width X and height Y.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])

 * @param[out] x
 *    ROI width.
 *
 * @param[out] y
 *    ROI height.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if x or y is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_roi_xy(uint16_t dev, uint16_t *x, uint16_t *y);

/***************************************************************************//**
 * @brief
 * This function programs the new user ROI center,
 * please to be aware that there is no check in this function.
 * if the ROI center vs ROI size is out of border
 * the ranging function return error #13
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])

 * @param[in] center
 *    ROI center.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_set_roi_center(uint16_t dev, uint8_t center);

/***************************************************************************//**
 * @brief
 * This function returns the current user ROI center.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])

 * @param[out] center
 *    Return the configured ROI center.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if center is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_roi_center(uint16_t dev, uint8_t *center);

/***************************************************************************//**
 * @brief
 * This function programs a new signal threshold in kcps (default=1024 kcps)
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])

 * @param[in] signal
 *    Signal threshold in kcps. (default=1024 kcps)
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_set_signal_threshold(uint16_t dev, uint16_t signal);

/***************************************************************************//**
 * @brief
 * This function returns the current signal threshold in kcps
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])

 * @param[out] signal
 *    Signal threshold in kcps. (default=1024 kcps)
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if signal is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_signal_threshold(uint16_t dev, uint16_t *signal);

/***************************************************************************//**
 * @brief
 * This function programs a new sigma threshold in mm (default=15 mm)
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])

 * @param[in] signal
 *    Sigma threshold in mm. (default=15 mm)
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_set_sigma_threshold(uint16_t dev, uint16_t sigma);

/***************************************************************************//**
 * @brief
 * This function returns the current sigma threshold in mm.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])

 * @param[out] sigma
 *    Sigma threshold in mm. (default=15 mm)
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if sigma is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_get_sigma_threshold(uint16_t dev, uint16_t *sigma);

/**
 * @brief This function performs the temperature calibration.
 * It is recommended to call this function any time the temperature might have changed by more than 8 deg C
 * without sensor ranging activity for an extended period.
 */

/***************************************************************************//**
 * @brief
 * This function performs the temperature calibration.
 * It is recommended to call this function any time the temperature might have changed by more than 8 deg C
 * without sensor ranging activity for an extended period.
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_start_temperature_update(uint16_t dev);

/***************************************************************************//**
 * @brief
 *    This function returns the Core Software driver version
 *
 * @param[in] pVersion
 *    A structure pointer to store the version of the core driver.
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if pVersion is invalid.
 ******************************************************************************/
sl_status_t vl53l1x_get_core_sw_version(vl53l1x_core_version_t *version);

/***************************************************************************//**
 * @brief
 *    This function performs the offset calibration.\n
 * 	  The function returns the offset value found and programs
 * 	  the offset compensation into the device.
 * 	  Target reflectance = grey17%, recommended distance is 100 mm
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[in] target_distance_in_mm
 *    Target distance in mm, ST recommended 100 mm
 *
 * @param[out] offset
 *    Offset pointer contains the offset found in mm
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if offset is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_calibrate_offset(uint16_t dev,
		uint16_t target_distance_in_mm, int16_t *offset);

/***************************************************************************//**
 * @brief
 *    This function performs the xtalk calibration.\n
 *    The function returns the xtalk value found and programs
 *    the xtalk compensation to the device.
 *    The target distance :
 *
 * @param[in] dev
 *    Device address. (Default: 0x29[0x52])
 *
 * @param[in] target_distance_in_mm
 *    Target distance in mm, ST recommended 100 mm\n
 *    the distance where the sensor start to "under range"\n
 *    due to the influence of the photons reflected back from the
 *    cover glass becoming strong\n
 *    It's also called inflection point\n
 *
 *
 * @param[out] xtalk
 *    Pointer contains the xtalk value found in cps
 *    (number of photons in count per second)
 *
 * @return
 *    SL_STATUS_OK if there are no errors.
 *    SL_STATUS_INVALID_PARAMETER if offset is invalid.
 *    SL_STATUS_TRANSMIT if I2C transmit failed.
 ******************************************************************************/
sl_status_t vl53l1x_calibrate_xtalk(uint16_t dev,
		uint16_t target_distance_in_mm, uint16_t *xtalk);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* VL53L1X_H_ */
