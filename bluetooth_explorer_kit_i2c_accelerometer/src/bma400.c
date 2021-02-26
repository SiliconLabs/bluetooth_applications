/***************************************************************************//**
* @file
* @brief Silicon Labs BMA400 driver
*
* BMA400 accelerometer I2C driver source file.
*******************************************************************************
* # License
* <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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
* # Evaluation Quality
* This code has been minimally tested to ensure that it builds and is suitable
* as a demonstration for evaluation purposes only. This code will be maintained
* at the sole discretion of Silicon Labs.
******************************************************************************/

/* ########################################################################## */
/*                           System includes                                  */
/* ########################################################################## */
#include <string.h> /* string defns, memcpy, etc ... */

/* ########################################################################## */
/*                          Non system includes                               */
/* ########################################################################## */
#include "bma400.h" /* Module header */

#include "bma400_conf.h"   /* module configuration */
#include "em_i2c.h"        /* I2C module definitions */
#include "em_cmu.h"        /* clock module */
#include "em_emu.h"        /* energy mode control */
#include "bgm220pc22hna.h" /* BGM22 definitions */

#include "ustimer.h" /* us timer */

/* ########################################################################## */
/*                             Mecros Defn                                    */
/* ########################################################################## */
#define I2C_BUFFER_SIZE 10

/* Array macros */
#define mArraySize(Array) (sizeof(Array)/sizeof(Array[0]))

/* ########################################################################## */
/*                          Typedefs and Enums                                */
/* ########################################################################## */
struct selftest_delta_limit /* Accel self test diff xyz data structure */

{
  int32_t x; /* Accel X  data */
  int32_t y; /* Accel Y  data */
  int32_t z; /* Accel Z  data */
};

/* ########################################################################## */
/*                              Local variables                              */
/* ########################################################################## */
static I2C_Init_TypeDef Init_Def = I2C_INIT_DEFAULT;

static uint8_t i2c_txBuffer[I2C_BUFFER_SIZE]; /* i2c transmit buffer */
static uint8_t i2c_rxBuffer[I2C_BUFFER_SIZE]; /* i2c receive buffer */

static stBMA400_Data_t stData_Default;

static stBMA400_Com_t stCom_Default = {
  cmuClock_I2C0,
  BMA400_I2C0_PORT,
  BMA400_I2C0_PIN_SDA,
  BMA400_I2C0_PIN_SCL,
  I2C0,
  I2C0_IRQn,
  BMA400_I2C_ADDRESS_SDO_HIGH
};

static stBMA400_Desc_t stDesc[] = {
  { /* I2C0 */
    eBMA400_Intfc_I2C0,
    &stData_Default,
    &stCom_Default
  } /* Note that this array of structure could be extented to fit your needs */
};

/* ########################################################################## */
/*                       Local functions declarations                         */
/* ########################################################################## */
static int32_t power(int16_t base, uint8_t resolution);
static void convert_lsb_g(const struct selftest_delta_limit *accel_data_diff,
                          struct selftest_delta_limit *accel_data_diff_mg);

/* register routines */
static int8_t get_regs(uint8_t reg_addr, uint8_t *reg_data, uint8_t len, const eBMA400_Intfc eIntfc);
static int8_t set_regs(uint8_t reg_addr, uint8_t *reg_data, uint8_t len, const eBMA400_Intfc eIntfc);

/* get and set modes */
static int8_t set_accel_conf(const struct bma400_acc_conf *accel_conf, const eBMA400_Intfc eIntfc);
static int8_t get_accel_data(uint8_t data_sel, struct bma400_sensor_data *accel, const eBMA400_Intfc eIntfc);
static int8_t get_accel_conf(struct bma400_acc_conf *accel_conf, const eBMA400_Intfc eIntfc);

static int8_t set_auto_wakeup(uint8_t conf, const eBMA400_Intfc eIntfc);
static int8_t set_auto_low_power(const struct bma400_auto_lp_conf *auto_lp_conf, const eBMA400_Intfc eIntfc);
static int8_t get_auto_low_power(struct bma400_auto_lp_conf *auto_lp_conf, const eBMA400_Intfc eIntfc);

static int8_t set_autowakeup_timeout(const struct bma400_auto_wakeup_conf *wakeup_conf, const eBMA400_Intfc eIntfc);
static int8_t set_autowakeup_interrupt(const struct bma400_wakeup_conf *wakeup_conf, const eBMA400_Intfc eIntfc);
static int8_t get_autowakeup_timeout(struct bma400_auto_wakeup_conf *wakeup_conf, const eBMA400_Intfc eIntfc);
static int8_t get_autowakeup_interrupt(struct bma400_wakeup_conf *wakeup_conf, const eBMA400_Intfc eIntfc);

static int8_t set_tap_conf(const struct bma400_tap_conf *tap_set, const eBMA400_Intfc eIntfc);
static int8_t get_tap_conf(struct bma400_tap_conf *tap_set, const eBMA400_Intfc eIntfc);

static int8_t set_activity_change_conf(const struct bma400_act_ch_conf *act_ch_set, const eBMA400_Intfc eIntfc);
static int8_t get_activity_change_conf(struct bma400_act_ch_conf *act_ch_set, const eBMA400_Intfc eIntfc);

static int8_t get_sensor_conf(struct bma400_sensor_conf *conf, uint16_t n_sett, const eBMA400_Intfc eIntfc);
static int8_t set_sensor_conf(const struct bma400_sensor_conf *conf, uint16_t n_sett, const eBMA400_Intfc eIntfc);

/* Device interrupts */
static int8_t set_gen1_int(const struct bma400_gen_int_conf *gen_int_set, const eBMA400_Intfc eIntfc);
static int8_t set_gen2_int(const struct bma400_gen_int_conf *gen_int_set, const eBMA400_Intfc eIntfc);
static int8_t set_orient_int(const struct bma400_orient_int_conf *orient_conf, const eBMA400_Intfc eIntfc);
static int8_t get_gen1_int(struct bma400_gen_int_conf *gen_int_set, const eBMA400_Intfc eIntfc);
static int8_t get_gen2_int(struct bma400_gen_int_conf *gen_int_set, const eBMA400_Intfc eIntfc);
static int8_t get_orient_int(struct bma400_orient_int_conf *orient_conf, const eBMA400_Intfc eIntfc);

static void check_mapped_interrupts(uint8_t int_1_map, uint8_t int_2_map, enum bma400_int_chan *int_map);

/* Pin related routines */
static void map_int_pin(uint8_t *data_array, uint8_t int_enable, enum bma400_int_chan int_map);
static void get_int_pin_map(const uint8_t *data_array, uint8_t int_enable, enum bma400_int_chan *int_map);
static int8_t set_int_pin_conf(struct bma400_int_pin_conf int_conf, const eBMA400_Intfc eIntfc);
static int8_t get_int_pin_conf(struct bma400_int_pin_conf *int_conf, const eBMA400_Intfc eIntfc);

/* FIFO routines */
static int8_t set_fifo_conf(const struct bma400_fifo_conf *fifo_conf, const eBMA400_Intfc eIntfc);
static int8_t get_fifo_conf(struct bma400_fifo_conf *fifo_conf, const eBMA400_Intfc eIntfc);
static int8_t get_fifo_length(uint16_t *fifo_byte_cnt, const eBMA400_Intfc eIntfc);
static int8_t read_fifo(const struct bma400_fifo_data *fifo, const eBMA400_Intfc eIntfc);

/* Acceleration frames */
static void check_frame_available(const struct bma400_fifo_data *fifo,
                                  uint8_t *frame_available,
                                  uint8_t accel_width,
                                  uint8_t data_en,
                                  uint16_t *data_index);

static void unpack_accel(const struct bma400_fifo_data *fifo,
                         struct bma400_sensor_data *accel_data,
                         uint16_t *data_index,
                         uint8_t accel_width,
                         uint8_t frame_header);

static void unpack_sensortime_frame(struct bma400_fifo_data *fifo, uint16_t *data_index);

/* Test COMs Chip Id read/write */
static bool CHIPID_read(void);
static bool CHIPID_write(const uint8_t chipid);

/* bud read/write operations */
static I2C_TransferReturn_TypeDef I2C_MasterWrite(uint16_t slaveAddress, uint8_t regAddres,uint8_t *txBuff, uint8_t numBytes);
static I2C_TransferReturn_TypeDef I2C_MasterRead(uint16_t slaveAddress, uint8_t targetAddress, uint8_t *rxBuff, uint8_t numBytes);

/* ########################################################################## */
/*                                Public API                                  */
/* ########################################################################## */
eBMA400_State bma400_Setup(void)
{
  eBMA400_State eRetVal;
  uint8_t idx;
  stBMA400_Desc_t *pDesc;

  /* Initialise locals */
  eRetVal=BMA400_E_FAILURE;
  pDesc=NULL;

  /* Initialise array of descriptors */
  for(idx=0;idx<mArraySize(stDesc);idx++)
  {
#if 0
    bma400_ResetData(&stDesc[idx]);
#else
    bma400_Init(stDesc[idx].intf);
    stDesc[idx].pstData->start = false;
#endif

    CMU_ClockEnable(stDesc[idx].pstCom->clock, true); /* Need clock to read the register */

    // Using PC0 (SDA) and PC1 (SCL)
    GPIO_PinModeSet(stDesc[idx].pstCom->port, stDesc[idx].pstCom->SDAPin, gpioModeWiredAndPullUpFilter, 1); /* SDA */
    GPIO_PinModeSet(stDesc[idx].pstCom->port, stDesc[idx].pstCom->SCLPin, gpioModeWiredAndPullUpFilter, 1); /* SCL */

    // Route GPIO pins to I2C module
    GPIO->I2CROUTE[stDesc[idx].intf].SDAROUTE = ( GPIO->I2CROUTE[0].SDAROUTE & ~_GPIO_I2C_SDAROUTE_MASK)
                                                  | (stDesc[idx].pstCom->port << _GPIO_I2C_SDAROUTE_PORT_SHIFT
                                                  | (stDesc[idx].pstCom->SDAPin << _GPIO_I2C_SDAROUTE_PIN_SHIFT) );

    GPIO->I2CROUTE[stDesc[idx].intf].SCLROUTE = ( GPIO->I2CROUTE[0].SCLROUTE & ~_GPIO_I2C_SCLROUTE_MASK)
                                                  | (stDesc[idx].pstCom->port << _GPIO_I2C_SCLROUTE_PORT_SHIFT
                                                  | (stDesc[idx].pstCom->SCLPin << _GPIO_I2C_SCLROUTE_PIN_SHIFT) );

    GPIO->I2CROUTE[stDesc[idx].intf].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;

    // Initializing the I2C
    I2C_Init(stDesc[idx].pstCom->pI2CIntfc, &Init_Def);
    stDesc[idx].pstCom->pI2CIntfc->CTRL = I2C_CTRL_AUTOSN;
    stDesc[idx].pstCom->pI2CIntfc->IEN = I2C_IEN_TXBL|I2C_IEN_ACK; /*  */

    /* Enable I2C0 interrupt to catch button press that changes slew rate */
    NVIC_ClearPendingIRQ(stDesc[idx].pstCom->interrupt);
    // NVIC_EnableIRQ(stDesc[idx].pstCom->interrupt);

    stDesc[idx].pstCom->pI2CIntfc->CMD = I2C_CMD_ABORT|I2C_CMD_CLEARPC|I2C_CMD_CLEARTX;
  }

  /* Init module */
  eRetVal = BMA400_OK;

  return eRetVal;
}

/*
 * @details This API reads the chip-id of the sensor which is the first step to
 * verify the sensor and also it configures the read mechanism of SPI and
 * I2C interface. As this API is the entry point, call this API before using other APIs.
 *
 * @param[in,out] dev : Structure instance of bma400_dev
 *
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
eBMA400_State bma400_Init(const eBMA400_Intfc eIntfc)
{
  eBMA400_State eRetVal;

  /* Initialise locals */
  eRetVal=BMA400_E_FAILURE;

  stDesc[eIntfc].pstData->bufferLen = I2C_BUFFER_SIZE;
  stDesc[eIntfc].pstData->pRxData = i2c_rxBuffer;
  stDesc[eIntfc].pstData->pRxData = i2c_rxBuffer;

  /* Reset local data */
  memset(&stDesc[eIntfc].pstData->pTxData[0],0x00,stDesc[0].pstData->bufferLen);
  memset(&stDesc[eIntfc].pstData->pRxData[0],0x00,stDesc[0].pstData->bufferLen);

  /* Init delay timer */
  USTIMER_Init();

  return eRetVal;
}

/* ########################################################################## */
/*                              I2C state machine                             */
/* ########################################################################## */
void bma400_I2C_ISR_Handler(const eBMA400_Intfc eIntfc)
{
  uint32_t iflags;
  I2C_TypeDef * pI2CIntfc;

  /* short handle to the I2C def */
  pI2CIntfc = stDesc[eIntfc].pstCom->pI2CIntfc;

  /* Get all even interrupts. */
  iflags = pI2CIntfc->IF;

  if( 0x00000000 != (iflags & I2C_IF_TXBL) )
  {
    /* Kisk statr the I2C communication state machine with the sensor */
    I2C_IntClear(pI2CIntfc,I2C_IF_TXBL);
    pI2CIntfc->IEN &= (~I2C_IEN_TXBL); /* disable interrupt */
    stDesc[eIntfc].pstData->start = true;     /* trigger the state machine */
  }

  if( 0x00000000 != (iflags & I2C_IF_NACK ) )
  {
    /* issue CONT command in case of NACK*/
    I2C_IntClear(pI2CIntfc, I2C_IF_NACK);
    pI2CIntfc->CMD = I2C_CMD_CONT;
  }

  if( 0x00000000 != (iflags & I2C_IF_ACK ) )
  {
    I2C_Transfer(pI2CIntfc);
    /* ACK is taken care of by the emlib driver but it doesn't harm to do it again */
    I2C_IntClear(pI2CIntfc, I2C_IF_ACK);
  }

  if( 0x00000000 != (iflags & I2C_IF_RXDATAV) )
  {
    I2C_Transfer(pI2CIntfc);
    I2C_IntClear(pI2CIntfc,I2C_IF_RXDATAV);
  }

  if( 0x00000000 != (iflags & I2C_IF_BUSERR) )
  {
    I2C_IntClear(pI2CIntfc,I2C_IF_BUSERR);
  }

  if( 0x00000000 != (iflags & I2C_IF_ARBLOST) )
  {
    I2C_IntClear(pI2CIntfc,I2C_IF_ARBLOST);
  }

  if( 0x00000000 != (iflags & I2C_IF_START) )
  {
    I2C_IntClear(pI2CIntfc,I2C_IF_START);
  }

  if( 0x00000000 != (iflags & I2C_IF_TXC) )
  {
    I2C_IntClear(pI2CIntfc,I2C_IF_TXC);
  }

  if( 0x00000000 != (iflags & ( I2C_IF_MSTOP) ) )
  {
    I2C_Transfer(pI2CIntfc); /* Need to keep the internal state machine updated */
    I2C_IntClear(pI2CIntfc,I2C_IF_SSTOP);
    I2C_IntClear(pI2CIntfc,I2C_IF_MSTOP);
  }

  return;
}

/* Read chip id, self test */
uint8_t bma400_chipid(const eBMA400_Intfc eIntfc)
{
  uint8_t uRetVal;

  if (true == stDesc[eIntfc].pstData->start)
  {
    /* Transmission complete */
    stDesc[eIntfc].pstData->start = false;
  }

  /* Transmitting data */
  if( true == CHIPID_read())
  {
    /* Get the data */
    uRetVal = stDesc[eIntfc].pstData->pRxData[0];
  }

#if 0
    /* Enter EM2. TBD. Lit and LED for the time being */
    EMU_EnterEM2(true);
#endif

  return uRetVal;
}

/* @details This API soft-resets the sensor where all the registers are reset to their default values except 0x4B.
 *
 * @param[in] dev       : Structure instance of bma400_dev.
 *
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
int8_t bma400_soft_reset(const eBMA400_Intfc eIntfc)
{
  int8_t rslt;
  uint8_t data = BMA400_SOFT_RESET_CMD;

  /* Reset the device */
  rslt = set_regs(BMA400_COMMAND_REG_ADDR, &data, 1, eIntfc);
  USTIMER_Delay(BMA400_SOFT_RESET_DELAY_MS);

  if ((rslt == BMA400_OK) && (eIntfc != eBMA400_Intfc_I2C0))
  {
    /* Dummy read of 0x7F register to enable SPI Interface
     * if SPI is used
     */
    rslt = get_regs(0x7F, &data, 1, eIntfc);
  }

  return rslt;
}

