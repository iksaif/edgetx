/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 */

#include "stm32_hal_ll.h"
#include "stm32_gpio.h"
#include "hal/gpio.h"
#include "hal/key_driver.h"

#if !defined(BOOT)
  #include "edgetx.h"
#endif

#include "board.h"
#include "debug.h"

#if defined(RADIO_TANGO)
uint8_t g_trimState = 0;
#endif

uint32_t readKeys()
{
  uint32_t result = 0;

  if (!gpio_read(KEYS_GPIO_PIN_ENTER))
    result |= 1 << KEY_ENTER;
  if (!gpio_read(KEYS_GPIO_PIN_MENU))
    result |= 1 << KEY_MENU;
  if (!gpio_read(KEYS_GPIO_PIN_PAGE))
    result |= 1 << KEY_PAGEDN;
  if (!gpio_read(KEYS_GPIO_PIN_EXIT))
    result |= 1 << KEY_EXIT;

  return result;
}

uint32_t readTrims()
{
  // TODO(port Phase B): TANGO trim events come through per10ms() → g_trimState
  // in the legacy code. Wire to the modern key driver once the TANGO trim
  // matrix is mapped. MAMBO trims are sampled via ADC and handled by the
  // generic switch driver.
#if defined(RADIO_TANGO)
  uint32_t result = g_trimState;
  g_trimState = 0;
  return result;
#else
  return 0;
#endif
}

void keysInit()
{
  gpio_init(KEYS_GPIO_PIN_MENU,  GPIO_IN_PU, GPIO_PIN_SPEED_LOW);
  gpio_init(KEYS_GPIO_PIN_EXIT,  GPIO_IN_PU, GPIO_PIN_SPEED_LOW);
  gpio_init(KEYS_GPIO_PIN_PAGE,  GPIO_IN_PU, GPIO_PIN_SPEED_LOW);
  gpio_init(KEYS_GPIO_PIN_ENTER, GPIO_IN_PU, GPIO_PIN_SPEED_LOW);

  // Rotary encoder pins are configured by the common rotary encoder driver
  // (targets/common/arm/stm32/rotary_encoder_driver.cpp) from
  // rotaryEncoderInit() — don't double-init here.
}
