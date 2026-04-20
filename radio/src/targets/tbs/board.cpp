/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 */

#include "stm32_hal_ll.h"
#include "stm32_gpio.h"

#include "hal/adc_driver.h"
#include "hal/trainer_driver.h"
#include "hal/switch_driver.h"
#include "hal/module_port.h"
#include "hal/abnormal_reboot.h"
#include "hal/usb_driver.h"
#include "hal/gpio.h"

#include "board.h"
#include "debug.h"
#include "rtc.h"
#include "fifo.h"

#include "io/crsf/crsf_utilities.h"
#include "io/crsf/crossfire.h"

#if !defined(BOOT)
  #include "edgetx.h"
  #include "crsf_tasks.h"
  #include "FreeRTOS.h"
  #include "task.h"
  #include "semphr.h"
  #include "queue.h"
#endif

#include "stm32_adc.h"

HardwareOptions hardwareOptions;

#if !defined(BOOT)
static uint32_t trampoline[TRAMPOLINE_INDEX_COUNT] = {0};
#endif

#include "stm32_timer.h"

#if !defined(WAS_RESET_BY_WATCHDOG)
#define WAS_RESET_BY_WATCHDOG()             (RCC->CSR & (RCC_CSR_WDGRSTF | RCC_CSR_WWDGRSTF))
#endif

// Start TIMER at 2000000Hz
void init2MhzTimer()
{
  stm32_timer_enable_clock(TIMER_2MHz_TIMER);
  TIMER_2MHz_TIMER->PSC = (PERI1_FREQUENCY * TIMER_MULT_APB1) / 2000000 - 1; // 0.5 uS, 2 MHz
  TIMER_2MHz_TIMER->ARR = 65535;
  TIMER_2MHz_TIMER->CR2 = 0;
  TIMER_2MHz_TIMER->CR1 = TIM_CR1_CEN;
}

void watchdogInit(unsigned int duration)
{
  IWDG->KR = 0x5555;      // Unlock registers
  IWDG->PR = 3;           // Divide by 32 => 1kHz clock
  IWDG->KR = 0x5555;      // Unlock registers
  IWDG->RLR = duration;
  IWDG->KR = 0xAAAA;      // Start WDT
}

void watchdogReset()
{
  IWDG->KR = 0xAAAA;
}

void watchdogEnable(uint32_t duration)
{
  watchdogInit(duration);
}

#if defined(SPORT_UPDATE_PWR_GPIO)
void sportUpdatePowerOn()
{
  gpio_init(SPORT_UPDATE_PWR_GPIO, GPIO_OUT, GPIO_PIN_SPEED_LOW);
  gpio_set(SPORT_UPDATE_PWR_GPIO);
}

void sportUpdatePowerOff()
{
  gpio_clear(SPORT_UPDATE_PWR_GPIO);
}
#endif

typedef Fifo<uint8_t, 256> ModuleFifo;
ModuleFifo intmoduleFifo;

#if !defined(BOOT)
void INTERNAL_MODULE_OFF()
{
  crossfireTurnOffRf(false);
}

void INTERNAL_MODULE_ON()
{
  crossfireTurnOnRf();
}

bool IS_INTERNAL_MODULE_ON()
{
  return isCrossfireRfOn();
}

void intmoduleSendBuffer(unsigned char const*, unsigned char)
{
  // TODO
}
#endif

void checkBattery() {
  // TODO: implement
}

uint32_t bkregGetStatusFlag(uint32_t flag) {
  return RTC->BKP0R & (1 << flag);
}

void bkregSetStatusFlag(uint32_t flag) {
  RTC->BKP0R |= (1 << flag);
}

void bkregClrStatusFlag(uint32_t flag) {
  RTC->BKP0R &= ~(1 << flag);
}

static uint8_t isDisableBoardOff() {
  return bkregGetStatusFlag(BKREG_SKIP_BOARD_OFF);
}

void runPwrOffCharging()
{
  uint32_t tmrAdc = g_tmr10ms;
  uint32_t tmrPressed = g_tmr10ms;

  while (1) {
    if (g_tmr10ms - tmrAdc >= 10) {
      checkBattery();
      tmrAdc = g_tmr10ms;
    }

    if (g_tmr10ms - tmrPressed >= 10) {
      if (pwrPressed()) {
        pwrOn();
        break;
      }
      tmrPressed = g_tmr10ms;
    }

#if !defined(BOOT)
    if (getCrsfFlag(CRSF_FLAG_CHARGING) && !getCrsfFlag(CRSF_FLAG_CHARGING_FAULT) && usbPlugged()) {
      // simplified charging logic
    }
#endif

    if (!usbPlugged()) {
      pwrOff();
    }
    
    watchdogReset();
  }
}

