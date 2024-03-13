/***************************************************************************//**
 * @file vl53l1x_app.c
 * @brief Distance sampling and people counting module with VL53L1X Sensor
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
 * 1. The origin of this software must not be misrepresented; you must not
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
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "user_config_nvm3.h"
#include "vl53l1x_app.h"
#include "sparkfun_vl53l1x.h"
#include "sparkfun_vl53l1x_config.h"
#include "sl_i2cspm_instances.h"
#include "sl_sleeptimer.h"

// -----------------------------------------------------------------------------
// Led
#if defined(SL_CATALOG_SIMPLE_LED_PRESENT) && defined(SL_CATALOG_LED0_PRESENT)
#include "sl_simple_led_instances.h"
#define led0_on()   sl_led_turn_on(&sl_led_led0);
#define led0_off()  sl_led_turn_off(&sl_led_led0);
#else
#define led0_on()
#define led0_off()
#endif

/***************************************************************************//**
 * @addtogroup vl53l1x_app
 * @brief  VL53L1X Application.
 * @details
 * @{
 ******************************************************************************/

// -----------------------------------------------------------------------------
// Helper macros

#define someone_enter_patterns(path_track_4_patterns) \
  (((path_track_4_patterns)[1] == 1)                  \
   && ((path_track_4_patterns)[2] == 3)               \
   && ((path_track_4_patterns)[3] == 2))

#define someone_leave_patterns(path_track_4_patterns) \
  (((path_track_4_patterns)[1] == 2)                  \
   && ((path_track_4_patterns)[2] == 3)               \
   && ((path_track_4_patterns)[3] == 1))

// -----------------------------------------------------------------------------
// Defines

#define DISTANCES_ARRAY_SIZE      10   // number of samples
#define DISTANCE_MODE             VL53L1X_DISTANCE_MODE_LONG

#define FRONT_ZONE_CENTER         175
#define BACK_ZONE_CENTER          231

#define PATTERN_ZONE_LEFT         (1 << 0)
#define PATTERN_ZONE_RIGHT        (1 << 1)

enum ZONE_STATUS {
  NOBODY,
  SOMEONE
};

enum ZONE {
  LEFT,
  RIGHT
};

enum ZONE_EVENT {
  NO_EVENT,
  INVALID_PATTERN,
  FILLING_PATTERN,
  SOMEONE_ENTER,
  SOMEONE_LEAVE,
};

typedef struct {
  enum ZONE_STATUS last_zone_status_left;
  enum ZONE_STATUS last_zone_status_right;
  uint8_t filling_size;
  uint8_t filling_patterns[4];
}path_track_state_t;

// -----------------------------------------------------------------------------
// Private variables

static const uint8_t roi_center[2] = { FRONT_ZONE_CENTER, BACK_ZONE_CENTER };
static uint8_t zone = 0;
static uint16_t min_distance;
static uint16_t max_distance;
static uint16_t measured_distance = 0;

static uint32_t invalid_count = 0;
static uint16_t people_count = 0;
static uint32_t people_entered_so_far;

static uint16_t distance_threshold;

static path_track_state_t path_track_state = {
  .last_zone_status_left = NOBODY,
  .last_zone_status_right = NOBODY,
  .filling_size = 0,
  .filling_patterns = { 0, 0, 0, 0 }
};

// -----------------------------------------------------------------------------
// Private function declarations

static uint16_t recalculate_distance(uint16_t distance, uint8_t range_status);
static void change_timing_budget(uint16_t timing_budget);
static uint16_t process_people_counting_data(int16_t distance, enum ZONE zone);
static enum ZONE_EVENT update_path_track_state(path_track_state_t *pt_state,
                                               enum ZONE_STATUS current_zone_status,
                                               enum ZONE zone);

// -----------------------------------------------------------------------------
// Public function definitions

/***************************************************************************//**
 * Application Init.
 ******************************************************************************/
