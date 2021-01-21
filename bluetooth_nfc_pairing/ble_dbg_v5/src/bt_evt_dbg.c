/***************************************************************************//**
 * @file   sl_bt_evt_dbg.c
 * @brief  Implementation for Debug helper for bluetooth le events.
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
 ******************************************************************************/

/* ...
*
* EXPERIMENTAL QUALITY
* This code has not been formally tested and is provided as-is.  It is not suitable for production environments.
* This code will not be maintained.
*
... */

#include <stdio.h>
#include "sl_bluetooth.h"

/**************************************************************************//**
 * @brief
 *  Prints bluetooth LE event and related information.
 *
 * @param[in] evt
 *  Bluetooth LE event.
 *****************************************************************************/
void sl_bt_evt_log (sl_bt_msg_t *evt) {
    printf("\r\n");
    printf("BLE event ID: ");
    switch (SL_BT_MSG_ID(evt->header)) {
        case sl_bt_evt_dfu_boot_id:
            printf("sl_bt_evt_dfu_boot_id\r\n");
            break;
        case sl_bt_evt_dfu_boot_failure_id:
            printf("sl_bt_evt_dfu_boot_failure_id\r\n");
            break;
        case sl_bt_evt_system_boot_id:
            printf("sl_bt_evt_system_boot_id\r\n");
            printf("    Event Parameters:\r\n");
            printf("        major:      0x%04x\r\n",  evt->data.evt_system_boot.major);
            printf("        minor:      0x%04x\r\n",  evt->data.evt_system_boot.minor);
            printf("        patch:      0x%04x\r\n",  evt->data.evt_system_boot.patch);
            printf("        build:      0x%04x\r\n",  evt->data.evt_system_boot.build);
            printf("        bootloader: 0x%08lx\r\n", evt->data.evt_system_boot.bootloader);
            printf("        hw:         0x%04x\r\n",  evt->data.evt_system_boot.major);
            printf("        hash:       0x%08lx\r\n", evt->data.evt_system_boot.hash);
            break;
        case sl_bt_evt_system_external_signal_id:
            printf("sl_bt_evt_system_external_signal_id\r\n");
            break;
        case sl_bt_evt_system_awake_id:
            printf("sl_bt_evt_system_awake_id\r\n");
            break;
        case sl_bt_evt_system_soft_timer_id:
            printf("sl_bt_evt_system_soft_timer_id\r\n");
            break;
        case sl_bt_evt_system_hardware_error_id:
            printf("sl_bt_evt_system_hardware_error_id\r\n");
            break;
        case sl_bt_evt_system_error_id:
            printf("sl_bt_evt_system_error_id\r\n");
            break;
        case sl_bt_evt_scanner_scan_report_id:
            printf("sl_bt_evt_scanner_scan_report_id\r\n");
            break;
        case sl_bt_evt_advertiser_timeout_id:
            printf("sl_bt_evt_advertiser_timeout_id\r\n");
            break;
        case sl_bt_evt_advertiser_scan_request_id:
            printf("sl_bt_evt_advertiser_scan_request_id\r\n");
            break;
        case sl_bt_evt_advertiser_periodic_advertising_status_id:
            printf("sl_bt_evt_advertiser_periodic_advertising_status_id\r\n");
            break;
        case sl_bt_evt_sync_opened_id:
            printf("sl_bt_evt_sync_opened_id\r\n");
            break;
        case sl_bt_evt_sync_closed_id:
            printf("sl_bt_evt_sync_closed_id\r\n");
            break;
        case sl_bt_evt_sync_data_id:
            printf("sl_bt_evt_sync_data_id\r\n");
            break;
        case sl_bt_evt_connection_opened_id:
            printf("sl_bt_evt_connection_opened_id\r\n");
            printf("    Event Parameters:\r\n");
            printf  ("        address:      %02x\r\n", evt->data.evt_connection_opened.address.addr[0]);
            printf  (":%02x", evt->data.evt_connection_opened.address.addr[1]);
            printf  (":%02x", evt->data.evt_connection_opened.address.addr[2]);
            printf  (":%02x", evt->data.evt_connection_opened.address.addr[3]);
            printf  (":%02x", evt->data.evt_connection_opened.address.addr[4]);
            printf(":%02x\r\n", evt->data.evt_connection_opened.address.addr[5]);
            printf("        address_type: 0x%02x\r\n", evt->data.evt_connection_opened.address_type);
            printf("        master:       0x%02x\r\n", evt->data.evt_connection_opened.master);
            printf("        connection:   0x%02x\r\n", evt->data.evt_connection_opened.connection);
            printf("        bonding:      0x%02x\r\n", evt->data.evt_connection_opened.bonding);
            printf("        advertiser:   0x%02x\r\n", evt->data.evt_connection_opened.advertiser);
            break;
        case sl_bt_evt_connection_closed_id:
            printf("sl_bt_evt_connection_closed_id\r\n");
            printf("    Event Parameters:\r\n");
            printf("        reason:     0x%04x\r\n", evt->data.evt_connection_closed.reason);
            printf("        connection: 0x%02x\r\n", evt->data.evt_connection_closed.connection);
            break;
        case sl_bt_evt_connection_parameters_id:
            printf("sl_bt_evt_connection_parameters_id\r\n");
            printf("    Event Parameters:\r\n");
            printf("        connection:    0x%02x\r\n", evt->data.evt_connection_parameters.connection);
            printf("        interval:      0x%04x\r\n", evt->data.evt_connection_parameters.interval);
            printf("        latency:       0x%04x\r\n", evt->data.evt_connection_parameters.latency);
            printf("        timeout:       0x%04x\r\n", evt->data.evt_connection_parameters.timeout);
            printf("        security_mode: %d",     evt->data.evt_connection_parameters.security_mode);
            printf("        txsize:        0x%04x\r\n", evt->data.evt_connection_parameters.txsize);
            break;
        case sl_bt_evt_connection_rssi_id:
            printf("sl_bt_evt_connection_rssi_id\r\n");
            break;
        case sl_bt_evt_connection_phy_status_id:
            printf("sl_bt_evt_connection_phy_status_id\r\n");
            printf("    Event Parameters:\r\n");
            printf("        connection: 0x%02x\r\n", evt->data.evt_connection_phy_status.connection);
            printf("        phy:        0x%02x\r\n", evt->data.evt_connection_phy_status.phy);
            break;
        case sl_bt_evt_gatt_mtu_exchanged_id:
            printf("sl_bt_evt_gatt_mtu_exchanged_id\r\n");
            printf("    Event Parameters:\r\n");
            printf("        connection: 0x%02x\r\n", evt->data.evt_gatt_mtu_exchanged.connection);
            printf("        mtu:        0x%04x\r\n", evt->data.evt_gatt_mtu_exchanged.mtu);
            break;
        case sl_bt_evt_gatt_service_id:
            printf("sl_bt_evt_gatt_service_id\r\n");
            break;
        case sl_bt_evt_gatt_characteristic_id:
            printf("sl_bt_evt_gatt_characteristic_id\r\n");
            break;
        case sl_bt_evt_gatt_descriptor_id:
            printf("sl_bt_evt_gatt_descriptor_id\r\n");
            break;
        case sl_bt_evt_gatt_characteristic_value_id:
            printf("sl_bt_evt_gatt_characteristic_value_id\r\n");
            break;
        case sl_bt_evt_gatt_descriptor_value_id:
            printf("sl_bt_evt_gatt_descriptor_value_id\r\n");
            break;
        case sl_bt_evt_gatt_procedure_completed_id:
            printf("sl_bt_evt_gatt_procedure_completed_id\r\n");
            break;
        case sl_bt_evt_gatt_server_attribute_value_id:
            printf("sl_bt_evt_gatt_server_attribute_value_id\r\n");
            break;
        case sl_bt_evt_gatt_server_user_read_request_id:
            printf("sl_bt_evt_gatt_server_user_read_request_id\r\n");
            break;
        case sl_bt_evt_gatt_server_user_write_request_id:
            printf("sl_bt_evt_gatt_server_user_write_request_id\r\n");
            break;
        case sl_bt_evt_gatt_server_characteristic_status_id:
            printf("sl_bt_evt_gatt_server_characteristic_status_id\r\n");
            break;
        case sl_bt_evt_gatt_server_execute_write_completed_id:
            printf("sl_bt_evt_gatt_server_execute_write_completed_id\r\n");
            break;
        case sl_bt_evt_test_dtm_completed_id:
            printf("sl_bt_evt_test_dtm_completed_id\r\n");
            break;
        case sl_bt_evt_sm_passkey_display_id:
            printf("sl_bt_evt_sm_passkey_display_id\r\n");
            break;
        case sl_bt_evt_sm_passkey_request_id:
            printf("sl_bt_evt_sm_passkey_request_id\r\n");
            break;
        case sl_bt_evt_sm_confirm_passkey_id:
            printf("sl_bt_evt_sm_confirm_passkey_id\r\n");
            break;
        case sl_bt_evt_sm_bonded_id:
            printf("sl_bt_evt_sm_bonded_id\r\n");
            break;
        case sl_bt_evt_sm_bonding_failed_id:
            printf("sl_bt_evt_sm_bonding_failed_id\r\n");
            printf("    Event Parameters:\r\n");
            printf("        connection: 0x%02x\r\n", evt->data.evt_sm_bonding_failed.connection);
            printf("        reason:     0x%04x\r\n", evt->data.evt_sm_bonding_failed.reason);
            break;
        case sl_bt_evt_sm_list_bonding_entry_id:
            printf("sl_bt_evt_sm_list_bonding_entry_id\r\n");
            break;
        case sl_bt_evt_sm_list_all_bondings_complete_id:
            printf("sl_bt_evt_sm_list_all_bondings_complete_id\r\n");
            printf("    Event Parameters:\r\n");
            printf("        bonding:      0x%02x\r\n", evt->data.evt_sm_list_bonding_entry.bonding);
            printf  ("        address:      %02x", evt->data.evt_sm_list_bonding_entry.address.addr[0]);
            printf  (":%02x", evt->data.evt_sm_list_bonding_entry.address.addr[1]);
            printf  (":%02x", evt->data.evt_sm_list_bonding_entry.address.addr[2]);
            printf  (":%02x", evt->data.evt_sm_list_bonding_entry.address.addr[3]);
            printf  (":%02x", evt->data.evt_sm_list_bonding_entry.address.addr[4]);
            printf(":%02x\r\n", evt->data.evt_sm_list_bonding_entry.address.addr[5]);
            printf("        address type: 0x%02x\r\n", evt->data.evt_sm_list_bonding_entry.address_type);
            break;
        case sl_bt_evt_sm_confirm_bonding_id:
            printf("sl_bt_evt_sm_confirm_bonding_id\r\n");
            printf("    Event Parameters:\r\n");
            printf("        connection:     0x%02x\r\n", evt->data.evt_sm_confirm_bonding.connection);
            printf("        bonding_handle: %02d",   evt->data.evt_sm_confirm_bonding.bonding_handle);
            break;
//        case sl_bt_evt_homekit_setupcode_display_id:
//            printf("sl_bt_evt_homekit_setupcode_display_id\r\n");
//            break;
//        case sl_bt_evt_homekit_paired_id:
//            printf("sl_bt_evt_homekit_paired_id\r\n");
//            break;
//        case sl_bt_evt_homekit_pair_verified_id:
//            printf("sl_bt_evt_homekit_pair_verified_id\r\n");
//            break;
//        case sl_bt_evt_homekit_connection_opened_id:
//            printf("sl_bt_evt_homekit_connection_opened_id\r\n");
//            break;
//        case sl_bt_evt_homekit_connection_closed_id:
//            printf("sl_bt_evt_homekit_connection_closed_id\r\n");
//            break;
//        case sl_bt_evt_homekit_identify_id:
//            printf("sl_bt_evt_homekit_identify_id\r\n");
//            break;
//        case sl_bt_evt_homekit_write_request_id:
//            printf("sl_bt_evt_homekit_write_request_id\r\n");
//            break;
//        case sl_bt_evt_homekit_read_request_id:
//            printf("sl_bt_evt_homekit_read_request_id\r\n");
//            break;
//        case sl_bt_evt_homekit_disconnection_required_id:
//            printf("sl_bt_evt_homekit_disconnection_required_id\r\n");
//            break;
//        case sl_bt_evt_homekit_pairing_removed_id:
//            printf("sl_bt_evt_homekit_pairing_removed_id\r\n");
//            break;
//        case sl_bt_evt_homekit_setuppayload_display_id:
//            printf("sl_bt_evt_homekit_setuppayload_display_id\r\n");
//            break;
        case sl_bt_evt_l2cap_coc_connection_request_id:
            printf("sl_bt_evt_l2cap_coc_connection_request_id\r\n");
            break;
        case sl_bt_evt_l2cap_coc_connection_response_id:
            printf("sl_bt_evt_l2cap_coc_connection_response_id\r\n");
            break;
        case sl_bt_evt_l2cap_coc_le_flow_control_credit_id:
            printf("sl_bt_evt_l2cap_coc_le_flow_control_credit_id\r\n");
            break;
        case sl_bt_evt_l2cap_coc_channel_disconnected_id:
            printf("sl_bt_evt_l2cap_coc_channel_disconnected_id\r\n");
            break;
        case sl_bt_evt_l2cap_coc_data_id:
            printf("sl_bt_evt_l2cap_coc_data_id\r\n");
            break;
        case sl_bt_evt_l2cap_command_rejected_id:
            printf("sl_bt_evt_l2cap_command_rejected_id\r\n");
            break;
//        case sl_bt_evt_cte_receiver_iq_report_id:
//            printf("sl_bt_evt_cte_receiver_iq_report_id\r\n");
//            break;
        case sl_bt_evt_user_message_to_host_id:
            printf("sl_bt_evt_user_message_to_host_id\r\n");
            break;
        default:
            printf("Event not recognized.\r\n");
            break;
    }
}
