/*
 * ota_dfu.h
 *
 *  Created on: 2017. szept. 12.
 *      Author: arkalvac
 */

#ifndef OTA_DFU_MULTISLOT_H_
#define OTA_DFU_MULTISLOT_H_

#include "sl_bluetooth.h"

#define BLE_OPCODE_GATT_WRITE_REQUEST 18
#define BLE_OPCODE_GATT_WRITE_COMMAND 82

#define   OTA_SUCCESS               0
#define   OTA_ERROR                -1

typedef enum {
	  OTA_NoError    = 0x00,
	  OTA_SlotError  = 0x81,
	  OTA_SizeError  = 0x82,
	  OTA_WriteError = 0x83,
	  OTA_EraseError = 0x84
} OTAErrorCodes_t;

void multislot_ota_dfu_on_event(sl_bt_msg_t* evt);

#endif /* OTA_DFU_MULTISLOT_H_ */
