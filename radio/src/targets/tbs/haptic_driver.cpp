/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hal/gpio.h"
#include "stm32_gpio.h"

#include "board.h"

// No PWM on Tango II / Mambo — simple GPIO on/off
void hapticInit()
{
  gpio_init(HAPTIC_GPIO, GPIO_OUT, GPIO_PIN_SPEED_LOW);
}

void hapticOff()
{
  gpio_clear(HAPTIC_GPIO);
}

void hapticOn()
{
  gpio_set(HAPTIC_GPIO);
}
