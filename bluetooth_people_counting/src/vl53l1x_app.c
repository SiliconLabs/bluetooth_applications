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
#include "vl53l1x_config.h"
#include "vl53l1x.h"
#include "vl53l1x_app.h"

// -----------------------------------------------------------------------------
// Logging
#define TAG "vl53l1x"
// use applog for the log printing
#if defined(SL_CATALOG_APP_LOG_PRESENT) && APP_LOG_ENABLE
#include "app_log.h"
#define log_info(fmt, ...)  app_log_info("[" TAG "]" fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) app_log_error("[" TAG "]" fmt, ##__VA_ARGS__)
// use stdio printf for the log printing
#elif defined(SL_CATALOG_RETARGET_STDIO_PRESENT)
#define log_info(fmt, ...)   printf("[" TAG "]" fmt, ##__VA_ARGS__)
#define log_error(fmt, ...)  printf("[" TAG "]" fmt, ##__VA_ARGS__)
#else  // the logging is disabled
#define log_info(...)
#define log_error(...)
#endif // #if defined(SL_CATALOG_APP_LOG_PRESENT)

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
// Defines

#define NOBODY                    0
#define SOMEONE                   1
#define LEFT                      0
#define RIGHT                     1

#define DISTANCES_ARRAY_SIZE      10   // number of samples
#define DISTANCE_MODE             VL53L1X_DISTANCE_MODE_LONG

#define FRONT_ZONE_CENTER         175
#define BACK_ZONE_CENTER          231

// -----------------------------------------------------------------------------
// Private variables

static const uint8_t roi_center[2] = {FRONT_ZONE_CENTER, BACK_ZONE_CENTER};
static uint8_t zone = 0;
static uint16_t min_distance;
static uint16_t max_distance;
static uint16_t measured_distance = 0;

static uint32_t invalid_count = 0;
static uint16_t people_count = 0;
static uint32_t people_entered_so_far;

static uint16_t distance_threshold;

// -----------------------------------------------------------------------------
// Private function declarations

static void change_timing_budget(uint16_t timing_budget);
static uint16_t process_people_counting_data(int16_t distance, uint8_t zone);

// -----------------------------------------------------------------------------
// Public function definitions

/***************************************************************************//**
 * Application Init.
 ******************************************************************************/