/*
 * @details This API is used to set the power mode of the sensor.
 *
 * @param[in] power_mode  : Macro to select power mode of the sensor.
 * @param[in] dev         : Structure instance of bma400_dev.
 *
 * Possible value for power_mode :
 * @code
 *   BMA400_NORMAL_MODE
 *   BMA400_SLEEP_MODE
 *   BMA400_LOW_POWER_MODE
 * @endcode
 *
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
int8_t bma400_set_power_mode(uint8_t power_mode, const eBMA400_Intfc eIntfc)
{
  int8_t rslt;
  uint8_t reg_data = 0;

  /* Init locals */
  rslt = get_regs(BMA400_ACCEL_CONFIG_0_ADDR, &reg_data, 1, eIntfc);

  if (rslt == BMA400_OK)
  {
    reg_data = BMA400_SET_BITS_POS_0(reg_data, BMA400_POWER_MODE, power_mode);

    /* Set the power mode of sensor */
    rslt = set_regs(BMA400_ACCEL_CONFIG_0_ADDR, &reg_data, 1, eIntfc);
    if (power_mode == BMA400_LOW_POWER_MODE)
    {
      /* A delay of 1/ODR is required to switch power modes
       * Low power mode has 25Hz frequency and hence it needs
       * 40ms delay to enter low power mode
       */
      USTIMER_Delay(40);
    }
    else
    {
      USTIMER_Delay(10); /* TBC */
    }
  }

  return rslt;
}

/* @details This API is used to get the power mode of the sensor.
 * @param[out] power_mode  : power mode of the sensor.
 * @param[in] dev          : Structure instance of bma400_dev.
 *
 * * Possible value for power_mode :
 * @code
 *   BMA400_NORMAL_MODE
 *   BMA400_SLEEP_MODE
 *   BMA400_LOW_POWER_MODE
 * @endcode
 *
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
int8_t bma400_get_power_mode(uint8_t *power_mode, const eBMA400_Intfc eIntfc)
{
  int8_t rslt;
  uint8_t reg_data;

  /* Proceed if null check is fine */
  if (rslt == BMA400_OK)
  {
    rslt = get_regs(BMA400_STATUS_ADDR, &reg_data, 1, eIntfc);
    *power_mode = BMA400_GET_BITS(reg_data, BMA400_POWER_MODE_STATUS);
  }

  return rslt;
}

/*
 * @details This API is used to get the accelerometer data along with the sensor-time.
 *
 * @param[in] data_sel     : Variable to select sensor data only
 *                           or data along with sensortime
 * @param[in,out] accel    : Structure instance to store data
 * @param[in] dev          : Structure instance of bma400_dev
 *
 * Assignable macros for "data_sel" :
 * @code
 *   - BMA400_DATA_ONLY
 *   - BMA400_DATA_SENSOR_TIME
 * @endcode
 *
 * @note The accelerometer data value is in LSB, based on the range selected.
 *
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
int8_t bma400_get_accel_data(uint8_t data_sel, struct bma400_sensor_data *accel, const eBMA400_Intfc eIntfc)
{
  int8_t rslt;

  rslt = BMA400_OK;

  /* Proceed if null check is fine */
  if ((rslt == BMA400_OK) || (accel != NULL))
  {
    /* Read and store the accel data */
    rslt = get_accel_data(data_sel, accel, eIntfc);
  }
  else
  {
    rslt = BMA400_E_NULL_PTR;
  }

  return rslt;
}

int8_t bma400_set_device_conf(const struct bma400_device_conf *conf, uint8_t n_sett, const eBMA400_Intfc eIntfc)
{
  int8_t rslt = BMA400_OK;
  uint16_t idx;
  uint8_t data_array[3] = { 0 };

  if (conf == NULL)
  {
    rslt = BMA400_E_NULL_PTR;
    return rslt;
  }

  /* Read the interrupt pin mapping configurations */
  rslt = get_regs(BMA400_INT_MAP_ADDR, data_array, 3, eIntfc);

  for (idx = 0; (idx < n_sett) && (rslt == BMA400_OK); idx++)
  {
    switch (conf[idx].type)
    {
      case BMA400_AUTOWAKEUP_TIMEOUT:
        rslt = set_autowakeup_timeout(&conf[idx].param.auto_wakeup, eIntfc);
        break;
      case BMA400_AUTOWAKEUP_INT:
        rslt = set_autowakeup_interrupt(&conf[idx].param.wakeup, eIntfc);
        if (rslt == BMA400_OK)
        {
            /* Interrupt pin mapping */
            map_int_pin(data_array, BMA400_WAKEUP_INT_MAP, conf[idx].param.wakeup.int_chan);
        }
        break;
      case BMA400_AUTO_LOW_POWER:
        rslt = set_auto_low_power(&conf[idx].param.auto_lp, eIntfc);
        break;
      case BMA400_INT_PIN_CONF:
        rslt = set_int_pin_conf(conf[idx].param.int_conf, eIntfc);
        break;
      case BMA400_INT_OVERRUN_CONF:
        /* Interrupt pin mapping */
        map_int_pin(data_array, BMA400_INT_OVERRUN_MAP, conf[idx].param.overrun_int.int_chan);
        break;
      case BMA400_FIFO_CONF:
        rslt = set_fifo_conf(&conf[idx].param.fifo_conf, eIntfc);
        if (rslt == BMA400_OK)
        {
            /* Interrupt pin mapping */
            map_int_pin(data_array, BMA400_FIFO_WM_INT_MAP, conf[idx].param.fifo_conf.fifo_wm_channel);
            map_int_pin(data_array, BMA400_FIFO_FULL_INT_MAP, conf[idx].param.fifo_conf.fifo_full_channel);
        }
        break;
      default:
        rslt = BMA400_E_INVALID_CONFIG;
    }
  }

  if (rslt == BMA400_OK)
  {
    /* Set the interrupt pin mapping configurations */
    rslt = set_regs(BMA400_INT_MAP_ADDR, data_array, 3, eIntfc);
  }

  return rslt;
}

/* @details This API is used to get the device specific settings and store
 * them in the corresponding structure instance.
 *
 * @param[in] conf         : Structure instance of the configuration structure
 * @param[in] n_sett       : Number of settings to be obtained
 *
 * @note Once the API is called, the settings structure will be updated
 * in the settings structure.
 *
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
int8_t bma400_get_device_conf(struct bma400_device_conf *conf, uint8_t n_sett, const eBMA400_Intfc eIntfc)
{
  int8_t rslt;
  uint16_t idx = 0;
  uint8_t data_array[3] = { 0 };

  /* Proceed if null check is fine */
  if (rslt == BMA400_OK)
  {
    /* Read the interrupt pin mapping configurations */
    rslt = get_regs(BMA400_INT_MAP_ADDR, data_array, 3, eIntfc);
    if (rslt == BMA400_OK)
    {
      for (idx = 0; idx < n_sett; idx++)
      {
        switch (conf[idx].type)
        {
          case BMA400_AUTOWAKEUP_TIMEOUT:
            rslt = get_autowakeup_timeout(&conf[idx].param.auto_wakeup, eIntfc);
            break;
          case BMA400_AUTOWAKEUP_INT:
            rslt = get_autowakeup_interrupt(&conf[idx].param.wakeup, eIntfc);
            if (rslt == BMA400_OK)
            {
                /* Get the INT pin mapping */
                get_int_pin_map(data_array, BMA400_WAKEUP_INT_MAP, &conf[idx].param.wakeup.int_chan);
            }
            break;
          case BMA400_AUTO_LOW_POWER:
            rslt = get_auto_low_power(&conf[idx].param.auto_lp, eIntfc);
            break;
          case BMA400_INT_PIN_CONF:
            rslt = get_int_pin_conf(&conf[idx].param.int_conf, eIntfc);
            break;
          case BMA400_INT_OVERRUN_CONF:
            get_int_pin_map(data_array, BMA400_INT_OVERRUN_MAP, &conf[idx].param.overrun_int.int_chan);
            break;
          case BMA400_FIFO_CONF:
            rslt = get_fifo_conf(&conf[idx].param.fifo_conf, eIntfc);
            if (rslt == BMA400_OK)
            {
              get_int_pin_map(data_array,
                              BMA400_FIFO_FULL_INT_MAP,
                              &conf[idx].param.fifo_conf.fifo_full_channel);
              get_int_pin_map(data_array,
                              BMA400_FIFO_WM_INT_MAP,
                              &conf[idx].param.fifo_conf.fifo_wm_channel);
            }
            break;
        }
      }
    }
  }

  return rslt;
}

/* @details This API is used to check if the interrupts are asserted and return the status.
 *
 * @param[in] int_status   : Interrupt status of sensor
 * @param[in] dev          : Structure instance of bma400_dev.
 *
 * @note Interrupt status of the sensor determines which all interrupts are asserted at any instant of time.
 * @code
 *  BMA400_WAKEUP_INT_ASSERTED
 *  BMA400_ORIENT_CH_INT_ASSERTED
 *  BMA400_GEN1_INT_ASSERTED
 *  BMA400_GEN2_INT_ASSERTED
 *  BMA400_FIFO_FULL_INT_ASSERTED
 *  BMA400_FIFO_WM_INT_ASSERTED
 *  BMA400_DRDY_INT_ASSERTED
 *  BMA400_INT_OVERRUN_ASSERTED
 *  BMA400_STEP_INT_ASSERTED
 *  BMA400_S_TAP_INT_ASSERTED
 *  BMA400_D_TAP_INT_ASSERTED
 *  BMA400_ACT_CH_X_ASSERTED
 *  BMA400_ACT_CH_Y_ASSERTED
 *  BMA400_ACT_CH_Z_ASSERTED
 *@endcode
 * @note Call the API and then use the above macros to check whether the interrupt is asserted or not.
 *@code
 * if (int_status & BMA400_FIFO_FULL_INT_ASSERTED) {
 *     printf("\n FIFO FULL INT ASSERTED");
 * }
 *@endcode
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
int8_t bma400_get_interrupt_status(uint16_t *int_status, const eBMA400_Intfc eIntfc)
{
  int8_t rslt;
  uint8_t reg_data[3];

  /* Proceed if null check is fine */
  if (rslt == BMA400_OK)
  {
    /* Read the interrupt status registers */
    rslt = get_regs(BMA400_INT_STAT0_ADDR, reg_data, 3, eIntfc);
    reg_data[1] = BMA400_SET_BITS(reg_data[1], BMA400_INT_STATUS, reg_data[2]);

    /* Concatenate the interrupt status to the output */
    *int_status = ((uint16_t)reg_data[1] << 8) | reg_data[0];
  }

  return rslt;
}

/* @details This API is used to set the step counter's configuration parameters from the registers 0x59 to 0x71.
 *
 * @param[in] sccr_conf : sc config parameter
 * @param[in] dev    : Structure instance of bma400_dev.
 *
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error / failure
 */
int8_t bma400_set_step_counter_param(uint8_t *sccr_conf, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;

    /* Proceed if null check is fine */
    if (rslt == BMA400_OK)
    {
        /* Set the step counter parameters in the sensor */
        rslt = set_regs(0x59, sccr_conf, 25, eIntfc);
    }

    return rslt;
}

/* @details This API is used to get the step counter output in form of number of steps in the step_count value.
 *
 * @param[out] step_count      : Number of step counts
 * @param[out] activity_data   : Activity data WALK/STILL/RUN
 * @param[in] dev              : Structure instance of bma400_dev.
 *
 *  activity_data   |  Status
 * -----------------|------------------
 *  0x00            | BMA400_STILL_ACT
 *  0x01            | BMA400_WALK_ACT
 *  0x02            | BMA400_RUN_ACT
 *
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
int8_t bma400_get_steps_counted(uint32_t *step_count, uint8_t *activity_data, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_arrray[4];

    /* Proceed if null check is fine */
    if (rslt == BMA400_OK)
    {
        rslt = get_regs(BMA400_STEP_CNT_0_ADDR, data_arrray, 4, eIntfc);
        *step_count = ((uint32_t)data_arrray[2] << 16) | ((uint16_t)data_arrray[1] << 8) | data_arrray[0];
        *activity_data = data_arrray[3];
    }

    return rslt;
}

/* @details This API is used to get the temperature data output.
 *
 * @note Temperature data output must be divided by a factor of 10
 * Consider temperature_data = 195 ,
 * Then the actual temperature is 19.5 degree Celsius.
 *
 * @param[in,out] temperature_data   : Temperature data
 * @param[in] dev                    : Structure instance of bma400_dev.
 *
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
int8_t bma400_get_temperature_data(int16_t *temperature_data, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t reg_data;

    /* Proceed if null check is fine */
    if (rslt == BMA400_OK)
    {
        rslt = get_regs(BMA400_TEMP_DATA_ADDR, &reg_data, 1, eIntfc);

        /* Temperature data calculations */
        *temperature_data = (((int16_t)((int8_t)reg_data)) - 2) * 5 + 250;
    }

    return rslt;
}

/* @details This API is used to get the enable/disable status of the various interrupts.
 *
 * @param[in] int_select   : Structure to select specific interrupts
 * @param[in] n_sett       : Number of interrupt settings enabled / disabled
 * @param[in] dev          : Structure instance of bma400_dev.
 *
 * @note Select the needed interrupt type for which the status of it whether
 * it is enabled/disabled is to be known in the int_select->int_sel, and the
 * output is stored in int_select->conf either as BMA400_ENABLE/BMA400_DISABLE
 *
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
int8_t bma400_get_interrupts_enabled(struct bma400_int_enable *int_select, uint8_t n_sett, const eBMA400_Intfc eIntfc)
{
  int8_t rslt;
  uint8_t idx = 0;
  uint8_t reg_data[2];
  uint8_t wkup_int;

  /* Proceed if null check is fine */
  if (rslt == BMA400_OK)
  {
    rslt = get_regs(BMA400_INT_CONF_0_ADDR, reg_data, 2, eIntfc);
    if (rslt == BMA400_OK)
    {
      for (idx = 0; idx < n_sett; idx++)
      {
        /* Read the enable/disable of interrupts
         * based on user selection
         */
        switch (int_select[idx].type)
        {
            case BMA400_DRDY_INT_EN:
                int_select[idx].conf = BMA400_GET_BITS(reg_data[0], BMA400_EN_DRDY);
                break;
            case BMA400_FIFO_WM_INT_EN:
                int_select[idx].conf = BMA400_GET_BITS(reg_data[0], BMA400_EN_FIFO_WM);
                break;
            case BMA400_FIFO_FULL_INT_EN:
                int_select[idx].conf = BMA400_GET_BITS(reg_data[0], BMA400_EN_FIFO_FULL);
                break;
            case BMA400_GEN2_INT_EN:
                int_select[idx].conf = BMA400_GET_BITS(reg_data[0], BMA400_EN_GEN2);
                break;
            case BMA400_GEN1_INT_EN:
                int_select[idx].conf = BMA400_GET_BITS(reg_data[0], BMA400_EN_GEN1);
                break;
            case BMA400_ORIENT_CHANGE_INT_EN:
                int_select[idx].conf = BMA400_GET_BITS(reg_data[0], BMA400_EN_ORIENT_CH);
                break;
            case BMA400_LATCH_INT_EN:
                int_select[idx].conf = BMA400_GET_BITS(reg_data[1], BMA400_EN_LATCH);
                break;
            case BMA400_ACTIVITY_CHANGE_INT_EN:
                int_select[idx].conf = BMA400_GET_BITS(reg_data[1], BMA400_EN_ACTCH);
                break;
            case BMA400_DOUBLE_TAP_INT_EN:
                int_select[idx].conf = BMA400_GET_BITS(reg_data[1], BMA400_EN_D_TAP);
                break;
            case BMA400_SINGLE_TAP_INT_EN:
                int_select[idx].conf = BMA400_GET_BITS(reg_data[1], BMA400_EN_S_TAP);
                break;
            case BMA400_STEP_COUNTER_INT_EN:
                int_select[idx].conf = BMA400_GET_BITS_POS_0(reg_data[1], BMA400_EN_STEP_INT);
                break;
            case BMA400_AUTO_WAKEUP_EN:
                rslt = get_regs(BMA400_AUTOWAKEUP_1_ADDR, &wkup_int, 1, eIntfc);
                if (rslt == BMA400_OK)
                {
                    /* Auto-Wakeup int status */
                    int_select[idx].conf = BMA400_GET_BITS(wkup_int, BMA400_WAKEUP_INTERRUPT);
                }
                break;
            default:
                rslt = BMA400_E_INVALID_CONFIG;
                break;
        }
      }
    }
  }

  return rslt;
}

