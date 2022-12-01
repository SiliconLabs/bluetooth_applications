/***************************************************************************//**
 * @file ak9753.h
 * @brief AK9753 Prototypes
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

#ifndef AK9753_H_
#define AK9753_H_
#include "sl_status.h"
#include "sl_i2cspm.h"
#include "em_gpio.h"
#include "gpiointerrupt.h"

#if (defined(SL_CATALOG_POWER_MANAGER_PRESENT))
#include "sl_power_manager.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup mma8452q MMA8452Q Driver API*/

/***************************************************************************//**
 * @addtogroup ak9753
 * @brief  AK9753 Driver API
 * @details
 * @{
 ******************************************************************************/

/***************************************************************************//**
* @name    SOFTWARE VERSION DEFINITION
*******************************************************************************/
#define AK9753_MAJOR_VERSION                0
#define AK9753_MINOR_VERSION                1
#define AK9753_BUILD_VERSION                0
#define AK9753_REVISION_VERSION             0

/************************************************/
/**\name    AK9753 Register addresses           */
/************************************************/
#define AK975X_WIA1                         0x00 // Company Code
#define AK975X_WIA2                         0x01 // Device ID
#define AK975X_INTST                        0x04
#define AK975X_ST1                          0x05
#define AK975X_IR1                          0x06
#define AK975X_IR2                          0x08
#define AK975X_IR3                          0x0A
#define AK975X_IR4                          0x0C
#define AK975X_TMP                          0x0E
#define AK975X_ST2                          0x10 // Dummy register
#define AK975X_ETH13H_LOW                   0x11
#define AK975X_ETH13H_HIGH                  0x12
#define AK975X_ETH13L_LOW                   0x13
#define AK975X_ETH13L_HIGH                  0x14
#define AK975X_ETH24H_LOW                   0x15
#define AK975X_ETH24H_HIGH                  0x16
#define AK975X_ETH24L_LOW                   0x17
#define AK975X_ETH24L_HIGH                  0x18
#define AK975X_EHYS13                       0x19
#define AK975X_EHYS24                       0x1A
#define AK975X_EINTEN                       0x1B
#define AK975X_ECNTL1                       0x1C
#define AK975X_CNTL2                        0x19
#define AK975X_EKEY                         0x50

/************************************************/
/**\name    AK9753 EEPROM addresses             */
/************************************************/
#define AK975X_EETH13H_LOW                  0X51
#define AK975X_EETH13H_HIGH                 0X52
#define AK975X_EETH13L_LOW                  0X53
#define AK975X_EETH13L_HIGH                 0X54
#define AK975X_EETH24H_LOW                  0X55
#define AK975X_EETH24H_HIGH                 0X56
#define AK975X_EETH24L_LOW                  0X57
#define AK975X_EETH24L_HIGH                 0X58
#define AK975X_EEHYS13                      0X59
#define AK975X_EEHYS24                      0X5A
#define AK975X_EEINTEN                      0X5B

/***************************************************************************//**
 * @brief
 *    Typedef for Valid sensor modes - Register ECNTL1
 ******************************************************************************/
typedef enum ak9753_mode
{
  AK975X_MODE_STANDBY = 0b000,
  AK975X_MODE_EEPROM_ACCESS = 0b001,
  AK975X_MODE_SINGLE_SHOT = 0b010,
  AK975X_MODE_0 = 0b100,
  AK975X_MODE_1 = 0b101,
  AK975X_MODE_2 = 0b110,
  AK975X_MODE_3 = 0b111
}ak9753_mode_t;

/***************************************************************************//**
 * @brief
 *    Typedef for Valid digital filter cutoff frequencies.
 ******************************************************************************/
typedef enum ak9753_cutoff_freq
{
  AK975X_FREQ_0_3HZ = 0b000,
  AK975X_FREQ_0_6HZ = 0b001,
  AK975X_FREQ_1_1HZ = 0b010,
  AK975X_FREQ_2_2HZ = 0b011,
  AK975X_FREQ_4_4HZ = 0b100,
  AK975X_FREQ_8_8HZ = 0b101
}ak9753_cutoff_freq_t;

// EEPROM functions
#define AK975X_EEPROM_MODE         0b11000001
#define AK975X_EKEY_ON             0b10100101 // 0b10100101=0xA5

/***************************************************************************//**
 * @brief
 *    Typedef for specifying the software version of the core driver.
 ******************************************************************************/
typedef struct
{
  uint8_t major;   /*!< major number */
  uint8_t minor;   /*!< minor number */
  uint8_t build;   /*!< build number */
  uint32_t revision;   /*!< revision number */
} ak9753_core_version_t;

