/* 
 * This file is part of VL53L1 Platform
 *
 * Copyright (c) 2016, STMicroelectronics - All Rights Reserved
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
#include <string.h>
#include "vl53l1_platform.h"
#include "vl53l1x_config.h"

static sl_i2cspm_t *_vl53l1x_i2cspm_instance = VL53L1X_CONFIG_I2C_INSTANCE;

static sl_status_t
i2c_write_blocking(uint8_t addr, uint16_t index, const uint8_t *src, int len);
static sl_status_t
i2c_write_read_blocking(uint8_t addr, uint16_t index, uint8_t *data, int len);

void vl53l1x_platform_set_i2cspm_instance(sl_i2cspm_t *i2cspm_instance) {
	_vl53l1x_i2cspm_instance = i2cspm_instance;
}

sl_status_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata,
		uint32_t count) {
	return i2c_write_read_blocking(dev, index, pdata, count);
}

sl_status_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data) {
	return i2c_write_blocking(dev, index, &data, 1);
}

sl_status_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data) {
	uint8_t dataBytes[2] = { data >> 8, data & 0x00FF, };

	return i2c_write_blocking(dev, index, dataBytes, 2);
}

sl_status_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data) {
	uint8_t dataBytes[4] = { (data >> 24) & 0xFF, (data >> 16) & 0xFF, (data
			>> 8) & 0xFF, (data >> 0) & 0xFF };

	return i2c_write_blocking(dev, index, dataBytes, 4);
}

sl_status_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *data) {
	return i2c_write_read_blocking(dev, index, data, 1);
}

sl_status_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *data) {
	uint8_t receivedData[2];
	sl_status_t status = SL_STATUS_OK;

	status = i2c_write_read_blocking(dev, index, receivedData, 2);
	*data = ((uint16_t) receivedData[0] << 8) + (uint16_t) receivedData[1];

	return status;
}

sl_status_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *data) {
	uint8_t receivedData[4];
	sl_status_t status = SL_STATUS_OK;

	status = i2c_write_read_blocking(dev, index, receivedData, 4);

	*data = ((uint32_t) receivedData[0] << 24)
			+ ((uint32_t) receivedData[1] << 16)
			+ ((uint32_t) receivedData[2] << 8) + (uint32_t) receivedData[3];

	return status;
}

// Silicon Labs I2C platform component integration

static sl_status_t i2c_write_blocking(uint8_t addr, uint16_t index,
		const uint8_t *src, int len) {

	I2C_TransferSeq_TypeDef seq;
	uint8_t i2c_write_data[len + 2];

	seq.addr = addr << 1;
	seq.flags = I2C_FLAG_WRITE;

	i2c_write_data[0] = index >> 8;
	i2c_write_data[1] = index & 0xFF;

	memcpy(&i2c_write_data[2], src, len);

	/*Write buffer*/
	seq.buf[0].data = i2c_write_data;
	seq.buf[0].len = len + 2;

	if (I2CSPM_Transfer(_vl53l1x_i2cspm_instance, &seq) != i2cTransferDone) {
		return SL_STATUS_TRANSMIT;
	}
	return SL_STATUS_OK;
}

static sl_status_t i2c_write_read_blocking(uint8_t addr, uint16_t index,
		uint8_t *data, int len) {
	I2C_TransferSeq_TypeDef seq;
	uint8_t i2c_write_data[len];

	seq.addr = addr << 1;
	seq.flags = I2C_FLAG_WRITE_READ;

	i2c_write_data[0] = index >> 8;
	i2c_write_data[1] = index & 0xFF;

	/*Write buffer*/
	seq.buf[0].data = i2c_write_data;
	seq.buf[0].len = 2;

	/*Read buffer*/
	seq.buf[1].data = data;
	seq.buf[1].len = len;

	if (I2CSPM_Transfer(_vl53l1x_i2cspm_instance, &seq) != i2cTransferDone) {
		return SL_STATUS_TRANSMIT;
	}
	return SL_STATUS_OK;
}
