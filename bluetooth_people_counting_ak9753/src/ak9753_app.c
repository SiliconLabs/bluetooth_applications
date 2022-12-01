/***************************************************************************//**
 * @file ak9753_app.c
 * @brief People counting module with AK9753 sensor.
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
 * 1. The origin of this software must not be misrepresented{} you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#include <stdio.h>

#include "app_assert.h"
#include "user_config_nvm3.h"

#include "ak9753_config.h"
#include "ak9753.h"
#include "ak9753_app.h"

// Entering pattern
#define someone_enter_patterns(path_track_4_patterns) \
  (((path_track_4_patterns)[1] == 1)                  \
   && ((path_track_4_patterns)[2] == 3)               \
   && ((path_track_4_patterns)[3] == 2))

// Leaving pattern
#define someone_leave_patterns(path_track_4_patterns) \
  (((path_track_4_patterns)[1] == 2)                  \
   && ((path_track_4_patterns)[2] == 3)               \
   && ((path_track_4_patterns)[3] == 1))

// Convert number to int12 number
#define int12(num) \
  (((num) < 0) ? (((uint16_t)(num) & 0x7ff) | (1 << 11)) : (num))

// Convert threshold in pA to value is written to register
#define threshold_to_value_register(pa)  int12((int16_t)((double)(pa) \
                                                         / 3.4788F))

// Convert hysteresis in pA to value is written to register
#define hysteresis_to_value_register(pa) (uint8_t)((double)(pa) / 3.4788F)

#define SOMEONE_IN_IR_THRESHOLD ((int16_t)((double)(ir_threshold) / 0.4375F))

/***************************************************************************//**
 * @brief
 *    Typedef for occupied zone
 ******************************************************************************/
typedef enum zone {
  OUTSIDE,
  FRONT,
  BACK,
  MIDDLE
} zone_t;

/***************************************************************************//**
 * @brief
 *    Typedef for detecting event
 ******************************************************************************/
typedef enum detect_event {
  NO_EVENT,
  INVALID_PATTERN,
  FILLING_PATTERN,
  SOMEONE_ENTER,
  SOMEONE_LEAVE,
} detect_event_t;

/***************************************************************************//**
 * @brief
 *    Typedef for infrared data
 ******************************************************************************/
typedef struct ir_data {
  bool ir24_int_h; ///< trigger if ir2 - ir4 > upper threshold
  bool ir24_int_l; ///< trigger if ir2 - ir4 < lower threshold
  int16_t ir2; ///<  ir2 data
  int16_t ir4; ///<  ir4 data
} ir_data_t;

/***************************************************************************//**
 * @brief
 *    Typedef for path track state data
 ******************************************************************************/
typedef struct path_track_state {
  uint8_t filling_patterns[4];
  uint8_t filling_size;
  zone_t last_zone;
} path_track_state_t;

// Local variable
static int16_t lower_threshold;
static int16_t upper_threshold;
static uint16_t hysteresis;
static int16_t ir_threshold;
static uint16_t people_count = 0;
static uint32_t people_entered_so_far = 0;
static bool notification_status = false;
static path_track_state_t pt_state = {
  .filling_size = 1,
  .filling_patterns = { 0 },
  .last_zone = OUTSIDE
};

// Funtion to process people counting algorithm
static uint16_t process_people_counting_data(ir_data_t *ir_data);
static detect_event_t update_path_track_state(path_track_state_t *pt_state,
                                              ir_data_t *ir_data);

/***************************************************************************//**
 * AK9753 application initialize.
 ******************************************************************************/
void ak9753_app_init(void)
{
  sl_status_t sc = SL_STATUS_OK;

  // Load configuration parameters from nvm
  lower_threshold = user_config_nvm3_get_lower_threshold();
  upper_threshold = user_config_nvm3_get_upper_threshold();
  hysteresis = user_config_nvm3_get_hysteresis();
  ir_threshold = user_config_nvm3_get_ir_threshold();
  people_entered_so_far = user_config_nvm3_get_people_entered_so_far();

  app_log("\r\nSENSOR CONFIGURATION PARAMETERS\
           \r\nLower threshold: %d\
           \r\nUpper threshold: %d\
           \r\nHysteresis: %d\
           \r\nIR threshold: %d",
          lower_threshold,
          upper_threshold,
          hysteresis,
          ir_threshold);

  ak9753_config_t ak9753_cfg = {
    .I2C_address = AK9753_ADDR,
    .ak9753_i2cspm_instance = AK9753_CONFIG_I2C_INSTANCE,
    .cut_off_freq = AK975X_FREQ_8_8HZ,
    .mode = AK975X_MODE_0,
    .upper_threshold13 = 0,
    .lower_threshold13 = 0,
    .upper_threshold24 = threshold_to_value_register(upper_threshold),
    .lower_threshold24 = threshold_to_value_register(lower_threshold),
    .hysteresis_value24 = hysteresis_to_value_register(hysteresis),
    .hysteresis_value13 = 0,
    .int_source_setting = 0x00,
    .int_present = false,
    .PDN_present = false
  };

  // Initialize the sensor
  sc = ak9753_init(&ak9753_cfg);
  app_assert_status(sc);
  sc = ak9753_set_hysteresis_eeprom_ir13(0);
  app_assert_status(sc);
  sc = ak9753_set_interrupts(0x1f);
  app_assert_status(sc);
}

/***************************************************************************//**
 * AK9753 application set people entered so far.
 ******************************************************************************/
