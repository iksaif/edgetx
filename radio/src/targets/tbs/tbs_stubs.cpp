/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 */

// Phase-A stubs that keep the TBS firmware target linking while the real
// drivers are ported. See PORTING-PLAN.md.
// Each block has a TODO pointing at the eventual home for the real impl.

#include <cstdint>
#include "fifo.h"
#include "board.h"
#include "hal/serial_port.h"

// ----------------------------------------------------------------------------
// Telemetry
//
// Called indirectly: io/crsf/crossfire.cpp uses intCrsfTelemetryFifo to buffer
// telemetry bytes. The legacy tbs/telemetry_driver.cpp also defined
// telemetryFifo and a real telemetryPortInit(); both are Phase B work.
// ----------------------------------------------------------------------------
#ifndef TELEMETRY_FIFO_SIZE
#define TELEMETRY_FIFO_SIZE 128
#endif
Fifo<uint8_t, TELEMETRY_FIFO_SIZE> intCrsfTelemetryFifo;
Fifo<uint8_t, TELEMETRY_FIFO_SIZE> telemetryFifo;

// TODO(port Phase B): real UART6 telemetry port on PC.6/PC.7 (see hal.h
// TELEMETRY_USART). Modern EdgeTX would wire this through
// hal/module_port.h or a custom stm32_usart_t.
void telemetryPortInit(uint32_t /*baudrate*/, uint8_t /*mode*/) {}
void telemetryPortInit1kHz(uint32_t /*baudrate*/, uint8_t /*mode*/) {}
void telemetryPortInit10mHz(uint32_t /*baudrate*/, uint8_t /*mode*/) {}
void telemetryPortSetDirectionInput() {}
void telemetryPortSetDirectionOutput() {}

// ----------------------------------------------------------------------------
// USB charger
// ----------------------------------------------------------------------------
// TODO(port Phase B): real charger state + fault pins (see
// CHARGER_STATE_GPIO* / CHARGER_FAULT_GPIO* that the legacy
// usb_charger_driver.cpp referenced). Stubbed for Phase A link.
void usbChargerInit() {}
bool usbChargerLed() { return false; }

// ----------------------------------------------------------------------------
// CRSF USB HID glue
//
// io/crsf/crossfire.cpp has a port table that references crsfToUsbHid and
// usbAgentWrite. The legacy usb_agent_driver.cpp implementation uses the old
// STM32 USB stack (usb_dcd_int.h / USB_OTG_CORE_HANDLE) — incompatible with
// modern EdgeTX which uses the STM32 USB Device Library. Full port is Phase
// C or later. For Phase A, no-op.
// ----------------------------------------------------------------------------
void crsfToUsbHid(uint8_t* /*pArr*/) {}
void usbAgentWrite(uint8_t* /*pData*/) {}

// ----------------------------------------------------------------------------
// Services that would normally be provided by linker script symbols, other
// target board.cpp files, or io/crsf/*.cpp. All stubbed for Phase A —
// each block flags the real home.
// ----------------------------------------------------------------------------

// CCM heap bounds — exported by boards/generic_stm32/linker/stm32f40x/
// extra_sections.ld on targets that use the modern generic linker. TBS still
// uses its own stm32f4_flash.ld which doesn't declare them. Provide weak
// zero symbols so the Lua CCM allocator bails out gracefully. Phase B:
// migrate to the generic linker and delete these.
extern "C" {
__attribute__((weak)) char _ccm_heap_start[1] = {0};
__attribute__((weak)) char _ccm_heap_end[1]   = {0};
}

// ----------------------------------------------------------------------------
// io/crsf/ functions not currently compiled — crsfInit / crsfGetHWID /
// crossfireTurnOn/OffRf / crossfirePowerOff / isCrossfireRfOn. They live in
// radio/src/io/crsf/*.cpp which is not in any CMakeLists. Wire those files
// in as part of Phase C (they depend on the blob / shared memory layout).
// ----------------------------------------------------------------------------
void crsfInit() {}
uint32_t crsfGetHWID() { return 0; }
void crossfireTurnOnRf() {}
void crossfireTurnOffRf(bool /*ask*/) {}
void crossfirePowerOff() {}
bool isCrossfireRfOn() { return false; }

// ----------------------------------------------------------------------------
// Speaker volume, mixer scheduler, RTC, rotary encoder, pulse ISR
// ----------------------------------------------------------------------------
// usbPlugged: the shared common/arm/stm32/usb_driver.cpp only defines it
// when USB_GPIO_VBUS is set in hal.h. Tango's VBUS sense pin isn't mapped
// out yet — return "not plugged" for Phase A.
int usbPlugged() { return 0; }

void audioSetVolume(uint8_t /*volume*/) {}
void mixerSchedulerStart() {}
void mixerSchedulerEnableTrigger() {}
void mixerSchedulerDisableTrigger() {}
void mixerSchedulerResetTimer() {}
// mixerSchedulerISRTrigger() is defined in mixer_scheduler.cpp.
// rotaryEncoderInit / rotaryEncoderGetValue come from the common
// targets/common/arm/stm32/rotary_encoder_driver.cpp now.

struct gtm;
void rtcSetTime(const gtm* /*t*/) {}
void rtcInit() {}

// lcdLoadBitmap — 128x64 bitmap loader from SD. The real one is in
// gui/128x64/bmp.cpp which we already include. The lua api references a
// 4-arg version — provide a stub if the symbol isn't satisfied.
// (No-op here; if bmp.cpp's definition binds we drop this at link time.)

// menuModelExpoOne is only defined when the target enables a specific
// layout. Provide an extern stub alias in case we need it.

// Audio DAC: real driver is targets/common/arm/stm32/audio_dac_driver.cpp,
// now wired into FIRMWARE_BOARD_EXTRA_SRC with AUDIO_* hal.h defines.

// auxSerialGetPort is provided by boards/generic_stm32/aux_ports.cpp, which
// reads the port definitions generated from hw_defs/tango.json.