void vl53l1x_app_init(void)
{
  sl_status_t sc;
  uint8_t boot_state = 0;
  uint16_t timing_budget;

  app_log("[NVM3]: Loading data from NVM memory ...\n");
  min_distance = user_config_nvm3_get_min_distance();
  app_log("[NVM3]: Min Distance: %d\n", min_distance);
  max_distance = user_config_nvm3_get_max_distance();
  app_log("[NVM3]: Max Distance: %d\n", max_distance);
  distance_threshold = user_config_nvm3_get_distance_threshold();
  app_log("[NVM3]: Distance Threshold: %d\n", distance_threshold);
  timing_budget = user_config_nvm3_get_timing_budget();
  app_log("[NVM3]: Timing Budget: %d\n", timing_budget);
  people_entered_so_far = user_config_nvm3_get_people_entered_so_far();
  app_log("[NVM3]: People Entered So Far: %ld\n", people_entered_so_far);

  // Initialize VL53L1X
  sc = vl53l1x_init(VL53L1X_ADDR, sl_i2cspm_qwiic);
  if (sc == SL_STATUS_OK) {
    app_log("[VL53L1X]: SparkFun Distance Sensor initialized success.\n");
  } else {
    app_log("[VL53L1X]: SparkFun Distance Sensor initialized failed!\n");
  }
  // Waiting for device to boot up...
  while (0 == boot_state) {
    // Read sensor's state (0 = boot state, 1 = device is booted )
    sc = vl53l1x_get_boot_state(VL53L1X_ADDR, &boot_state);

    if (SL_STATUS_OK != sc) {
      break;
    }
    // Wait for 2 ms
    sl_sleeptimer_delay_millisecond(2);
  }

  if (SL_STATUS_OK == sc) {
    app_log("[VL53L1X]: Platform I2C communication is OK.\n");
  } else {
    app_log("[VL53L1X]: Platform I2C communication test has been failed.\n");
    return;
  }
  app_log("[VL53L1X]: Sensor is booted.\n");
  // Configure distance mode to LONG distance mode
  sc = vl53l1x_set_distance_mode(VL53L1X_ADDR, DISTANCE_MODE);
  app_assert_status(sc);

  // Set timing budget
  change_timing_budget(timing_budget);

  // Set region of interest center
  sc = vl53l1x_set_roi_center(VL53L1X_ADDR, BACK_ZONE_CENTER);
  app_assert_status(sc);

  // Set region of interest SPADs 8x16
  sc = vl53l1x_set_roi_xy(VL53L1X_ADDR, 8, 16);
  app_assert_status(sc);

  // Start ranging
  app_log("=============== Start counting people ================\r\n");
  sc = vl53l1x_start_ranging(VL53L1X_ADDR);
  app_assert_status(sc);
}

/***************************************************************************//**
 * VL53L1X Process Action.
 ******************************************************************************/
void vl53l1x_app_process_action(void)
{
  uint8_t is_data_ready = 0;
  uint8_t range_status = 0;
  uint16_t distance = 0;
  uint16_t signal_per_spad;
  sl_status_t sc;

  sc = vl53l1x_check_for_data_ready(VL53L1X_ADDR, &is_data_ready);
  app_assert_status(sc);
  if (is_data_ready) {
    sc = vl53l1x_get_range_status(VL53L1X_ADDR, &range_status);
    app_assert_status(sc);

    sc = vl53l1x_get_distance(VL53L1X_ADDR, &distance);
    app_assert_status(sc);
    measured_distance = distance;

    sc = vl53l1x_get_signal_per_spad(VL53L1X_ADDR, &signal_per_spad);
    app_assert_status(sc);

    sc = vl53l1x_clear_interrupt(VL53L1X_ADDR);
    app_assert_status(sc);

    // re-calculate distance base on range status
    distance = recalculate_distance(distance, range_status);

    // add new ranged distance sample to the people counting algorithm
    people_count = process_people_counting_data(distance, zone);

    zone++;
    zone %= 2;

    sc = vl53l1x_set_roi_center(VL53L1X_ADDR, roi_center[zone]);
    app_assert_status(sc);
  }
}

/***************************************************************************//**
 * VL53L1X Get People Count.
 ******************************************************************************/
uint16_t vl53l1x_app_get_people_count(void)
{
  return people_count;
}

/***************************************************************************//**
 * VL53L1X Get Measured Distance.
 ******************************************************************************/
uint16_t vl53l1x_app_get_current_measured_distance(void)
{
  return measured_distance;
}

/***************************************************************************//**
 * VL53L1X Clear People Count.
 ******************************************************************************/
