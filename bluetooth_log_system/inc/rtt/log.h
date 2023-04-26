#ifndef LOG_H
#define LOG_H

#include "SEGGER_RTT.h"

#if (LOCAL_LOG_OFF == 1)
#define GK_LOGD(_tag_, _prefix_, ...)
#define GK_LOGI(_tag_, _prefix_, ...)
#define GK_LOGW(_tag_, _prefix_, ...)
#define GK_LOGE(_tag_, _prefix_, ...)
#define GK_LOGV(_tag_, _prefix_, ...)
#define GK_UINT8_ARRAY_DUMP(array_base, array_size)
#define GK_ADDRESSING()
#define GK_CHECK(tag__, x)
#define LOG_ASSERT(x)
#define LOG(...)
#define LOGN()
#define UINT8_ARRAY_DUMP(array_base, array_size)
#define LOG_DIRECT_ERR(_prefix_, ...)
#define LOGE(_prefix_, ...)
#define LOGW(_prefix_, ...)
#define LOGI(_prefix_, ...)
#define LOGD(_prefix_, ...)
#define LOGV(_prefix_, ...)
#define ERROR_ADDRESSING()
#define INIT_LOG()
#define EVT_LOG_C(_evt_name_, _attached_, ...)
#define EVT_LOG_I(_evt_name_, _attached_, ...)
#define EVT_LOG_V(_evt_name_, _attached_, ...)
#define SE_CALL(x) \
  do {             \
    x;             \
  } while (0)
#define GENERAL_ERR_CHECK(x)
#else

/* General part - High level definitions */
#ifndef LOG_LEVEL
#define LOG_LEVEL                       LVL_VERBOSE
#endif

#define NO_LOG                          0
#define LVL_ERROR                       1
#define LVL_WARNING                     2
#define LVL_INFO                        3
#define LVL_DEBUG                       4
#define LVL_VERBOSE                     5

#ifndef GENERAL_SUCCESS
#define GENERAL_SUCCESS                 0
#endif

// The expected format of the logging message is
// [LOGGING_LEVEL]<MODULE_NAME>: xxxx...
#define ASSERT_FLAG                     "[ASSERT] "
#define ERROR_FLAG                      "[E] "
#define WARNING_FLAG                    "[W] "
#define INFO_FLAG                       "[I] "
#define DEBUG_FLAG                      "[D] "
#define VERBOSE_FLAG                    "[V] "

#ifndef SUB_MODULE_NAME
#define SUB_MODULE_NAME                 ""
#endif

#define END_OF_LOG_HEADER               ":> "

#define LOG_ASSERT_PREFIX                                 \
  RTT_CTRL_TEXT_BRIGHT_RED ASSERT_FLAG "[Assert-assert] " \
  SUB_MODULE_NAME END_OF_LOG_HEADER
#define LOG_ERROR_PREFIX              \
  RTT_CTRL_TEXT_BRIGHT_RED ERROR_FLAG \
  SUB_MODULE_NAME END_OF_LOG_HEADER
#define LOG_WARNING_PREFIX                 \
  RTT_CTRL_TEXT_BRIGHT_YELLOW WARNING_FLAG \
  SUB_MODULE_NAME END_OF_LOG_HEADER
#define LOG_INFO_PREFIX               \
  RTT_CTRL_TEXT_BRIGHT_CYAN INFO_FLAG \
  SUB_MODULE_NAME END_OF_LOG_HEADER
#define LOG_DEBUG_PREFIX                \
  RTT_CTRL_TEXT_BRIGHT_GREEN DEBUG_FLAG \
  SUB_MODULE_NAME END_OF_LOG_HEADER
#define LOG_VERBOSE_PREFIX    \
  RTT_CTRL_RESET VERBOSE_FLAG \
  SUB_MODULE_NAME END_OF_LOG_HEADER

#define LOG(...)                        SEGGER_RTT_printf(0, __VA_ARGS__)

#define LOG_ASSERT_MSG()                                                  \
  do {                                                                    \
    LOG(LOG_ASSERT_PREFIX "ASSERT ERROR at %s:%d\n", __FILE__, __LINE__); \
  } while (0)

#define LOG_ASSERT(x)             \
  do {                            \
    if (!x) { LOG_ASSERT_MSG(); } \
  } while (0)

#define UINT8_ARRAY_DUMP(array_base, array_size)                \
  do {                                                          \
    for (int i_log_exlusive = 0; i_log_exlusive < (array_size); \
         i_log_exlusive++) {                                    \
      LOG((i_log_exlusive + 1) % 16 ? "%02X " : "%02X\n",       \
          ((char *)(array_base))[i_log_exlusive]);              \
    }                                                           \
    LOG("\n");                                                  \
  } while (0)

#define LOGN()   \
  do {           \
    LOG("\r\n"); \
  } while (0)

