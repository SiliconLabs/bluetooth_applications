#include <ota_dfu_multislot.h>
#include "gatt_db.h"
#include "sl_bluetooth.h"
#include "sl_app_assert.h"
#include "btl_interface.h"
#include "btl_interface_storage.h"

// OTA DFU block size
#define SWITCHED_MULTIPROTOCOL_OTA_DFU_START        0x00
#define SWITCHED_MULTIPROTOCOL_OTA_DFU_COMMIT       0x03

uint32_t ota_data_len;

#define MAX_SLOT_ID 0xFFFFFFFF
static uint32_t ota_dfu_offset;
static uint32_t ota_dfu_slot_id = MAX_SLOT_ID;
static uint8_t conn_handle = 0;
static sl_status_t sc;

uint8_t multislot_ota_dfu_transaction_begin();
uint8_t multislot_ota_dfu_transaction_finish();
uint8_t multislot_ota_dfu_data_received(uint8_t len, uint8_t *data);

void multislot_ota_dfu_on_event(sl_bt_msg_t* evt)
{
  // Do not try to process NULL event.
  if (NULL == evt) {
    return;
  }

  // Handle events
  switch (SL_BT_MSG_ID(evt->header)) {

    case  sl_bt_evt_system_boot_id:

    		bootloader_init();

    	break;

    case  sl_bt_evt_connection_opened_id:

    	  conn_handle = evt->data.evt_connection_opened.connection;
        
        //set timeout to 10 sec, because erasing the slot may take several seconds!!!
   	    sc = sl_bt_connection_set_parameters(conn_handle,6,6,0,1000,0,0xffff);

   	   sl_app_assert(sc == SL_STATUS_OK, "[E: 0x%04x] Check status code at https://docs.silabs.com/gecko-platform/latest/common/api/group-status\n", (int)sc);

    	break;

    case  sl_bt_evt_gatt_server_user_write_request_id:{

      uint32_t  connection      = evt->data.evt_gatt_server_user_write_request.connection;
      uint32_t  characteristic  = evt->data.evt_gatt_server_user_write_request.characteristic;
      uint8_t   *charValueData  = evt->data.evt_gatt_server_user_write_request.value.data;

      uint8_t writeRequestAttOpcode = evt->data.evt_gatt_server_user_write_request.att_opcode;
      uint8_t charLen = evt->data.evt_gatt_server_user_write_request.value.len;

      /*****  OTA Control  *****/
      if (characteristic == gattdb_ota_control) {
    	OTAErrorCodes_t responseVal = OTA_NoError;
        uint8_t ota_dfu_command = charValueData[0];

        switch (ota_dfu_command) {
          case SWITCHED_MULTIPROTOCOL_OTA_DFU_START:
              ota_data_len = 0;
              responseVal = multislot_ota_dfu_transaction_begin();
            break;

          case SWITCHED_MULTIPROTOCOL_OTA_DFU_COMMIT:
        	    responseVal = multislot_ota_dfu_transaction_finish();
            break;

          default:
              //responseVal = OTA_WrongValueError;
        	    sc = sl_bt_gatt_server_send_user_write_response(connection, characteristic, OTA_NoError);

        	    sl_app_assert(sc == SL_STATUS_OK, "[E: 0x%04x] Check status code at https://docs.silabs.com/gecko-platform/latest/common/api/group-status\n", (int)sc);

        	    sc = sl_bt_connection_close(evt->data.evt_gatt_server_user_write_request.connection);

        	    sl_app_assert(sc == SL_STATUS_OK, "[E: 0x%04x] Check status code at https://docs.silabs.com/gecko-platform/latest/common/api/group-status\n", (int)sc);

            break;
        }

        sc = sl_bt_gatt_server_send_user_write_response(connection, characteristic, responseVal);

        sl_app_assert(sc == SL_STATUS_OK, "[E: 0x%04x] Check status code at https://docs.silabs.com/gecko-platform/latest/common/api/group-status\n", (int)sc);

      } else

      /*****  OTA Data  *****/
      if (characteristic == gattdb_ota_data) {
        OTAErrorCodes_t responseVal = OTA_NoError;

        ota_data_len += charLen;

        responseVal = multislot_ota_dfu_data_received(charLen, charValueData);

        // Send back response in case of data was sent as a write request
        if (BLE_OPCODE_GATT_WRITE_REQUEST == writeRequestAttOpcode) {
          sc = sl_bt_gatt_server_send_user_write_response(connection, characteristic, responseVal);

          sl_app_assert(sc == SL_STATUS_OK, "[E: 0x%04x] Check status code at https://docs.silabs.com/gecko-platform/latest/common/api/group-status\n", (int)sc);
        }
      }

      break;
    }

    // Value of attribute changed from the local database by remote GATT client
    case sl_bt_evt_gatt_server_attribute_value_id:

      /*****  OTA Data  *****/
      if (evt->data.evt_gatt_server_attribute_value.attribute == gattdb_ota_data) {
        uint8_t *charValueData = evt->data.evt_gatt_server_attribute_value.value.data;

        uint8_t len = evt->data.evt_gatt_server_user_write_request.value.len;

        ota_data_len += len;

        multislot_ota_dfu_data_received(len, charValueData);
      } else
      /***** Boot slot *****/
      if (evt->data.evt_gatt_server_attribute_value.attribute == gattdb_boot_slot) {

    	  if (BOOTLOADER_OK == bootloader_setImageToBootload(evt->data.evt_gatt_server_attribute_value.value.data[0])) {
    	      bootloader_rebootAndInstall();
    	    }
      }

      break;

    default:
      break;
  }
}


uint8_t multislot_ota_dfu_transaction_begin()
{
  int32_t rv;

  ota_dfu_slot_id = 0x00000000;

  sc = sl_bt_gatt_server_read_attribute_value(gattdb_upload_slot, 0, 1, NULL, (uint8_t*)&ota_dfu_slot_id);

  sl_app_assert(sc == SL_STATUS_OK, "[E: 0x%04x] Check status code at https://docs.silabs.com/gecko-platform/latest/common/api/group-status\n", (int)sc);

  ota_dfu_offset = 0;

  //erasing slot. This may take several seconds!!!
  rv = bootloader_eraseStorageSlot(ota_dfu_slot_id);

  return (BOOTLOADER_OK == rv) ? OTA_NoError : OTA_EraseError;
}

uint8_t multislot_ota_dfu_transaction_finish()
{
  int8_t ret = OTA_NoError;

  ota_dfu_slot_id = MAX_SLOT_ID;

  return ret;
}

uint8_t multislot_ota_dfu_data_received(uint8_t len, uint8_t *data)
{
  int32_t rv;
  BootloaderStorageSlot_t storageSlot;

  rv = bootloader_getStorageSlotInfo(ota_dfu_slot_id, &storageSlot);
  if (BOOTLOADER_OK != rv) {
    return OTA_SlotError;
  }

  if ((ota_dfu_offset + len) > storageSlot.length) {
	return OTA_SizeError;
  }

  rv = bootloader_writeStorage(ota_dfu_slot_id, ota_dfu_offset, data, len);

  if (BOOTLOADER_OK == rv) {
    ota_dfu_offset += len;
    return OTA_NoError;
  }
  return OTA_WriteError;

}