void ak9753_app_clear_people_entered_so_far(void)
{
  people_entered_so_far = 0;
  user_config_nvm3_set_people_entered_so_far(0);
}

/***************************************************************************//**
 * AK9753 application get people entered so far.
 ******************************************************************************/
uint16_t ak9753_app_get_people_entered_so_far(void)
{
  return people_entered_so_far;
}

/***************************************************************************//**
 * AK9753 application set people entered count.
 ******************************************************************************/
void ak9753_app_clear_people_count(void)
{
  people_count = 0;
}

/***************************************************************************//**
 * AK9753 application get people entered count.
 ******************************************************************************/
uint16_t ak9753_app_get_people_count(void)
{
  return people_count;
}

/***************************************************************************//**
 * AK9753 application set notification.
 ******************************************************************************/
void ak9753_app_set_notification_status(bool status)
{
  notification_status = status;
}

/***************************************************************************//**
 * AK9753 application get notification.
 ******************************************************************************/
bool ak9753_app_get_notification_status(void)
{
  return notification_status;
}

/***************************************************************************//**
 * AK7953 Process Action.
 ******************************************************************************/
void ak9753_app_process_action(void)
{
  bool is_ir13_h;
  bool is_ir13_l;
  bool is_ir24_h;
  bool is_ir24_l;
  bool is_data_ready;
  bool is_data_overrun;
  int16_t ir2_data;
  int16_t ir4_data;
  ir_data_t ir_data;

  // Check interrupt status
  ak9753_is_interrupt(&is_ir13_h,
                      &is_ir13_l,
                      &is_ir24_h,
                      &is_ir24_l,
                      &is_data_ready,
                      &is_data_overrun);

  // Process people counting data if data is ready
  if (is_data_ready) {
    ak9753_get_ir2_data(&ir2_data);
    ak9753_get_ir4_data(&ir4_data);
    ak9753_get_dummy();
    ir_data.ir24_int_h = is_ir24_h;
    ir_data.ir24_int_l = is_ir24_l;
    ir_data.ir2 = ir2_data;
    ir_data.ir4 = ir4_data;
    people_count = process_people_counting_data(&ir_data);
  }
}

static uint16_t process_people_counting_data(ir_data_t *ir_data)
{
  detect_event_t detect_event = NO_EVENT;

  detect_event = update_path_track_state(&pt_state,
                                         ir_data);

  switch (detect_event) {
    case SOMEONE_ENTER:
      // someone enter the room
      people_count++;
      people_entered_so_far++;
      user_config_nvm3_set_people_entered_so_far(people_entered_so_far);
      app_log("\r\nSomeone In, People Count = %d", people_count);
      break;

    case SOMEONE_LEAVE:
      // someone leave the room
      if (people_count) {
        people_count--;
      }
      app_log("\r\nSomeone Out, People Count = %d", people_count);
      break;

    case INVALID_PATTERN:
      // reset the table filling size
      app_log("\r\nInvalid pattern");
      break;

    default:
      break;
  }
  return people_count;
}

static detect_event_t update_path_track_state(path_track_state_t *pt_state,
                                              ir_data_t *ir_data)
{
  bool zone_is_changed = false;
  zone_t zone = OUTSIDE;
  detect_event_t detect_event = NO_EVENT;

  if (!ir_data->ir24_int_h && !ir_data->ir24_int_l) {
    // No ir24 interrupt is detected
    // Check if someone is in the checking area
    // If last zone is FONT or BACK then current zone is MIDDLE
    if ((ir_data->ir2 > SOMEONE_IN_IR_THRESHOLD)
        || (ir_data->ir4 > SOMEONE_IN_IR_THRESHOLD)) {
      if ((pt_state->last_zone == BACK)
          || (pt_state->last_zone == FRONT)) {
        zone = MIDDLE;
        zone_is_changed = true;
      }
    } else {
      // Nobody is in the checking area
      if (pt_state->last_zone != OUTSIDE) {
        zone = OUTSIDE;
        zone_is_changed = true;
      }
    }
  } else if (ir_data->ir24_int_l) {
    // Someone is in the BACK zone
    if (pt_state->last_zone != BACK) {
      zone = BACK;
      zone_is_changed = true;
    }
  } else if (ir_data->ir24_int_h) {
    // Someone is in the FRONT zone
    if (pt_state->last_zone != FRONT) {
      zone = FRONT;
      zone_is_changed = true;
    }
  }

  if (zone_is_changed) {
    if (pt_state->filling_size < 4) {
      pt_state->filling_size++;
    }

    // if nobody is in the checking area,
    // lets check if an exit or entry has happened
    if (zone == OUTSIDE) {
      if (pt_state->filling_size == 4) {
        // check pattern only if pattern filling size
        // is 4 and nobody is in the checking area
        if (someone_enter_patterns(pt_state->filling_patterns)) {
          detect_event = SOMEONE_ENTER;
        } else if (someone_leave_patterns(pt_state->filling_patterns)) {
          detect_event = SOMEONE_LEAVE;
        } else {
          detect_event = INVALID_PATTERN;
        }
      }
      // reset the pattern buffer
      pt_state->filling_size = 1;
    } else {
      // update path_track
      pt_state->filling_patterns[pt_state->filling_size - 1] = zone;
      detect_event = FILLING_PATTERN;
    }
    pt_state->last_zone = zone;
  }
  return detect_event;
}
