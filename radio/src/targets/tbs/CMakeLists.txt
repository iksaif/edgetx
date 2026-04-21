# TBS Tango II and Mambo support

option(DISK_CACHE "Enable SD card disk cache" OFF)
option(INTERNAL_MODULE_CRSF "Support for crossfire internal module" ON)

set(CPU_TYPE STM32F4)
set(CPU_TYPE_FULL STM32F407xE)
set(LINKER_SCRIPT targets/tbs/stm32f4_flash.ld)
# RTC_BACKUP_RAM uses the 4KB BKPSRAM peripheral (available on F407/F417 but
# NOT on F413 — Tango II is F413). The TBS code uses RTC backup registers
# (RTC->BKP0R/BKP1R) instead, which is a different mechanism. Leave the 4KB
# BKPSRAM-based RamBackup disabled.
# set(RTC_BACKUP_RAM YES)
set(HAPTIC YES)
set(USB_CHARGER YES)
set(PPM_LIMITS_SYMETRICAL YES)
set(RADIO_CALIBRATION_HALL YES)
set(MODEL_HAVE_NO_BITMAP YES)
set(HAVE_NO_RTC_BATTERY YES)

if (PCB STREQUAL TANGO)
  set(FLAVOUR tango)
  set(LCD_DEPTH 4)
  set(CPU_TYPE_FULL STM32F413xG)
  set(TARGET_LINKER_DIR stm32f413)
  set(TARGET_FLASH_SIZE 1M)
  add_definitions(-DRADIO_TANGO)
  add_definitions(-DSTM32F40_41xxx -DSTM32F413xx -DPCBTANGO)
elseif(PCB STREQUAL MAMBO)
  set(FLAVOUR mambo)
  set(LCD_DEPTH 1)
  set(CPU_TYPE_FULL STM32F407xE)
  set(TARGET_LINKER_DIR stm32f40x)
  set(TARGET_FLASH_SIZE 1M)
  add_definitions(-DHARDWARE_TRIMS)
  add_definitions(-DRADIO_MAMBO)
  add_definitions(-DSTM32F40_41xxx -DSTM32F407xx -DPCBMAMBO)
endif()

add_definitions(-DRADIO_FAMILY_TBS)
add_definitions(-DMANUFACTURER_TBS)
add_definitions(-DMODELS_IN_POP_UP_MENU)

add_definitions(-DDEFAULT_MODE=2)

set(GUI_DIR 128x64)
set(BITMAPS_DIR 128x64)
set(NAVIGATION_TYPE x7)
set(BITMAPS_TARGET 9x_bitmaps)
set(FONTS_TARGET 9x_fonts_1bit)
set(LCD_DRIVER lcd_driver_spi.cpp)

if(INTERNAL_MODULE_CRSF)
  add_definitions(-DHARDWARE_INTERNAL_MODULE)
  add_definitions(-DINTERNAL_MODULE_CRSF)
endif()

set(HSE_VALUE 8000000)
set(SDCARD YES)
set(EEPROM SDCARD)
set(TARGET_DIR tbs)
set(CMSIS_SRC targets/tbs/system_clock.c)

add_definitions(-DAUDIO -DVOICE -DRTCLOCK)

# Common drivers
# NOTE: most drivers (timers_driver, diskio_sdio, pwr_driver,
# abnormal_reboot, delays_driver) are already compiled into the
# stm32_drivers / stm32_drivers_w_dbg_fw obj libs via
# targets/common/arm/stm32/CMakeLists.txt — listing them here would cause
# multiple-definition link errors. audio_dac_driver.cpp needs a long list
# of AUDIO_* defines (DMA, timer, output GPIO/pin+AF) that are not mapped
# out yet in hal.h for TBS; Phase B work — for now stub audioInit /
# audioMute / audioUnmute in tbs_stubs.cpp.

# Target specific (relative to radio/src)
# NOTE: extmodule_driver.cpp and diskio.cpp from the legacy tbs-merge branch
# are intentionally dropped. extmodule_driver.cpp (legacy StdPeriph) is dead
# code in modern EdgeTX — external-module pulse/serial paths now run through
# hal/module_port.h + boards/generic_stm32/module_ports.cpp using the
# EXTMODULE_* defines in hal.h. diskio.cpp is superseded by
# targets/common/arm/stm32/diskio_sdio.cpp (already in FIRMWARE_SRC below).
set(TARGET_SRC
  malloc.c
)

