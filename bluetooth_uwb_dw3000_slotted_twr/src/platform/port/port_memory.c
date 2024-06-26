/*! ----------------------------------------------------------------------------
 * @file    port_memory.c
 * @brief   Project specific definitions and functions for dynamic memory
 *   handling
 *
 * @author  Decawave
 *
 * @attention
 *
 * Copyright 2018 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

/*
 * The standard syscall malloc/free used in sscanf/sprintf.
 * We want them to be replaced with FreeRTOS's implementation.
 *
 * This leads that the memory allocation will be managed by FreeRTOS heap4
 *   memory.
 * */
void * _calloc_r(struct _reent *re, size_t num, size_t size)
{
  (void)re;

  void *ptr = pvPortMalloc(num * size);
  if (ptr) {
    memset(ptr, 0, num * size);
  }
  return ptr;
}

void * _malloc_r(struct _reent *re, size_t size)
{
  (void)re;
  return pvPortMalloc(size);
}

void _free_r(struct _reent *re, void *ptr)
{
  (void)re;
  vPortFree(ptr);
}
