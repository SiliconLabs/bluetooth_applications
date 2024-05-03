/***************************************************************************//**
 * @file main.c
 * @brief main() function.
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: LicenseRef-MSLA
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement
 * By installing, copying or otherwise using this software, you agree to the
 * terms of the MSLA.
 *
 ******************************************************************************/

#include "sl_component_catalog.h"
#include "sl_system_init.h"
#include "app.h"
#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
#include "sl_power_manager.h"
#endif // SL_CATALOG_POWER_MANAGER_PRESENT
#if defined(SL_CATALOG_KERNEL_PRESENT)
#include "sl_system_kernel.h"
#else // SL_CATALOG_KERNEL_PRESENT
#include "sl_system_process_action.h"
#endif // SL_CATALOG_KERNEL_PRESENT

#include <em_cmu.h>
#include <em_gpio.h>
#include <em_vdac.h>
#include <em_eusart.h>

#if 0

static void routeABUS(GPIO_Port_TypeDef port,
                      uint8_t pin,
                      uint8_t bus,
                      uint8_t what)
{
  volatile uint32_t *alloc_rd;
  volatile uint32_t *alloc_set;

  if (port == gpioPortA) {
    alloc_rd = &GPIO->ABUSALLOC;
    alloc_set = &GPIO->ABUSALLOC_SET;
  } else if (port == gpioPortB) {
    alloc_rd = &GPIO->BBUSALLOC;
    alloc_set = &GPIO->BBUSALLOC_SET;
  } else {
    alloc_rd = &GPIO->CDBUSALLOC;
    alloc_set = &GPIO->CDBUSALLOC_SET;
  }

  if (pin & 1) {
    uint32_t shift =
      (bus) ? _GPIO_ABUSALLOC_AODD1_SHIFT : _GPIO_ABUSALLOC_AODD0_SHIFT;
    uint32_t mask =
      (bus) ? _GPIO_ABUSALLOC_AODD1_MASK : _GPIO_ABUSALLOC_AODD0_MASK;
    uint32_t oddn = (*alloc_rd & mask) >> shift;

    // block if ODDn routed to other than TRISTATE or the requested peripheral
    *alloc_set = (what << shift) & mask;
  } else {
    uint32_t shift =
      (bus) ? _GPIO_ABUSALLOC_AEVEN1_SHIFT : _GPIO_ABUSALLOC_AEVEN0_SHIFT;
    uint32_t mask =
      (bus) ? _GPIO_ABUSALLOC_AEVEN1_MASK : _GPIO_ABUSALLOC_AEVEN0_MASK;
    uint32_t evenn = (*alloc_rd & mask) >> shift;

    // block if EVEN0 routed to other than TRISTATE or the requested peripheral
    *alloc_set = (what << shift) & mask;
  }
}

static void init_data_channel(void)
{
  VDAC_Init_TypeDef dacInit = VDAC_INIT_DEFAULT;
  VDAC_InitChannel_TypeDef initChannel = VDAC_INITCHANNEL_DEFAULT;

  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_VDAC1, false);
  CMU_ClockSelectSet(cmuClock_VDAC1, cmuSelect_FSRCO);
  CMU_ClockEnable(cmuClock_VDAC1, true);

  GPIO_PinModeSet(VDAC1_CH0_ABUS_OUT_PORT,
                  VDAC1_CH0_ABUS_OUT_PIN,
                  gpioModeDisabled,
                  0);
  routeABUS(VDAC1_CH0_ABUS_OUT_PORT,
            VDAC1_CH0_ABUS_OUT_PIN,
            0,
            _GPIO_ABUSALLOC_AODD0_VDAC1CH0);

  dacInit.biasKeepWarm = false;
  // 3.3V is not stable, so have to use an internal reference
  dacInit.reference = vdacRef2V5;
  dacInit.prescaler = VDAC_PrescaleCalc(VDAC1, 1000000);

  initChannel.enable = true;
  initChannel.mainOutEnable = false;
  initChannel.auxOutEnable = true;
  initChannel.port = VDAC1_CH0_ABUS_OUT_PORT + 1;
  initChannel.pin = VDAC1_CH0_ABUS_OUT_PIN;
  initChannel.powerMode = vdacPowerModeHighPower;
  initChannel.highCapLoadEnable = false;
  initChannel.warmupKeepOn = false;
  initChannel.sampleOffMode = false;

  VDAC_Init(VDAC1, &dacInit);
  VDAC_InitChannel(VDAC1, &initChannel, 0);
}

void BLE_DEBUG_CHANNEL(int channel)
{
  // 3.3V: 82.983 mV/step, 58.008 mV offset, max voltage is 3294.36 mV
  // VDAC_Channel0OutputSet(VDAC1, channel * 103 + 72);
  // 2.5V: 61.035 mV/step, 93.994 mV offset, max voltage is 2474.365 mV
  VDAC_Channel0OutputSet(VDAC1, channel * 100 + 154);
}

#endif

#if 0

