/***************************************************************************//**
 * @file fingerprint_app.h
 * @brief fingerprint doorlock app header file.
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
#ifndef FINGERPRINT_APP_H_
#define FINGERPRINT_APP_H_

#include <stdbool.h>
#include <stdint.h>
#include "sl_status.h"

/***************************************************************************//**
 * @brief Initial fingerprint function.
 *
 ******************************************************************************/
void fingerprint_init(void);

/***************************************************************************//**
 * @brief : Add new fingerprint function.
 *
 * @param fp_index  index of new fingerprint.
 * @return sl_status_t  status of funtion.
 *
 ******************************************************************************/
sl_status_t fingerprint_add(uint8_t index);

/***************************************************************************//**
 * @brief Remove fingerprint function.
 *
 * @param remove_index  index of fingerprint that need to remove.
 * @return sl_status_t  status of function.
 *
 ******************************************************************************/
sl_status_t fingerprint_remove(uint8_t index);

/***************************************************************************//**
 * @brief Check the slot is free or not.
 *
 * @param slot : slot.
 * @param is_free : free or not.
 * @return sl_status_t status of function.
 ******************************************************************************/
sl_status_t fingerprint_check_slot(uint8_t slot, bool *is_free);

/***************************************************************************//**
 * @brief Get the number of authorized fingerprints function.
 *
 * @return uint8_t number of authorized fingerprints.
 *
 ******************************************************************************/
uint8_t fingerprint_get_num_of_fps(void);

/***************************************************************************//**
 * @brief Compare new fingerprint with the authorized fingerprints.
 *
 * @return sl_status_t status of fucntion.
 *
 ******************************************************************************/
sl_status_t fingerprint_compare(void);

/***************************************************************************//**
 * @brief Get free slot function.
 *
 * @param free_slot free slot.
 * @return sl_status_t status of function.
 *
 ******************************************************************************/
sl_status_t fingerprint_get_free_slot(uint8_t *free_slot);

/***************************************************************************//**
 * @brief Get fingerprint index when compare fingerprint with authorized
 *   fingerprints is pass.
 *
 * @return uint8_t : index of authorized fingerprint.
 *
 ******************************************************************************/
uint8_t fingerprint_get_authorized_index(void);

#endif /* FINGERPRINT_APP_H_ */
