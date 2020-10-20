/***************************************************************************//**
 * @file app.c
 * @brief Silicon Labs Empty Example Project
 * This example demonstrates the implementation of IR signal generate and 
 * 4x4 matrix key scan with BLE on our EFR32 device(lynx). 
 * Need to ensure IR signal generate work well in case that BLE in a heavy communication.
 * @version 1.0.1
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
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
 * # EVALUATION QUALITY
 * This code has been minimally tested to ensure that it builds with the specified
 * dependency versions and is suitable as a demonstration for evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 ******************************************************************************/

/* Bluetooth stack headers */

#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"

#include "gpiointerrupt.h"
#include "app.h"
#include "sleep.h"
#include "ir_generate.h"
#include "key_scan.h"

#define TIMER_TEST_NOTIFY	1
#define TIMER_KEY_SCAN		2
#define TIMER_IR_REPEAT		3

#define TICKS_PER_SECOND    (32768)
#define TICKS_STOP			(0)
#define TICKS_NOTIFY		(TICKS_PER_SECOND/50)		//20ms
#define TICKS_KEY_SCAN		(TICKS_PER_SECOND/100)		//10ms
#define TICKS_IR_REPEAT		(TICKS_PER_SECOND*9/200) 	//45ms

#define REPEATING			0
#define ONCE 				1

static uint8 connect_handle = 0xFF;
static uint8 battery_alert_nodif_data = 0;
/* Print boot message */
static void bootMessage(struct gecko_msg_system_boot_evt_t *bootevt);

/* Flag for indicating DFU Reset must be performed */
static uint8_t boot_to_dfu = 0;
#if !D_KEYSCAN
#define BUTTON0		(uint32)(1 << 0)   // Bit flag to external signal command
#define BUTTON1		(uint32)(1 << 1)   // Bit flag to external signal command
/**
 * @brief handle_button_change
 * Callback function to handle buttons press and release actions
 * @param pin -  push button mask
 */
void handle_button_change(uint8_t pin) {

  if(pin == BSP_BUTTON0_PIN) {
    if(!GPIO_PinInGet(BSP_BUTTON0_PORT, BSP_BUTTON0_PIN))
      // PB0 pressed down
      gecko_external_signal(BUTTON0);
  } else if(pin == BSP_BUTTON1_PIN) {
    if (!GPIO_PinInGet(BSP_BUTTON1_PORT, BSP_BUTTON1_PIN))
      // PB1 pressed
    	gecko_external_signal(BUTTON1);
  }
}
/**
 * @brief setup_pins_interrupts
 * Configure push buttons and button interrupts
 */
void setup_pins_interrupts(void) {
  // Configure PB0 and PB1 as inputs on the WSTK
  GPIO_PinModeSet(BSP_BUTTON0_PORT, BSP_BUTTON0_PIN, gpioModeInputPullFilter, 1);
  GPIO_PinModeSet(BSP_BUTTON1_PORT, BSP_BUTTON1_PIN, gpioModeInputPullFilter, 1);
  // Initialize interrupts
  GPIOINT_Init();
  // Configuring push buttons PB0 and PB1 on the WSTK to generate interrupt
  GPIO_ExtIntConfig(BSP_BUTTON0_PORT, BSP_BUTTON0_PIN, BSP_BUTTON0_PIN, true, true, true);
  GPIOINT_CallbackRegister(BSP_BUTTON0_PIN, handle_button_change);

  GPIO_ExtIntConfig(BSP_BUTTON1_PORT, BSP_BUTTON1_PIN, BSP_BUTTON1_PIN, true, true, true);
  GPIOINT_CallbackRegister(BSP_BUTTON1_PIN, handle_button_change);
}
#endif

uint8_t key_test = 0xFF;
uint8_t key_repeat = 0xFF;
/**
 * @brief key press wakeup callback
 *
 * @param interrupt pin
 *
 * @return none
 *
 */
void app_key_wakeup(uint8_t pin)
{
  //Test code, key jitter, avoid multiple set the timer.
  if(key_test == pin)
	  return;
  key_test = pin;

  //printLog("key wakeup %d\r\n", pin);
  //Start the key timer
  gecko_cmd_hardware_set_soft_timer(TICKS_KEY_SCAN, TIMER_KEY_SCAN, REPEATING);
}

/**
 * @brief key detect callback
 *
 * @param key value, KEY_NONE means key release.
 *
 * @return none
 *
 */
