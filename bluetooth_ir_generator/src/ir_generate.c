/***************************************************************************//**
 * @file ir_generate.c
 * @brief IR Generator driver, APIs.
 * @version 0.0.1
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
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
 * # EVALUATION QUALITY
 * This code has been minimally tested to ensure that it builds with the specified
 * dependency versions and is suitable as a demonstration for evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 ******************************************************************************/
#include "em_device.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_chip.h"
#include "em_gpio.h"
#include "em_prs.h"
#include "em_timer.h"
#include "ir_generate.h"

const uint8_t ir_table[TABLE_INDEX_NUM][2] = {
  {0x00, 0x00},
  {0x2A, 0x15},  //SONY Projector [POWER]
  {0x2A, 0x57},  //SONY Projector [INPUT]
  {0x46, 0x44},
  {0x47, 0x39},
  {0x1C, 0x1C},
  {0x1B, 0x1B},
  {0x44, 0x44},
  {0x12, 0x12},
  {0x13, 0x13},
  {0x46, 0x44},
  {0x47, 0x39},
  {0x1C, 0x1C},
  {0x1B, 0x1B},
  {0x44, 0x44},
  {0x46, 0x44},
  {0x47, 0x39},
  {0x45, 0x45}
};

static ir_t ir = {
  .code = CODE_NEC,
  .carrier = {38000, 40000},
  .timebase = {1769, 1659},
  .dutycycle = {0.335, 0.335},
  .head_bit_size = {{16,8}, {4,1}},
  .address_length = {8,7},
  .command_length = {8,8},
  .stream_active = false,
  .stream_index = 0,
};

static ir_callback_t ir_complete_callback = 0;

__STATIC_INLINE void ir_generate_stream_head(bool * stream, bool repeat)
{
  uint8_t i;
  uint8_t spaceSize = ir.head_bit_size[ir.code][HEAD_SPACE];

  if ((repeat == true) && (ir.code == CODE_NEC)){
	spaceSize = NEC_REPEAT_HEAD_SPACE_BIT_SIZE; //2.25ms for repeat stream
  }

  // pulse, NEC: 16 for 9ms, SONY: 4 for 2.4ms
  for (i = 0; i < ir.head_bit_size[ir.code][HEAD_PULSE]; i++){
	stream[ir.index++] = true;
  }

  // space, NEC: 8 for 4.5ms, SONY: 1 for 0.6ms
  for (i = 0; i < spaceSize; i++){
	stream[ir.index++] = false;
  }
}

__STATIC_INLINE void ir_generate_stream_logic(bool * stream, bool bit)
{
  bool * start_point = stream;
  switch (ir.code) {
    case CODE_NEC:
      // 1(1 pulse, 3 spaces),0(1 pulse, 1 space)
      *stream++ = true;
      *stream++ = false;
      if (bit){
        *stream++ = false;
        *stream++ = false;
      }
      break;

    case CODE_SONY:
	  // 1(2 pulse, 1 spaces), 0(1 pulse, 1 space)
      *stream++ = true;
      if (bit){
        *stream++ = true;
      }
      *stream++ = false;
      break;

    default:
      *stream = false;
      break;
  }
  ir.index += (stream - start_point);
}

__STATIC_INLINE void ir_generate_stream_byte(uint16_t stream_data, uint8_t length)
{
  //NEC, SONY send in LSB
  for (uint8_t i = 0; i < length; i++){
    ir_generate_stream_logic(&ir.stream[ir.index], (stream_data&BIT(0)));
    stream_data >>= 1; //move left to get next bit
  }
}

/**
 * @brief configure ir signal stream.
 *
 * @param address, command, repeat flag for NEC protocol
 *
 * @return none
 *
 */
void ir_generate_stream(uint16_t address, uint16_t command, bool repeat)
{  
  while (ir.stream_active); // Wait until done transmitting last code

  ir.index = 0;
  ir_generate_stream_head(&ir.stream[ir.index], repeat);
  switch (ir.code) {
    case CODE_NEC:
      // address -> address complemented -> command -> command complemented
      if(repeat == false){
        ir_generate_stream_byte(address, ir.address_length[ir.code]);
        ir_generate_stream_byte((~address), ir.address_length[ir.code]);
        ir_generate_stream_byte(command, ir.command_length[ir.code]);
        ir_generate_stream_byte((~command), ir.command_length[ir.code]);
      }
      // Send trailing (pulse)
      ir.stream[ir.index++] = true;
      break;

    case CODE_SONY:
      // command -> address
      ir_generate_stream_byte(command, ir.command_length[ir.code]);
      ir_generate_stream_byte(address, ir.address_length[ir.code]);
      break;

    default:
      break;
  }
  ir.stream[ir.index] = false;

  ir.stream_active = true;			// Enable sending code
  ir.stream_index = 0;				// start from 0, stop when ir.stream_index >= ir.index
  
  TIMER_Enable(TIMER1, true);
}

