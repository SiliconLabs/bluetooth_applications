/***************************************************************************//**
 * @file hand_signal_display.h
 * @brief hand signal display header file.
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

#ifndef HAND_SIGNAL_DISPLAY_H_
#define HAND_SIGNAL_DISPLAY_H_

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @brief
 *    OLED initialize.
 *
 ******************************************************************************/
void hand_signal_oled_init(void);

/***************************************************************************//**
 * @brief
 *    Application Display initialize.
 *
 ******************************************************************************/
void hand_signal_display_init(void);

/***************************************************************************//**
 * Show error when hand_signal_recognition_init error
 ******************************************************************************/
void hand_signal_display_error(void);

/***************************************************************************//**
 * @brief
 *    Application Hand Signal Display.
 *
 * @param[in] result
 *    Classification result, this is number >= 0.
 * @param[len] score
 *    The score of the result. This is number represents
 *    the confidence of the result classification.
 *
 * @details
 *    This function displays the hand signal in OLED.
 *
 ******************************************************************************/
void hand_signal_display_update(uint8_t result, uint8_t score);

#ifdef __cplusplus
}
#endif

#endif /* HAND_SIGNAL_DISPLAY_H_ */
