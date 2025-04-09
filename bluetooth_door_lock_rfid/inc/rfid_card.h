/***************************************************************************//**
 * @file rfid_card.h
 * @brief RFID CARD app
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
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

#ifndef RFID_CARD_H_
#define RFID_CARD_H_

#define RFID_CARD_TAG_COUNT 10

/***************************************************************************//**
 * @brief
 *    RFID CARD init function.
 *
 ******************************************************************************/
sl_status_t rfid_card_init(void);

/***************************************************************************//**
 * @brief
 *    RFID card check free slot function
 *
 * @param[in] slot
 *    Location of the card on database want to check.
 * @param[out] is_free
 *    Output is free value.
 *
 ******************************************************************************/
sl_status_t rfid_card_check_free_slot(uint8_t slot, bool *is_free);

/***************************************************************************//**
 * @brief
 *    RFID card add function
 *
 * @param[in] id_tag
 *    Array that contain the card data to add.
 *
 ******************************************************************************/
sl_status_t rfid_card_add(uint8_t *id_tag);

/***************************************************************************//**
 * @brief
 *    RFID card remove function with slow
 *
 * @param[in] slot
 *    Slot of the card to remove.
 *
 ******************************************************************************/
sl_status_t rfid_card_remove_slot(uint8_t slot);

/***************************************************************************//**
 * @brief
 *    RFID card remove function with slow
 *
 * @param[in] slot
 *    Slot of the card to get.
 *
 * @param[out] id_tag
 *    Data of the card output.
 *
 ******************************************************************************/
sl_status_t rfid_card_get(uint8_t slot, uint8_t *id_tag);

/***************************************************************************//**
 * @brief
 *    RFID card get number of cards in the database.
 *
 * @param[out] number_of_cards
 *    Number of cards in the database.
 *
 ******************************************************************************/
sl_status_t rfid_card_get_number_of_cards(uint8_t *number_of_cards);

/***************************************************************************//**
 * @brief
 *    RFID card find the card in the database.
 *
 * @param[in] id_tag
 *     Data of the card intput.
 *
 ******************************************************************************/
bool rfid_card_find(uint8_t *id_tag);

#endif /* RFID_CARD_H */