void app_key_detect(key_code_t key)
{
  if(key == KEY_NONE){
	printLog("key release\r\n");
	key_test =0xFF;
	//Key have release, stop the key timer
	gecko_cmd_hardware_set_soft_timer(TICKS_STOP, TIMER_KEY_SCAN, ONCE);
	gecko_cmd_hardware_set_soft_timer(TICKS_STOP, TIMER_IR_REPEAT, ONCE);

  }else{
	key_repeat = key;
    printLog("key detect %d\r\n", key);
    SLEEP_SleepBlockBegin(sleepEM2);	// block the EM2, otherwise, timer1 may stop.
    ir_generate_stream(ir_table[key%18][0],ir_table[key%18][1], false);
    gecko_cmd_hardware_set_soft_timer(TICKS_IR_REPEAT, TIMER_IR_REPEAT, ONCE);
  }
}

/**
 * @brief ir sent callback
 *
 * @param none
 *
 * @return none
 *
 */
void app_ir_complete(void)
{
  printLog("ir complete\r\n");
  SLEEP_SleepBlockEnd(sleepEM2); // Here should consider whether it can stop, other component may still need to blocking sleep.
}

/* Main application */
void appMain(gecko_configuration_t *pconfig)
{
  #if DISABLE_SLEEP > 0
  pconfig->sleep.flags = 0;
  #endif
  #if !D_KEYSCAN
  setup_pins_interrupts();
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

        /* Set advertising parameters. 100ms advertisement interval.
         * The first parameter is advertising set handle
         * The next two parameters are minimum and maximum advertising interval, both in
         * units of (milliseconds * 1.6).
         * The last two parameters are duration and maxevents left as default. */
        gecko_cmd_le_gap_set_advertise_timing(0, 160, 160, 0, 0);

        /* Start general advertising and enable connections. */
        gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable);
		#if D_KEYSCAN
        key_init(app_key_detect, app_key_wakeup);
		#endif
		#if D_IR
        ir_generate_init(CODE_SONY, app_ir_complete);
		#endif
        break;

      case gecko_evt_le_connection_opened_id:
        printLog("connection opened\r\n");
        connect_handle = evt->data.evt_le_connection_opened.connection;
        gecko_cmd_hardware_set_soft_timer(TICKS_NOTIFY, TIMER_TEST_NOTIFY, REPEATING);
        break;

      case gecko_evt_le_connection_closed_id:

        printLog("connection closed, reason: 0x%2.2x\r\n", evt->data.evt_le_connection_closed.reason);

        /* Check if need to boot to OTA DFU mode */
        if (boot_to_dfu) {
          /* Enter to OTA DFU mode */
          gecko_cmd_system_reset(2);
        } else {
          /* Restart advertising after client has disconnected */
          gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable);
        }
        break;

      case gecko_evt_system_external_signal_id:
    	  // The capability status was changed
          #if !D_KEYSCAN
      	  if(evt->data.evt_system_external_signal.extsignals == BUTTON0) {
      		printLog("BUTTON0\r\n");
			#if D_IR
      		ir_generate_stream(ir_table[0][0],ir_table[0][1], false);
			#endif
      	  }else if(evt->data.evt_system_external_signal.extsignals == BUTTON1) {
      		printLog("BUTTON1\r\n");
			#if D_IR
      		ir_generate_stream(ir_table[0][0],ir_table[0][1], true);
			#endif
      	  }
          #endif
    	  break;

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
      case gecko_evt_hardware_soft_timer_id:
		  switch(evt->data.evt_hardware_soft_timer.handle){
		    case TIMER_TEST_NOTIFY:
		      {
		      uint8_t d = battery_alert_nodif_data++;
			  uint16_t result = gecko_cmd_gatt_server_send_characteristic_notification(connect_handle, gattdb_my_test, 250, &d)->result;
			  result = result;
			  //printLog("Battery Notity,result:%d\r\n",result);
		      }
		      break;
		    case TIMER_KEY_SCAN:
			  //printLog("key scan\r\n");
			  //IrGenelateStream(ir_table[0][0],ir_table[0][1], false);
			  #if D_KEYSCAN
			  key_scan();
			  #endif
		  	  break;
		    case TIMER_IR_REPEAT:
		      SLEEP_SleepBlockBegin(sleepEM2);	// block the EM2, otherwise, timer1 may stop.
		      ir_generate_stream(ir_table[key_repeat%18][0],ir_table[key_repeat%18][1], false);
		      gecko_cmd_hardware_set_soft_timer(TICKS_IR_REPEAT, TIMER_IR_REPEAT, ONCE);
		      break;
		  }
		  break;
		  
      default:
        break;
    }
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
