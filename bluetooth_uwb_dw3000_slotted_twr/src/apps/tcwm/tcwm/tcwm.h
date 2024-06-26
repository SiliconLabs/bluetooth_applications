/**
 * @file      tcwm.h
 *
 * @brief     Header file for prototype of continuous wave test
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef __TCWM_H_
#define __TCWM_H_    1

#ifdef __cplusplus
extern "C" {
#endif

void tcwm_process_init(void);
void tcwm_process_run(void);
void tcwm_process_terminate(void);

void tcXm_configure_test_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_INC_TCWM_H_ */