/* @details This API is used to enable the various interrupts.
 *
 * @param[in] int_select   : Structure to enable specific interrupts
 * @param[in] n_sett       : Number of interrupt settings enabled / disabled
 * @param[in] dev          : Structure instance of bma400_dev.
 *
 * @note Multiple interrupt can be enabled/disabled by
 * @code
 *    struct interrupt_enable int_select[2];
 *
 *    int_select[0].int_sel = BMA400_FIFO_FULL_INT_EN;
 *    int_select[0].conf = BMA400_ENABLE;
 *
 *    int_select[1].int_sel = BMA400_FIFO_WM_INT_EN;
 *    int_select[1].conf = BMA400_ENABLE;
 *
 *    rslt = bma400_enable_interrupt(&int_select, 2, dev);
 *@endcode
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
int8_t bma400_enable_interrupt(const struct bma400_int_enable *int_select, uint8_t n_sett, const eBMA400_Intfc eIntfc)
{
  int8_t rslt;
  uint8_t conf, idx = 0;
  uint8_t reg_data[2];

  /* Proceed if null check is fine */
  if (rslt == BMA400_OK)
  {
    rslt = get_regs(BMA400_INT_CONF_0_ADDR, reg_data, 2, eIntfc);
    if (rslt == BMA400_OK)
    {
      for (idx = 0; idx < n_sett; idx++)
      {
        conf = int_select[idx].conf;

        /* Enable the interrupt based on user selection */
        switch (int_select[idx].type)
        {
          case BMA400_DRDY_INT_EN:
              reg_data[0] = BMA400_SET_BITS(reg_data[0], BMA400_EN_DRDY, conf);
              break;
          case BMA400_FIFO_WM_INT_EN:
              reg_data[0] = BMA400_SET_BITS(reg_data[0], BMA400_EN_FIFO_WM, conf);
              break;
          case BMA400_FIFO_FULL_INT_EN:
              reg_data[0] = BMA400_SET_BITS(reg_data[0], BMA400_EN_FIFO_FULL, conf);
              break;
          case BMA400_GEN2_INT_EN:
              reg_data[0] = BMA400_SET_BITS(reg_data[0], BMA400_EN_GEN2, conf);
              break;
          case BMA400_GEN1_INT_EN:
              reg_data[0] = BMA400_SET_BITS(reg_data[0], BMA400_EN_GEN1, conf);
              break;
          case BMA400_ORIENT_CHANGE_INT_EN:
              reg_data[0] = BMA400_SET_BITS(reg_data[0], BMA400_EN_ORIENT_CH, conf);
              break;
          case BMA400_LATCH_INT_EN:
              reg_data[1] = BMA400_SET_BITS(reg_data[1], BMA400_EN_LATCH, conf);
              break;
          case BMA400_ACTIVITY_CHANGE_INT_EN:
              reg_data[1] = BMA400_SET_BITS(reg_data[1], BMA400_EN_ACTCH, conf);
              break;
          case BMA400_DOUBLE_TAP_INT_EN:
              reg_data[1] = BMA400_SET_BITS(reg_data[1], BMA400_EN_D_TAP, conf);
              break;
          case BMA400_SINGLE_TAP_INT_EN:
              reg_data[1] = BMA400_SET_BITS(reg_data[1], BMA400_EN_S_TAP, conf);
              break;
          case BMA400_STEP_COUNTER_INT_EN:
              reg_data[1] = BMA400_SET_BITS_POS_0(reg_data[1], BMA400_EN_STEP_INT, conf);
              break;
          case BMA400_AUTO_WAKEUP_EN:
              rslt = set_auto_wakeup(conf, eIntfc);
              break;
          default:
              rslt = BMA400_E_INVALID_CONFIG;
              break;
        }
      }
      if (rslt == BMA400_OK)
      {
          /* Set the configurations in the sensor */
          rslt = set_regs(BMA400_INT_CONF_0_ADDR, reg_data, 2, eIntfc);
      }
    }
  }

  return rslt;
}

/* @details This API reads the FIFO data from the sensor.
 *
 * @note User has to allocate the FIFO buffer along with
 * corresponding FIFO read length from his side before calling this API
 * as mentioned in the readme.md
 *
 * @note User must specify the number of bytes to read from the FIFO in
 * fifo->length , It will be updated by the number of bytes actually
 * read from FIFO after calling this API
 *
 * @param[in,out] fifo      : Pointer to the FIFO structure.
 *
 * @param[in,out] dev       : Structure instance of bma400_dev.
 *
 * @return Result of API execution status
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
int8_t bma400_get_fifo_data(struct bma400_fifo_data *fifo, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data;
    uint16_t fifo_byte_cnt = 0;
    uint16_t user_fifo_len = 0;

    /* Proceed if null check is fine */
    if (rslt == BMA400_OK)
    {
        /* Resetting the FIFO data byte index */
        fifo->accel_byte_start_idx = 0;

        /* Reading the FIFO length */
        rslt = get_fifo_length(&fifo_byte_cnt, eIntfc);
        if (rslt == BMA400_OK)
        {
            /* Get the FIFO configurations
             * from the sensor */
            rslt = get_regs(BMA400_FIFO_CONFIG_0_ADDR, &data, 1, eIntfc);
            if (rslt == BMA400_OK)
            {
                /* Get the data from FIFO_CONFIG0 register */
                fifo->fifo_8_bit_en = BMA400_GET_BITS(data, BMA400_FIFO_8_BIT_EN);
                fifo->fifo_data_enable = BMA400_GET_BITS(data, BMA400_FIFO_AXES_EN);
                fifo->fifo_time_enable = BMA400_GET_BITS(data, BMA400_FIFO_TIME_EN);
                fifo->fifo_sensor_time = 0;
                user_fifo_len = fifo->length;
                if (fifo->length > fifo_byte_cnt)
                {
                    /* Handling case where user requests
                     * more data than available in FIFO
                     */
                    fifo->length = fifo_byte_cnt;
                }

                /* Reading extra bytes as per the macro
                 * "BMA400_FIFO_BYTES_OVERREAD"
                 * when FIFO time is enabled
                 */
                if ((fifo->fifo_time_enable == BMA400_ENABLE) &&
                    (fifo_byte_cnt + BMA400_FIFO_BYTES_OVERREAD <= user_fifo_len))
                {
                    /* Handling sensor time availability*/
                    fifo->length = fifo->length + BMA400_FIFO_BYTES_OVERREAD;
                }

                /* Read the FIFO data */
                rslt = read_fifo(fifo, eIntfc);
            }
        }
    }

    return rslt;
}

/* @details This API parses and extracts the accelerometer frames, FIFO time
 * and control frames from FIFO data read by the "BMA400_get_fifo_data" API
 * and stores it in the "accel_data" structure instance.
 *
 * @note The bma400_extract_accel API should be called only after
 * reading the FIFO data by calling the BMA400_get_fifo_data() API
 * Please refer the readme.md for usage.
 *
 * @param[in,out] fifo        : Pointer to the FIFO structure.
 *
 * @param[out] accel_data     : Structure instance of bma400_sensor_data where
 *                              the accelerometer data from FIFO is extracted
 *                              and stored after calling this API
 *
 * @param[in,out] frame_count : Number of valid accelerometer frames requested
 *                              by user is given as input and it is updated by
 *                              the actual frames parsed from the FIFO
 *
 * @param[in] dev             : Structure instance of bma400_dev.
 *
 * @return Result of API execution status
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
int8_t bma400_extract_accel(struct bma400_fifo_data *fifo,
                            struct bma400_sensor_data *accel_data,
                            uint16_t *frame_count,
                            const eBMA400_Intfc eIntfc)
{
  int8_t rslt;
  uint8_t frame_header = 0;                /* Frame header information is stored */
  uint8_t accel_width;                     /* Accel data width is stored */
  uint16_t data_index;                     /* Data index of the parsed byte from FIFO */
  uint16_t accel_index = 0;                /* Number of accel frames parsed */
  uint8_t frame_available = BMA400_ENABLE; /* Variable to check frame availability */

  /* Proceed if null check is fine */
  if (rslt == BMA400_OK)
  {
    /* Check if this is the first iteration of data unpacking
     * if yes, then consider dummy byte on SPI
     */
    if (fifo->accel_byte_start_idx == 0)
    {
      /* Dummy byte included */
      fifo->accel_byte_start_idx = 0x00; /* was dev->dummy_byte before */
    }
    for (data_index = fifo->accel_byte_start_idx; data_index < fifo->length;)
    {
      /*Header byte is stored in the variable frame_header*/
      frame_header = fifo->data[data_index];

      /* Store the Accel 8 bit or 12 bit mode */
      accel_width = BMA400_GET_BITS(frame_header, BMA400_FIFO_8_BIT_EN);

      /* Exclude the 8/12 bit mode data from frame header */
      frame_header = frame_header & BMA400_AWIDTH_MASK;

      /*Index is moved to next byte where the data is starting*/
      data_index++;
      switch (frame_header)
      {
        case BMA400_FIFO_XYZ_ENABLE:
          check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_XYZ_ENABLE, &data_index);
          if (frame_available != BMA400_DISABLE)
          {
              /* Extract and store accel xyz data */
              unpack_accel(fifo, &accel_data[accel_index], &data_index, accel_width, frame_header);
              accel_index++;
          }
          break;
        case BMA400_FIFO_X_ENABLE:
          check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_X_ENABLE, &data_index);
          if (frame_available != BMA400_DISABLE)
          {
              /* Extract and store accel x data */
              unpack_accel(fifo, &accel_data[accel_index], &data_index, accel_width, frame_header);
              accel_index++;
          }
          break;
        case BMA400_FIFO_Y_ENABLE:
          check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_Y_ENABLE, &data_index);
          if (frame_available != BMA400_DISABLE)
          {
              /* Extract and store accel y data */
              unpack_accel(fifo, &accel_data[accel_index], &data_index, accel_width, frame_header);
              accel_index++;
          }
          break;
        case BMA400_FIFO_Z_ENABLE:
          check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_Z_ENABLE, &data_index);
          if (frame_available != BMA400_DISABLE)
          {
              /* Extract and store accel z data */
              unpack_accel(fifo, &accel_data[accel_index], &data_index, accel_width, frame_header);
              accel_index++;
          }
          break;
        case BMA400_FIFO_XY_ENABLE:
          check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_XY_ENABLE, &data_index);
          if (frame_available != BMA400_DISABLE)
          {
              /* Extract and store accel xy data */
              unpack_accel(fifo, &accel_data[accel_index], &data_index, accel_width, frame_header);
              accel_index++;
          }
          break;
        case BMA400_FIFO_YZ_ENABLE:
          check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_YZ_ENABLE, &data_index);
          if (frame_available != BMA400_DISABLE)
          {
              /* Extract and store accel yz data */
              unpack_accel(fifo, &accel_data[accel_index], &data_index, accel_width, frame_header);
              accel_index++;
          }
          break;
        case BMA400_FIFO_XZ_ENABLE:
          check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_YZ_ENABLE, &data_index);
          if (frame_available != BMA400_DISABLE)
          {
              /* Extract and store accel xz data */
              unpack_accel(fifo, &accel_data[accel_index], &data_index, accel_width, frame_header);
              accel_index++;
          }
          break;
        case BMA400_FIFO_SENSOR_TIME:
          check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_SENSOR_TIME, &data_index);
          if (frame_available != BMA400_DISABLE)
          {
              /* Unpack and store the sensor time data */
              unpack_sensortime_frame(fifo, &data_index);
          }
          break;
        case BMA400_FIFO_EMPTY_FRAME:

          /* Update the data index as complete */
          data_index = fifo->length;
          break;
        case BMA400_FIFO_CONTROL_FRAME:
          check_frame_available(fifo, &frame_available, accel_width, BMA400_FIFO_CONTROL_FRAME, &data_index);
          if (frame_available != BMA400_DISABLE)
          {
              /* Store the configuration change data from FIFO */
              fifo->conf_change = fifo->data[data_index++];
          }
          break;
        default:

          /* Update the data index as complete */
          data_index = fifo->length;
          break;
        }

        if (*frame_count == accel_index)
        {
          /* Frames read completely*/
          break;
        }
    }

    /* Update the data index */
    fifo->accel_byte_start_idx = data_index;

    /* Update number of accel frame index */
    *frame_count = accel_index;
  }

  return rslt;
}

/*  @details This API writes the fifo_flush command into command register.
 *  This action clears all data in the FIFO.
 *
 *  @param[in] dev           : Structure instance of bma400_dev.
 *
 * @return Result of API execution status
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
int8_t bma400_set_fifo_flush(const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data = BMA400_FIFO_FLUSH_CMD;

    /* Proceed if null check is fine */
    if (rslt == BMA400_OK)
    {
        /* FIFO flush command is set */
        rslt = set_regs(BMA400_COMMAND_REG_ADDR, &data, 1, eIntfc);
    }

    return rslt;
}

/* This API performs a self test of the accelerometer in BMA400.
 *
 * First get the acceleration configuration,
 * @note The return value of this API is the result of self test.
 * A self test does not soft reset of the sensor. Hence, the user can
 * define the required settings after performing the self test.
 *
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error / failure
 */
int8_t bma400_perform_self_test(const eBMA400_Intfc eIntfc)
{
  int8_t rslt;
  int8_t self_test_rslt;
  uint8_t idx;
  struct bma400_sensor_data accel_pos, accel_neg;
  struct bma400_sensor_conf accel_setting; /* Accelerometer setting structure */

  /* Pessimistic init of locals */
  rslt = BMA400_W_SELF_TEST_FAIL;
  accel_setting.type = BMA400_ACCEL; /* Select the type of configuration to be modified */
  self_test_rslt = 0;

  /* Get sensor corresonding to the bus */
  for(idx=0;idx<eBMA400_Intfc_Total;idx++)
  {
    if( eIntfc == stDesc[idx].intf )
    {
      break;
    }
  }

  /* Get the accel configurations which are set in the sensor */
  rslt = get_sensor_conf(&accel_setting, 1, eIntfc);
  if (rslt == BMA400_OK)
  {
    /* Modify to the desired configurations */
    accel_setting.param.accel.odr = BMA400_ODR_100HZ;

    /*accel_setting.param.accel.range = BMA400_8G_RANGE; */
    accel_setting.param.accel.range = BMA400_4G_RANGE;
    accel_setting.param.accel.osr = BMA400_ACCEL_OSR_SETTING_3;
    accel_setting.param.accel.data_src = BMA400_DATA_SRC_ACCEL_FILT_1;

    /* Set the desired configurations in the sensor */
    rslt = set_sensor_conf(&accel_setting, 1, eIntfc);
    if (rslt == BMA400_OK)
    {
      /* self test enabling delay */
      USTIMER_Delay(BMA400_SELF_TEST_DELAY_MS);
      rslt = BMA400_set_power_mode(BMA400_NORMAL_MODE, eIntfc);
    }

    if (rslt == BMA400_OK)
    {
      /* @brief This API performs self test with positive excitation */
      uint8_t reg_data = BMA400_ENABLE_POSITIVE_SELF_TEST;

      /* Enable positive excitation for all 3 axes */
      if ( BMA400_OK == set_regs(BMA400_SELF_TEST_ADDR, &reg_data, 1, eIntfc) )
      {
        /* Read accel data after 50ms delay */
        USTIMER_Delay(BMA400_SELF_TEST_DATA_READ_MS);
      }

      if (BMA400_OK == BMA400_get_accel_data(BMA400_DATA_ONLY, &accel_pos, eIntfc) )
      {
        /* performs self test with negative excitation */
        uint8_t reg_data = BMA400_ENABLE_NEGATIVE_SELF_TEST;

        /* Enable negative excitation for all 3 axes */
        rslt = set_regs(BMA400_SELF_TEST_ADDR, &reg_data, 1, eIntfc);
        if (rslt == BMA400_OK)
        {
          /* Read accel data after 50ms delay */
          USTIMER_Delay(BMA400_SELF_TEST_DATA_READ_MS);
          rslt = BMA400_get_accel_data(BMA400_DATA_ONLY, &accel_neg, eIntfc);
          if (rslt == BMA400_OK)
          {
            /* Disable self test */
            reg_data = BMA400_DISABLE_SELF_TEST;
            rslt = set_regs(BMA400_SELF_TEST_ADDR, &reg_data, 1, eIntfc);
          }
        }

        /* validates the self test results */
        if (rslt == BMA400_OK)
        {
          /* Structure for difference of accel values in g */
          struct selftest_delta_limit accel_data_diff = { 0, 0, 0 };

          /* Structure for difference of accel values in mg */
          struct selftest_delta_limit accel_data_diff_mg = { 0, 0, 0 };

          accel_data_diff.x = accel_pos.x - accel_neg.x;
          accel_data_diff.y = accel_pos.y - accel_neg.y;
          accel_data_diff.z = accel_pos.z - accel_neg.z;

          /* Converting LSB of the differences of accel
           * values to mg
           */
          convert_lsb_g(&accel_data_diff, &accel_data_diff_mg);

          /* Validate the results of self test
           * Self test value of x,y axes should be 800mg
           * and z axes should be 400 mg
           */
          if (((accel_data_diff_mg.x) > BMA400_ST_ACC_X_AXIS_SIGNAL_DIFF) &&
              ((accel_data_diff_mg.y) > BMA400_ST_ACC_Y_AXIS_SIGNAL_DIFF) &&
              ((accel_data_diff_mg.z) > BMA400_ST_ACC_Z_AXIS_SIGNAL_DIFF))
          {
            /*
             *   if (((accel_pos->x - accel_neg->x) > 205) && ((accel_pos->y - accel_neg->y) > 205) &&
             *          ((accel_pos->z - accel_neg->z) > 103))
             */

            /* Self test pass condition */
            rslt = BMA400_OK;
          }
          else
          {
            /* Self test failed */
            rslt = BMA400_W_SELF_TEST_FAIL;
          }
        }
      }
    }
  }

  /* Check to ensure bus error does not occur */
  if (rslt >= BMA400_OK)
  {
    /* Store the status of self test result */
    self_test_rslt = rslt;

    /* Perform soft reset */
    rslt = BMA400_soft_reset(eIntfc);
  }

  /* Check to ensure bus operations are success */
  if (rslt == BMA400_OK)
  {
    /* Restore self_test_rslt as return value */
    rslt = self_test_rslt;
  }

  return rslt;
}

