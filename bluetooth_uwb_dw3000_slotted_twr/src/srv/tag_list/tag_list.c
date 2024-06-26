/**
 * @file    tag_list.c
 * @brief    functions to manage
 *
 * @author Decawave
 *
 * @attention Copyright 2017 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */
#include <string.h>
#include "app.h"
#include "port_common.h"
#include "tag_list.h"
// ----------------------------------------------------------------------------
// There 2 Tag's lists in the Node:
// Discovered Tags List (DList) and Known Tags List (KList)
//
// DList[MAX_DISCOVERED_TAG_LIST_SIZE] is temporary list for harvesting of tags
//   from the air.
// KList[MAX_KNOWN_TAG_LIST_SIZE] is permanent list and a part of NVM.
//
static uint64_t        DList[MAX_DISCOVERED_TAG_LIST_SIZE];

/* array implementation of knownTagList: array is faster than linked list and it
 *   easier for "del"
 *
 * init_knownTagList
 * get_knownTagList
 * get_knownTagList_Size
 * get_tag64_from_knownTagList
 * get_tag16_from_knownTagList
 * get_free_slot_from_knownTagList
 * add_tag_to_knownTagList
 * del_tag16_from_knownTagList
 * del_tag64_from_knownTagList
 *
 * For PC<->NODE communication protocol see cmd_fn.c
 *
 * PC->NODE
 * "getKList" : Node returns the knownTagList (long string)."<SLOT16> <ADDR16>
 *   <ADDR64>"
 * "getDList" : Node returns the knownTagList (long string)."<SLOT16> <ADDR16>
 *   <ADDR64>"
 * "addtag <ADDR64> <ADDR16> <some params>"
 *
 * NODE->PC
 * JSON format strings:
 * "JSxxxx{"New Tag": <ADDR64>"}
 * "JSxxxx{"TWR": <ADDR16> ....}"
 *
 * */

/* @brief
 * @return the pointer to the first element of knownTagList
 * */
tag_addr_slot_t *get_knownTagList(void)
{
  return (app.pConfig->knownTagList);
}

/* @brief
 *         knownTagList can have gaps in the middle
 * @return the numeber of elements in the knownTagList
 * */
uint16_t get_knownTagList_size(void)
{
  uint16_t         size = 0;
  tag_addr_slot_t *klist = get_knownTagList();

  // KList can be with gaps, so need to scan it whole
  for (int i = 0; i < MAX_KNOWN_TAG_LIST_SIZE; i++)
  {
    if (klist->slot != (uint16_t)(0)) {
      size++;
    }

    klist++;
  }

  return (size);
}

/*
 * */
void init_knownTagList(void)
{
  memset(app.pConfig->knownTagList, 0, sizeof(app.pConfig->knownTagList));
}

/* brief
 * returns the address of the known tag if it is in known_list;
 * otherwise returns NULL.
 *
 * */
tag_addr_slot_t *
get_tag64_from_knownTagList(uint64_t addr64)
{
  int i;
  int size = sizeof(app.pConfig->knownTagList)
             / sizeof(app.pConfig->knownTagList[0]);

  tag_addr_slot_t *klist = get_knownTagList();

  for (i = 0; i < size; i++)
  {
    if ((klist[i].addr64 == addr64) && (klist[i].slot != 0)) {
      return &klist[i];
    }
  }
  return NULL;
}

/*
 * */
tag_addr_slot_t *
get_tag16_from_knownTagList(uint16_t addr16)
{
  int i;
  int size = sizeof(app.pConfig->knownTagList)
             / sizeof(app.pConfig->knownTagList[0]);

  tag_addr_slot_t *klist = get_knownTagList();

  for (i = 0; i < size; i++)
  {
    if ((klist[i].addr16 == addr16) && (klist[i].slot != 0)) {
      return &klist[i];
    }
  }
  return NULL;
}

/*
 * return [1..(size+1)] for free slot
 *
 * return 0 if there no free slots found
 *
 * */
uint16_t get_free_slot_from_knownTagList(void)
{
  int16_t i;
  int size = sizeof(app.pConfig->knownTagList)
             / sizeof(app.pConfig->knownTagList[0]);

  tag_addr_slot_t *klist = get_knownTagList();

  for (i = 0; i < size; i++)
  {
    if (klist[i].slot == (uint16_t)(0)) {
      return (i + 1);
    }
  }
  return (0);
}

/*
 * @brief Checks and adds the tag to the list, if it's not in the list yet.
 *
 *     returns the pointer to the tag in the list;
 *     returns NULL if the list is full;
 *
 * */
tag_addr_slot_t *add_tag_to_knownTagList(uint64_t addr64,
                                         uint16_t addr16,
                                         uint16_t fast,
                                         uint16_t slow,
                                         uint16_t mode)
{
  int16_t slot;
  tag_addr_slot_t *tag, *klist;

  int size = sizeof(app.pConfig->knownTagList)
             / sizeof(app.pConfig->knownTagList[0]);

  tag = get_tag64_from_knownTagList(addr64);

  if (!tag) {
    klist = get_knownTagList();

    slot = get_free_slot_from_knownTagList();

    if ((slot > 0) && (slot <= size)) {
      /* Duplicate 16-bit addresses are not allowed
       * use any next available address which is not
       * in known addresses space instead
       * */
      while (get_tag16_from_knownTagList(addr16))
      {
        addr16++;
      }

      /* add tag to KList */
      tag = &klist[slot - 1];         // slot=[1..MAX_KNOWN_TAG_LIST_SIZE]
      tag->slot = slot;               // klist[0].slot = 1; klist[1].slot = 2,
                                      //   etc.
      tag->addr16 = addr16;
      tag->addr64 = addr64;

      /* use default parameters for all tags : used in automatic adding of all
       *   tags : "d2k" command */
      tag->multFast = fast;
      tag->multSlow = slow;
      tag->mode = mode;
    }
  }

  return (tag);
}

/*
 * */
void del_tag16_from_knownTagList(uint16_t addr16)
{
  tag_addr_slot_t *p;

  p = get_tag16_from_knownTagList(addr16);

  if (p) {
    memset(p, 0, sizeof(tag_addr_slot_t));
  }
}

/*
 * */
void del_tag64_from_knownTagList(uint64_t addr64)
{
  tag_addr_slot_t *p;

  p = get_tag64_from_knownTagList(addr64);

  if (p) {
    memset(p, 0, sizeof(tag_addr_slot_t));
  }
}

// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// for harvesting Tags offline : we have limited space : use DList
//
uint16_t getDList_size(void)
{
  uint16_t    len = 0;
  for (size_t i = 0; i < sizeof(DList) / sizeof(DList[0]); i++)
  {
    if (!DList[i]) {
      break;
    }
    len++;
  }
  return (len);
}

uint64_t * getDList(void)
{
  return (DList);
}

/* @brief
 * return 1 : tag successfully added to discovered tag list / no space left in
 *   discovered tag list
 * return 0 : already in the list
 * */
int addTagToDList(uint64_t addr64)
{
  int ret = 1;

  for (size_t i = 0; i < sizeof(DList) / sizeof(DList[0]); i++)
  {
    if (DList[i] == addr64) {
      ret = 0;
      break;
    }

    if (!DList[i]) {
      DList[i] = addr64;
      ret = 1;
      break;
    }
  }
  return(ret);
}

void initDList(void)
{
  memset(DList, 0, sizeof(DList));
}