void vl53l1x_app_clear_people_count(void)
{
  people_count = 0;
}

/***************************************************************************//**
 * VL53L1X Get People Entered So Far.
 ******************************************************************************/
uint32_t vl53l1x_app_get_people_entered_so_far(void)
{
  return people_entered_so_far;
}

/***************************************************************************//**
 * VL53L1X Get Invalid Count.
 ******************************************************************************/
uint32_t vl53l1x_app_get_invalid_count(void)
{
  return invalid_count;
}

/***************************************************************************//**
 * VL53L1X Clear People Entered So Far Counter.
 ******************************************************************************/
void vl53l1x_app_clear_people_entered_so_far(void)
{
  people_entered_so_far = 0;
  user_config_nvm3_set_people_entered_so_far(0);
}

/***************************************************************************//**
 * VL53L1X Change Timing Budget.
 ******************************************************************************/
sl_status_t vl53l1x_app_change_timing_budget_in_ms(uint16_t timing_budget)
{
  change_timing_budget(timing_budget);
  return user_config_nvm3_set_timing_budget(timing_budget);
}

// -----------------------------------------------------------------------------
// Private function
static uint16_t recalculate_distance(uint16_t distance, uint8_t range_status)
{
  switch (range_status) {
    case 0:  // VL53L1_RANGESTATUS_RANGE_VALID Ranging measurement is valid
    case 4:  // VL53L1_RANGESTATUS_OUTOFBOUNDS_ FAIL Raised when phase
    // is out of bounds
    case 7:  // VL53L1_RANGESTATUS_WRAP_TARGET_ FAIL Wrapped target,
             // not matching phases
      // wraparound case see the explanation at the constants definition place
      if (distance <= min_distance) {
        distance = max_distance + min_distance;
      }
      break;
    case 1:  // VL53L1_RANGESTATUS_SIGMA_FAIL Raised if sigma estimator check
    // is above the internal defined threshold
    case 2:  // VL53L1_RANGESTATUS_SIGNAL_FAIL Raised if signal value
    // is below the internal defined threshold
    case 5:  // VL53L1_RANGESTATUS_HARDWARE_FAIL Raised in case
    // of HW or VCSEL failure
    case 8:  // VL53L1_RANGESTATUS_PROCESSING_FAIL
    // Internal algorithm underflow or overflow
    case 14: // VL53L1_RANGESTATUS_RANGE_INVALID The reported range is invalid
      distance = max_distance;
      break;
    case 13: // The 13 simply means the hardware was not able to select
             // that particular ROI with that specific center location.
             // This situation happens a lot when moving the ROI
             // as close to the edge
             // as possible. To avoid it, reduce the X or Y
             // dimensions or move the ROI_Center one SPAD toward the middle.
      app_assert(0, "The configuration is not correct: see UM2555\r\n");
      break;
    default:
      app_log("[VL53L1X]: Unknown range status: %d\r\n", range_status);
  }
  return distance;
}

static void change_timing_budget(uint16_t timing_budget)
{
  sl_status_t sc;

  // Set timing budget
  sc = vl53l1x_set_timing_budget_in_ms(VL53L1X_ADDR, timing_budget);
  if (sc != SL_STATUS_OK) {
    app_log("[VL53L1X]: Set budget timing error: %d\r\n", (int)sc);
  }

  // Set inter-measurement
  sc = vl53l1x_set_inter_measurement_in_ms(VL53L1X_ADDR, timing_budget);
  if (sc != SL_STATUS_OK) {
    app_log("[VL53L1X]: Set inter-measurement timing error: %d\r\n", (int)sc);
  }
}