/* ########################################################################## */
/*                        Local functions definitions                         */
/* ########################################################################## */

/* @details This API is used to define sensor settings such as:
 *    - Accelerometer configuration (Like ODR,OSR,range...)
 *    - Tap configuration
 *    - Activity change configuration
 *    - Gen1/Gen2 configuration
 *    - Orientation change configuration
 *    - Step counter configuration
 *
 * @param[in] conf         : Structure instance of the configuration structure
 * @param[in] n_sett       : Number of settings to be set
 * @param[in] dev          : Structure instance of bma400_dev
 *
 * @note Before calling this API, fill in the value of the required configurations in the conf structure
 * (Examples are mentioned in the readme.md).
 *
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
static int8_t set_sensor_conf(const struct bma400_sensor_conf *conf, uint16_t n_sett, const eBMA400_Intfc eIntfc)
{
  int8_t rslt;
  uint16_t idx = 0;
  uint8_t data_array[3] = { 0 };

  /* Read the interrupt pin mapping configurations */
  rslt = get_regs(BMA400_INT_MAP_ADDR, data_array, 3, eIntfc);
  if (rslt == BMA400_OK)
  {
    for (idx = 0; idx < n_sett; idx++)
    {
      switch (conf[idx].type)
      {
        case BMA400_ACCEL:
          /* Setting Accel configurations */
          rslt = set_accel_conf(&conf[idx].param.accel, eIntfc);
          if (rslt == BMA400_OK)
          {
            /* Int pin mapping settings */
            map_int_pin(data_array, BMA400_DATA_READY_INT_MAP, conf[idx].param.accel.int_chan);
          }
          break;
        case BMA400_TAP_INT:
          /* Setting TAP configurations */
          rslt = set_tap_conf(&conf[idx].param.tap, eIntfc);
          if (rslt == BMA400_OK)
          {
              /* Int pin mapping settings */
              map_int_pin(data_array, BMA400_TAP_INT_MAP, conf[idx].param.tap.int_chan);
          }
          break;
        case BMA400_ACTIVITY_CHANGE_INT:
          /* Setting activity change config */
          rslt = set_activity_change_conf(&conf[idx].param.act_ch, eIntfc);
          if (rslt == BMA400_OK)
          {
              /* Int pin mapping settings */
              map_int_pin(data_array, BMA400_ACT_CH_INT_MAP, conf[idx].param.act_ch.int_chan);
          }
          break;
        case BMA400_GEN1_INT:
          /* Setting Generic int 1 config */
          rslt = set_gen1_int(&conf[idx].param.gen_int, eIntfc);
          if (rslt == BMA400_OK)
          {
              /* Int pin mapping settings */
              map_int_pin(data_array, BMA400_GEN1_INT_MAP, conf[idx].param.gen_int.int_chan);
          }
          break;
        case BMA400_GEN2_INT:
          /* Setting Generic int 2 config */
          rslt = set_gen2_int(&conf[idx].param.gen_int, eIntfc);
          if (rslt == BMA400_OK)
          {
              /* Int pin mapping settings */
              map_int_pin(data_array, BMA400_GEN2_INT_MAP, conf[idx].param.gen_int.int_chan);
          }
          break;
        case BMA400_ORIENT_CHANGE_INT:
          /* Setting orient int config */
          rslt = set_orient_int(&conf[idx].param.orient, eIntfc);
          if (rslt == BMA400_OK)
          {
              /* Int pin mapping settings */
              map_int_pin(data_array, BMA400_ORIENT_CH_INT_MAP, conf[idx].param.orient.int_chan);
          }
          break;
        case BMA400_STEP_COUNTER_INT:

            /* Int pin mapping settings */
            map_int_pin(data_array, BMA400_STEP_INT_MAP, conf[idx].param.step_cnt.int_chan);
            break;
      }
    }

    if (rslt == BMA400_OK)
    {
      /* Set the interrupt pin mapping configurations */
      rslt = set_regs(BMA400_INT_MAP_ADDR, data_array, 3, eIntfc);
    }
  }

  return rslt;
}


/*
 * @details This API reads the data from the given register address of sensor.
 *
 * @param[in] reg_addr  : Register address from where the data to be read
 * @param[out] reg_data : Pointer to data buffer to store the read data.
 * @param[in] len       : No of bytes of data to be read.
 * @param[in] dev       : Structure instance of bma400_dev.
 *
 * @note Auto increment applies to most of the registers, with the
 * exception of a few registers that trap the address. For e.g.,
 * Register address - 0x14(BMA400_FIFO_DATA_ADDR)
 *
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
static int8_t get_regs(uint8_t reg_addr, uint8_t *reg_data, uint8_t len, const eBMA400_Intfc eIntfc)
{
  int8_t rslt;
  uint16_t index;
  uint16_t temp_len = len + 0x00; /* was dev->dummy_byte before */
  uint8_t temp_buff[temp_len];
  uint8_t idx;
  stBMA400_Com_t * pCom;

  /* Initialize locals */
  pCom = NULL;
  rslt = BMA400_OK;

  /* Proceed if null check is fine */
  if ((rslt == BMA400_OK) && (reg_data != NULL))
  {
    if (eIntfc != eBMA400_Intfc_I2C0)
    {
      /* If interface selected is SPI */
      reg_addr = reg_addr | BMA400_SPI_RD_MASK;
    }

    /* Get the address */
    for(idx=0;idx<mArraySize(stDesc);idx++)
    {
      if( eIntfc == stDesc[idx].intf )
      {
        break;
      }
    }

    pCom = stDesc[idx].pstCom;

    /* Rea
     * d the data from the reg_addr */
    if (i2cTransferInProgress != I2C_MasterRead(pCom->address, reg_addr, temp_buff, temp_len))
    {
      for (index = 0; index < len; index++)
      {
        /* Parse the data read and store in "reg_data"
         * buffer so that the dummy byte is removed
         * and user will get only valid data
         */
        reg_data[index] = temp_buff[index + 0x00]; /* was dev->dummy_byte before */
      }
    }
    else
    {
      /* Failure case */
      rslt = BMA400_E_COM_FAIL;
    }
  }
  else
  {
    rslt = BMA400_E_NULL_PTR;
  }

  return rslt;
}

/* @details This API writes the given data to the register address of the sensor.
 *
 * @param[in] reg_addr : Register address from where the data to be written.
 * @param[in] reg_data : Pointer to data buffer which is to be written
 *                       in the reg_addr of sensor.
 * @param[in] len      : No of bytes of data to write..
 * @param[in] dev      : Structure instance of bma400_dev.
 *
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
static int8_t set_regs(uint8_t reg_addr, uint8_t *reg_data, uint8_t len, const eBMA400_Intfc eIntfc)
{
  int8_t rslt;
  uint8_t count;
  uint8_t idx;
  stBMA400_Com_t * pCom;

  /* Initialize locals */
  pCom = NULL;
  rslt = BMA400_OK;

  /* Proceed if null check is fine */
  if ((rslt == BMA400_OK) && (reg_data != NULL))
  {
    /* Get the address */
    for(idx=0;idx<mArraySize(stDesc);idx++)
    {
      if( eIntfc == stDesc[idx].intf )
      {
        break;
      }
    }

    pCom = stDesc[idx].pstCom;

    /* SPI write requires to set The MSB of reg_addr as 0
     * but in default the MSB is always 0
     */
    if (len == 1)
    {
      if (i2cTransferInProgress != I2C_MasterWrite(pCom->address, reg_addr, reg_data, len))
      {
        /* Failure case */
        rslt = BMA400_OK;
      }
      else
      {
        /* Failure case */
        rslt = BMA400_E_COM_FAIL;
      }
    }

    /* Burst write is not allowed thus we split burst case write
     * into single byte writes Thus user can write multiple bytes
     * with ease
     */
    if (len > 1)
    {
      for (count = 0; count < len; count++)
      {
        if (i2cTransferInProgress != I2C_MasterWrite(pCom->address, reg_addr, &reg_data[count], 1))
        {
          /* Failure case */
          rslt = BMA400_OK;
        }
        else
        {
          /* Failure case */
          rslt = BMA400_E_COM_FAIL;
        }
        reg_addr++;
      }
    }
  }
  else
  {
    rslt = BMA400_E_NULL_PTR;
  }

  return rslt;
}

/*
 * @brief This internal API is used to set the accel configurations in sensor
 *
 * @param[in] accel_conf : Structure instance with accel configurations
 * @param[in] dev             : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t set_accel_conf(const struct bma400_acc_conf *accel_conf, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[3] = { 0, 0, 0xE0 };

    /* Update the accel configurations from the user structure
     * accel_conf
     */
    rslt = get_regs(BMA400_ACCEL_CONFIG_0_ADDR, data_array, 3, eIntfc);
    if (rslt == BMA400_OK)
    {
      data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_FILT_1_BW, accel_conf->filt1_bw);
      data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_OSR_LP, accel_conf->osr_lp);
      data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_ACCEL_RANGE, accel_conf->range);
      data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_OSR, accel_conf->osr);
      data_array[1] = BMA400_SET_BITS_POS_0(data_array[1], BMA400_ACCEL_ODR, accel_conf->odr);
      data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_DATA_FILTER, accel_conf->data_src);

      /* Set the accel configurations in the sensor */
#if 0
      rslt = set_regs(BMA400_ACCEL_CONFIG_0_ADDR, data_array, 3, eIntfc);
#else
      rslt = set_regs(BMA400_ACCEL_CONFIG_0_ADDR, &data_array[0], 1, eIntfc);
      rslt = set_regs(BMA400_ACCEL_CONFIG_1_ADDR, &data_array[1], 1, eIntfc);
      rslt = set_regs(BMA400_ACCEL_CONFIG_2_ADDR, &data_array[2], 1, eIntfc);
#endif
    }

    return rslt;
}

/*
 * @brief This internal API is used to get the accel configurations in sensor
 *
 * @param[in,out] accel_conf  : Structure instance of basic
 *                              accelerometer configuration
 * @param[in] dev             : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t get_accel_conf(struct bma400_acc_conf *accel_conf, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[3];

    rslt = get_regs(BMA400_ACCEL_CONFIG_0_ADDR, data_array, 3, eIntfc);
    if (rslt == BMA400_OK)
    {
        accel_conf->filt1_bw = BMA400_GET_BITS(data_array[0], BMA400_FILT_1_BW);
        accel_conf->osr_lp = BMA400_GET_BITS(data_array[0], BMA400_OSR_LP);
        accel_conf->range = BMA400_GET_BITS(data_array[1], BMA400_ACCEL_RANGE);
        accel_conf->osr = BMA400_GET_BITS(data_array[1], BMA400_OSR);
        accel_conf->odr = BMA400_GET_BITS_POS_0(data_array[1], BMA400_ACCEL_ODR);
        accel_conf->data_src = BMA400_GET_BITS(data_array[2], BMA400_DATA_FILTER);
    }

    return rslt;
}

/*
 * @brief This API reads accel data along with sensor time
 *
 * @param[in] data_sel   : Variable to select the data to be read
 * @param[in,out] accel  : Structure instance to store the accel data
 * @param[in] dev        : Structure instance of bma400_dev
 *
 * Assignable values for data_sel:
 *   - BMA400_DATA_ONLY
 *   - BMA400_DATA_SENSOR_TIME
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t get_accel_data(uint8_t data_sel, struct bma400_sensor_data *accel, const eBMA400_Intfc eIntfc)
{
  int8_t rslt;
  uint8_t data_array[9] = { 0 };
  uint16_t lsb;
  uint8_t msb;
  uint8_t time_0;
  uint16_t time_1;
  uint32_t time_2;

  if (data_sel == BMA400_DATA_ONLY)
  {
    /* Read the sensor data registers only */
    rslt = get_regs(BMA400_ACCEL_DATA_ADDR, data_array, 6, eIntfc);
  }
  else if (data_sel == BMA400_DATA_SENSOR_TIME)
  {
    /* Read the sensor data along with sensor time */
    rslt = get_regs(BMA400_ACCEL_DATA_ADDR, data_array, 9, eIntfc);
  }
  else
  {
    /* Invalid use of "data_sel" */
    rslt = BMA400_E_INVALID_CONFIG;
  }
  if (rslt == BMA400_OK)
  {
    lsb = data_array[0];
    msb = data_array[1];

    /* accel X axis data */
    accel->x = (int16_t)(((uint16_t)msb * 256) + lsb);
    if (accel->x > 2047)
    {
      /* Computing accel data negative value */
      accel->x = accel->x - 4096;
    }
    lsb = data_array[2];
    msb = data_array[3];

    /* accel Y axis data */
    accel->y = (int16_t)(((uint16_t)msb * 256) | lsb);
    if (accel->y > 2047)
    {
      /* Computing accel data negative value */
      accel->y = accel->y - 4096;
    }

    lsb = data_array[4];
    msb = data_array[5];

    /* accel Z axis data */
    accel->z = (int16_t)(((uint16_t)msb * 256) | lsb);
    if (accel->z > 2047)
    {
      /* Computing accel data negative value */
      accel->z = accel->z - 4096;
    }
    if (data_sel == BMA400_DATA_ONLY)
    {
      /* Update sensortime as 0 */
      accel->sensortime = 0;
    }
    if (data_sel == BMA400_DATA_SENSOR_TIME)
    {
      /* Sensor-time data*/
      time_0 = data_array[6];
      time_1 = ((uint16_t)data_array[7] << 8);
      time_2 = ((uint32_t)data_array[8] << 16);
      accel->sensortime = (uint32_t)(time_2 + time_1 + time_0);
    }
  }

  return rslt;
}

/*
 * @brief This API enables the auto-wakeup feature
 * of the sensor using a timeout value
 *
 * @param[in] wakeup_conf : Structure instance of wakeup configurations
 * @param[in] dev         : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t set_autowakeup_timeout(const struct bma400_auto_wakeup_conf *wakeup_conf, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[2];
    uint8_t lsb;
    uint8_t msb;

    rslt = get_regs(BMA400_AUTOWAKEUP_1_ADDR, &data_array[1], 1, eIntfc);
    if (rslt == BMA400_OK)
    {
        data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_WAKEUP_TIMEOUT, wakeup_conf->wakeup_timeout);

        /* LSB of timeout threshold */
        lsb = BMA400_GET_BITS_POS_0(wakeup_conf->timeout_thres, BMA400_WAKEUP_THRES_LSB);

        /* MSB of timeout threshold */
        msb = BMA400_GET_BITS(wakeup_conf->timeout_thres, BMA400_WAKEUP_THRES_MSB);

        /* Set the value in the data_array */
        data_array[0] = msb;
        data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_WAKEUP_TIMEOUT_THRES, lsb);
        rslt = set_regs(BMA400_AUTOWAKEUP_0_ADDR, data_array, 2, eIntfc);
    }

    return rslt;
}

/*
 * @brief This API gets the set sensor settings for auto-wakeup timeout feature
 *
 * @param[in,out] wakeup_conf : Structure instance of wake-up configurations
 * @param[in] dev             : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t get_autowakeup_timeout(struct bma400_auto_wakeup_conf *wakeup_conf, const eBMA400_Intfc eIntfc)
{
  int8_t rslt;
  uint8_t data_array[2];
  uint8_t lsb;
  uint8_t msb;

  if ( BMA400_OK == get_regs(BMA400_AUTOWAKEUP_0_ADDR, data_array, 2, eIntfc) )
  {
    wakeup_conf->wakeup_timeout = BMA400_GET_BITS(data_array[1], BMA400_WAKEUP_TIMEOUT);
    msb = data_array[0];
    lsb = BMA400_GET_BITS(data_array[1], BMA400_WAKEUP_TIMEOUT_THRES);

    /* Store the timeout value in the wakeup structure */
    wakeup_conf->timeout_thres = msb << 4 | lsb;
  }

  return rslt;
}

/*
 * @brief This API enables the auto-wakeup feature of the sensor
 *
 * @param[in] conf  : Configuration value to enable/disable
 *                    auto-wakeup interrupt
 * @param[in] dev   : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t set_auto_wakeup(uint8_t conf, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t reg_data;

    rslt = get_regs(BMA400_AUTOWAKEUP_1_ADDR, &reg_data, 1, eIntfc);
    if (rslt == BMA400_OK)
    {
        reg_data = BMA400_SET_BITS(reg_data, BMA400_WAKEUP_INTERRUPT, conf);

        /* Enabling the Auto wakeup interrupt */
        rslt = set_regs(BMA400_AUTOWAKEUP_1_ADDR, &reg_data, 1, eIntfc);
    }

    return rslt;
}