static void init_data_channel(void)
{
  EUSART_SpiAdvancedInit_TypeDef data_if_adv = EUSART_SPI_ADVANCED_INIT_DEFAULT;
  EUSART_SpiInit_TypeDef data_if_config = EUSART_SPI_MASTER_INIT_DEFAULT_HF;

  CMU_ClockEnable(cmuClock_EUSART1, false);
  CMU_ClockSelectSet(cmuClock_EUSART1, cmuSelect_HFXO);
  CMU_ClockEnable(cmuClock_EUSART1, true);

  data_if_adv.autoCsEnable = false;
  data_if_adv.msbFirst = true;
  data_if_adv.TxFifoWatermark = eusartTxFiFoWatermark1Frame;

  data_if_config.enable = eusartEnableTx;
  data_if_config.databits = eusartDataBits8;
  data_if_config.bitRate = 39000000 / 4;
  data_if_config.clockMode = eusartClockMode1;
  data_if_config.advancedSettings = &data_if_adv;

  CMU_ClockEnable(cmuClock_GPIO, true);
  GPIO_PinModeSet(EUSART1_TX_PORT, EUSART1_TX_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(EUSART1_SCLK_PORT, EUSART1_SCLK_PIN, gpioModePushPull, 0);

  GPIO->EUSARTROUTE[1].ROUTEEN = 0;
  GPIO->EUSARTROUTE[1].TXROUTE =
    (EUSART1_TX_PORT << _GPIO_EUSART_TXROUTE_PORT_SHIFT)
    | (EUSART1_TX_PIN  << _GPIO_EUSART_TXROUTE_PIN_SHIFT);
  GPIO->EUSARTROUTE[1].SCLKROUTE =
    (EUSART1_SCLK_PORT << _GPIO_EUSART_SCLKROUTE_PORT_SHIFT)
    | (EUSART1_SCLK_PIN  << _GPIO_EUSART_SCLKROUTE_PIN_SHIFT);
  GPIO->EUSARTROUTE[1].ROUTEEN = GPIO_EUSART_ROUTEEN_TXPEN
                                 | GPIO_EUSART_ROUTEEN_SCLKPEN;

  EUSART_SpiInit(EUSART1, &data_if_config);
  // EUSART_RxBlock(EUSART2, eusartBlockRxEnable);
}

void BLE_DEBUG_CHANNEL(int channel)
{
  EUSART1->TXDATA = channel;
}

#endif

#if 1

static void init_data_channel(void)
{
}

void BLE_DEBUG_CHANNEL(int channel)
{
  (void)channel;
}

#endif

int main(void)
{
  // Initialize Silicon Labs device, system, service(s) and protocol stack(s).
  // Note that if the kernel is present, processing task(s) will be created by
  // this call.
  sl_system_init();

#if 0 && defined(CMU_CLKOUT2_PORT)
  GPIO_PinModeSet(CMU_CLKOUT2_PORT, CMU_CLKOUT2_PIN, gpioModePushPull, 0);
  BUS_RegBitWrite(&GPIO->CMUROUTE.ROUTEEN, _GPIO_CMU_ROUTEEN_CLKOUT2PEN_SHIFT,
                  0);
  // divider is n+1
  BUS_RegMaskedWrite(&CMU->EXPORTCLKCTRL,
                     _CMU_EXPORTCLKCTRL_PRESC_MASK,
                     15U << _CMU_EXPORTCLKCTRL_PRESC_SHIFT);
  BUS_RegMaskedWrite(&CMU->EXPORTCLKCTRL,
                     _CMU_EXPORTCLKCTRL_CLKOUTSEL2_MASK,
                     CMU_EXPORTCLKCTRL_CLKOUTSEL2_HFEXPCLK);
  GPIO->CMUROUTE.CLKOUT2ROUTE =
    (CMU_CLKOUT2_PORT << _GPIO_CMU_CLKOUT2ROUTE_PORT_SHIFT)
    | (CMU_CLKOUT2_PIN << _GPIO_CMU_CLKOUT2ROUTE_PIN_SHIFT);
  BUS_RegBitWrite(&GPIO->CMUROUTE.ROUTEEN, _GPIO_CMU_ROUTEEN_CLKOUT2PEN_SHIFT,
                  1);

  while (1) {}
#endif

#if 0 && defined(CMU_CLKOUT1_PORT)
  GPIO_PinModeSet(CMU_CLKOUT1_PORT, CMU_CLKOUT1_PIN, gpioModePushPull, 0);
  BUS_RegBitWrite(&GPIO->CMUROUTE.ROUTEEN, _GPIO_CMU_ROUTEEN_CLKOUT1PEN_SHIFT,
                  0);
  BUS_RegMaskedWrite(&CMU->EXPORTCLKCTRL,
                     _CMU_EXPORTCLKCTRL_CLKOUTSEL1_MASK,
                     CMU_EXPORTCLKCTRL_CLKOUTSEL1_LFXO);
  GPIO->CMUROUTE.CLKOUT1ROUTE =
    (CMU_CLKOUT1_PORT << _GPIO_CMU_CLKOUT1ROUTE_PORT_SHIFT)
    | (CMU_CLKOUT1_PIN << _GPIO_CMU_CLKOUT1ROUTE_PIN_SHIFT);
  BUS_RegBitWrite(&GPIO->CMUROUTE.ROUTEEN, _GPIO_CMU_ROUTEEN_CLKOUT1PEN_SHIFT,
                  1);
#endif

  init_data_channel();

  // Initialize the application. For example, create periodic timer(s) or
  // task(s) if the kernel is present.
  app_init();

#if defined(SL_CATALOG_KERNEL_PRESENT)
  // Start the kernel. Task(s) created in app_init() will start running.
  sl_system_kernel_start();
#else // SL_CATALOG_KERNEL_PRESENT
  while (1) {
    // Do not remove this call: Silicon Labs components process action routine
    // must be called from the super loop.
    sl_system_process_action();

    // Application process.
    app_process_action();

#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
    // Let the CPU go to sleep if the system allows it.
    sl_power_manager_sleep();
#endif
  }
#endif // SL_CATALOG_KERNEL_PRESENT
}
