/***************************************************************************//**
 * @file rfid_card.c
 * @brief RFID Card application code
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
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "rfid_card.h"

#define RFID_CARD_NVM_ADDRESS 0x4001

typedef struct nvm_saved_data {
  uint8_t id_tag[6];
}nvm_saved_data_t;

static nvm_saved_data_t nvm_saved_data[RFID_CARD_TAG_COUNT];
static uint8_t card_count = 0;

#define card_slot_is_empty(slot)               \
  ((nvm_saved_data[slot].id_tag[0] == 0x00) && \
   (nvm_saved_data[slot].id_tag[1] == 0x00) && \
   (nvm_saved_data[slot].id_tag[2] == 0x00) && \
   (nvm_saved_data[slot].id_tag[3] == 0x00) && \
   (nvm_saved_data[slot].id_tag[4] == 0x00) && \
   (nvm_saved_data[slot].id_tag[5] == 0x00))

/***************************************************************************//**
 * RFID CARD init Function.
 ******************************************************************************/
sl_status_t rfid_card_init(void)
{
  size_t value_length;
  sl_status_t sc = SL_STATUS_OK;

  for (int i = 0; i < RFID_CARD_TAG_COUNT; i++) {
    sc = sl_bt_nvm_load(RFID_CARD_NVM_ADDRESS + i,
                        6,
                        &value_length,
                        (uint8_t *)&nvm_saved_data[i]);
    app_log("nvm :%d \r\n", value_length);
    if ((SL_STATUS_OK == sc)
        && (value_length == 6)
        && !card_slot_is_empty(i)) {
      card_count++;
    } else {
      nvm_saved_data[i].id_tag[0] = 0x00;
      nvm_saved_data[i].id_tag[1] = 0x00;
      nvm_saved_data[i].id_tag[2] = 0x00;
      nvm_saved_data[i].id_tag[3] = 0x00;
      nvm_saved_data[i].id_tag[4] = 0x00;
      nvm_saved_data[i].id_tag[5] = 0x00;
    }
  }
  app_log("nvm: card count: %d \r\n", card_count);
  return sc;
}

/***************************************************************************//**
 * RFID CARD check free slot Function.
 ******************************************************************************/
sl_status_t rfid_card_check_free_slot(uint8_t slot, bool *is_free)
{
  if (slot >= RFID_CARD_TAG_COUNT) {
    return SL_STATUS_INVALID_INDEX;
  }

  if (card_slot_is_empty(slot)) {
    *is_free = true;
  } else {
    *is_free = false;
  }

  return SL_STATUS_OK;
}

sl_status_t rfid_card_add(uint8_t *id_tag)
{
  if (id_tag == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  if (rfid_card_find(id_tag)) {
    return SL_STATUS_ALREADY_EXISTS;
  }

  for (uint8_t i = 0; i < RFID_CARD_TAG_COUNT; i++) {
    if (card_slot_is_empty(i)) {
      memcpy(nvm_saved_data[i].id_tag, id_tag, 6);
      sl_bt_nvm_save(RFID_CARD_NVM_ADDRESS + i, 6,
                     (uint8_t *)&nvm_saved_data[i]);
      app_log("Card has been inserted to location %d\r\n", i);
      card_count++;
      return SL_STATUS_OK;
    }
  }
  return SL_STATUS_FULL;
}

/***************************************************************************//**
 * RFID CARD remove Function.
 ******************************************************************************/
sl_status_t rfid_card_remove_slot(uint8_t slot)
{
  if (slot >= RFID_CARD_TAG_COUNT) {
    return SL_STATUS_INVALID_INDEX;
  }

  if (card_slot_is_empty(slot)) {
    return SL_STATUS_NOT_FOUND;
  }

  memset(nvm_saved_data[slot].id_tag, 0, 6);
  card_count--;

  app_log("Card has been removed from location %d \r\n", slot);
  return sl_bt_nvm_save(RFID_CARD_NVM_ADDRESS + slot,
                        6,
                        (uint8_t *)&nvm_saved_data[slot]);
}

/***************************************************************************//**
 * RFID CARD get Function.
 ******************************************************************************/
sl_status_t rfid_card_get(uint8_t slot, uint8_t *id_tag)
{
  if (slot >= RFID_CARD_TAG_COUNT) {
    return SL_STATUS_INVALID_INDEX;
  }
  if (id_tag == NULL) {
    return SL_STATUS_NULL_POINTER;
  }
  if (card_slot_is_empty(slot)) {
    return SL_STATUS_NOT_FOUND;
  }
  memcpy(id_tag, nvm_saved_data[slot].id_tag, 6);
  return SL_STATUS_OK;
}

/***************************************************************************//**
 * RFID CARD find Function.
 ******************************************************************************/
bool rfid_card_find(uint8_t *id_tag)
{
  uint8_t i;

  if (NULL != id_tag) {
    for (i = 0; i < RFID_CARD_TAG_COUNT; i++) {
      if (0 == memcmp(nvm_saved_data[i].id_tag, id_tag, 6)) {
        return true;
      }
    }
  }
  return false;
}

/***************************************************************************//**
 * RFID CARD get number of cards Function.
 ******************************************************************************/
sl_status_t rfid_card_get_number_of_cards(uint8_t *number_of_cards)
{
  if (number_of_cards == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  *number_of_cards = card_count;

  return SL_STATUS_OK;
}
