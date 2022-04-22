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
 * @file  vl53l1x_calibration.c
 * @brief Calibration functions implementation
 */
#include "vl53l1x_calibration.h"
#include "vl53l1_platform.h"
#include "vl53l1x_core.h"

#define ALGO__PART_TO_PART_RANGE_OFFSET_MM	0x001E
#define MM_CONFIG__INNER_OFFSET_MM			0x0020
#define MM_CONFIG__OUTER_OFFSET_MM 			0x0022

VL53L1X_ERROR VL53L1X_CalibrateOffset(uint16_t dev, uint16_t TargetDistInMm, int16_t *offset)
{
	uint8_t i, tmp;
	int16_t AverageDistance = 0;
	uint16_t distance;
	VL53L1X_ERROR status = 0;

	status |= VL53L1_WrWord(dev, ALGO__PART_TO_PART_RANGE_OFFSET_MM, 0x0);
	status |= VL53L1_WrWord(dev, MM_CONFIG__INNER_OFFSET_MM, 0x0);
	status |= VL53L1_WrWord(dev, MM_CONFIG__OUTER_OFFSET_MM, 0x0);
	status |= VL53L1X_StartRanging(dev);	/* Enable VL53L1X sensor */
	for (i = 0; i < 50; i++) {
		tmp = 0;
		while (tmp == 0){
			status |= VL53L1X_CheckForDataReady(dev, &tmp);
		}
		status |= VL53L1X_GetDistance(dev, &distance);
		status |= VL53L1X_ClearInterrupt(dev);
		AverageDistance = AverageDistance + distance;
	}
	status |= VL53L1X_StopRanging(dev);
	AverageDistance = AverageDistance / 50;
	*offset = TargetDistInMm - AverageDistance;
	status |= VL53L1_WrWord(dev, ALGO__PART_TO_PART_RANGE_OFFSET_MM, *offset*4);
	return status;
}

VL53L1X_ERROR VL53L1X_CalibrateXtalk(uint16_t dev, uint16_t TargetDistInMm, uint16_t *xtalk)
{
	uint8_t i, tmp;
	float AverageSignalRate = 0;
	float AverageDistance = 0;
	float AverageSpadNb = 0;
	uint16_t distance = 0, spadNum;
	uint16_t sr;
	VL53L1X_ERROR status = 0;
	uint32_t calXtalk;

	status |= VL53L1_WrWord(dev, 0x0016,0);
	status |= VL53L1X_StartRanging(dev);
	for (i = 0; i < 50; i++) {
		tmp = 0;
		while (tmp == 0){
			status |= VL53L1X_CheckForDataReady(dev, &tmp);
		}
		status |= VL53L1X_GetSignalRate(dev, &sr);
		status |= VL53L1X_GetDistance(dev, &distance);
		status |= VL53L1X_ClearInterrupt(dev);
		AverageDistance = AverageDistance + distance;
		status = VL53L1X_GetSpadNb(dev, &spadNum);
		AverageSpadNb = AverageSpadNb + spadNum;
		AverageSignalRate =
		    AverageSignalRate + sr;
	}
	status |= VL53L1X_StopRanging(dev);
	AverageDistance = AverageDistance / 50;
	AverageSpadNb = AverageSpadNb / 50;
	AverageSignalRate = AverageSignalRate / 50;
	/* Calculate Xtalk value */
	calXtalk = (uint16_t)(512*(AverageSignalRate*(1-(AverageDistance/TargetDistInMm)))/AverageSpadNb);
	if(calXtalk  > 0xffff)
		calXtalk = 0xffff;
	*xtalk = (uint16_t)((calXtalk*1000)>>9);
	status |= VL53L1_WrWord(dev, 0x0016, (uint16_t)calXtalk);
	return status;
}
