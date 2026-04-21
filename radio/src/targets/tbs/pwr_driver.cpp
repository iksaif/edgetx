/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 */

// TBS Tango II / Mambo power driver.
//
// The common targets/common/arm/stm32/pwr_driver.cpp assumes an
// active-low power switch (idle pulled HIGH, pressed pulls LOW). The TBS
// hardware is wired the opposite way — idle is LOW, pressed drives HIGH —
// so we need pull-down on init and invert the sense of pwrPressed /
// pwrOffPressed. Keeping this file TBS-local avoids touching the common
// driver and keeps the polarity change contained.

#include "hal/gpio.h"
#include "stm32_gpio.h"

#include "board.h"

void pwrInit()
{
  // Active-high switch → pull-down so idle reads 0, press reads 1.
  gpio_init(PWR_SWITCH_GPIO, GPIO_IN_PD, GPIO_PIN_SPEED_LOW);

  // Soft-latch output, driven high to keep the radio alive.
  gpio_init(PWR_ON_GPIO, GPIO_OUT, GPIO_PIN_SPEED_LOW);
}

void pwrOn()
{
  gpio_set(PWR_ON_GPIO);
}

void pwrOff()
{
  gpio_clear(PWR_ON_GPIO);
}

bool pwrPressed()
{
  return gpio_read(PWR_SWITCH_GPIO) != 0;
}

bool pwrOffPressed()
{
  return pwrPressed();
}
