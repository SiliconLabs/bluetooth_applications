/*
 * @file   task_trilat.c
 * @brief
 *
 * @author Decawave
 *
 * @attention Copyright 2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include "port_common.h"
#include "node.h"
#include "task_node.h"
#include "dwm-math.h"
#include "dwm-math.h"

/** @defgroup Trilateration
 * @brief Parameters change section : allowed only in app.mode = mIdle
 * @{
 */

// -----------------------------------------------------------------------------
// extern functions to report output data
extern int trilat_solve(vec3d_t *bn_pos,
                        double *meas,
                        int cnt,
                        vec3d_t *pos_est,
                        uint8_t *qf);
extern error_e port_tx_msg(uint8_t *str, int len);

#define PFIL(x) (int)(x / 1000.0)
#define PFIR(x) (int)(((float)x - 1000.0 * (int)(x / 1000)))

// -----------------------------------------------------------------------------

static struct trilat_buf_s {
  struct tag_res_s {
    uint16_t addr16;
    uint32_t resTime_us;         // reception time of the end of the Final from
                                 //   the Tag wrt node's SuperFrame start,
                                 //   microseconds
    double   dist_mm;
    vec3d_t  bn_pos;
  } tag_res[6];

  int cnt;
} ttmp_buf[2] = { 0 };

static struct trilat_buf_s *p_ttmp = &ttmp_buf[0];
static struct trilat_buf_s *p_ttmp_calc = &ttmp_buf[1];

/* Design:
 * Assuming the SF (100ms) is enough time to calculate the position in
 *   low-priority TrilatTask().
 *
 * Rx of the Final:
 *  - The Node is performing calculation of the tag position in the CalcTask;
 *  - results from all ranges during SF stored in the ttmp_buf by calling
 *   trilat_extension_put(result_t *);
 *  - max of 6 ranges can be saved;
 *
 * On RTC:
 *  - The cb called trilat_extension_SF_cb()
 *    - The ttmp_buf copied to the tcalc_buf, then ttmp_buf is cleared and ready
 *   for next SF ranges.
 *    - The signal is also sent
 *
 * Trilat_Task:
 *  - receiving the signal and the tcalc_buf to perform the location
 *  - uses statically known positions of the FixedElement;
 *
 * */

/* @brief save distances to FixedElements into the current p_ttmp buffer
 *
 * */
void trilat_extension_node_put(result_t *pRes)
{
  const int size = sizeof(p_ttmp->tag_res) / sizeof(p_ttmp->tag_res[0]);

  struct tag_res_s *p = &p_ttmp->tag_res[p_ttmp->cnt];

  if (p_ttmp->cnt < size - 1) {
    p->addr16 = pRes->addr16;
    p->resTime_us = pRes->resTime_us;
    p->dist_mm = pRes->dist_cm * 10.0;

    p->bn_pos.x = (double)pRes->acc_x;         // Fixed unit sending its X
                                               //   location instead of accel
                                               //   data, mm
    p->bn_pos.y = (double)pRes->acc_y;         // Fixed unit sending its Y
                                               //   location instead of accel
                                               //   data, mm
    p->bn_pos.z = (double)pRes->acc_z;         // Fixed unit sending its Z
                                               //   location instead of accel
                                               //   data, mm

    p_ttmp->cnt++;
  }
}

/* @brief Trilateration SuperFrame callback
 *        Called every superframe.
 *        Send stored via previous SF ranges in the p_ttmp to the low-priority
 *   trilatTask fn()
 *        clear
 *  */
void trilat_SF_cb(result_t *pRes)
{
  (void) pRes;
  p_ttmp_calc = p_ttmp;
  p_ttmp = (p_ttmp == &ttmp_buf[0])?(&ttmp_buf[1]):(&ttmp_buf[0]);
  p_ttmp->cnt = 0;

  osThreadFlagsSet(app.trilatTask.Handle, app.trilatTask.Signal);
}

/*
 * @brief Trilat Node variant implementation
 *          This is a low priority task, which is awaking every superframe
 *          to trilaterate position wrt fixed infrastructure elements
 *          from ranges, measured during the previous SF
 * */
