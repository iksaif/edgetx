/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 */

#pragma once

#include <inttypes.h>
#include "hal.h"
#include "hal/serial_port.h"
#include "hal/watchdog_driver.h"

#include "definitions.h"
#include "edgetx_constants.h"
#include "board_common.h"

#if defined(ROTARY_ENCODER_NAVIGATION)
void rotaryEncoderInit();
void rotaryEncoderCheck();
#endif
// The common rotary_encoder_driver.cpp checks ROTARY_ENCODER_INVERTED
// (with -ED, matches all other targets). Previous spelling was a no-op.
#if defined(RADIO_MAMBO)
#define ROTARY_ENCODER_INVERTED
#endif

// TBS Tango II / Mambo have no trainer port — SLAVE_MODE always false.
#define SLAVE_MODE()                    (false)

#if defined(RADIO_TANGO)
#define MY_DEVICE_NAME                  "Tango II"
#elif defined(RADIO_MAMBO)
#define MY_DEVICE_NAME                  "Mambo"
#endif

// F413 has 1 MB flash total. We partition it:
//   [0x08000000 .. +BOOTLOADER_SIZE)   bootloader  (32 KB)
//   [+BOOTLOADER_SIZE .. CROSSFIRE_TASK_ADDRESS)  firmware  (~736 KB usable)
//   [0x080C0020 .. 0x08100000)          CRSF blob region (~256 KB)
// FLASHSIZE is the usable region the bootloader writes to (stops before
// stepping on the blob sector). BOOTLOADER_SIZE must match the
// BOOTLOADER_SIZE value in
// boards/generic_stm32/linker/stm32f413/layout.ld (currently 0x8000);
// otherwise bootloader boot.cpp's jumpTo(APP_START_ADDRESS) lands 16 KB
// past the real .isr_vector and the firmware won't start.
#define FLASHSIZE                       0xC0000
#define FLASH_PAGESIZE                  256
#define BOOTLOADER_SIZE                 0x8000
#define FIRMWARE_ADDRESS                0x08000000
#define APP_START_ADDRESS               (uint32_t)(FIRMWARE_ADDRESS + BOOTLOADER_SIZE)
#define CROSSFIRE_TASK_ADDRESS          0x080C0020
#define SHARED_MEMORY_ADDRESS           0x10000000

#define LUA_MEM_MAX                     (0)

#define PERI1_FREQUENCY                 42000000
#define PERI2_FREQUENCY                 84000000

#define TIMER_MULT_APB1                 2
#define TIMER_MULT_APB2                 2

extern uint16_t sessionTimer;

// Board driver
void boardInit();
void boardOff();

void INTERNAL_MODULE_ON();
void INTERNAL_MODULE_OFF();

// Timers driver
void init2MhzTimer();

// Telemetry driver
void check_telemetry_exti();

// PCBREV driver
#if defined(RADIO_TANGO)
enum {
  PCBREV_Tango2_Unknown = 0,
  PCBREV_Tango2_V1,
  PCBREV_Tango2_V2,
  PCBREV_Tango2_V3,
};
#elif defined(RADIO_MAMBO)
enum {
  PCBREV_Mambo_Unknown = 0,
  PCBREV_Mambo_V1,
  PCBREV_Mambo_V2,
};
#endif

#define IS_SHIFT_KEY(index)             (false)
#define IS_SHIFT_PRESSED()              (false)

#if defined(RADIO_TANGO)
#define STORAGE_NUM_SWITCHES 6
#define DEFAULT_SWITCH_CONFIG                                         \
  (SWITCH_TOGGLE << 10) + (SWITCH_TOGGLE << 8) + (SWITCH_2POS << 6) + \
      (SWITCH_3POS << 4) + (SWITCH_3POS << 2) + (SWITCH_2POS << 0)
#elif defined(RADIO_MAMBO)
#define STORAGE_NUM_SWITCHES 6
#define DEFAULT_SWITCH_CONFIG                                       \
  (SWITCH_TOGGLE << 10) + (SWITCH_3POS << 8) + (SWITCH_3POS << 6) + \
      (SWITCH_3POS << 4) + (SWITCH_3POS << 2) + (SWITCH_3POS << 0)