static enum ZONE_EVENT update_path_track_state
(
  path_track_state_t *pt_state,
  enum ZONE_STATUS current_zone_status,
  enum ZONE zone
)
{
  bool current_zone_status_is_changed = false;
  uint8_t pattern = 0;
  enum ZONE_EVENT zone_event = NO_EVENT;

  if (zone == LEFT) {
    if (current_zone_status != pt_state->last_zone_status_left) {
      // left zone status is changed
      current_zone_status_is_changed = true;

      if (current_zone_status == SOMEONE) {
        pattern |= PATTERN_ZONE_LEFT;
      }
      // check right zone
      if (pt_state->last_zone_status_right == SOMEONE) {
        // mark the right zone
        pattern |= PATTERN_ZONE_RIGHT;
      }
      // store current zone status
      pt_state->last_zone_status_left = current_zone_status;
    }
  } else {
    if (current_zone_status != pt_state->last_zone_status_right) {
      // right zone status is changed
      current_zone_status_is_changed = true;
      if (current_zone_status == SOMEONE) {
        pattern |= PATTERN_ZONE_RIGHT;
      }
      // check the left zone
      if (pt_state->last_zone_status_left == SOMEONE) {
        // mark the left zone
        pattern |= PATTERN_ZONE_LEFT;
      }
      // store current zone status
      pt_state->last_zone_status_right = current_zone_status;
    }
  }

  // if zone status is changed
  if (current_zone_status_is_changed) {
    if (pt_state->filling_size < 4) {
      pt_state->filling_size++;
    }

    // if nobody is in the checking area,
    // lets check if an exit or entry has happened
    if ((pt_state->last_zone_status_right == NOBODY)
        && (pt_state->last_zone_status_left == NOBODY)) {
      // check pattern only if pattern filling size
      // is 4 and nobobdy is in the checking area
      if (pt_state->filling_size == 4) {
        // no need to check filling_patterns[0] == 0

        if (someone_enter_patterns(pt_state->filling_patterns)) {
          // someone enter the room
          zone_event = SOMEONE_ENTER;
        } else if (someone_leave_patterns(pt_state->filling_patterns)) {
          // someone leave the room
          zone_event = SOMEONE_LEAVE;
        } else {
          // invalid pattern, assume that no event has happened
          zone_event = INVALID_PATTERN;
        }
      }
      // reset the pattern buffer
      pt_state->filling_size = 1;
    } else {
      // update path_track
      pt_state->filling_patterns[pt_state->filling_size - 1] = pattern;
      zone_event = FILLING_PATTERN;
    }
  }
  return zone_event;
}

static uint16_t process_people_counting_data(int16_t distance, enum ZONE zone)
{
  static uint16_t distances[2][DISTANCES_ARRAY_SIZE];
  static uint16_t distances_sample_count[2] = { 0, 0 };
  uint16_t min_distance;
  uint8_t i;
  enum ZONE_STATUS current_zone_status = NOBODY;
  enum ZONE_EVENT zone_event;

  // Add just picked distance to the table of the corresponding zone
  distances[zone][distances_sample_count[zone] % DISTANCES_ARRAY_SIZE]
    = distance;
  distances_sample_count[zone]++;

  // pick up the min distance
  min_distance = distances[zone][0];
  for (i = 1; i < DISTANCES_ARRAY_SIZE
       && i < distances_sample_count[zone]; i++) {
    if (distances[zone][i] < min_distance) {
      min_distance = distances[zone][i];
    }
  }

  if (min_distance < distance_threshold) {
    // Someone is in !
    current_zone_status = SOMEONE;
    led0_on();
  } else {
    led0_off();
  }

  zone_event = update_path_track_state(&path_track_state,
                                       current_zone_status,
                                       zone);
  switch (zone_event) {
    case SOMEONE_ENTER:
      // someone enter the room
      people_count++;
      people_entered_so_far++;
      user_config_nvm3_set_people_entered_so_far(people_entered_so_far);
      // reset the table filling size
      distances_sample_count[0] = 0;
      distances_sample_count[1] = 0;
      app_log("[VL53L1X]: Someone In, People Count=%d\r\n", people_count);
      break;

    case SOMEONE_LEAVE:
      // someone leave the room
      if (people_count) {
        people_count--;
      }
      // reset the table filling size
      distances_sample_count[0] = 0;
      distances_sample_count[1] = 0;
      app_log("[VL53L1X]: Someone Out, People Count=%d\r\n", people_count);
      break;

    case INVALID_PATTERN:
      // reset the table filling size
      distances_sample_count[0] = 0;
      distances_sample_count[1] = 0;
      app_log("[VL53L1X]: Invalid pattern\r\n");
      break;

    default:
      break;
  }
  return people_count;
}

/** @} (end group vl53l1x_app) */
