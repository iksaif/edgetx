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

// TODO(port): Tango II RGB LED on PB4 driven via TIM3 CC4 + DMA1 Stream2 in
// WS2812-like bit-bang pattern (see
// context_backup/porting_context/tbs-merge/radio/src/targets/tbs/led_driver.cpp
// for the original). Hardware defines missing from hal.h (LED_GPIO,
// LED_GPIO_AF, LED_TIMER, LED_DMA_STREAM, LED_DMA_CHANNEL, LED_DMA_FLAG_TC).
// Stubbed for now so the firmware links; port once basic boot works.

#include <stdint.h>

void ledInit() {}
void ledSetColour(uint8_t, uint8_t, uint8_t) {}
void ledOff() {}
void ledRed() {}
void ledGreen() {}
void ledBlue() {}
