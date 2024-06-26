/*! ----------------------------------------------------------------------------
 * @file    port_error.c
 * @brief   Project specific definitions and functions for error handling
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

#include "app.h"
#include "port_common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "em_gpio.h"

#define HAL_ERROR_LOG(...) hal_trace_debug_print(__VA_ARGS__)

void error_handler(int block, error_e err)
{
  app.lastErrorCode = err;

  /* Flash Error Led*/
  HAL_ERROR_LOG("\r\n Error: %d\r\n", err);
  while (block)
  {
    for (int i = err; i > 0; i--)
    {
      for (int j = 3; j > 0; j--)
      {
#if DEBUG
        wdt_refresh();
#endif

#if defined(HAL_IO_PORT_LED_ERROR) && defined(HAL_IO_PIN_LED_ERROR)
        GPIO_PinModeSet(HAL_IO_PORT_LED_ERROR,
                        HAL_IO_PIN_LED_ERROR,
                        gpioModePushPull,
                        0);
        usleep(100000);
        GPIO_PinModeSet(HAL_IO_PORT_LED_ERROR,
                        HAL_IO_PIN_LED_ERROR,
                        gpioModePushPull,
                        1);
#endif
        usleep(100000);
      }
      usleep(1000000);
    }
  }
}

static void port_error_assert(const char *file,
                              int line,
                              const char *function_name,
                              const char *expression,
                              error_e err)
{
  HAL_ERROR_LOG("ASSERT!");
  HAL_ERROR_LOG("\r\n File: %s", file);
  HAL_ERROR_LOG("\r\n Line: %d", line);
  HAL_ERROR_LOG("\r\n Function: %s", function_name);
  HAL_ERROR_LOG("\r\n Expression: %s", expression);

  usleep(1000000);   // printing might be asynch
  taskDISABLE_INTERRUPTS();
  error_handler(1, err);
}

void __assert_func(const char *file,
                   int line,
                   const char *function_name,
                   const char *expression)
{
  port_error_assert(file, line, function_name, expression, _ERR_General_Error);
  while (1) { // Calm the compiler that we won't return
  }
}

void assertEFM(const char *file, int line)
{
  port_error_assert(file, line, __FUNCTION__, "", _ERR_General_Error);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  port_error_assert(pcTaskName,
                    (int)xTask,
                    __FUNCTION__,
                    "",
                    _ERR_Stack_Overflow);
}

void vApplicationMallocFailedHook(void)
{
  port_error_assert(__FILE__, __LINE__, __FUNCTION__, "", _ERR_Malloc_Failed);
}
