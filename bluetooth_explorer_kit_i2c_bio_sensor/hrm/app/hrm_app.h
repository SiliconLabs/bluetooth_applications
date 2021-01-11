#ifndef HRM_APP_H__
#define HRM_APP_H__

#define HRM_DEMO_NAME "si117xHRM Demo"
#define HRM_DEMO_VERSION "1.0.0"

/*  External Events  */
#define MAXM86161_IRQ_EVENT     0x1
#define BTN0_IRQ_EVENT          0x2

typedef enum HRMSpO2State
{
   HRM_STATE_IDLE,
   HRM_STATE_NOSIGNAL,
   HRM_STATE_ACQUIRING,
   HRM_STATE_ACTIVE,
   HRM_STATE_INVALID
}hrm_spo2_state_t;

/**************************************************************************//**
 * @brief Initialize the HRM application.
 *****************************************************************************/
void hrm_init_app(void);

void hrm_process_event(uint32_t event_flags);

void hrm_loop(void);
/**************************************************************************//**
 * @brief This function returns the current heart rate.
 *****************************************************************************/
int16_t hrm_get_heart_rate(void);

/**************************************************************************//**
 * @brief This function returns the current finger contact status.
 *****************************************************************************/
bool hrm_get_status(void);

int16_t hrm_get_spo2(void);

#endif    //HRM_APP_H__
