/***************************************************************************//**
 * @file
 * @brief Core application logic.
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
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "sl_simple_button_instances.h"
#include "sl_sleeptimer.h"

// Notes to MIDI code
#define    C__4 60
#define    Ci_4 61
#define    D__4 62
#define    Di_4 63
#define    E__4 64
#define    F__4 65
#define    Fi_4 66
#define    G__4 67
#define    Gi_4 68
#define    A__4 69
#define    Ai_4 70
#define    B__4 71
#define    C__5 72
#define    Ci_5 73
#define    D__5 74
#define    Di_5 75
#define    E__5 76
#define    F__5 77
#define    Fi_5 78
#define    G__5 79
#define    Gi_5 80
#define    A__5 81
#define    Ai_5 82
#define    B__5 83

#define SIGNAL_BTN_0_PRESSED  0x01
#define SIGNAL_BTN_0_RELEASED 0x02
#define SIGNAL_BTN_1_PRESSED  0x04

PACKSTRUCT( typedef struct
{
  uint8_t  header;
  uint8_t  timestamp;
  uint8_t  status;
  uint8_t  note;
  uint8_t  velocity;

})midi_event_packet_t;

typedef union {
  midi_event_packet_t  midi_event;
  uint8_t payload[5];
}midi_data_t;

#define MELODY_SIZE 18
static uint8_t melody[MELODY_SIZE] = {A__4, A__4, A__4, F__4, C__5, A__4, F__4, C__5, A__4,
                    E__5, E__5, E__5, F__5, C__5, Gi_4, F__4, C__5, A__4};
static uint8_t playhead = 0;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static uint8_t conn_handle = 0xFF;

static void midi_note_off(uint8_t note, uint8_t velocity);
static void midi_note_on(uint8_t note, uint8_t velocity);
/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Create an advertising set.
      sl_bt_advertiser_create_set(&advertising_set_handle);
      // Set advertising interval to 100ms.
      sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events

      // Start general advertising and enable connections.
      sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      conn_handle = evt->data.evt_connection_opened.connection;
      // Connection parameter constrains for BLE MIDI
      sl_bt_connection_set_parameters(conn_handle, 0x12, 0x12, 0, 0x64, 0, 0xFFFF);
      break;

    case sl_bt_evt_system_external_signal_id:
      if(evt->data.evt_system_external_signal.extsignals & SIGNAL_BTN_0_PRESSED){
          midi_note_on(melody[playhead], 100);
      }
      if(evt->data.evt_system_external_signal.extsignals & SIGNAL_BTN_0_RELEASED){
          midi_note_off(melody[playhead], 100);
          // Next note in melody
          playhead++;
          // If the melody is over reset the playhead
          if (playhead >= MELODY_SIZE)
          {
            playhead = 0;
          }
      }
      if(evt->data.evt_system_external_signal.extsignals & SIGNAL_BTN_1_PRESSED){
          // Start the melody from beginning
          midi_note_off(melody[playhead], 100);
          playhead = 0;
      }
      break;
    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      conn_handle = 0xFF;
      midi_note_off(melody[playhead], 100);
      playhead = 0;
      // Restart advertising after client has disconnected.
      sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

void sl_button_on_change(const sl_button_t *handle)
{
  if(&sl_button_btn0 == handle){
      if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED){
          sl_bt_external_signal(SIGNAL_BTN_0_PRESSED);
      }
      else{
          sl_bt_external_signal(SIGNAL_BTN_0_RELEASED);
      }
  }
  else if(&sl_button_btn1 == handle){
      if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED){
          sl_bt_external_signal(SIGNAL_BTN_1_PRESSED);
      }
  }
}

static void midi_note_on(uint8_t note, uint8_t velocity)
{
  static midi_data_t note_on;

  uint32_t tick;
  uint32_t temp;

  tick = sl_sleeptimer_get_tick_count();
  temp = sl_sleeptimer_tick_to_ms(tick);
  temp = temp & 0x00001fff;                                // Mask it - only the lower 13 bit needed

  note_on.midi_event.header     = 0x80 | (temp >> 7);     // Header byte = 0b10xxxxxx where xxxxxxx is top 6 bits of timestamp
  note_on.midi_event.timestamp  = 0x80 | (temp & 0x003f); // Timestamp byte = 0b1xxxxxxx where xxxxxxx is lower 7 bits of timestamp
  note_on.midi_event.status     = 0x90;                   // Status byte = 0b1sssnnnn where sss is message type and nnnn is channel
  note_on.midi_event.note       = note;                   // Setting the note parameter
  note_on.midi_event.velocity   = velocity;               // Setting the velocity parameter

  // Sending the assembled midi message
  sl_bt_gatt_server_send_notification(conn_handle, gattdb_xgatt_midi, 5, (uint8_t const *) &note_on);
}

static void midi_note_off(uint8_t note, uint8_t velocity)
{
  static midi_data_t note_off;

  uint32_t tick;
  uint32_t temp;

  tick = sl_sleeptimer_get_tick_count();
  temp = sl_sleeptimer_tick_to_ms(tick);

  temp = temp & 0x00001fff;
  temp = temp & 0x00001fff;                                  // Only the lower 13 bit needed

  note_off.midi_event.header    = 0x80 | (temp >> 7);       // Header byte = 0b10xxxxxx where xxxxxxx is top 6 bits of timestamp
  note_off.midi_event.timestamp = 0x80 | (temp & 0x003f);   // Timestamp byte = 0b1xxxxxxx where xxxxxxx is lower 7 bits of timestamp
  note_off.midi_event.status    = 0x80;                     // Status byte = 0b1sssnnnn where sss is message type and nnnn is channel
  note_off.midi_event.note      = note;                     // Setting the note parameter
  note_off.midi_event.velocity  = velocity;                 // Setting the velocity parameter

  sl_bt_gatt_server_send_notification(conn_handle, gattdb_xgatt_midi, 5, (uint8_t const*)&note_off);
}
