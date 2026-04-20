/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 */

#include "stm32_hal_ll.h"
#include "stm32_gpio.h"
#include "hal/gpio.h"

#if !defined(BOOT)
  #include "edgetx.h"
#endif

#include "board.h"
#include "debug.h"

#if defined(ROTARY_ENCODER_NAVIGATION)
uint32_t rotencPositionValue;
#endif

#if defined(RADIO_TANGO)
uint8_t  g_trimState = 0;
#endif

uint32_t readKeys()
{
  uint32_t result = 0;

  if (!gpio_read(KEYS_GPIO_REG_ENTER, KEYS_GPIO_PIN_ENTER))
    result |= 1 << KEY_ENTER;

#if defined(KEYS_GPIO_PIN_MENU)
  if (!gpio_read(KEYS_GPIO_REG_MENU, KEYS_GPIO_PIN_MENU))
    result |= 1 << KEY_MENU;
#endif

#if defined(KEYS_GPIO_PIN_PAGE)
  if (!gpio_read(KEYS_GPIO_REG_PAGE, KEYS_GPIO_PIN_PAGE))
    result |= 1 << KEY_PAGE;
#endif

  if (!gpio_read(KEYS_GPIO_REG_EXIT, KEYS_GPIO_PIN_EXIT))
    result |= 1 << KEY_EXIT;

  return result;
}

uint32_t readTrims()
{
  uint32_t result = 0;

#if defined(RADIO_TANGO)
  // the trim state from the events of per10ms()
  result = g_trimState;
  g_trimState = 0;
#elif defined(RADIO_MAMBO)
  // Mambo trims are handled via ADC (defined in JSON)
  // but they still need to be reported here for legacy reasons?
  // Actually modern EdgeTX handles ADC trims via switch/input driver.
#endif

  return result;
}

void keysInit()
{
  gpio_init(KEYS_GPIO_REG_MENU, GPIO_IN_PU, GPIO_PIN_SPEED_LOW);
  gpio_init(KEYS_GPIO_REG_EXIT, GPIO_IN_PU, GPIO_PIN_SPEED_LOW);
  gpio_init(KEYS_GPIO_REG_PAGE, GPIO_IN_PU, GPIO_PIN_SPEED_LOW);
  gpio_init(KEYS_GPIO_REG_ENTER, GPIO_IN_PU, GPIO_PIN_SPEED_LOW);

#if defined(ROTARY_ENCODER_NAVIGATION)
  gpio_init(ENC_GPIO, ENC_GPIO_PIN_A, GPIO_IN_PU, GPIO_PIN_SPEED_LOW);
  gpio_init(ENC_GPIO, ENC_GPIO_PIN_B, GPIO_IN_PU, GPIO_PIN_SPEED_LOW);
  rotencPositionValue = ROTARY_ENCODER_POSITION();
#endif
}
