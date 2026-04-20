/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 */

#pragma once

#include "definitions.h"

#define USB_VID                        0x0483
#define USB_PID_STORAGE                0x5720
#define USB_PID_JOYSTICK               0x5710

#define USB_MANUFACTURER               'T', 'B', 'S', ' ', ' ', ' ', ' ', ' '
#if defined(RADIO_TANGO)
#define USB_NAME                       "Tango II"
#define USB_PRODUCT                    'T', 'a', 'n', 'g', 'o', ' ', '2', ' '
#else
#define USB_NAME                       "Mambo"
#define USB_PRODUCT                    'M', 'a', 'm', 'b', 'o', ' ', ' ', ' '
#endif

#define USB_MANUFACTURER_STRING        "TBS"
#define USB_PRODUCT_STRING             USB_NAME

#define USB_CONFIGURATION_STRING       "Default Configuration"
#define USB_INTERFACE_STRING           "Default Interface"
