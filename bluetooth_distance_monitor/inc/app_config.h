/***************************************************************************//**
 * @file app_config.h
 * @brief Application Configuration
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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
 *
 * # EXPERIMENTAL QUALITY
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 *
 ******************************************************************************/

#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Default BLE passkey for bonding
#define DISTANCE_MONITOR_PASSKEY                        123456u

// Distance sensor threshold window modes
#define DISTANCE_MONITOR_THRESHOLD_BELOW                0
#define DISTANCE_MONITOR_THRESHOLD_ABOVE                1
#define DISTANCE_MONITOR_THRESHOLD_IN                   2
#define DISTANCE_MONITOR_THRESHOLD_OUT                  3

// Notification status
#define DISTANCE_MONITOR_NOTIFICATION_DISABLED          0
#define DISTANCE_MONITOR_NOTIFICATION_ENABLED           1

// Distance sensor range mode
#define DISTANCE_MONITOR_RANGE_MODE_SHORT               1
#define DISTANCE_MONITOR_RANGE_MODE_LONG                2

// Distance sensor limits
#define DISTANCE_MONITOR_RANGE_LOWER_LIMIT              40
#define DISTANCE_MONITOR_RANGE_MODE_SHORT_UPPER_LIMIT   1300
#define DISTANCE_MONITOR_RANGE_MODE_LONG_UPPER_LIMIT    4000

// Application sampling configuration
#define DISTANCE_MONITOR_SAMPLE_BUFFER_LENGTH           10
#define DISTANCE_MONITOR_MAIN_TIMER_PERIOD              100

// Application supported features
#define DISTANCE_MONITOR_BLE_FEATURE_LENGTH             6

/***************************************************************************//**
 * @brief
 *    Typedef for BLE user request callback function.
 ******************************************************************************/
typedef void (*parameter_change_callback)(void);

/***************************************************************************//**
 * @brief
 *    Typedef for holding application configuration parameters.
 ******************************************************************************/
typedef struct {
  uint16_t threshold_value_lower; ///<  Lower distance threshold value in mm.
  uint16_t threshold_value_upper; ///<  Upper distance threshold value in mm.
  ///<  Threshold mode (0: below, 1: above, 2: in, 3: out)
  uint8_t threshold_mode;
  uint8_t range_mode; ///<  Sensor range mode (1: short, 2: long)
  ///<  Notification status (0: disabled, 1: enabled)
  uint8_t notification_status;
  uint8_t buzzer_volume; ///<  Buzzer volume (0-1000)
} distance_monitor_config_data_t;

/***************************************************************************//**
 * @brief
 *    Typedef for holding application supported features.
 ******************************************************************************/
typedef struct {
  uint8_t *data; ///< Feature data pointer.
  uint16_t nvm_key; ///< Parameter NVM key.
  uint8_t data_length; ///< Size of the referenced data structure.
  uint16_t char_id; ///< BLE Characteristic identifier
  uint16_t value_min; ///< Data min. value
  uint16_t value_max; ///< Data max. value
  parameter_change_callback param_change_callback; ///< Callback function
} distance_monitor_feature_t;

// Distance monitor runtime data-set default values
#define DISTANCE_MONITOR_RUNTIME_DEFAULT_DATASET { \
    .calculated_average_distance = 0,              \
    .is_notification_active = false,               \
    .is_distance_out_of_range = false,             \
}                                                  \

// Distance monitor application default configuration parameters
#define DISTANCE_MONITOR_DEFAULT_CONFIG          { \
    .notification_status = 0,                      \
    .buzzer_volume = 20,                           \
    .range_mode = 2,                               \
    .threshold_mode = 0,                           \
    .threshold_value_lower = 250,                  \
    .threshold_value_upper = 450,                  \
}                                                  \

// Distance monitor application BLE features, see distance_monitor_feature_t
#define DISTANCE_MONITOR_FEATURES                {                             \
    { .char_id = gattdb_threshold_mode,                                        \
      .data = (uint8_t *) &distance_monitor_config.threshold_mode,             \
      .data_length = sizeof(distance_monitor_config.threshold_mode),           \
      .nvm_key = 0x4001,                                                       \
      .value_min = 0,                                                          \
      .value_max = 3,                                                          \
      .param_change_callback = app_logic_draw_parameter_line,                  \
    },                                                                         \
    { .char_id = gattdb_lower_threshold_value,                                 \
      .data = (uint8_t *) &distance_monitor_config.threshold_value_lower,      \
      .data_length = sizeof(distance_monitor_config.threshold_value_lower),    \
      .nvm_key = 0x4002,                                                       \
      .value_min = 50,                                                         \
      .value_max = 4000,                                                       \
      .param_change_callback = app_logic_draw_parameter_line,                  \
    },                                                                         \
    { .char_id = gattdb_upper_threshold_value,                                 \
      .data = (uint8_t *) &distance_monitor_config.threshold_value_upper,      \
      .data_length = sizeof(distance_monitor_config.threshold_value_upper),    \
      .nvm_key = 0x4003,                                                       \
      .value_min = 50,                                                         \
      .value_max = 4000,                                                       \
      .param_change_callback = app_logic_draw_parameter_line,                  \
    },                                                                         \
    { .char_id = gattdb_buzzer_volume,                                         \
      .data = (uint8_t *) &distance_monitor_config.buzzer_volume,              \
      .data_length = sizeof(distance_monitor_config.buzzer_volume),            \
      .nvm_key = 0x4004,                                                       \
      .value_min = 0,                                                          \
      .value_max = 1000,                                                       \
      .param_change_callback = app_logic_configure_buzzer_volume,              \
    },                                                                         \
    { .char_id = gattdb_range_mode,                                            \
      .data = (uint8_t *) &distance_monitor_config.range_mode,                 \
      .data_length = sizeof(distance_monitor_config.range_mode),               \
      .nvm_key = 0x4005,                                                       \
      .value_min = 1,                                                          \
      .value_max = 2,                                                          \
      .param_change_callback = app_logic_configure_distance_sensor_range_mode, \
    },                                                                         \
    { .char_id = gattdb_notification_status,                                   \
      .data = (uint8_t *) &distance_monitor_config.notification_status,        \
      .data_length = sizeof(distance_monitor_config.notification_status),      \
      .nvm_key = 0x4006,                                                       \
      .value_min = 0,                                                          \
      .value_max = 1,                                                          \
      .param_change_callback = app_event_handler_notification_status_changed,  \
    },                                                                         \
}                                                                              \

extern distance_monitor_config_data_t distance_monitor_config;
extern distance_monitor_feature_t distance_monitor_features[];

#ifdef __cplusplus
}
#endif

#endif /* APP_CONFIG_H_ */