/*
 * @brief This API sets the parameters for auto-wakeup feature
 * of the sensor
 *
 * @param[in] wakeup_conf : Structure instance of wakeup configurations
 * @param[in] dev         : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t set_autowakeup_interrupt(const struct bma400_wakeup_conf *wakeup_conf, const eBMA400_Intfc eIntfc)
{
  int8_t rslt;
  uint8_t data_array[5] = { 0 };

  /* Set the wakeup reference update */
  data_array[0] = BMA400_SET_BITS_POS_0(data_array[0], BMA400_WKUP_REF_UPDATE, wakeup_conf->wakeup_ref_update);

  /* Set the number of samples for interrupt condition evaluation */
  data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_SAMPLE_COUNT, wakeup_conf->sample_count);

  /* Enable low power wake-up interrupt for X,Y,Z axes*/
  data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_WAKEUP_EN_AXES, wakeup_conf->wakeup_axes_en);

  /* Set interrupt threshold configuration  */
  data_array[1] = wakeup_conf->int_wkup_threshold;

  /* Set the reference acceleration x-axis for the wake-up interrupt */
  data_array[2] = wakeup_conf->int_wkup_ref_x;

  /* Set the reference acceleration y-axis for the wake-up interrupt */
  data_array[3] = wakeup_conf->int_wkup_ref_y;

  /* Set the reference acceleration z-axis for the wake-up interrupt */
  data_array[4] = wakeup_conf->int_wkup_ref_z;

  /* Set the wakeup interrupt configurations in the sensor */
  rslt = set_regs(BMA400_WAKEUP_INT_CONF_0_ADDR, data_array, 5, eIntfc);

  return rslt;
}

/*
 * @brief This API gets the set sensor settings for
 * auto-wakeup interrupt feature
 *
 * @param[in,out] wakeup_conf : Structure instance of wake-up configurations
 * @param[in] dev               : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t get_autowakeup_interrupt(struct bma400_wakeup_conf *wakeup_conf, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[5];

    rslt = get_regs(BMA400_WAKEUP_INT_CONF_0_ADDR, data_array, 5, eIntfc);
    if (rslt == BMA400_OK)
    {
        /* get the wakeup reference update */
        wakeup_conf->wakeup_ref_update = BMA400_GET_BITS_POS_0(data_array[0], BMA400_WKUP_REF_UPDATE);

        /* Get the number of samples for interrupt condition evaluation */
        wakeup_conf->sample_count = BMA400_GET_BITS(data_array[0], BMA400_SAMPLE_COUNT);

        /* Get the axes enabled */
        wakeup_conf->wakeup_axes_en = BMA400_GET_BITS(data_array[0], BMA400_WAKEUP_EN_AXES);

        /* Get interrupt threshold configuration  */
        wakeup_conf->int_wkup_threshold = data_array[1];

        /* Get the reference acceleration x-axis for the wake-up interrupt */
        wakeup_conf->int_wkup_ref_x = data_array[2];

        /* Get the reference acceleration y-axis for the wake-up interrupt */
        wakeup_conf->int_wkup_ref_y = data_array[3];

        /* Get the reference acceleration z-axis for the wake-up interrupt */
        wakeup_conf->int_wkup_ref_z = data_array[4];
    }

    return rslt;
}

/*
 * @brief This API sets the sensor to enter low power mode
 * automatically  based on the configurations
 *
 * @param[in] auto_lp_conf : Structure instance of auto-low power settings
 * @param[in] dev            : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t set_auto_low_power(const struct bma400_auto_lp_conf *auto_lp_conf, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t reg_data;
    uint8_t timeout_msb;
    uint8_t timeout_lsb;

    rslt = get_regs(BMA400_AUTO_LOW_POW_1_ADDR, &reg_data, 1, eIntfc);
    if (rslt == BMA400_OK)
    {
        reg_data = BMA400_SET_BITS_POS_0(reg_data, BMA400_AUTO_LOW_POW, auto_lp_conf->auto_low_power_trigger);

        /* If auto Low power timeout threshold is enabled */
        if (auto_lp_conf->auto_low_power_trigger & 0x0C)
        {
            rslt = get_regs(BMA400_AUTO_LOW_POW_0_ADDR, &timeout_msb, 1, eIntfc);
            if (rslt == BMA400_OK)
            {
                /* Compute the timeout threshold MSB value */
                timeout_msb = BMA400_GET_BITS(auto_lp_conf->auto_lp_timeout_threshold, BMA400_AUTO_LP_THRES);

                /* Compute the timeout threshold LSB value */
                timeout_lsb = BMA400_GET_BITS_POS_0(auto_lp_conf->auto_lp_timeout_threshold, BMA400_AUTO_LP_THRES_LSB);
                reg_data = BMA400_SET_BITS(reg_data, BMA400_AUTO_LP_TIMEOUT_LSB, timeout_lsb);

                /* Set the timeout threshold MSB value */
                rslt = set_regs(BMA400_AUTO_LOW_POW_0_ADDR, &timeout_msb, 1, eIntfc);
            }
        }
        if (rslt == BMA400_OK)
        {
            /* Set the Auto low power configurations */
            rslt = set_regs(BMA400_AUTO_LOW_POW_1_ADDR, &reg_data, 1, eIntfc);
        }
    }

    return rslt;
}

/*
 * @brief This API gets the sensor to get the auto-low
 * power mode configuration settings
 *
 * @param[in,out] auto_lp_conf : Structure instance of low power
 *                                 configurations
 * @param[in] dev                : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t get_auto_low_power(struct bma400_auto_lp_conf *auto_lp_conf, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[2];
    uint8_t timeout_msb;
    uint8_t timeout_lsb;

    rslt = get_regs(BMA400_AUTO_LOW_POW_0_ADDR, data_array, 2, eIntfc);
    if (rslt == BMA400_OK)
    {
        /* Get the auto low power trigger */
        auto_lp_conf->auto_low_power_trigger = BMA400_GET_BITS_POS_0(data_array[1], BMA400_AUTO_LOW_POW);
        timeout_msb = data_array[0];
        timeout_lsb = BMA400_GET_BITS(data_array[1], BMA400_AUTO_LP_TIMEOUT_LSB);

        /* Get the auto low power timeout threshold */
        auto_lp_conf->auto_lp_timeout_threshold = timeout_msb << 4 | timeout_lsb;
    }

    return rslt;
}

/*
 * @brief This API sets the tap setting parameters
 *
 * @param[in] tap_set : Structure instance of tap configurations
 * @param[in] dev     : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t set_tap_conf(const struct bma400_tap_conf *tap_set, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t reg_data[2] = { 0, 0 };

    rslt = get_regs(BMA400_TAP_CONFIG_ADDR, reg_data, 2, eIntfc);
    if (rslt == BMA400_OK)
    {
        /* Set the axis to sense for tap */
        reg_data[0] = BMA400_SET_BITS(reg_data[0], BMA400_TAP_AXES_EN, tap_set->axes_sel);

        /* Set the threshold for tap sensing */
        reg_data[0] = BMA400_SET_BITS_POS_0(reg_data[0], BMA400_TAP_SENSITIVITY, tap_set->sensitivity);

        /* Set the Quiet_dt setting */
        reg_data[1] = BMA400_SET_BITS(reg_data[1], BMA400_TAP_QUIET_DT, tap_set->quiet_dt);

        /* Set the Quiet setting */
        reg_data[1] = BMA400_SET_BITS(reg_data[1], BMA400_TAP_QUIET, tap_set->quiet);

        /* Set the tics_th setting */
        reg_data[1] = BMA400_SET_BITS_POS_0(reg_data[1], BMA400_TAP_TICS_TH, tap_set->tics_th);

        /* Set the TAP configuration in the sensor*/
        rslt = set_regs(BMA400_TAP_CONFIG_ADDR, reg_data, 2, eIntfc);
    }

    return rslt;
}

/*
 * @brief This API sets the tap setting parameters
 *
 * @param[in,out] tap_set : Structure instance of tap configurations
 * @param[in] dev         : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t get_tap_conf(struct bma400_tap_conf *tap_set, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t reg_data[2];

    rslt = get_regs(BMA400_TAP_CONFIG_ADDR, reg_data, 2, eIntfc);
    if (rslt == BMA400_OK)
    {
        /* Get the axis enabled for tap sensing */
        tap_set->axes_sel = BMA400_GET_BITS(reg_data[0], BMA400_TAP_AXES_EN);

        /* Get the threshold for tap sensing */
        tap_set->sensitivity = BMA400_GET_BITS_POS_0(reg_data[0], BMA400_TAP_SENSITIVITY);

        /* Get the Quiet_dt setting */
        tap_set->quiet_dt = BMA400_GET_BITS(reg_data[1], BMA400_TAP_QUIET_DT);

        /* Get the Quiet setting */
        tap_set->quiet = BMA400_GET_BITS(reg_data[1], BMA400_TAP_QUIET);

        /* Get the tics_th setting */
        tap_set->tics_th = BMA400_GET_BITS_POS_0(reg_data[1], BMA400_TAP_TICS_TH);
    }

    return rslt;
}

/*
 * @brief This API sets the parameters for activity change detection
 *
 * @param[in] act_ch_set : Structure instance of activity change
 *                         configurations
 * @param[in] dev        : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t set_activity_change_conf(const struct bma400_act_ch_conf *act_ch_set, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[2] = { 0 };

    /* Set the activity change threshold */
    data_array[0] = act_ch_set->act_ch_thres;

    /* Set the axis to sense for activity change */
    data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_ACT_CH_AXES_EN, act_ch_set->axes_sel);

    /* Set the data source for activity change */
    data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_ACT_CH_DATA_SRC, act_ch_set->data_source);

    /* Set the Number of sample points(NPTS)
     * for sensing activity change
     */
    data_array[1] = BMA400_SET_BITS_POS_0(data_array[1], BMA400_ACT_CH_NPTS, act_ch_set->act_ch_ntps);

    /* Set the Activity change configuration in the sensor*/
    rslt = set_regs(BMA400_ACT_CH_CONFIG_0_ADDR, data_array, 2, eIntfc);

    return rslt;
}

/*
 * @brief This API gets the parameters for activity change detection
 *
 * @param[in,out] act_ch_set : Structure instance of activity
 *                             change configurations
 * @param[in] dev            : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t get_activity_change_conf(struct bma400_act_ch_conf *act_ch_set, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[2];

    rslt = get_regs(BMA400_ACT_CH_CONFIG_0_ADDR, data_array, 2, eIntfc);
    if (rslt == BMA400_OK)
    {
        /* Get the activity change threshold */
        act_ch_set->act_ch_thres = data_array[0];

        /* Get the axis enabled for activity change detection */
        act_ch_set->axes_sel = BMA400_GET_BITS(data_array[1], BMA400_ACT_CH_AXES_EN);

        /* Get the data source for activity change */
        act_ch_set->data_source = BMA400_GET_BITS(data_array[1], BMA400_ACT_CH_DATA_SRC);

        /* Get the Number of sample points(NPTS)
         * for sensing activity change
         */
        act_ch_set->act_ch_ntps = BMA400_GET_BITS_POS_0(data_array[1], BMA400_ACT_CH_NPTS);
    }

    return rslt;
}

/*
 * @brief This API sets the parameters for generic interrupt1 configuration
 *
 * @param[in] gen_int_set : Structure instance of generic interrupt
 *                          configurations
 * @param[in] dev         : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t set_gen1_int(const struct bma400_gen_int_conf *gen_int_set, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[11] = { 0 };

    /* Set the axes to sense for interrupt */
    data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_AXES_EN, gen_int_set->axes_sel);

    /* Set the data source for interrupt */
    data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_DATA_SRC, gen_int_set->data_src);

    /* Set the reference update mode */
    data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_REFU, gen_int_set->ref_update);

    /* Set the hysteresis for interrupt calculation */
    data_array[0] = BMA400_SET_BITS_POS_0(data_array[0], BMA400_INT_HYST, gen_int_set->hysteresis);

    /* Set the criterion to generate interrupt on either
     * ACTIVITY OR INACTIVITY
     */
    data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_GEN_INT_CRITERION, gen_int_set->criterion_sel);

    /* Set the interrupt axes logic (AND/OR) for the
     * enabled axes to generate interrupt
     */
    data_array[1] = BMA400_SET_BITS_POS_0(data_array[1], BMA400_GEN_INT_COMB, gen_int_set->evaluate_axes);

    /* Set the interrupt threshold */
    data_array[2] = gen_int_set->gen_int_thres;

    /* Set the MSB of gen int dur */
    data_array[3] = BMA400_GET_MSB(gen_int_set->gen_int_dur);

    /* Set the LSB of gen int dur */
    data_array[4] = BMA400_GET_LSB(gen_int_set->gen_int_dur);

    /* Handling case of manual reference update */
    if (gen_int_set->ref_update == BMA400_MANUAL_UPDATE)
    {
        /* Set the LSB of reference x threshold */
        data_array[5] = BMA400_GET_LSB(gen_int_set->int_thres_ref_x);

        /* Set the MSB of reference x threshold */
        data_array[6] = BMA400_GET_MSB(gen_int_set->int_thres_ref_x);

        /* Set the LSB of reference y threshold */
        data_array[7] = BMA400_GET_LSB(gen_int_set->int_thres_ref_y);

        /* Set the MSB of reference y threshold */
        data_array[8] = BMA400_GET_MSB(gen_int_set->int_thres_ref_y);

        /* Set the LSB of reference z threshold */
        data_array[9] = BMA400_GET_LSB(gen_int_set->int_thres_ref_z);

        /* Set the MSB of reference z threshold */
        data_array[10] = BMA400_GET_MSB(gen_int_set->int_thres_ref_z);

        /* Set the GEN1 INT configuration in the sensor */
        rslt = set_regs(BMA400_GEN1_INT_CONFIG_ADDR, data_array, 11, eIntfc);
    }
    else
    {
        /* Set the GEN1 INT configuration in the sensor */
        rslt = set_regs(BMA400_GEN1_INT_CONFIG_ADDR, data_array, 5, eIntfc);
    }

    return rslt;
}

/*
 * @brief This API gets the generic interrupt1 configuration
 *
 * @param[in,out] gen_int_set : Structure instance of generic
 *                               interrupt configurations
 * @param[in] dev             : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t get_gen1_int(struct bma400_gen_int_conf *gen_int_set, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[11];

    rslt = get_regs(BMA400_GEN1_INT_CONFIG_ADDR, data_array, 11, eIntfc);
    if (rslt == BMA400_OK)
    {
        /* Get the axes to sense for interrupt */
        gen_int_set->axes_sel = BMA400_GET_BITS(data_array[0], BMA400_INT_AXES_EN);

        /* Get the data source for interrupt */
        gen_int_set->data_src = BMA400_GET_BITS(data_array[0], BMA400_INT_DATA_SRC);

        /* Get the reference update mode */
        gen_int_set->ref_update = BMA400_GET_BITS(data_array[0], BMA400_INT_REFU);

        /* Get the hysteresis for interrupt calculation */
        gen_int_set->hysteresis = BMA400_GET_BITS_POS_0(data_array[0], BMA400_INT_HYST);

        /* Get the interrupt axes logic (AND/OR) to generate interrupt */
        gen_int_set->evaluate_axes = BMA400_GET_BITS_POS_0(data_array[1], BMA400_GEN_INT_COMB);

        /* Get the criterion to generate interrupt ACTIVITY/INACTIVITY */
        gen_int_set->criterion_sel = BMA400_GET_BITS(data_array[1], BMA400_GEN_INT_CRITERION);

        /* Get the interrupt threshold */
        gen_int_set->gen_int_thres = data_array[2];

        /* Get the interrupt duration */
        gen_int_set->gen_int_dur = ((uint16_t)data_array[3] << 8) | data_array[4];

        /* Get the interrupt threshold */
        data_array[6] = data_array[6] & 0x0F;
        gen_int_set->int_thres_ref_x = ((uint16_t)data_array[6] << 8) | data_array[5];
        data_array[8] = data_array[8] & 0x0F;
        gen_int_set->int_thres_ref_y = ((uint16_t)data_array[8] << 8) | data_array[7];
        data_array[10] = data_array[10] & 0x0F;
        gen_int_set->int_thres_ref_z = ((uint16_t)data_array[10] << 8) | data_array[9];
    }

    return rslt;
}