#define DEFAULT_POTS_CONFIG (POT_WITH_DETENT << 2) + (POT_WITH_DETENT << 0)
#define DEFAULT_SLIDERS_CONFIG SLIDER_NONE
#endif

#define NUM_FUNCTIONS_SWITCHES 0

void keysInit();
uint32_t readKeys();
uint32_t readTrims();

#define TRIMS_PRESSED()                 (readTrims())
#define KEYS_PRESSED()                  (readKeys())

#define NUM_STICKS 4

// ADC driver
enum Analogs {
  STICK1,
  STICK2,
  STICK3,
  STICK4,
#if defined(RADIO_MAMBO)
  POT_FIRST,
  POT1 = POT_FIRST,
  POT2,
  POT_LAST = POT2,
  SWITCH_TRIM,
  SWITCH_A,
  SWITCH_B,
  SWITCH_C,
  SWITCH_D,
#endif
  TX_VOLTAGE,
  TX_RTC_VOLTAGE,
  NUM_ANALOGS
};

#if defined(RADIO_TANGO)
#define NUM_POTS                        0
#define NUM_XPOTS                       0
#define NUM_SLIDERS                     0
#define NUM_TRIMS                       4
#define NUM_MOUSE_ANALOGS               0
#define STORAGE_NUM_MOUSE_ANALOGS       0
#define STORAGE_NUM_POTS                0
#define STORAGE_NUM_SLIDERS             0
#define NUM_TRIMS_KEYS                  8
#define STICKS_PWM_ENABLED()            false
#elif defined(RADIO_MAMBO)
#define NUM_POTS                        2
#define NUM_XPOTS                       STORAGE_NUM_POTS
#define NUM_SLIDERS                     0
#define NUM_TRIMS                       4
#define NUM_MOUSE_ANALOGS               0
#define STORAGE_NUM_POTS                2
#define STORAGE_NUM_SLIDERS             0
#define NUM_MOUSE_ANALOGS               0
#define STORAGE_NUM_MOUSE_ANALOGS       0
#define NUM_TRIMS_KEYS                  8
#define STICKS_PWM_ENABLED()            false
#endif

PACK(typedef struct {
  uint8_t pcbrev:4;
  uint8_t sticksPwmDisabled:1;
  uint8_t pxx2Enabled:1;
}) HardwareOptions;

extern HardwareOptions hardwareOptions;

enum CalibratedAnalogs {
  CALIBRATED_STICK1,
  CALIBRATED_STICK2,
  CALIBRATED_STICK3,
  CALIBRATED_STICK4,
#if defined(RADIO_MAMBO)
  CALIBRATED_POT_FIRST,
  CALIBRATED_POT_LAST = CALIBRATED_POT_FIRST + NUM_POTS - 1,
  CALIBRATED_SLIDER_FIRST,
  CALIBRATED_SLIDER_LAST = CALIBRATED_SLIDER_FIRST + NUM_SLIDERS - 1,
#endif
  NUM_CALIBRATED_ANALOGS
};

#if defined(RADIO_MAMBO)
  #define IS_POT(x)                   ((x)>=POT_FIRST && (x)<=POT_LAST) 
#else
  #define IS_POT(x)                   (false)
#endif

// NOTE: adcValues[] lives in hal/adc_driver.cpp (MAX_ANALOG_INPUTS sized) in
// modern EdgeTX; access it via anaIn() / getAnalogValue() rather than
// declaring it here.

// Battery driver
uint16_t getBatteryVoltage();
#define BATTERY_WARN                  34
#define BATTERY_CRITICAL              33
#define BATTERY_MIN                   33
#define BATTERY_MAX                   42
#define BATTERY_TYPE_FIXED

#define BATT_CALIB_OFFSET             5

// TBS-specific battery divider constants (legacy semantics):
// vbat = adc / TBS_BATT_SCALE + calibration
// Named with TBS_ prefix to avoid colliding with the modern
// generic battery_voltage.cpp BATT_SCALE (integer, different formula).
#if defined(RADIO_TANGO)
#define TBS_BATT_SCALE                (4.446f)
#define TBS_BATT_SCALE2               (4.162f)
#elif defined(RADIO_MAMBO)
#define TBS_BATT_SCALE                (4.55f)
#define TBS_BATT_SCALE2               TBS_BATT_SCALE
#endif

