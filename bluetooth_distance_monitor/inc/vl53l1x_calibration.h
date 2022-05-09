/*
 * This file is part of VL53L1 Core
 *
 * Copyright (c) 2017, STMicroelectronics - All Rights Reserved
 *
 * License terms: BSD 3-clause "New" or "Revised" License.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file  vl53l1x_calibration.h
 * @brief Calibration Functions definition
 */

#ifndef _VL53L1X_CALIBRATION_H_
#define _VL53L1X_CALIBRATION_H_

#include "vl53l1x_core.h"

/**
 * @brief This function performs the offset calibration.\n
 * The function returns the offset value found and programs the offset compensation into the device.
 * @param TargetDistInMm target distance in mm, ST recommended 100 mm
 * Target reflectance = grey17%
 * @return 0:success, !=0: failed
 * @return offset pointer contains the offset found in mm
 */
VL53L1X_ERROR VL53L1X_CalibrateOffset(uint16_t dev, uint16_t TargetDistInMm, int16_t *offset);

/**
 * @brief This function performs the xtalk calibration.\n
 * The function returns the xtalk value found and programs the xtalk compensation to the device
 * @param TargetDistInMm target distance in mm\n
 * The target distance : the distance where the sensor start to "under range"\n
 * due to the influence of the photons reflected back from the cover glass becoming strong\n
 * It's also called inflection point\n
 * Target reflectance = grey 17%
 * @return 0: success, !=0: failed
 * @return xtalk pointer contains the xtalk value found in cps (number of photons in count per second)
 */
VL53L1X_ERROR VL53L1X_CalibrateXtalk(uint16_t dev, uint16_t TargetDistInMm, uint16_t *xtalk);

#endif