/*
 * @brief This API sets the parameters for generic interrupt2 configuration
 *
 * @param[in] gen_int_set : Structure instance of generic interrupt
 *                          configurations
 * @param[in] dev         : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t set_gen2_int(const struct bma400_gen_int_conf *gen_int_set, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[11] = { 0 };

    /* Set the axes to sense for interrupt */
    data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_AXES_EN, gen_int_set->axes_sel);

    /* Set the data source for interrupt */
    data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_DATA_SRC, gen_int_set->data_src);

    /* Set the reference update mode */
    data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_REFU, gen_int_set->ref_update);

    /* Set the hysteresis for interrupt calculation */
    data_array[0] = BMA400_SET_BITS_POS_0(data_array[0], BMA400_INT_HYST, gen_int_set->hysteresis);

    /* Set the criterion to generate interrupt on either
     * ACTIVITY OR INACTIVITY
     */
    data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_GEN_INT_CRITERION, gen_int_set->criterion_sel);

    /* Set the interrupt axes logic (AND/OR) for the
     * enabled axes to generate interrupt
     */
    data_array[1] = BMA400_SET_BITS_POS_0(data_array[1], BMA400_GEN_INT_COMB, gen_int_set->evaluate_axes);

    /* Set the interrupt threshold */
    data_array[2] = gen_int_set->gen_int_thres;

    /* Set the MSB of gen int dur */
    data_array[3] = BMA400_GET_MSB(gen_int_set->gen_int_dur);

    /* Set the LSB of gen int dur */
    data_array[4] = BMA400_GET_LSB(gen_int_set->gen_int_dur);

    /* Handling case of manual reference update */
    if (gen_int_set->ref_update == BMA400_MANUAL_UPDATE)
    {
        /* Set the LSB of reference x threshold */
        data_array[5] = BMA400_GET_LSB(gen_int_set->int_thres_ref_x);

        /* Set the MSB of reference x threshold */
        data_array[6] = BMA400_GET_MSB(gen_int_set->int_thres_ref_x);

        /* Set the LSB of reference y threshold */
        data_array[7] = BMA400_GET_LSB(gen_int_set->int_thres_ref_y);

        /* Set the MSB of reference y threshold */
        data_array[8] = BMA400_GET_MSB(gen_int_set->int_thres_ref_y);

        /* Set the LSB of reference z threshold */
        data_array[9] = BMA400_GET_LSB(gen_int_set->int_thres_ref_z);

        /* Set the MSB of reference z threshold */
        data_array[10] = BMA400_GET_MSB(gen_int_set->int_thres_ref_z);

        /* Set the GEN2 INT configuration in the sensor */
        rslt = set_regs(BMA400_GEN2_INT_CONFIG_ADDR, data_array, 11, eIntfc);
    }
    else
    {
        /* Set the GEN2 INT configuration in the sensor */
        rslt = set_regs(BMA400_GEN2_INT_CONFIG_ADDR, data_array, 5, eIntfc);
    }

    return rslt;
}

/*
 * @brief This API gets the generic interrupt2 configuration
 *
 * @param[in,out] gen_int_set : Structure instance of generic
 *                              interrupt configurations
 * @param[in] dev             : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t get_gen2_int(struct bma400_gen_int_conf *gen_int_set, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[11];

    rslt = get_regs(BMA400_GEN2_INT_CONFIG_ADDR, data_array, 11, eIntfc);
    if (rslt == BMA400_OK)
    {
        /* Get the axes to sense for interrupt */
        gen_int_set->axes_sel = BMA400_GET_BITS(data_array[0], BMA400_INT_AXES_EN);

        /* Get the data source for interrupt */
        gen_int_set->data_src = BMA400_GET_BITS(data_array[0], BMA400_INT_DATA_SRC);

        /* Get the reference update mode */
        gen_int_set->ref_update = BMA400_GET_BITS(data_array[0], BMA400_INT_REFU);

        /* Get the hysteresis for interrupt calculation */
        gen_int_set->hysteresis = BMA400_GET_BITS_POS_0(data_array[0], BMA400_INT_HYST);

        /* Get the interrupt axes logic (AND/OR) to generate interrupt */
        gen_int_set->evaluate_axes = BMA400_GET_BITS_POS_0(data_array[1], BMA400_GEN_INT_COMB);

        /* Get the criterion to generate interrupt ACTIVITY/INACTIVITY */
        gen_int_set->criterion_sel = BMA400_GET_BITS(data_array[1], BMA400_GEN_INT_CRITERION);

        /* Get the interrupt threshold */
        gen_int_set->gen_int_thres = data_array[2];

        /* Get the interrupt duration */
        gen_int_set->gen_int_dur = ((uint16_t)data_array[3] << 8) | data_array[4];

        /* Get the interrupt threshold */
        data_array[6] = data_array[6] & 0x0F;
        gen_int_set->int_thres_ref_x = ((uint16_t)data_array[6] << 8) | data_array[5];
        data_array[8] = data_array[8] & 0x0F;
        gen_int_set->int_thres_ref_y = ((uint16_t)data_array[8] << 8) | data_array[7];
        data_array[10] = data_array[10] & 0x0F;
        gen_int_set->int_thres_ref_z = ((uint16_t)data_array[10] << 8) | data_array[9];
    }

    return rslt;
}

/*
 * @brief This API sets the parameters for orientation interrupt
 *
 * @param[in] orient_conf : Structure instance of orient interrupt
 *                          configurations
 * @param[in] dev         : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t set_orient_int(const struct bma400_orient_int_conf *orient_conf, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[10] = { 0 };

    /* Set the axes to sense for interrupt */
    data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_AXES_EN, orient_conf->axes_sel);

    /* Set the data source for interrupt */
    data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_DATA_SRC, orient_conf->data_src);

    /* Set the reference update mode */
    data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_INT_REFU, orient_conf->ref_update);

    /* Set the stability_mode for interrupt calculation */
    data_array[0] = BMA400_SET_BITS_POS_0(data_array[0], BMA400_STABILITY_MODE, orient_conf->stability_mode);

    /* Set the threshold for interrupt calculation */
    data_array[1] = orient_conf->orient_thres;

    /* Set the stability threshold */
    data_array[2] = orient_conf->stability_thres;

    /* Set the interrupt duration */
    data_array[3] = orient_conf->orient_int_dur;

    /* Handling case of manual reference update */
    if (orient_conf->ref_update == BMA400_MANUAL_UPDATE)
    {
        /* Set the LSB of reference x threshold */
        data_array[4] = BMA400_GET_LSB(orient_conf->orient_ref_x);

        /* Set the MSB of reference x threshold */
        data_array[5] = BMA400_GET_MSB(orient_conf->orient_ref_x);

        /* Set the MSB of reference x threshold */
        data_array[6] = BMA400_GET_LSB(orient_conf->orient_ref_y);

        /* Set the LSB of reference y threshold */
        data_array[7] = BMA400_GET_MSB(orient_conf->orient_ref_y);

        /* Set the MSB of reference y threshold */
        data_array[8] = BMA400_GET_LSB(orient_conf->orient_ref_z);

        /* Set the LSB of reference z threshold */
        data_array[9] = BMA400_GET_MSB(orient_conf->orient_ref_z);

        /* Set the orient configurations in the sensor */
        rslt = set_regs(BMA400_ORIENTCH_INT_CONFIG_ADDR, data_array, 10, eIntfc);
    }
    else
    {
        /* Set the orient configurations in the sensor excluding
         * reference values of x,y,z
         */
        rslt = set_regs(BMA400_ORIENTCH_INT_CONFIG_ADDR, data_array, 4, eIntfc);
    }

    return rslt;
}

/*
 * @brief This API gets the parameters for orientation interrupt
 *
 * @param[in,out] orient_conf : Structure instance of orient
 *                              interrupt configurations
 * @param[in] dev             : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t get_orient_int(struct bma400_orient_int_conf *orient_conf, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[10];

    rslt = get_regs(BMA400_ORIENTCH_INT_CONFIG_ADDR, data_array, 10, eIntfc);
    if (rslt == BMA400_OK)
    {
        /* Get the axes to sense for interrupt */
        orient_conf->axes_sel = BMA400_GET_BITS(data_array[0], BMA400_INT_AXES_EN);

        /* Get the data source for interrupt */
        orient_conf->data_src = BMA400_GET_BITS(data_array[0], BMA400_INT_DATA_SRC);

        /* Get the reference update mode */
        orient_conf->ref_update = BMA400_GET_BITS(data_array[0], BMA400_INT_REFU);

        /* Get the stability_mode for interrupt calculation */
        orient_conf->stability_mode = BMA400_GET_BITS_POS_0(data_array[0], BMA400_STABILITY_MODE);

        /* Get the threshold for interrupt calculation */
        orient_conf->orient_thres = data_array[1];

        /* Get the stability threshold */
        orient_conf->stability_thres = data_array[2];

        /* Get the interrupt duration */
        orient_conf->orient_int_dur = data_array[3];

        /* Get the interrupt reference values */
        data_array[5] = data_array[5] & 0x0F;
        orient_conf->orient_ref_x = ((uint16_t)data_array[5] << 8) | data_array[4];
        data_array[5] = data_array[7] & 0x0F;
        orient_conf->orient_ref_y = ((uint16_t)data_array[7] << 8) | data_array[6];
        data_array[5] = data_array[9] & 0x0F;
        orient_conf->orient_ref_z = ((uint16_t)data_array[9] << 8) | data_array[8];
    }

    return rslt;
}

/*
 * @brief This API sets the selected interrupt to be mapped to
 * the hardware interrupt pin of the sensor
 *
 * @param[in,out] data_array  : Data array of interrupt pin configurations
 * @param[in] int_enable      : Interrupt selected for pin mapping
 * @param[in] int_map         : Interrupt channel to be mapped
 *
 * @return Nothing
 */
static void map_int_pin(uint8_t *data_array, uint8_t int_enable, enum bma400_int_chan int_map)
{
    switch (int_enable)
    {
        case BMA400_DATA_READY_INT_MAP:
            if (int_map == BMA400_INT_CHANNEL_1)
            {
                /* Mapping interrupt to INT pin 1*/
                data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_DRDY, BMA400_ENABLE);
            }
            if (int_map == BMA400_INT_CHANNEL_2)
            {
                /* Mapping interrupt to INT pin 2*/
                data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_DRDY, BMA400_ENABLE);
            }
            if (int_map == BMA400_UNMAP_INT_PIN)
            {
                data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_DRDY);
                data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_DRDY);
            }
            if (int_map == BMA400_MAP_BOTH_INT_PINS)
            {
                data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_DRDY, BMA400_ENABLE);
                data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_DRDY, BMA400_ENABLE);
            }
            break;
        case BMA400_FIFO_WM_INT_MAP:
            if (int_map == BMA400_INT_CHANNEL_1)
            {
                /* Mapping interrupt to INT pin 1*/
                data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_FIFO_WM, BMA400_ENABLE);
            }
            if (int_map == BMA400_INT_CHANNEL_2)
            {
                /* Mapping interrupt to INT pin 2*/
                data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_FIFO_WM, BMA400_ENABLE);
            }
            if (int_map == BMA400_UNMAP_INT_PIN)
            {
                data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_FIFO_WM);
                data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_FIFO_WM);
            }
            if (int_map == BMA400_MAP_BOTH_INT_PINS)
            {
                data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_FIFO_WM, BMA400_ENABLE);
                data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_FIFO_WM, BMA400_ENABLE);
            }
            break;
        case BMA400_FIFO_FULL_INT_MAP:
            if (int_map == BMA400_INT_CHANNEL_1)
            {
                /* Mapping interrupt to INT pin 1 */
                data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_FIFO_FULL, BMA400_ENABLE);
            }
            if (int_map == BMA400_INT_CHANNEL_2)
            {
                /* Mapping interrupt to INT pin 2 */
                data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_FIFO_FULL, BMA400_ENABLE);
            }
            if (int_map == BMA400_UNMAP_INT_PIN)
            {
                data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_FIFO_FULL);
                data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_FIFO_FULL);
            }
            if (int_map == BMA400_MAP_BOTH_INT_PINS)
            {
                data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_FIFO_FULL, BMA400_ENABLE);
                data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_FIFO_FULL, BMA400_ENABLE);
            }
            break;
        case BMA400_INT_OVERRUN_MAP:
            if (int_map == BMA400_INT_CHANNEL_1)
            {
                /* Mapping interrupt to INT pin 1 */
                data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_INT_OVERRUN, BMA400_ENABLE);
            }
            if (int_map == BMA400_INT_CHANNEL_2)
            {
                /* Mapping interrupt to INT pin 2 */
                data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_INT_OVERRUN, BMA400_ENABLE);
            }
            if (int_map == BMA400_UNMAP_INT_PIN)
            {
                data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_INT_OVERRUN);
                data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_INT_OVERRUN);
            }
            if (int_map == BMA400_MAP_BOTH_INT_PINS)
            {
                data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_INT_OVERRUN, BMA400_ENABLE);
                data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_INT_OVERRUN, BMA400_ENABLE);
            }
            break;
        case BMA400_GEN2_INT_MAP:
            if (int_map == BMA400_INT_CHANNEL_1)
            {
                /* Mapping interrupt to INT pin 1 */
                data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_GEN2, BMA400_ENABLE);
            }
            if (int_map == BMA400_INT_CHANNEL_2)
            {
                /* Mapping interrupt to INT pin 2 */
                data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_GEN2, BMA400_ENABLE);
            }
            if (int_map == BMA400_UNMAP_INT_PIN)
            {
                data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_GEN2);
                data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_GEN2);
            }
            if (int_map == BMA400_MAP_BOTH_INT_PINS)
            {
                data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_GEN2, BMA400_ENABLE);
                data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_GEN2, BMA400_ENABLE);
            }
            break;
        case BMA400_GEN1_INT_MAP:
            if (int_map == BMA400_INT_CHANNEL_1)
            {
                /* Mapping interrupt to INT pin 1 */
                data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_GEN1, BMA400_ENABLE);
            }
            if (int_map == BMA400_INT_CHANNEL_2)
            {
                /* Mapping interrupt to INT pin 2 */
                data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_GEN1, BMA400_ENABLE);
            }
            if (int_map == BMA400_UNMAP_INT_PIN)
            {
                data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_GEN1);
                data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_GEN1);
            }
            if (int_map == BMA400_MAP_BOTH_INT_PINS)
            {
                data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_GEN1, BMA400_ENABLE);
                data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_GEN1, BMA400_ENABLE);
            }
            break;
        case BMA400_ORIENT_CH_INT_MAP:
            if (int_map == BMA400_INT_CHANNEL_1)
            {
                /* Mapping interrupt to INT pin 1 */
                data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_ORIENT_CH, BMA400_ENABLE);
            }
            if (int_map == BMA400_INT_CHANNEL_2)
            {
                /* Mapping interrupt to INT pin 2 */
                data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_ORIENT_CH, BMA400_ENABLE);
            }
            if (int_map == BMA400_UNMAP_INT_PIN)
            {
                data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_ORIENT_CH);
                data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_ORIENT_CH);
            }
            if (int_map == BMA400_MAP_BOTH_INT_PINS)
            {
                data_array[0] = BMA400_SET_BITS(data_array[0], BMA400_EN_ORIENT_CH, BMA400_ENABLE);
                data_array[1] = BMA400_SET_BITS(data_array[1], BMA400_EN_ORIENT_CH, BMA400_ENABLE);
            }
            break;
        case BMA400_WAKEUP_INT_MAP:
            if (int_map == BMA400_INT_CHANNEL_1)
            {
                /* Mapping interrupt to INT pin 1 */
                data_array[0] = BMA400_SET_BITS_POS_0(data_array[0], BMA400_EN_WAKEUP_INT, BMA400_ENABLE);
            }
            if (int_map == BMA400_INT_CHANNEL_2)
            {
                /* Mapping interrupt to INT pin 2 */
                data_array[1] = BMA400_SET_BITS_POS_0(data_array[1], BMA400_EN_WAKEUP_INT, BMA400_ENABLE);
            }
            if (int_map == BMA400_UNMAP_INT_PIN)
            {
                data_array[0] = BMA400_SET_BIT_VAL_0(data_array[0], BMA400_EN_WAKEUP_INT);
                data_array[1] = BMA400_SET_BIT_VAL_0(data_array[1], BMA400_EN_WAKEUP_INT);
            }
            if (int_map == BMA400_MAP_BOTH_INT_PINS)
            {
                data_array[0] = BMA400_SET_BITS_POS_0(data_array[0], BMA400_EN_WAKEUP_INT, BMA400_ENABLE);
                data_array[1] = BMA400_SET_BITS_POS_0(data_array[1], BMA400_EN_WAKEUP_INT, BMA400_ENABLE);
            }
            break;
        case BMA400_ACT_CH_INT_MAP:
            if (int_map == BMA400_INT_CHANNEL_1)
            {
                /* Mapping interrupt to INT pin 1 */
                data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_ACTCH_MAP_INT1, BMA400_ENABLE);
            }
            if (int_map == BMA400_INT_CHANNEL_2)
            {
                /* Mapping interrupt to INT pin 2 */
                data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_ACTCH_MAP_INT2, BMA400_ENABLE);
            }
            if (int_map == BMA400_UNMAP_INT_PIN)
            {
                data_array[2] = BMA400_SET_BIT_VAL_0(data_array[2], BMA400_ACTCH_MAP_INT1);
                data_array[2] = BMA400_SET_BIT_VAL_0(data_array[2], BMA400_ACTCH_MAP_INT2);
            }
            if (int_map == BMA400_MAP_BOTH_INT_PINS)
            {
                data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_ACTCH_MAP_INT1, BMA400_ENABLE);
                data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_ACTCH_MAP_INT2, BMA400_ENABLE);
            }
            break;
        case BMA400_TAP_INT_MAP:
            if (int_map == BMA400_INT_CHANNEL_1)
            {
                /* Mapping interrupt to INT pin 1 */
                data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_TAP_MAP_INT1, BMA400_ENABLE);
            }
            if (int_map == BMA400_INT_CHANNEL_2)
            {
                /* Mapping interrupt to INT pin 2 */
                data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_TAP_MAP_INT2, BMA400_ENABLE);
            }
            if (int_map == BMA400_UNMAP_INT_PIN)
            {
                data_array[2] = BMA400_SET_BIT_VAL_0(data_array[2], BMA400_TAP_MAP_INT1);
                data_array[2] = BMA400_SET_BIT_VAL_0(data_array[2], BMA400_TAP_MAP_INT2);
            }
            if (int_map == BMA400_MAP_BOTH_INT_PINS)
            {
                data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_TAP_MAP_INT1, BMA400_ENABLE);
                data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_TAP_MAP_INT2, BMA400_ENABLE);
            }
            break;
        case BMA400_STEP_INT_MAP:
            if (int_map == BMA400_INT_CHANNEL_1)
            {
                /* Mapping interrupt to INT pin 1 */
                data_array[2] = BMA400_SET_BITS_POS_0(data_array[2], BMA400_EN_STEP_INT, BMA400_ENABLE);
            }
            if (int_map == BMA400_INT_CHANNEL_2)
            {
                /* Mapping interrupt to INT pin 2 */
                data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_STEP_MAP_INT2, BMA400_ENABLE);
            }
            if (int_map == BMA400_UNMAP_INT_PIN)
            {
                data_array[2] = BMA400_SET_BIT_VAL_0(data_array[2], BMA400_EN_STEP_INT);
                data_array[2] = BMA400_SET_BIT_VAL_0(data_array[2], BMA400_STEP_MAP_INT2);
            }
            if (int_map == BMA400_MAP_BOTH_INT_PINS)
            {
                data_array[2] = BMA400_SET_BITS_POS_0(data_array[2], BMA400_EN_STEP_INT, BMA400_ENABLE);
                data_array[2] = BMA400_SET_BITS(data_array[2], BMA400_STEP_MAP_INT2, BMA400_ENABLE);
            }
            break;
        default:
            break;
    }
}