static void TrilatTask(void const *arg)
{
  node_info_t *pNodeInfo;
  char        str[256];

  while (!(pNodeInfo = getNodeInfoPtr()))
  {
    osDelay(5);
  }

  const osMutexAttr_t thread_mutex_attr = { .attr_bits = osMutexPrioInherit };
  app.trilatTask.MutexId = osMutexNew(&thread_mutex_attr);

  do {
    osMutexRelease(app.trilatTask.MutexId);

    osThreadFlagsWait(app.trilatTask.Signal, osFlagsWaitAny, osWaitForever);

    osMutexAcquire(app.trilatTask.MutexId, 0);

    sprintf(str, "res: ");

    for (int i = 0; i < (int)p_ttmp_calc->cnt; i++)
    {
      struct tag_res_s *p = &p_ttmp_calc->tag_res[i];

      sprintf(&str[strlen(str)],
              "%04x[%2d.%-3d][%2d.%-2d,%2d.%-2d,%2d.%-2d]=%2d.%-2d ",
              p->addr16,
              PFIL(p->resTime_us),
              PFIR(p->resTime_us),
              PFIL(p->bn_pos.x),
              PFIR(p->bn_pos.x),
              PFIL(p->bn_pos.y),
              PFIR(p->bn_pos.y),
              PFIL(p->bn_pos.z),
              PFIR(p->bn_pos.z),
              PFIL(p->dist_mm),
              PFIR(p->dist_mm));
    }

    /* Assuming the time required for Trilateration is less than
     * Superframe time, thus we are sure about integrity of the data in the
     * p_ttmp_calc buffer.
     * */
    if (p_ttmp_calc->cnt >= 3) {
      vec3d_t pos_est;
      uint8_t qf;
      vec3d_t bn_pos[p_ttmp_calc->cnt];
      double  dist[p_ttmp_calc->cnt];

      for ( int i = 0; i < p_ttmp_calc->cnt; i++)
      {
        bn_pos[i].x = p_ttmp_calc->tag_res[i].bn_pos.x / 1000.0;
        bn_pos[i].y = p_ttmp_calc->tag_res[i].bn_pos.y / 1000.0;
        bn_pos[i].z = p_ttmp_calc->tag_res[i].bn_pos.z / 1000.0;

        dist[i] = p_ttmp_calc->tag_res[i].dist_mm / 1000.0;
      }

      int ret = trilat_solve(bn_pos,
                             dist,
                             p_ttmp_calc->cnt,
                             &pos_est,
                             &qf);

      if (ret >= 0) {
        sprintf(&str[strlen(str)], "qf:%3d est[%2d.%-2d,%2d.%-2d,%2d.%-2d] ",
                (int) qf,
                PFIL((int)(1000 * pos_est.x)), PFIR((int)(1000 * pos_est.x)),
                PFIL((int)(1000 * pos_est.y)), PFIR((int)(1000 * pos_est.y)),
                PFIL((int)(1000 * pos_est.z)), PFIR((int)(1000 * pos_est.z)));
      } else {
        sprintf(&str[strlen(str)], "no_est, errno: %d ", ret);
      }
    } else {
      sprintf(&str[strlen(str)], "no_trilat n:%2d ", (int)p_ttmp_calc->cnt);
    }

    sprintf(&str[strlen(str)], "\r\n");

    port_tx_msg((uint8_t *)str, strlen(str));
  }while (1);

  UNUSED(arg);
}

// -----------------------------------------------------------------------------

/* @brief Terminate all tasks and timers related to Trilateration functionality,
 *   if any
 *        DW3000's RX and IRQ shall be switched off before task termination,
 *        that IRQ will not produce unexpected Signal
 * */
void trilat_terminate(void)
{
  node_terminate();

  TERMINATE_STD_TASK(app.trilatTask);
}

/* @fn         trilat_helper
 * @brief      this is a service function which starts the
 *             TWR Node functionality plus Trilateration service
 *             Note: the previous instance of TWR shall be killed
 *             with node_terminate_tasks();
 *
 *             Note: the node_process_init() will allocate the memory of
 *   sizeof(node_info_t)
 *                      from the caller's task stack, see _malloc_r() !
 *
 * */
void trilat_helper(void const *argument)
{
  CREATE_NEW_TASK(TrilatTask,
                  NULL,
                  "trilatTask",
                  512,
                  PRIO_TrilatTask,
                  &app.trilatTask.Handle);
  app.trilatTask.Signal = 1;

  if (app.trilatTask.Handle == NULL) {
    error_handler(1, _ERR_Create_Task_Bad);
  }

  node_helper(argument);
}