# The TBS board sources (board.cpp, crsf_tasks.cpp, keys_driver.cpp, etc.)
# need the FreeRTOS include path (blob ABI — see below). Compile them through
# the `board` OBJECT library (defined later in this file), which scopes the
# include to those files.
#
# Startup and vector table come from targets/common/arm/stm32/system_init.c
# and targets/common/arm/stm32/f4/vectors_stm32f407xx.c — do not add our own
# startup.s, it duplicates Default_Handler / g_pfnVectors.
set(FIRMWARE_TARGET_SRC)

# Shared by bootloader and firmware: board.cpp, haptic, LCD, and the common
# stm32 drivers that the board.cpp code transitively needs (pwr, delays,
# abnormal_reboot — not pulled in by stm32_drivers).
set(BOARD_COMMON_SRC
  targets/tbs/board.cpp
  targets/tbs/haptic_driver.cpp
  targets/tbs/${LCD_DRIVER}
  # TBS-specific pwr_driver (active-high power switch — the common
  # targets/common/arm/stm32/pwr_driver.cpp assumes active-low).
  targets/tbs/pwr_driver.cpp
  targets/common/arm/stm32/delays_driver.cpp
  targets/common/arm/stm32/abnormal_reboot.cpp
)

# Bootloader board library
add_library(board_bl OBJECT EXCLUDE_FROM_ALL
  ${BOARD_COMMON_SRC}
)

# Firmware board library — additionally compiles backlight, keys, crsf_tasks,
# and (TANGO) the LED driver. Phase C will also put the CRSF system task
# here. Also pulls in the shared stm32 pulse / softserial / switch /
# mixer-scheduler / module-timer drivers that boards/generic_stm32/
# module_ports.cpp and switches.cpp reference.
set(FIRMWARE_BOARD_EXTRA_SRC
  targets/tbs/backlight_driver.cpp
  targets/tbs/crsf_tasks.cpp
  targets/tbs/keys_driver.cpp
  targets/tbs/tbs_stubs.cpp
  targets/common/arm/stm32/stm32_pulse_driver.cpp
  targets/common/arm/stm32/stm32_softserial_driver.cpp
  targets/common/arm/stm32/stm32_switch_driver.cpp
  targets/common/arm/stm32/module_timer_driver.cpp
  targets/common/arm/stm32/heartbeat_driver.cpp
  targets/common/arm/stm32/rotary_encoder_driver.cpp
  targets/common/arm/stm32/audio_dac_driver.cpp
  # mixer_scheduler_driver.cpp needs MIXER_SCHEDULER_TIMER in hal.h — Tango's
  # free timers need mapping (Phase B). The CRSF blob publishes mixer_schedule
  # via crossfireSharedData anyway. Stubbed in tbs_stubs.cpp for now.
)
if (PCB STREQUAL TANGO)
  list(APPEND FIRMWARE_BOARD_EXTRA_SRC
    targets/tbs/led_driver.cpp
  )
endif()

add_library(board OBJECT EXCLUDE_FROM_ALL
  ${BOARD_COMMON_SRC}
  ${FIRMWARE_BOARD_EXTRA_SRC}
)

# TBS board.cpp / crsf_tasks.cpp directly include FreeRTOS.h / task.h /
# semphr.h (legacy path, kept for blob ABI — see PORTING-PLAN.md "Phase C").
# Other modern EdgeTX targets wrap through os/*.h and don't need this.
target_include_directories(board PRIVATE
  ${RTOS_DIR}/include
  ${RTOS_DIR}/portable/GCC/ARM_CM4F
)
target_include_directories(board_bl PRIVATE
  ${RTOS_DIR}/include
  ${RTOS_DIR}/portable/GCC/ARM_CM4F
)

set(FIRMWARE_SRC ${FIRMWARE_SRC} $<TARGET_OBJECTS:board>)
set(BOOTLOADER_SRC ${BOOTLOADER_SRC} $<TARGET_OBJECTS:board_bl>)

# Per-target GUI sources that gui/128x64/CMakeLists.txt doesn't list
# globally — each target CMakeLists opts into the ones it needs. These are
# the same set the Taranis X7 target uses, since TBS shares its navigation
# and menu shape.
set(GUI_SRC ${GUI_SRC}
  model_input_edit.cpp
  model_mix_edit.cpp
  model_display.cpp
  radio_diagkeys.cpp
  radio_diaganas.cpp
  view_channels.cpp
  view_telemetry.cpp
  view_about.cpp
  bmp.cpp
)