/*
 * @brief This API checks whether the interrupt is mapped to the INT pin1
 * or INT pin2 of the sensor
 *
 * @param[in] int_1_map     : Variable to denote whether the interrupt is
 *                            mapped to INT1 pin or not
 * @param[in] int_2_map     : Variable to denote whether the interrupt is
 *                            mapped to INT2 pin or not
 * @param[in,out] int_map   : Interrupt channel which is mapped
 *                            INT1/INT2/NONE/BOTH
 *
 * @return Nothing
 */
static void check_mapped_interrupts(uint8_t int_1_map, uint8_t int_2_map, enum bma400_int_chan *int_map)
{
  if ((int_1_map == BMA400_ENABLE) && (int_2_map == BMA400_DISABLE))
  {
    /* INT 1 mapped INT 2 not mapped */
    *int_map = BMA400_INT_CHANNEL_1;
  }
  if ((int_1_map == BMA400_DISABLE) && (int_2_map == BMA400_ENABLE))
  {
    /* INT 1 not mapped INT 2 mapped */
    *int_map = BMA400_INT_CHANNEL_2;
  }
  if ((int_1_map == BMA400_ENABLE) && (int_2_map == BMA400_ENABLE))
  {
    /* INT 1 ,INT 2 both mapped */
    *int_map = BMA400_MAP_BOTH_INT_PINS;
  }
  if ((int_1_map == BMA400_DISABLE) && (int_2_map == BMA400_DISABLE))
  {
    /* INT 1 ,INT 2 not mapped */
    *int_map = BMA400_UNMAP_INT_PIN;
  }
}

/*
 * @brief This API gets the selected interrupt and its mapping to
 * the hardware interrupt pin of the sensor
 *
 * @param[in,out] data_array  : Data array of interrupt pin configurations
 * @param[in] int_enable      : Interrupt selected for pin mapping
 * @param[out] int_map        : Interrupt channel which is mapped
 *
 * @return Nothing
 */
static void get_int_pin_map(const uint8_t *data_array, uint8_t int_enable, enum bma400_int_chan *int_map)
{
    uint8_t int_1_map;
    uint8_t int_2_map;

    switch (int_enable)
    {
        case BMA400_DATA_READY_INT_MAP:

            /* Interrupt 1 pin mapping status */
            int_1_map = BMA400_GET_BITS(data_array[0], BMA400_EN_DRDY);

            /* Interrupt 2 pin mapping status */
            int_2_map = BMA400_GET_BITS(data_array[1], BMA400_EN_DRDY);

            /* Check the mapped interrupt pins */
            check_mapped_interrupts(int_1_map, int_2_map, int_map);
            break;
        case BMA400_FIFO_WM_INT_MAP:

            /* Interrupt 1 pin mapping status */
            int_1_map = BMA400_GET_BITS(data_array[0], BMA400_EN_FIFO_WM);

            /* Interrupt 2 pin mapping status */
            int_2_map = BMA400_GET_BITS(data_array[1], BMA400_EN_FIFO_WM);

            /* Check the mapped interrupt pins */
            check_mapped_interrupts(int_1_map, int_2_map, int_map);
            break;
        case BMA400_FIFO_FULL_INT_MAP:

            /* Interrupt 1 pin mapping status */
            int_1_map = BMA400_GET_BITS(data_array[0], BMA400_EN_FIFO_FULL);

            /* Interrupt 2 pin mapping status */
            int_2_map = BMA400_GET_BITS(data_array[1], BMA400_EN_FIFO_FULL);

            /* Check the mapped interrupt pins */
            check_mapped_interrupts(int_1_map, int_2_map, int_map);
            break;
        case BMA400_GEN2_INT_MAP:

            /* Interrupt 1 pin mapping status */
            int_1_map = BMA400_GET_BITS(data_array[0], BMA400_EN_GEN2);

            /* Interrupt 2 pin mapping status */
            int_2_map = BMA400_GET_BITS(data_array[1], BMA400_EN_GEN2);

            /* Check the mapped interrupt pins */
            check_mapped_interrupts(int_1_map, int_2_map, int_map);
            break;
        case BMA400_GEN1_INT_MAP:

            /* Interrupt 1 pin mapping status */
            int_1_map = BMA400_GET_BITS(data_array[0], BMA400_EN_GEN1);

            /* Interrupt 2 pin mapping status */
            int_2_map = BMA400_GET_BITS(data_array[1], BMA400_EN_GEN1);

            /* Check the mapped interrupt pins */
            check_mapped_interrupts(int_1_map, int_2_map, int_map);
            break;
        case BMA400_ORIENT_CH_INT_MAP:

            /* Interrupt 1 pin mapping status */
            int_1_map = BMA400_GET_BITS(data_array[0], BMA400_EN_ORIENT_CH);

            /* Interrupt 2 pin mapping status */
            int_2_map = BMA400_GET_BITS(data_array[1], BMA400_EN_ORIENT_CH);

            /* Check the mapped interrupt pins */
            check_mapped_interrupts(int_1_map, int_2_map, int_map);
            break;
        case BMA400_WAKEUP_INT_MAP:

            /* Interrupt 1 pin mapping status */
            int_1_map = BMA400_GET_BITS_POS_0(data_array[0], BMA400_EN_WAKEUP_INT);

            /* Interrupt 2 pin mapping status */
            int_2_map = BMA400_GET_BITS_POS_0(data_array[1], BMA400_EN_WAKEUP_INT);

            /* Check the mapped interrupt pins */
            check_mapped_interrupts(int_1_map, int_2_map, int_map);
            break;
        case BMA400_ACT_CH_INT_MAP:

            /* Interrupt 1 pin mapping status */
            int_1_map = BMA400_GET_BITS(data_array[2], BMA400_ACTCH_MAP_INT1);

            /* Interrupt 2 pin mapping status */
            int_2_map = BMA400_GET_BITS(data_array[2], BMA400_ACTCH_MAP_INT2);

            /* Check the mapped interrupt pins */
            check_mapped_interrupts(int_1_map, int_2_map, int_map);
            break;
        case BMA400_TAP_INT_MAP:

            /* Interrupt 1 pin mapping status */
            int_1_map = BMA400_GET_BITS(data_array[2], BMA400_TAP_MAP_INT1);

            /* Interrupt 2 pin mapping status */
            int_2_map = BMA400_GET_BITS(data_array[2], BMA400_TAP_MAP_INT2);

            /* Check the mapped interrupt pins */
            check_mapped_interrupts(int_1_map, int_2_map, int_map);
            break;
        case BMA400_STEP_INT_MAP:

            /* Interrupt 1 pin mapping status */
            int_1_map = BMA400_GET_BITS_POS_0(data_array[2], BMA400_EN_STEP_INT);

            /* Interrupt 2 pin mapping status */
            int_2_map = BMA400_GET_BITS(data_array[2], BMA400_STEP_MAP_INT2);

            /* Check the mapped interrupt pins */
            check_mapped_interrupts(int_1_map, int_2_map, int_map);
            break;
        case BMA400_INT_OVERRUN_MAP:

            /* Interrupt 1 pin mapping status */
            int_1_map = BMA400_GET_BITS(data_array[0], BMA400_EN_INT_OVERRUN);

            /* Interrupt 2 pin mapping status */
            int_2_map = BMA400_GET_BITS(data_array[1], BMA400_EN_INT_OVERRUN);

            /* Check the mapped interrupt pins */
            check_mapped_interrupts(int_1_map, int_2_map, int_map);
            break;
        default:
            break;
    }
}

/*
 * @brief This API is used to set the interrupt pin configurations
 *
 * @param[in] int_conf     : Interrupt pin configuration
 * @param[in] dev          : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t set_int_pin_conf(struct bma400_int_pin_conf int_conf, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t reg_data;

    rslt = get_regs(BMA400_INT_12_IO_CTRL_ADDR, &reg_data, 1, eIntfc);
    if (rslt == BMA400_OK)
    {
        if (int_conf.int_chan == BMA400_INT_CHANNEL_1)
        {
            /* Setting interrupt pin configurations */
            reg_data = BMA400_SET_BITS(reg_data, BMA400_INT_PIN1_CONF, int_conf.pin_conf);
        }
        if (int_conf.int_chan == BMA400_INT_CHANNEL_2)
        {
            /* Setting interrupt pin configurations */
            reg_data = BMA400_SET_BITS(reg_data, BMA400_INT_PIN2_CONF, int_conf.pin_conf);
        }

        /* Set the configurations in the sensor */
        rslt = set_regs(BMA400_INT_12_IO_CTRL_ADDR, &reg_data, 1, eIntfc);
    }

    return rslt;
}

/*
 * @brief This API is used to set the interrupt pin configurations
 *
 * @param[in,out] int_conf     : Interrupt pin configuration
 * @param[in] dev              : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t get_int_pin_conf(struct bma400_int_pin_conf *int_conf, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t reg_data;

    rslt = get_regs(BMA400_INT_12_IO_CTRL_ADDR, &reg_data, 1, eIntfc);
    if (rslt == BMA400_OK)
    {
        if (int_conf->int_chan == BMA400_INT_CHANNEL_1)
        {
            /* reading Interrupt pin configurations */
            int_conf->pin_conf = BMA400_GET_BITS(reg_data, BMA400_INT_PIN1_CONF);
        }
        if (int_conf->int_chan == BMA400_INT_CHANNEL_2)
        {
            /* Setting interrupt pin configurations */
            int_conf->pin_conf = BMA400_GET_BITS(reg_data, BMA400_INT_PIN2_CONF);
        }
    }

    return rslt;
}

/*
 * @brief This API is used to get the FIFO configurations
 *
 * @param[in,out] fifo_conf       : Structure instance containing the FIFO
 *                                  configuration set in the sensor
 * @param[in] dev                 : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t get_fifo_conf(struct bma400_fifo_conf *fifo_conf, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[3];

    /* Proceed if null check is fine */
    if (rslt == BMA400_OK)
    {
        /* Get the FIFO configurations and water-mark
         * values from the sensor
         */
        rslt = get_regs(BMA400_FIFO_CONFIG_0_ADDR, data_array, 3, eIntfc);
        if (rslt == BMA400_OK)
        {
            /* Get the data of FIFO_CONFIG0 register */
            fifo_conf->conf_regs = data_array[0];

            /* Get the MSB of FIFO water-mark  */
            data_array[2] = BMA400_GET_BITS_POS_0(data_array[2], BMA400_FIFO_BYTES_CNT);

            /* FIFO water-mark value is stored */
            fifo_conf->fifo_watermark = ((uint16_t)data_array[2] << 8) | ((uint16_t)data_array[1]);
        }
    }

    return rslt;
}

/*
 * @brief This API is used to set the FIFO configurations
 *
 * @param[in,out] fifo_conf       : Structure instance containing the FIFO
 *                                  configuration set in the sensor
 * @param[in] dev                 : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t set_fifo_conf(const struct bma400_fifo_conf *fifo_conf, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[3];
    uint8_t sens_data[3];

    /* Proceed if null check is fine */
    if (rslt == BMA400_OK)
    {
        /* Get the FIFO configurations and water-mark
         * values from the sensor
         */
        rslt = get_regs(BMA400_FIFO_CONFIG_0_ADDR, sens_data, 3, eIntfc);
        if (rslt == BMA400_OK)
        {
            /* FIFO configurations */
            data_array[0] = fifo_conf->conf_regs;
            if (fifo_conf->conf_status == BMA400_DISABLE)
            {
                /* Disable the selected interrupt status */
                data_array[0] = sens_data[0] & (~data_array[0]);
            }

            /* FIFO water-mark values */
            data_array[1] = BMA400_GET_LSB(fifo_conf->fifo_watermark);
            data_array[2] = BMA400_GET_MSB(fifo_conf->fifo_watermark);
            data_array[2] = BMA400_GET_BITS_POS_0(data_array[2], BMA400_FIFO_BYTES_CNT);
            if ((data_array[1] == sens_data[1]) && (data_array[2] == sens_data[2]))
            {
                /* Set the FIFO configurations in the
                 * sensor excluding the watermark value
                 */
                rslt = set_regs(BMA400_FIFO_CONFIG_0_ADDR, data_array, 1, eIntfc);
            }
            else
            {
                /* Set the FIFO configurations in the sensor*/
                rslt = set_regs(BMA400_FIFO_CONFIG_0_ADDR, data_array, 3, eIntfc);
            }
        }
    }

    return rslt;
}

/*
 * @brief This API is used to get the number of bytes filled in FIFO
 *
 * @param[in,out] fifo_byte_cnt   : Number of bytes in the FIFO buffer
 *                                  actually filled by the sensor
 * @param[in] dev                 : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t get_fifo_length(uint16_t *fifo_byte_cnt, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t data_array[2] = { 0 };

    rslt = get_regs(BMA400_FIFO_LENGTH_ADDR, data_array, 2, eIntfc);
    if (rslt == BMA400_OK)
    {
        data_array[1] = BMA400_GET_BITS_POS_0(data_array[1], BMA400_FIFO_BYTES_CNT);

        /* Available data in FIFO is stored in fifo_byte_cnt*/
        *fifo_byte_cnt = ((uint16_t)data_array[1] << 8) | ((uint16_t)data_array[0]);
    }

    return rslt;
}

/*
 * @brief This API is used to read the FIFO of BMA400
 *
 * @param[in,out] fifo : Pointer to the fifo structure.
 *
 * @param[in] dev      : Structure instance of bma400_dev
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static int8_t read_fifo(const struct bma400_fifo_data *fifo, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint8_t reg_data;
    uint8_t fifo_addr = BMA400_FIFO_DATA_ADDR;
    uint8_t idx;
    stBMA400_Com_t * pCom;

    if (eIntfc != eBMA400_Intfc_I2C0)
    {
        /* SPI mask is added */
        fifo_addr = fifo_addr | BMA400_SPI_RD_MASK;
    }

    /* Read the FIFO enable bit */
    rslt = get_regs(BMA400_FIFO_READ_EN_ADDR, &reg_data, 1, eIntfc);
    if (rslt == BMA400_OK)
    {
      /* Get the address */
      for(idx=0;idx<mArraySize(stDesc);idx++)
      {
        if( eIntfc == stDesc[idx].intf )
        {
          break;
        }
      }

      pCom = stDesc[idx].pstCom;

      /* FIFO read disable bit */
      if (reg_data == 0)
      {
        /* Read FIFO Buffer since FIFO read is enabled */

        /* Read the data from the reg_addr */
        if (i2cTransferInProgress != I2C_MasterRead(pCom->address, fifo_addr, fifo->data, fifo->length))
        {
          rslt = BMA400_OK;
        }
        else
        {
          rslt = BMA400_E_COM_FAIL;
        }
      }
      else
      {
        /* Enable FIFO reading */
        reg_data = 0;
        rslt = set_regs(BMA400_FIFO_READ_EN_ADDR, &reg_data, 1, eIntfc);
        if (rslt == BMA400_OK)
        {
          /* Delay to enable the FIFO */
          USTIMER_Delay(1);

          /* Read FIFO Buffer since FIFO read is enabled*/
          if (i2cTransferInProgress != I2C_MasterRead(pCom->address, fifo_addr, fifo->data, fifo->length))
          {
            /* Disable FIFO reading */
            reg_data = 1;
            rslt = set_regs(BMA400_FIFO_READ_EN_ADDR, &reg_data, 1, eIntfc);
          }
        }
      }
    }

    return rslt;
}