void boardInit()
{
  bool skipCharging = false;

  pwrInit();
  pwrOn();
  keysInit();

#if defined(ROTARY_ENCODER_NAVIGATION)
  rotaryEncoderInit();
#endif
  delaysInit();

#if !defined(BOOT)
  if (!adcInit(&_adc_driver))
    TRACE("adcInit failed");
#endif

#if defined(RADIO_MAMBO)
  backlightInit();
  BACKLIGHT_ENABLE();
#endif
  lcdInit(); 
  audioInit();
  init2MhzTimer();
  timersInit();
#if !defined(BOOT)
  crsfInit();
#endif
  usbInit();
#if defined(CHARGING_LEDS)
  ledInit();
#endif
#if defined(USB_CHARGER)
  usbChargerInit();
#endif
  __enable_irq();

#if !defined(BOOT)
  hardwareOptions.pcbrev = crsfGetHWID() & 0x0F;
#endif

#if defined(RTCLOCK)
  rtcInit();
#endif

  if (!isDisableBoardOff() && !WAS_RESET_BY_WATCHDOG()) {
    skipCharging = true;
    if (WAS_RESET_BY_SOFTWARE()) {
      LL_RCC_ClearResetFlags();
    }
  }

#if defined(HAPTIC)
  hapticInit();
#endif

  if (!UNEXPECTED_SHUTDOWN())
    sdInit();

  if (skipCharging) {
    runPwrOffCharging();
  }

#if !defined(BOOT)
  crossfireTasksStart();
#endif
}

void boardOff()
{
#if defined(AUDIO_MUTE_GPIO_PIN)
  gpio_set(AUDIO_MUTE_GPIO_PIN);
#endif

#if !defined(BOOT)
  crossfirePowerOff();
  crossfireTasksStop();
#endif

#if defined(HAPTIC)
  hapticOff();
#endif

  BACKLIGHT_DISABLE();

  while (pwrPressed()) {
    watchdogReset();
  }

  pwrOff();
}

uint16_t getBatteryVoltage()
{
  uint16_t instant_vbat = getAnalogValue(TX_VOLTAGE);
  float batt_scale = TBS_BATT_SCALE;

#if defined(RADIO_TANGO)
  if (hardwareOptions.pcbrev == PCBREV_Tango2_V1)
    batt_scale = TBS_BATT_SCALE;
  else
    batt_scale = TBS_BATT_SCALE2;
#endif

#if !defined(BOOT)
  return (uint16_t)(instant_vbat / batt_scale + g_eeGeneral.txVoltageCalibration);
#else
  return (uint16_t)(instant_vbat / batt_scale);
#endif
}

uint16_t anaIn(uint8_t index)
{
  return getAnalogValue(index);
}

void boardReboot2bootloader(uint32_t isNeedFlash, uint32_t HwId, uint32_t sn)
{
  usbStop();
#if !defined(BOOT)
  crossfirePowerOff();
  
  if (crossfireTaskId) {
    xSemaphoreGive(get_task_sem(XF_TASK_SEM));
    vTaskDelay(pdMS_TO_TICKS(10));
  }
#endif

  SET_POWER_REASON(SOFTRESET_REQUEST);
  NVIC_SystemReset();
}

void loadDefaultRadioSettings()
{
#if !defined(BOOT)
  g_eeGeneral.backlightMode = e_backlight_mode_keys;
#endif
}

void onUSBConnectMenu(const char * result)
{
  // TODO
}

void trampolineInit()
{
#if !defined(BOOT)
  trampoline[RTOS_WAIT_SEM_TRAMPOILINE] = (uint32_t)(&xSemaphoreTake);
  trampoline[RTOS_CLEAR_SEM_TRAMPOILINE] = (uint32_t)(&xSemaphoreGive);
#endif
}

extern "C" void EXTI15_10_IRQHandler()
{
#if !defined(BOOT)
  if (LL_EXTI_IsActiveFlag_0_31(INTERRUPT_EXTI_LINE)) {
    LL_EXTI_ClearFlag_0_31(INTERRUPT_EXTI_LINE);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(get_task_sem(XF_TASK_SEM), &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
#endif
}

extern "C" void INTERRUPT_TIM13_IRQHandler()
{
#if !defined(BOOT)
  if (LL_TIM_IsActiveFlag_UPDATE(INTERRUPT_NOT_TIMER)) {
    LL_TIM_ClearFlag_UPDATE(INTERRUPT_NOT_TIMER);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(get_task_sem(XF_TASK_SEM), &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
#endif
}

// Bootloader stubs
#if defined(BOOT)
#include "bootloader/boot.h"
void bootloaderInitScreen() {}
void bootloaderDrawScreen(BootloaderState, int, const char*) {}
void bootloaderDrawFilename(const char*, uint8_t, bool) {}
uint32_t bootloaderGetMenuItemCount(int) { return 0; }
bool bootloaderRadioMenu(uint32_t, event_t) { return false; }
bool isFirmwareStart(const uint8_t*) { return true; }
void flashWrite(uint32_t*, const uint32_t*) {}
void blExit() {}

// Minimal stubs for generic bootloader (real drivers pulled in for firmware)
void rotaryEncoderInit() {}
int usbPlugged() { return 0; }
#endif
uint32_t rotaryEncoderGetValue() { return 0; }