__STATIC_INLINE void ir_generate_send(void)
{
  // Clear flag for TIMER1 overflow interrupt
  TIMER_IntClear(TIMER1, TIMER_IF_OF);

  if (ir.stream_active){
    // Send a pulse
    if (ir.stream[ir.stream_index]) {
      TIMER_Enable(TIMER0, true);
      GPIO_PinOutSet(BSP_MODULATION_PORT, BSP_MODULATION_PIN);
    } else {
	  // Send a space
      TIMER_Enable(TIMER0, false);
      GPIO_PinOutClear(BSP_MODULATION_PORT, BSP_MODULATION_PIN);
    }
    ir.stream_index++;

    // Done sending the stream
    if (ir.stream_index >= ir.index) {
      ir.stream_index = 0;
      ir.stream_active = false;
    }
  } else {
	//TIMER_Enable(TIMER0, false);
    ir_generate_stop();
    GPIO_PinOutClear(BSP_MODULATION_PORT, BSP_MODULATION_PIN);
    ir_complete_callback();
  }
}

/**
 * @brief stop ir signal generate.
 *
 * @param none
 *
 * @return none
 *
 */
void ir_generate_stop(void)
{
  TIMER_Enable(TIMER0, false);
  TIMER_Enable(TIMER1, false);
}

__STATIC_INLINE void ir_generate_pin(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  GPIO_PinModeSet(BSP_CARRIER_PORT, BSP_CARRIER_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(BSP_MODULATION_PORT, BSP_MODULATION_PIN, gpioModePushPull, 0);
}

#if 0	//no need a interrupt for change the duty cycle
void TIMER0_IRQHandler(void)
{
  // Acknowledge the interrupt
  uint32_t flags = TIMER_IntGet(TIMER0);
  TIMER_IntClear(TIMER0, flags);

  // Update CCVB to alter duty cycle starting next period
  //TIMER_CompareBufSet(TIMER0, 0, (uint32_t)(topValue * dutyCycle));
}
#endif

__STATIC_INLINE void ir_generate_carrier(void)
{
  uint32_t timerFreq = 0;
  uint32_t topValue = 0;

  CMU_ClockEnable(cmuClock_TIMER0, true);

  // Initialize the timer
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  // Configure TIMER0 Compare/Capture for output compare
  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;

  // Use PWM mode, which sets output on overflow and clears on compare events
  timerInit.prescale = timerPrescale1;
  timerInit.enable = false;
  timerCCInit.mode = timerCCModePWM;

  // Configure but do not start the timer
  TIMER_Init(TIMER0, &timerInit);

  // Route Timer0 CC0 output to PC0
  GPIO->TIMERROUTE[0].ROUTEEN  = GPIO_TIMER_ROUTEEN_CC0PEN;
  GPIO->TIMERROUTE[0].CC0ROUTE = (BSP_CARRIER_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
								| (BSP_CARRIER_PIN << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);

  // Configure CC Channel 0
  TIMER_InitCC(TIMER0, 0, &timerCCInit);

  // set PWM period
  timerFreq = CMU_ClockFreqGet(cmuClock_TIMER0) / (timerInit.prescale + 1);
  topValue = (timerFreq / ir.carrier[ir.code]);
  // Set top value to overflow at the desired PWM_FREQ frequency
  TIMER_TopSet(TIMER0, topValue);

  // Set compare value for initial duty cycle
  TIMER_CompareSet(TIMER0, 0, (uint32_t)(topValue * ir.dutycycle[ir.code]));

  #if 0	//no need to start here and no need a interrupt for change the duty cycle
  // Start the timer
  TIMER_Enable(TIMER0, true);

  // Enable TIMER0 compare event interrupts to update the duty cycle
  TIMER_IntEnable(TIMER0, TIMER_IEN_CC0);
  NVIC_EnableIRQ(TIMER0_IRQn);
  #endif
}

void TIMER1_IRQHandler(void)
{
  // Acknowledge the interrupt
  uint32_t flags = TIMER_IntGet(TIMER1);
  TIMER_IntClear(TIMER1, flags);
  ir_generate_send();
}

__STATIC_INLINE void ir_generate_timebase(void)
{
  uint32_t timerFreq = 0;

  CMU_ClockEnable(cmuClock_TIMER1, true);

  // Initialize the timer
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  timerInit.prescale = timerPrescale1;
  timerInit.enable = false;

  // Configure but do not start the timer
  TIMER_Init(TIMER1, &timerInit);

  timerFreq = CMU_ClockFreqGet(cmuClock_TIMER1) / (timerInit.prescale + 1);
  TIMER_TopSet(TIMER1, (timerFreq / ir.timebase[ir.code]));

  // Start the timer
  //TIMER_Enable(TIMER1, true);

  // Enable TIMER1 compare event interrupts to update the duty cycle
  TIMER_IntEnable(TIMER1, TIMER_IF_OF);
  NVIC_EnableIRQ(TIMER1_IRQn);
}

__STATIC_INLINE void ir_generate_code(code_t ir_code)
{
  ir.code = ir_code;
}

/**
 * @brief Initializes IR generation with IR code/protocol.
 *
 * @param ir_code instance of ir_generate_init to initialize
 *
 * @return none
 *
 */
void ir_generate_init(code_t ir_code, ir_callback_t cb)
{
  ir_generate_code(ir_code);
  ir_generate_pin();
  ir_generate_carrier();
  ir_generate_timebase();
  //ir_generate_stream(0xFF, 0xFF, false); // for test
  ir_complete_callback = cb;
}