/* @details This API is used to get the sensor settings like sensor
 * configurations and interrupt configurations and store
 * them in the corresponding structure instance.
 *
 * @param[in] conf         : Structure instance of the configuration structure
 * @param[in] n_sett       : Number of settings to be obtained
 * @param[in] dev          : Structure instance of bma400_dev.
 *
 * @note Once the API is called, the settings structure will be updated in the settings structure.
 *
 * @return Result of API execution status.
 * @retval Zero Success
 * @retval Postive Warning
 * @retval Negative Error
 */
static int8_t get_sensor_conf(struct bma400_sensor_conf *conf, uint16_t n_sett, const eBMA400_Intfc eIntfc)
{
    int8_t rslt;
    uint16_t idx;
    uint8_t data_array[3] = { 0 };

    /* Read the interrupt pin mapping configurations */
    rslt = get_regs(BMA400_INT_MAP_ADDR, data_array, 3, eIntfc);

    for (idx = 0; (idx < n_sett) && (rslt == BMA400_OK); idx++)
    {
        switch (conf[idx].type)
        {
            case BMA400_ACCEL:

                /* Accel configuration settings */
                rslt = get_accel_conf(&conf[idx].param.accel, eIntfc);
                if (rslt == BMA400_OK)
                {
                    /* Get the INT pin mapping */
                    get_int_pin_map(data_array, BMA400_DATA_READY_INT_MAP, &conf[idx].param.accel.int_chan);
                }
                break;
            case BMA400_TAP_INT:

                /* TAP configuration settings */
                rslt = get_tap_conf(&conf[idx].param.tap, eIntfc);
                if (rslt == BMA400_OK)
                {
                    /* Get the INT pin mapping */
                    get_int_pin_map(data_array, BMA400_TAP_INT_MAP, &conf[idx].param.tap.int_chan);
                }
                break;
            case BMA400_ACTIVITY_CHANGE_INT:

                /* Activity change configurations */
                rslt = get_activity_change_conf(&conf[idx].param.act_ch, eIntfc);
                if (rslt == BMA400_OK)
                {
                    /* Get the INT pin mapping */
                    get_int_pin_map(data_array, BMA400_ACT_CH_INT_MAP, &conf[idx].param.act_ch.int_chan);
                }
                break;
            case BMA400_GEN1_INT:

                /* Generic int1 configurations */
                rslt = get_gen1_int(&conf[idx].param.gen_int, eIntfc);
                if (rslt == BMA400_OK)
                {
                    /* Get the INT pin mapping */
                    get_int_pin_map(data_array, BMA400_GEN1_INT_MAP, &conf[idx].param.gen_int.int_chan);
                }
                break;
            case BMA400_GEN2_INT:

                /* Generic int2 configurations */
                rslt = get_gen2_int(&conf[idx].param.gen_int, eIntfc);
                if (rslt == BMA400_OK)
                {
                    /* Get the INT pin mapping */
                    get_int_pin_map(data_array, BMA400_GEN2_INT_MAP, &conf[idx].param.gen_int.int_chan);
                }
                break;
            case BMA400_ORIENT_CHANGE_INT:

                /* Orient int configurations */
                rslt = get_orient_int(&conf[idx].param.orient, eIntfc);
                if (rslt == BMA400_OK)
                {
                    /* Get the INT pin mapping */
                    get_int_pin_map(data_array, BMA400_ORIENT_CH_INT_MAP, &conf[idx].param.orient.int_chan);
                }
                break;
            case BMA400_STEP_COUNTER_INT:

                /* Get int pin mapping settings */
                get_int_pin_map(data_array, BMA400_STEP_INT_MAP, &conf[idx].param.step_cnt.int_chan);
                break;
            default:
                rslt = BMA400_E_INVALID_CONFIG;
        }
    }

    return rslt;
}

/*
 * @brief This API is used to check for a frame availability in FIFO
 *
 * @param[in,out] fifo            : Pointer to the fifo structure.
 * @param[in,out] frame_available : Variable to denote availability of a frame
 * @param[in] accel_width         : Variable to denote 12/8 bit accel data
 * @param[in] data_en             : Data enabled in FIFO
 * @param[in,out] data_index      : Index of the currently parsed FIFO data
 *
 * @return Nothing
 */
static void check_frame_available(const struct bma400_fifo_data *fifo,
                                  uint8_t *frame_available,
                                  uint8_t accel_width,
                                  uint8_t data_en,
                                  uint16_t *data_index)
{
    switch (data_en)
    {
        case BMA400_FIFO_XYZ_ENABLE:

            /* Handling case of 12 bit/ 8 bit data available in FIFO */
            if (accel_width == BMA400_12_BIT_FIFO_DATA)
            {
                if ((*data_index + 6) > fifo->length)
                {
                    /* Partial frame available */
                    *data_index = fifo->length;
                    *frame_available = BMA400_DISABLE;
                }
            }
            else if ((*data_index + 3) > fifo->length)
            {
                /* Partial frame available */
                *data_index = fifo->length;
                *frame_available = BMA400_DISABLE;
            }
            break;
        case BMA400_FIFO_X_ENABLE:
        case BMA400_FIFO_Y_ENABLE:
        case BMA400_FIFO_Z_ENABLE:

            /* Handling case of 12 bit/ 8 bit data available in FIFO */
            if (accel_width == BMA400_12_BIT_FIFO_DATA)
            {
                if ((*data_index + 2) > fifo->length)
                {
                    /* Partial frame available */
                    *data_index = fifo->length;
                    *frame_available = BMA400_DISABLE;
                }
            }
            else if ((*data_index + 1) > fifo->length)
            {
                /* Partial frame available */
                *data_index = fifo->length;
                *frame_available = BMA400_DISABLE;
            }
            break;
        case BMA400_FIFO_XY_ENABLE:
        case BMA400_FIFO_YZ_ENABLE:
        case BMA400_FIFO_XZ_ENABLE:

            /* Handling case of 12 bit/ 8 bit data available in FIFO */
            if (accel_width == BMA400_12_BIT_FIFO_DATA)
            {
                if ((*data_index + 4) > fifo->length)
                {
                    /* Partial frame available */
                    *data_index = fifo->length;
                    *frame_available = BMA400_DISABLE;
                }
            }
            else if ((*data_index + 2) > fifo->length)
            {
                /* Partial frame available */
                *data_index = fifo->length;
                *frame_available = BMA400_DISABLE;
            }
            break;
        case BMA400_FIFO_SENSOR_TIME:
            if ((*data_index + 3) > fifo->length)
            {
                /* Partial frame available */
                *data_index = fifo->length;
                *frame_available = BMA400_DISABLE;
            }
            break;
        case BMA400_FIFO_CONTROL_FRAME:
            if ((*data_index + 1) > fifo->length)
            {
                /* Partial frame available */
                *data_index = fifo->length;
                *frame_available = BMA400_DISABLE;
            }
            break;
        default:
            break;
    }
}

/*
 * @brief This API is used to unpack the accelerometer xyz data from the FIFO
 * and store it in the user defined buffer
 *
 * @param[in,out] fifo         : Pointer to the fifo structure.
 * @param[in,out] accel_data   : Structure instance to store the accel data
 * @param[in,out] data_index   : Index of the currently parsed FIFO data
 * @param[in] accel_width      : Variable to denote 12/8 bit accel data
 * @param[in] frame_header     : Variable to get the data enabled
 *
 * @return Nothing
 */
static void unpack_accel(const struct bma400_fifo_data *fifo,
                         struct bma400_sensor_data *accel_data,
                         uint16_t *data_index,
                         uint8_t accel_width,
                         uint8_t frame_header)
{
    uint8_t data_lsb;
    uint8_t data_msb;

    /* Header information of enabled axes */
    frame_header = frame_header & BMA400_FIFO_DATA_EN_MASK;
    if (accel_width == BMA400_12_BIT_FIFO_DATA)
    {
        if (frame_header & BMA400_FIFO_X_ENABLE)
        {
            /* Accel x data */
            data_lsb = fifo->data[(*data_index)++];
            data_msb = fifo->data[(*data_index)++];
            accel_data->x = (int16_t)(((uint16_t)(data_msb << 4)) | data_lsb);
            if (accel_data->x > 2047)
            {
                /* Computing accel x data negative value */
                accel_data->x = accel_data->x - 4096;
            }
        }
        else
        {
            /* Accel x not available */
            accel_data->x = 0;
        }
        if (frame_header & BMA400_FIFO_Y_ENABLE)
        {
            /* Accel y data */
            data_lsb = fifo->data[(*data_index)++];
            data_msb = fifo->data[(*data_index)++];
            accel_data->y = (int16_t)(((uint16_t)(data_msb << 4)) | data_lsb);
            if (accel_data->y > 2047)
            {
                /* Computing accel y data negative value */
                accel_data->y = accel_data->y - 4096;
            }
        }
        else
        {
            /* Accel y not available */
            accel_data->y = 0;
        }
        if (frame_header & BMA400_FIFO_Z_ENABLE)
        {
            /* Accel z data */
            data_lsb = fifo->data[(*data_index)++];
            data_msb = fifo->data[(*data_index)++];
            accel_data->z = (int16_t)(((uint16_t)(data_msb << 4)) | data_lsb);
            if (accel_data->z > 2047)
            {
                /* Computing accel z data negative value */
                accel_data->z = accel_data->z - 4096;
            }
        }
        else
        {
            /* Accel z not available */
            accel_data->z = 0;
        }
    }
    else
    {
        if (frame_header & BMA400_FIFO_X_ENABLE)
        {
            /* Accel x data */
            data_msb = fifo->data[(*data_index)++];
            accel_data->x = (int16_t)((uint16_t)(data_msb << 4));
            if (accel_data->x > 2047)
            {
                /* Computing accel x data negative value */
                accel_data->x = accel_data->x - 4096;
            }
        }
        else
        {
            /* Accel x not available */
            accel_data->x = 0;
        }
        if (frame_header & BMA400_FIFO_Y_ENABLE)
        {
            /* Accel y data */
            data_msb = fifo->data[(*data_index)++];
            accel_data->y = (int16_t)((uint16_t)(data_msb << 4));
            if (accel_data->y > 2047)
            {
                /* Computing accel y data negative value */
                accel_data->y = accel_data->y - 4096;
            }
        }
        else
        {
            /* Accel y not available */
            accel_data->y = 0;
        }
        if (frame_header & BMA400_FIFO_Z_ENABLE)
        {
            /* Accel z data */
            data_msb = fifo->data[(*data_index)++];
            accel_data->z = (int16_t)((uint16_t)(data_msb << 4));
            if (accel_data->z > 2047)
            {
                /* Computing accel z data negative value */
                accel_data->z = accel_data->z - 4096;
            }
        }
        else
        {
            /* Accel z not available */
            accel_data->z = 0;
        }
    }
}

/*
 * @brief This API is used to parse and store the sensor time from the
 * FIFO data in the structure instance dev
 *
 * @param[in,out] fifo         : Pointer to the fifo structure.
 * @param[in,out] data_index   : Index of the FIFO data which has sensor time
 *
 * @return Nothing
 */
static void unpack_sensortime_frame(struct bma400_fifo_data *fifo, uint16_t *data_index)
{
    uint32_t time_msb;
    uint16_t time_lsb;
    uint8_t time_xlsb;

    time_msb = fifo->data[(*data_index) + 2] << 16;
    time_lsb = fifo->data[(*data_index) + 1] << 8;
    time_xlsb = fifo->data[(*data_index)];

    /* Sensor time */
    fifo->fifo_sensor_time = (uint32_t)(time_msb | time_lsb | time_xlsb);
    *data_index = (*data_index) + 3;
}

/*
 *  @brief This API converts lsb value of axes to mg for self-test
 *
 *  @param[in] accel_data_diff : Pointer variable used to pass accel difference
 *  values in g
 *  @param[out] accel_data_diff_mg : Pointer variable used to store accel
 *  difference values in mg
 *
 *  @return None *
 */
static void convert_lsb_g(const struct selftest_delta_limit *accel_data_diff,
                          struct selftest_delta_limit *accel_data_diff_mg)
{
    uint32_t lsb_per_g;

    /* Range considered for self-test is 4g */
    uint8_t range = BMA400_4G_RANGE;

    /* lsb_per_g for the respective resolution and 8g range*/
    lsb_per_g = (uint32_t)(power(2, 8) / (2 * range));

    /* accel x value in mg */
    accel_data_diff_mg->x = (accel_data_diff->x / (int32_t)lsb_per_g) * 1000;

    /* accel y value in mg */
    accel_data_diff_mg->y = (accel_data_diff->y / (int32_t)lsb_per_g) * 1000;

    /* accel z value in mg */
    accel_data_diff_mg->z = (accel_data_diff->z / (int32_t)lsb_per_g) * 1000;
}

/*
 * @brief This API is used to calculate the power of given
 * base value.
 *
 * @param[in] base : value of base
 * @param[in] resolution : resolution of the sensor
 *
 * @return : return the value of base^resolution
 */
static int32_t power(int16_t base, uint8_t resolution)
{
    uint8_t i = 1;

    /* Initialize variable to store the power of 2 value */
    int32_t value = 1;

    for (; i <= resolution; i++)
    {
        value = (int32_t)(value * base);
    }

    return value;
}

static bool CHIPID_read(void)
{
  bool retVal;

  /* Initialise locals */
  i2c_txBuffer[0]=BMA400_CHIP_ID_ADDR;
  retVal = false;

  /* The length of read has to be > 2 for some obscur reason */   /* write only one reg address */
  if( i2cTransferInProgress != I2C_MasterRead(BMA400_I2C_ADDRESS_SDO_HIGH, i2c_txBuffer[0], i2c_rxBuffer, 1) )
  {
    retVal = true;
  }

  return retVal;
}

static bool CHIPID_write(const uint8_t chipid)
{
  bool retVal;

  /* Initialise locals */
  i2c_txBuffer[0]=0x77; /* New chip id */
  retVal = false;

  /* write only one reg address */
  if( i2cTransferInProgress != I2C_MasterWrite(BMA400_I2C_ADDRESS_SDO_HIGH, BMA400_CHIP_ID_ADDR, &i2c_txBuffer[0], 1))
  {
    retVal = true;
  }

  return retVal;
}

/*******************************************************************************
 * @brief I2C read numBytes from slave device starting at target address
 ******************************************************************************/
static I2C_TransferReturn_TypeDef I2C_MasterRead(uint16_t slaveAddress, uint8_t targetAddress, uint8_t *rxBuff, uint8_t numBytes)
{
  // Transfer structure
  I2C_TransferSeq_TypeDef i2cTransfer;
  I2C_TransferReturn_TypeDef result;

  /* Initializing I2C transfer */
  i2cTransfer.addr          = slaveAddress;
  i2cTransfer.flags         = I2C_FLAG_WRITE_READ; /*  */
  i2cTransfer.buf[0].data   = &targetAddress;
  i2cTransfer.buf[0].len    = 1;
  i2cTransfer.buf[1].data   = rxBuff;
  i2cTransfer.buf[1].len    = numBytes;

  result = I2C_TransferInit(I2C0, &i2cTransfer);

  /* Reading data */
  while (result == i2cTransferInProgress)
  {
    result = I2C_Transfer(I2C0);
  }

  return result;
}

/***************************************************************************//**
 * @brief I2C write numBytes to slave device starting at target address
 ******************************************************************************/
static I2C_TransferReturn_TypeDef I2C_MasterWrite(uint16_t slaveAddress,uint8_t regAddress, uint8_t *txBuff, uint8_t numBytes)
{
  // Transfer structure
  I2C_TransferSeq_TypeDef i2cTransfer;
  I2C_TransferReturn_TypeDef result;
  uint8_t txBuffer[I2C_BUFFER_SIZE + 1];

  /* Register address tp write to */
  txBuffer[0] = regAddress;
  for(unsigned char i = 0; i < numBytes; i++)
  {
    txBuffer[i + 1] = txBuff[i];
  }

  // Initializing I2C transfer
  i2cTransfer.addr          = slaveAddress;
  i2cTransfer.flags         = I2C_FLAG_WRITE;
  i2cTransfer.buf[0].data   = txBuffer;
  i2cTransfer.buf[0].len    = numBytes+1;
  i2cTransfer.buf[1].data   = NULL;
  i2cTransfer.buf[1].len    = 0;

  result = I2C_TransferInit(I2C0, &i2cTransfer);

  /* Sending data */
  while (result == i2cTransferInProgress)
  {
    result = I2C_Transfer(I2C0);
  }
  
  return result;
}
