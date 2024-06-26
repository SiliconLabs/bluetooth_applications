/**
 * @file      tcfm.h
 *
 * @brief     Header file for TCFM test
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2020 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef TCFM_H_
#define TCFM_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "error.h"
#include "app.h"

error_e tcfm_process_init(tcfm_info_t *info);
void tcfm_process_run(void);
void tcfm_process_terminate(void);

extern void tcXm_configure_test_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* TCFM_H_ */