/***************************************************************************//**
 * @brief
 *    Structure to store the sensor configuration
 ******************************************************************************/
typedef struct {
  sl_i2cspm_t *ak9753_i2cspm_instance;
  uint8_t  I2C_address;
  ak9753_cutoff_freq_t cut_off_freq;
  ak9753_mode_t mode;
  uint16_t upper_threshold13;
  uint16_t lower_threshold13;
  uint16_t upper_threshold24;
  uint16_t lower_threshold24;
  uint8_t hysteresis_value24;
  uint8_t hysteresis_value13;
  uint8_t  int_source_setting;
  bool     int_present;          // INT hardware connection is present
  bool     PDN_present;          // PDN hardware connection is present
} ak9753_config_t;

/***************************************************************************//**
 * @brief
 *   Returns the information of the current software information
 *
 * @param[out] core_version
 *   The struct contain the data of the software version information
 ******************************************************************************/
void ak9753_get_core_version(ak9753_core_version_t *core_version);

/***************************************************************************//**
 * @brief
 *    This function set mode of the sensor
 *
 * @param[in] mode
 *    Sensor mode
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_set_mode(ak9753_mode_t mode);

/***************************************************************************//**
 * @brief
 *    If present, set the PDN pin to logic low to power down the AK9753
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 ******************************************************************************/
sl_status_t ak9753_power_down(void);

/***************************************************************************//**
 * @brief
 *    If present, set the PDN pin to logic high to power up the AK9753
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 ******************************************************************************/
sl_status_t ak9753_power_up(void);

