/**
 * @file    tag_list.h
 *
 * @brief
 *
 * @author Decawave
 *
 * @attention Copyright 2017 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef __INC_TAG_LIST_H__
#define __INC_TAG_LIST_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

// #include <stdint.h>

// maximum is limited by NUM_SLOTS; FCONFIG_SIZE and available memory size, see
//   default_config.h
#define MAX_KNOWN_TAG_LIST_SIZE                (20)
#define MAX_DISCOVERED_TAG_LIST_SIZE           (20)

/* Tag list */

typedef struct
{
  uint16_t    slot;
  union {
    uint8_t        addrShort[2];
    uint16_t    addr16;
  };

  union    {
    uint8_t        addrLong[8];
    uint64_t     addr64;
  };

  uint16_t    multFast;
  uint16_t    multSlow;
  uint16_t    mode;                             // IMU = bit 0

  union    {
    uint8_t        req;
    uint8_t        reqUpdatePending : 1;        // request to update Tag's
                                                //   configuration during range
                                                //   phase
  };
}__attribute__((packed))
tag_addr_slot_t;

tag_addr_slot_t *get_tag16_from_knownTagList(uint16_t addr16);
tag_addr_slot_t *get_tag64_from_knownTagList(uint64_t addr64);
tag_addr_slot_t *add_tag_to_knownTagList(uint64_t addr64,
                                         uint16_t addr16,
                                         uint16_t fast,
                                         uint16_t slow,
                                         uint16_t mode);
void del_tag64_from_knownTagList(uint64_t addr64);
void del_tag16_from_knownTagList(uint16_t addr16);

int      addTagToDList(uint64_t addr64);

uint16_t getDList_size(void);
uint64_t *getDList(void);

void init_knownTagList(void);
tag_addr_slot_t *get_knownTagList(void);
uint16_t get_knownTagList_size(void);

void initDList(void);

#ifdef __cplusplus
}
#endif

#endif /* __INC_TAG_LIST_H_ */
