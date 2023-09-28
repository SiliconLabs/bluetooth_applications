/***************************************************************************//**
 * @file app_config.h
 * @brief Application Configuration
 * @version 1.0.0
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
#include "mikroe_bma400_i2c_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Default BLE passkey for bonding
#define MD_PASSKEY                        123456

// Application supported features
#define MD_BLE_FEATURE_LENGTH             4

// Application time out configuration
#define MD_LAST_REQ_TIMEOUT_MS            20000

// Interrupt pin of accelerometer sensor BMA400.
#define BMA400_INT_PIN                    MIKROE_BMA400_INT1_PIN

// Sample threshold of application.
#define MD_SAMPLES_COUNT_THRESHOLD        10

// Movement counter threshold of application.
#define MD_MOVEMENT_COUNT_THRESHOLD       3

/***************************************************************************//**
 * @brief
 *    Typedef for holding application configuration parameters.
 ******************************************************************************/
typedef struct {
  uint8_t movement_threshold; ///<  Movement threshold value in mg.
  uint16_t wake_up_time_period; ///< Wake-Up time period value in ms.
  uint16_t notification_time; ///<  Notification time in ms.
  uint16_t notification_break_time; ///<  Notification break time in ms.
} md_config_data_t;

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
} md_feature_t;

// Movement detection runtime data-set default values
#define MD_RUNTIME_DEFAULT_DATASET { \
    .movement_counter = 0,           \
    .sample_counter = 0,             \
    .movement_flag = false,          \
}                                    \

// Movement detection application default configuration parameters
#define MD_DEFAULT_CONFIG          { \
    .movement_threshold = 10,        \
    .wake_up_time_period = 100,      \
    .notification_time = 5000,       \
    .notification_break_time = 5000, \
}                                    \

// Movement detection application BLE features, see md_feature_t
#define MD_FEATURES                {                            \
    { .char_id = gattdb_movement_threshold,                     \
      .data = (uint8_t *) &md_config.movement_threshold,        \
      .data_length = sizeof(md_config.movement_threshold),      \
      .nvm_key = 0x4001,                                        \
      .value_min = 0,                                           \
      .value_max = 255,                                         \
    },                                                          \
    { .char_id = gattdb_wake_up_time_period,                    \
      .data = (uint8_t *) &md_config.wake_up_time_period,       \
      .data_length = sizeof(md_config.wake_up_time_period),     \
      .nvm_key = 0x4002,                                        \
      .value_min = 100,                                         \
      .value_max = 65535,                                       \
    },                                                          \
    { .char_id = gattdb_notification_time,                      \
      .data = (uint8_t *) &md_config.notification_time,         \
      .data_length = sizeof(md_config.notification_time),       \
      .nvm_key = 0x4003,                                        \
      .value_min = 0,                                           \
      .value_max = 65535,                                       \
    },                                                          \
    { .char_id = gattdb_notification_break_time,                \
      .data = (uint8_t *) &md_config.notification_break_time,   \
      .data_length = sizeof(md_config.notification_break_time), \
      .nvm_key = 0x4004,                                        \
      .value_min = 0,                                           \
      .value_max = 65535,                                       \
    },                                                          \
}                                                               \

extern md_config_data_t md_config;
extern md_feature_t md_features[];

#ifdef __cplusplus
}
#endif

#endif /* APP_CONFIG_H_ */
