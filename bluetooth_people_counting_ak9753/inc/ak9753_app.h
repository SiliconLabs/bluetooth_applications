/***************************************************************************//**
 * @file ak9753_app.h
 * @brief People counting module with AK9753 sensor header file.
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
#ifndef AK9753_APP_H_
#define AK9753_APP_H_

/***************************************************************************//**
 * @brief
 *    AK9753 application initialize.
 *
 ******************************************************************************/
void ak9753_app_init(void);

/***************************************************************************//**
 * @brief
 *    AK9753 application clear people entered so far.
 *
 ******************************************************************************/
void ak9753_app_clear_people_entered_so_far(void);

/***************************************************************************//**
 * @brief
 *    AK9753 application get people entered so far.
 *
 * @return
 *    People entered so far.
 *
 ******************************************************************************/
uint16_t ak9753_app_get_people_entered_so_far(void);

/***************************************************************************//**
 * @brief
 *    AK9753 application clear people count.
 *
 * @param[in] value
 *    People count.
 *
 ******************************************************************************/
void ak9753_app_clear_people_count(void);

/***************************************************************************//**
 * @brief
 *    AK9753 application get people count.
 *
 * @return
 *    People count.
 *
 ******************************************************************************/
uint16_t ak9753_app_get_people_count(void);

/***************************************************************************//**
 * @brief
 *    AK7953 Process Action.
 *
 ******************************************************************************/
void ak9753_app_process_action(void);

#endif /* AK9753_APP_H_ */