#define DEBUG_BAUDRATE                  115200
#define LUA_DEFAULT_BAUDRATE            115200

const etx_serial_port_t* auxSerialGetPort(int port_nr);

// Power driver
void pwrInit();
void pwrOn();
void pwrOff();
bool pwrPressed();
uint32_t pwrCheck();

// Backlight driver
#define BACKLIGHT_TIMEOUT_MIN           2
#define BACKLIGHT_FORCED_ON             101
// On TANGO the "backlight" is really the LCD bias/ref voltage; for MAMBO
// there's a real backlight LED. Both flavours expose a function-like
// backlightDisable() — TANGO via macro aliasing to lcdOff(), MAMBO via a
// proper function in backlight_driver.cpp. BACKLIGHT_DISABLE() is then
// defined once below, resolving to whichever form this target provides.
#if defined(RADIO_TANGO)
  #define backlightDisable()              lcdOff()
  #define isBacklightEnabled()            isLcdOn()
#elif defined(RADIO_MAMBO)
  void backlightInit(void);
  void backlightDisable(void);
  uint8_t isBacklightEnabled(void);
#endif
void backlightEnable(uint8_t level);
void backlightFullOn();
#define BACKLIGHT_DISABLE()             backlightDisable()
#define BACKLIGHT_ENABLE()              backlightEnable(g_eeGeneral.backlightBright)
#define BACKLIGHT_LEVEL_MAX             100

void usbJoystickUpdate();
#define USB_FIRMWARE_DEFAULT_MODE       USB_AGENT_MODE
#define USB_NAME                        "TBS"
#define USB_MANUFACTURER                'T', 'B', 'S', ' ', ' ', ' ', ' ', ' '
#if defined(RADIO_TANGO)
#define USB_PRODUCT                     'T', 'a', 'n', 'g', 'o', ' ', '2', ' '
#else
#define USB_PRODUCT                     'M', 'a', 'm', 'b', 'o', ' ', ' ', ' '
#endif

#if defined(__cplusplus)
enum PowerReason {
  SHUTDOWN_REQUEST = 0xDEADBEEF,
  SOFTRESET_REQUEST = 0xCAFEDEAD,
};

constexpr uint32_t POWER_REASON_SIGNATURE = 0x0178746F;

// Defined in board.cpp — not inline because datacopy.inc is generated via
// libclang which parses board.h without STM32 peripheral headers and would
// fail on RTC->.
void SET_POWER_REASON(uint32_t value);
#endif

#if defined(__cplusplus) && !defined(SIMU)
extern "C" {
#endif
// INTERRUPT_NOT_IRQHandler is the TBS-specific name for the TIM13 update
// ISR; hal.h aliases it to TIM8_UP_TIM13_IRQHandler so it lines up with
// the F4 vector table slot. Implementation lives in board.cpp.
void INTERRUPT_NOT_IRQHandler();

uint32_t bkregGetStatusFlag(uint32_t flag);
void bkregSetStatusFlag(uint32_t flag);
void bkregClrStatusFlag(uint32_t flag);
#if defined(__cplusplus) && !defined(SIMU)
}
#endif

#include "debug.h"
#include "fifo.h"

// Common functions
void lcdInit();
void lcdOn();
void lcdOff();
bool isLcdOn();
void lcdRefresh(bool wait=false);
void lcdRefreshWait();
void lcdSetRefVolt(uint8_t val);
void lcdSetInvert(bool invert);
// lcdSetContrast is implemented by gui/128x64/lcd.cpp (shared code).
void lcdSetContrast(bool useDefault = false);
void audioInit();
void delaysInit();
void timersInit();
void usbInit();
void usbStop();
void usbChargerInit();
void ledInit();
void hapticInit();
void hapticOff();
void hapticOn();
void rtcInit();
void sdInit();

bool pwrOffPressed();