/***************************************************************************//**
 * @brief
 *    Returns the last set AK9753 mode of operation, stored in local config.
 *
 * @param[out] mode
 *    Mode of operation; see modes in driver definitions.
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_mode(ak9753_mode_t *mode);

/***************************************************************************//**
 * @brief
 *    This function set the filtering frequency
 *
 * @param[in] frequency
 *    Cutoff frequency
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_set_cutoff_freq(ak9753_cutoff_freq_t fc);

/***************************************************************************//**
 * @brief
 *    Returns the last set cutoff frequency, stored in local config.
 *
 * @param[out] fc
 *    Digital cutoff frequency setting.
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_cutoff_freq(uint8_t *fc);

/***************************************************************************//**
 * @brief
 *    This function reads sensor status register and returns true if data is
 *    ready to be read.
 *
 * @param[out] data_ready
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_is_data_ready(bool *data_ready);

/***************************************************************************//**
 * @brief
 *    Initialize the AK9753 sensor.
 *
 * @param[in] ak9753_cfg
 *   The AK9753 configuration typedef
 *
 * @details
 *    This function stores the I2C peripheral setting and I2C address in static
 *    config, powers up the sensor (if PDN option enabled), checks the
 *    connection by requesting/verifying Company Code and Device ID (WIA1/2)
 *    and reads the EEPROM threshold/hysteresis/interrupt settings and stores
 *    local configuration (default config on power up).
 *
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_init(ak9753_config_t *ak9753_config);

/***************************************************************************//**
 * @brief
 *   De-initalize the AK9753 sensor.
 *
 * @details
 *   This function returns the GPIO pin used for INT1 to disabled state, and
 *   powers down the device if PDN is present.
 *
 * @note
 *   The purpose of de-initialization is to shutdown the AK9753 as part
 *   of a complete shutdown sequence for an EFM32/EFR32-based system.
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_deinit(void);

/***************************************************************************//**
 * @brief
 *    This function refresh the device
 *
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_dummy(void);

/***************************************************************************//**
 * @brief
 *    This function get the data of the IR1
 *
 * @param[out] measurement_data
 *    Output value of the IR1
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_ir1_data(int16_t *measurement_data);

/***************************************************************************//**
 * @brief
 *    This function get the data of the IR2
 *
 * @param[out] measurement_data
 *    Output value of the IR2
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_ir2_data(int16_t *measurement_data);

/***************************************************************************//**
 * @brief
 *    This function get the data of the IR3
 *
 * @param[out] measurement_data
 *    Output value of the IR3
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_ir3_data(int16_t *measurement_data);

/***************************************************************************//**
 * @brief
 *    This function get the data of the IR4
 *
 * @param[out] measurement_data
 *    Output value of the IR4
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_ir4_data(int16_t *measurement_data);

/***************************************************************************//**
 * @brief
 *    This function reads the raw temperature sensor data value.
 *
 * @param[out] raw_temp_data
 *    TMP MSB/LSB register data - valid values: 0x0000 - 0xFFC0
 *    16-bit data is stored as 2's complement; only 10-bits of resolution.
 *
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_raw_temp(uint16_t *raw_temp_data);

/***************************************************************************//**
 * @brief
 *    This function get the data of the temperature in Celcius
 *
 * @param[in] tempC
 *    Output value of temperature
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_tempC(float *tempC);

/***************************************************************************//**
 * @brief
 *    This function get the data of the temperature in F
 *
 * @param[in] tempF
 *    Output value of temperature
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_tempF(float *tempF);

/***************************************************************************//**
 * @brief
 *    This function set high or low threshold  for Ir2-Ir4
 *
 * @param[in] height
 *    High or Low input data
 * @param[in] threshold_value
 *    Value of threshold input
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_set_threshold_ir24(bool height, uint16_t threshold_value);

/***************************************************************************//**
 * @brief
 *    Returns the threshold level for differential output IR2-IR4, stored in
 *    local config.
 *
 * @param[in] height
 *    Upper or lower threshold - true -> upper; false -> lower
 *
 * @param[out] threshold_value
 *    16-bit threshold value
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_threshold_ir24(bool height, uint16_t *threshold_value);

/***************************************************************************//**
 * @brief
 *    This function set high or low threshold  for Ir2-Ir4 to EEprom
 *
 * @param[in] height
 *    High or Low input data
 * @param[in] threshold_value
 *    Value of threshold input
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_set_threshold_eeprom_ir24(bool height,
                                             uint16_t threshold_value);

/***************************************************************************//**
 * @brief
 *    Returns the threshold level for differential output IR2-IR4, stored on
 *    sensor in EEPROM. Value also represents the initial register configuration
 *    after sensor reset
 *
 * @param[in] height
 *    Upper or lower threshold - true -> upper; false -> lower
 *
 * @param[out] threshold_value
 *    16-bit threshold value
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_eeprom_threshold_ir24(bool height,
                                             uint16_t *threshold_value);

/***************************************************************************//**
 * @brief
 *    This function set high or low threshold  for Ir1-Ir3
 *
 * @param[in] height
 *    High or Low input data
 * @param[in] threshold_value
 *    Value of threshold input
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_set_threshold_ir13(bool height, uint16_t threshold_value);

/***************************************************************************//**
 * @brief
 *    Returns the threshold level for differential output IR1-IR3, stored in
 *    local config.
 *
 * @param[in] height
 *    Upper or lower threshold - true -> upper; false -> lower
 *
 * @param[out] threshold_value
 *    16-bit threshold value
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_threshold_ir13(bool height, uint16_t *threshold_value);

/***************************************************************************//**
 * @brief
 *    This function set high or low threshold  for Ir1-Ir3 to EEprom
 *
 * @param[in] height
 *    High or Low input data
 * @param[in] threshold_value
 *    Value of threshold input
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_set_threshold_eeprom_ir13(bool height,
                                             uint16_t threshold_value);

/***************************************************************************//**
 * @brief
 *    Returns the threshold level for differential output IR1-IR3, stored on
 *    sensor in EEPROM. Value also represents the initial register configuration
 *    after sensor reset
 *
 * @param[in] height
 *    Upper or lower threshold - true -> upper; false -> lower
 *
 * @param[out] threshold_value
 *    16-bit threshold value
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_eeprom_threshold_ir13(bool height,
                                             uint16_t *thresholdValue);

/***************************************************************************//**
 * @brief
 *    This function set Hysteresis value for Ir2-Ir4
 *
 * @param[in] hysteresis_value
 *    Hysteresis value
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_set_hysteresis_ir24(uint8_t hysteresis_value);

/***************************************************************************//**
 * @brief
 *    Returns the hysteresis of threshold level for differential output IR2-IR4,
 *    stored in local config.
 *
 * @param[out] hysteresis_value
 *    8-bit hysteresis value
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_hysteresis_ir24(uint8_t *hysteresis_value);

/***************************************************************************//**
 * @brief
 *    This function set Hysteresis value to EEPROM EHYS24 for Ir2-Ir4
 *
 * @param[in] threshold_value
 *    Hysteresis value
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_set_hysteresis_eeprom_ir24(uint8_t hysteresis_value);

/***************************************************************************//**
 * @brief
 *    Returns the hysteresis of threshold level for differential output IR2-IR4,
 *    stored on sensor in EEPROM. Value also represents the initial register
 *    configuration after sensor reset
 *
 * @param[in] hysteresis_value
 *    8-bit hysteresis value
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_eeprom_hysteresis_ir24(uint8_t *hysteresis_value);

/***************************************************************************//**
 * @brief
 *    Returns the hysteresis of threshold level for differential output IR2-IR4,
 *    stored in local config.
 *
 * @param[out] hysteresis_value
 *    8-bit hysteresis value
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_hysteresis_ir24(uint8_t *hysteresis_value);

/***************************************************************************//**
 * @brief
 *    This function set Hysteresis value for Ir1-Ir3
 *
 * @param[in] hysteresis_value
 *    Hysteresis value
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_set_hysteresis_ir13(uint8_t hysteresis_value);

/***************************************************************************//**
 * @brief
 *    Returns the hysteresis of threshold level for differential output IR1-IR3,
 *    stored in local config.
 *
 * @param[out] hysteresis_value
 *    8-bit hysteresis value
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_hysteresis_ir13(uint8_t *hysteresis_value);

/***************************************************************************//**
 * @brief
 *    This function set Hysteresis value to EEprom EHYS24 for Ir2-Ir4
 *
 * @param[in] threshold_value
 *    Hysteresis value
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_set_hysteresis_eeprom_ir13(uint8_t hysteresis_value);

/***************************************************************************//**
 * @brief
 *    Returns the hysteresis of threshold level for differential output IR1-IR3,
 *    stored on sensor in EEPROM. Value also represents the initial register
 *    configuration after sensor reset
 *
 * @param[in] hysteresis_value
 *    8-bit hysteresis value
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_eeprom_hysteresis_ir13(uint8_t *hysteresis_value);

/***************************************************************************//**
 * @brief
 *    Set the interrupt sources that can activate INT output. Stored in local
 *    configuration.
 *
 * @param[in] int_source
 *    Lower five bits enable various interrupt sources:
 *    D0 - Data Ready
 *    D1 - IR24 above->below LOWER threshold
 *    D2 - IR24 below->above UPPER threshold
 *    D3 - IR13 above->below LOWER threshold
 *    D4 - IR13 below->above UPPER threshold
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_set_interrupts(uint8_t int_source);

/***************************************************************************//**
 * @brief
 *    Returns the interrupt source setting stored in local configuration.
 *
 * @param[out] int_source
 *    Lower five bits enable various interrupt sources:
 *    D0 - Data Ready
 *    D1 - IR24 above->below LOWER threshold
 *    D2 - IR24 below->above UPPER threshold
 *    D3 - IR13 above->below LOWER threshold
 *    D4 - IR13 below->above UPPER threshold
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_get_int_source(uint8_t *int_source);

/***************************************************************************//**
 * @brief
 *    This function reads sensor status register and returns true if a data
 *    read has been skipped.
 *
 * @param[out] data_overrun
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_is_data_overrun(bool *data_overrun);

/***************************************************************************//**
 * @brief
 *    This function soft reset the sensor
 *
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_sw_reset(void);

/***************************************************************************//**
 * @brief
 *    This function is called in the main application process action sequence
 *    with normal mode operation. The function checks the driver interrupt flag
 *    and if set, reads the sensor interrupt flag register and returns interrupt
 *    source status.
 *    In addition to reading the interrupt status register, this function reads
 *    the status register and checks the data overrun bit.
 *    Function not used in switch mode. Main application assumes production
 *    interrupt source and handles ISR flag accordingly.
 *
 * @param[out] isIR13H
 *    Returns true if IR13 differential value rises above the upper threshold
 * @param[out] isIR13L
 *    Returns true if IR13 differential value drops below the lower threshold
 * @param[out] isIR24H
 *    Returns true if IR24 differential value rises above the upper threshold
 * @param[out] isIR24L
 *    Returns true if IR24 differential value drops below the lower threshold
 * @param[out] isDataReady
 *    Returns true if data is ready to be read
 * @param[out] isDataOverrun
 *    Returns true if data is ready to be read
 *
 * @return
 *    @li @ref SL_STATUS_OK if there are no errors.
 *
 *    @li @ref SL_STATUS_TRANSMIT if I2C transmit failed.
 *
 *    @li @ref SL_INVALID_PARAMETER if invalid parameter.
 ******************************************************************************/
sl_status_t ak9753_is_interrupt(bool *isIR13H,
                                bool *isIR13L,
                                bool *isIR24H,
                                bool *isIR24L,
                                bool *isDataReady,
                                bool *isDataOverrun);

/** @} (end addtogroup ak9753) */
#ifdef __cplusplus
}
#endif
#endif /* AK9753_H_ */