void vl53l1x_app_init(void)
{
  sl_status_t sc;
  uint8_t boot_state;
  uint16_t sensor_id;
  uint16_t timing_budget;

  min_distance = user_config_nvm3_get_min_distance();
  max_distance = user_config_nvm3_get_max_distance();
  distance_threshold = user_config_nvm3_get_distance_threshold();
  timing_budget = user_config_nvm3_get_timing_budget();
  people_entered_so_far = user_config_nvm3_get_people_entered_so_far();
  log_info("distance: min %d, max %d, threshold %d\r\n",
           min_distance, max_distance, distance_threshold);
  log_info("timing budget: %d\r\n", timing_budget);

  sc = vl53l1x_get_sensor_id(VL53L1X_ADDR, &sensor_id);
  app_assert_status(sc);
  log_info("VL53L1X Model ID: %X\r\n", sensor_id);

  // Wait for boot the VL53L1X sensor up
  do {
    sc = vl53l1x_get_boot_state(VL53L1X_ADDR, &boot_state);
    app_assert_status(sc);
  } while(boot_state == 0);
  log_info("VL53L1X booted\r\n");

  // Initialize VL53L1X
  sc = vl53l1x_init(VL53L1X_ADDR);
  app_assert_status(sc);

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
  log_info("Start counting people\r\n");
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

    switch (range_status) {
      case 0:  // VL53L1_RANGESTATUS_RANGE_VALID Ranging measurement is valid
      case 4:  // VL53L1_RANGESTATUS_OUTOFBOUNDS_ FAIL Raised when phase
               // is out of bounds
      case 7:  // VL53L1_RANGESTATUS_WRAP_TARGET_ FAIL Wrapped target,
               // not matching phases
        // wraparound case see the explanation at the constants definition place
        if (distance <= min_distance)
          distance = max_distance + min_distance;
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
        log_error("Unknown range status: %d\r\n", range_status);
    }
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
static void change_timing_budget(uint16_t timing_budget)
{
  sl_status_t sc;

  //Set timing budget
  sc = vl53l1x_set_timing_budget_in_ms(VL53L1X_ADDR, timing_budget);
  if (sc != SL_STATUS_OK) {
    log_info("Set budget timing error: %d\r\n", (int)sc);
  }

  // Set inter-measurement
  sc = vl53l1x_set_inter_measurement_in_ms(VL53L1X_ADDR, timing_budget);
  if (sc != SL_STATUS_OK) {
    log_info("Set inter-measurement timing error: %d\r\n", (int)sc);
  }
}

static uint16_t process_people_counting_data(int16_t distance, uint8_t zone)
{
  static uint8_t path_track[] = {0,0,0,0};
  // init this to 1 as we start from state where nobody is any of the zones
  static uint8_t path_track_filling_size = 1;
  static uint8_t left_previous_status = NOBODY;
  static uint8_t right_previous_status = NOBODY;
  static uint16_t distances[2][DISTANCES_ARRAY_SIZE];
  static uint16_t distances_sample_count[2] = {0,0};

  uint16_t min_distance;
  uint8_t i;

  uint8_t current_zone_status = NOBODY;
  uint8_t all_zones_current_status = 0;
  uint8_t an_event_has_occured = 0;

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
  }

  if (zone == LEFT) {
    if (current_zone_status != left_previous_status) {
      // event in left zone has occured
      an_event_has_occured = 1;

      if (current_zone_status == SOMEONE) {
        all_zones_current_status += 1;
      }
      // need to check right zone as well ...
      if (right_previous_status == SOMEONE) {
        // event in left zone has occured
        all_zones_current_status += 2;
      }
      // store current zone status
      left_previous_status = current_zone_status;
    }
  } else {

    if (current_zone_status != right_previous_status) {

      // event in left zone has occured
      an_event_has_occured = 1;
      if (current_zone_status == SOMEONE) {
        all_zones_current_status += 2;
      }
      // need to left right zone as well ...
      if (left_previous_status == SOMEONE) {
        // event in left zone has occured
        all_zones_current_status += 1;
      }
      // store current zone status
      right_previous_status = current_zone_status;
    }
  }

  // if an event has occured
  if (an_event_has_occured) {
    if (path_track_filling_size < 4) {
      path_track_filling_size ++;
    }

    // if nobody anywhere lets check if an exit or entry has happened
    if ((left_previous_status == NOBODY) && (right_previous_status == NOBODY)) {

      // check exit or entry only if path_track_filling_size
      // is 4 (for example 0 1 3 2) and last event is 0 (nobobdy anywhere)
      if (path_track_filling_size == 4) {
        // check exit or entry. no need to check path_track[0] == 0 ,
        // it is always the case

        if ((path_track[1] == 1)
            && (path_track[2] == 3)
            && (path_track[3] == 2)) {
          // People enter the room
          people_count++;
          people_entered_so_far++;
          user_config_nvm3_set_people_entered_so_far(people_entered_so_far);

          // reset the table filling size in case an entry or exit just found
          distances_sample_count[0] = 0;
          distances_sample_count[1] = 0;
          log_info("Someone In, People Count=%d\r\n", people_count);
        } else if ((path_track[1] == 2)
                   && (path_track[2] == 3)
                   && (path_track[3] == 1)) {
          // People exit the room
          if (people_count) {
            people_count--;
          }

          // reset the table filling size in case an entry or exit just found
          distances_sample_count[0] = 0;
          distances_sample_count[1] = 0;
          log_info("Someone Out, People Count=%d\r\n", people_count);
        } else {
          // reset the table filling size also in case of unexpected path
          distances_sample_count[0] = 0;
          distances_sample_count[1] = 0;
          log_info("Invalid path\r\n");
        }
      }

      path_track_filling_size = 1;
      led0_off();
    } else {
      // update path_track
      // example of path_track update
      // 0
      // 0 1
      // 0 1 3
      // 0 1 3 1
      // 0 1 3 3
      // 0 1 3 2 ==> if next is 0 : check if exit
      path_track[path_track_filling_size-1] = all_zones_current_status;
    }
  }

  // output debug data to main host machine
  return people_count;
}

/** @} (end group vl53l1x_app) */