/* Direct way to output Error message */
#define LOG_DIRECT_ERR(_prefix_, ...)                       \
  do {                                                      \
    LOG(RTT_CTRL_TEXT_BRIGHT_RED _prefix_, ## __VA_ARGS__); \
  } while (0)

/**
 * Different level log system
 */

/* Error */
#define LOGE(_prefix_, ...)                           \
  do {                                                \
    if (LOG_LEVEL >= LVL_ERROR) {                     \
      LOG(LOG_ERROR_PREFIX _prefix_, ## __VA_ARGS__); \
    }                                                 \
  } while (0)

/* Warning */
#define LOGW(_prefix_, ...)                             \
  do {                                                  \
    if (LOG_LEVEL >= LVL_WARNING) {                     \
      LOG(LOG_WARNING_PREFIX _prefix_, ## __VA_ARGS__); \
    }                                                   \
  } while (0)

/* Information */
#define LOGI(_prefix_, ...)                          \
  do {                                               \
    if (LOG_LEVEL >= LVL_INFO) {                     \
      LOG(LOG_INFO_PREFIX _prefix_, ## __VA_ARGS__); \
    }                                                \
  } while (0)

/* DEBUG */
#define LOGD(_prefix_, ...)                           \
  do {                                                \
    if (LOG_LEVEL >= LVL_DEBUG) {                     \
      LOG(LOG_DEBUG_PREFIX _prefix_, ## __VA_ARGS__); \
    }                                                 \
  } while (0)

/* Vobase */
#define LOGV(_prefix_, ...)                             \
  do {                                                  \
    if (LOG_LEVEL >= LVL_VERBOSE) {                     \
      LOG(LOG_VERBOSE_PREFIX _prefix_, ## __VA_ARGS__); \
    }                                                   \
  } while (0)

/* Address error - file and line */
#define ERROR_ADDRESSING()                                       \
  do {                                                           \
    LOGE("  |--> File - %s, Line - %d\r\n", __FILE__, __LINE__); \
  } while (0)

#define INIT_LOG()                                     \
  do {                                                 \
    SEGGER_RTT_Init();                                 \
    LOGI("--------- Compiled - %s - %s ---------\r\n", \
         (uint32_t)__DATE__,                           \
         (uint32_t)__TIME__);                          \
  } while (0)

/**
 * Event Log Part
 */
#define EVENT_LOG_LEVEL         VERBOSE

#define NO_EVENT_LOG            0
#define CRITICAL                1
#define IMPORTANT               2
#define VERBOSE                 3

#define CRITICAL_COLOR          RTT_CTRL_TEXT_BRIGHT_RED
#define IMPORTANT_COLOR         RTT_CTRL_TEXT_BRIGHT_YELLOW
#define VERBOSE_COLOR           RTT_CTRL_RESET

#define FUNCTION                ""
#define EVT_CATEGORY            ""

/*FUNCTION Macro doesn't be used yet */
#define EVT_CRITICAL_PREFIX     CRITICAL_COLOR FUNCTION EVT_CATEGORY
#define EVT_IMPORTANT_PREFIX    IMPORTANT_COLOR FUNCTION EVT_CATEGORY
#define EVT_VERBOSE_PREFIX      VERBOSE_COLOR FUNCTION EVT_CATEGORY

#define COEX                    1
#define DFU                     1
#define ENDPOINT                1
#define PSTORE                  1
#define GATT                    1
#define GATT_SERVER             1
#define HARDWARE                1
#define LE_CONNECTION           1
#define LE_GAP                  1
#define LE_SM                   1
#define SYSTEM                  1
#define TEST                    1
#define USER                    1

/**
 * Critical events
 */
#define EVT_LOG_C(_evt_name_, _attached_, ...)                        \
  do {                                                                \
    if (EVENT_LOG_LEVEL >= CRITICAL) {                                \
      LOG(EVT_CRITICAL_PREFIX _evt_name_ _attached_, ## __VA_ARGS__); \
    }                                                                 \
  } while (0)

/**
 * Important events
 */
#define EVT_LOG_I(_evt_name_, _attached_, ...)                         \
  do {                                                                 \
    if (EVENT_LOG_LEVEL >= IMPORTANT) {                                \
      LOG(EVT_IMPORTANT_PREFIX _evt_name_ _attached_, ## __VA_ARGS__); \
    }                                                                  \
  } while (0)

/**
 * Verbose events
 */
#define EVT_LOG_V(_evt_name_, _attached_, ...)                       \
  do {                                                               \
    if (EVENT_LOG_LEVEL >= VERBOSE) {                                \
      LOG(EVT_VERBOSE_PREFIX _evt_name_ _attached_, ## __VA_ARGS__); \
    }                                                                \
  } while (0)

/**
 * Test purpose
 */
#define TRY_OUT_ALL_COLORS()                                                \
  do {                                                                      \
    for (uint8_t i_log_exlusive = 1; i_log_exlusive < 8;                    \
         i_log_exlusive++) {                                                \
      SEGGER_RTT_printf(0,                                                  \
                        "[2;3%dm" "Normal color. Test For Log out...\r\n", \
                        i_log_exlusive);                                    \
      SEGGER_RTT_printf(0,                                                  \
                        "[1;3%dm" "Bright color. Test For Log out...\r\n", \
                        i_log_exlusive);                                    \
    }                                                                       \
  } while (0)

/**
 * Function declarations
 */
sl_status_t error_checking(sl_status_t sc, uint8_t directly);
void log_events(sl_bt_msg_t *evt);

#define SE_CALL(x)                        \
  do {                                    \
    if (error_checking((x)->result, 0)) { \
      ERROR_ADDRESSING();                 \
    }                                     \
  } while (0)

#define SURROUNDING(x)          "<" x "> - "

#define GENERAL_ERR_CHECK(x)                            \
  do {                                                  \
    if (x != GENERAL_SUCCESS) {                         \
      LOGE("Error Return. Error code = 0x%04X\r\n", x); \
      ERROR_ADDRESSING();                               \
    }                                                   \
  } while (0)

#endif
#endif
