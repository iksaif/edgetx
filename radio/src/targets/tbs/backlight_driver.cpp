/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x 
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "stm32_hal_ll.h"
#include "stm32_gpio.h"
#include "stm32_timer.h"
#include "hal/gpio.h"

#include "board.h"

// Called by gui/common/stdlcd/draw_functions.cpp when the UI wants to force
// max backlight (e.g. during critical alerts). TODO(port Phase B): for
// TANGO the "backlight" really controls LCD contrast via lcdSetRefVolt; for
// MAMBO it's a PWM pin. Stubbed no-op for now.
void backlightFullOn() {}

#if defined(RADIO_TANGO)
void backlightEnable(uint8_t level)
{
  // the scale is divided into two groups since the affect of contrast configuration is not so linear
  // system brightness 0  to 84  map to screen contrast 0   to 127
  // system brightness 81 to 100 map to screen contrast 127 to 255

  uint8_t value = 100 - level;
  if (value >= 84)
    value = ((value-84) << 3) + 127;        // (value-84)*128/16+127;
  else
    value = (value << 5) / 21;              // value*128/84

  lcdSetRefVolt(value);
  lcdOn();
}
#elif defined(RADIO_MAMBO)
// TODO(port): Mambo backlight PWM driver — still uses legacy StdPeriph API,
// needs porting to stm32_gpio / stm32_timer once BACKLIGHT_* pin defines are
// added to hal.h.
void backlightInit() {}
void backlightEnable(uint8_t) {}
void backlightDisable() {}
uint8_t isBacklightEnabled() { return 0; }
#endif
