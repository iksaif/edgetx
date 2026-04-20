# TBS Tango II and Mambo support

option(DISK_CACHE "Enable SD card disk cache" OFF)
option(INTERNAL_MODULE_CRSF "Support for crossfire internal module" ON)

set(CPU_TYPE STM32F4)
set(CPU_TYPE_FULL STM32F407xE)
set(LINKER_SCRIPT targets/tbs/stm32f4_flash.ld)
set(RTC_BACKUP_RAM YES)
set(HAPTIC YES)
set(USB_CHARGER YES)
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
set(FIRMWARE_SRC
  ${FIRMWARE_SRC}
  targets/common/arm/stm32/pwr_driver.cpp
  targets/common/arm/stm32/delays_driver.cpp
  targets/common/arm/stm32/timers_driver.cpp
  targets/common/arm/stm32/audio_dac_driver.cpp
  targets/common/arm/stm32/diskio_sdio.cpp
  targets/common/arm/stm32/abnormal_reboot.cpp
)

# Target specific (relative to radio/src)
set(TARGET_SRC
  extmodule_driver.cpp
  malloc.c
  crsf_tasks.cpp
)

set(FIRMWARE_TARGET_SRC
  startup_stm32f40_41xxx.s
  ${LCD_DRIVER}
  board.cpp
  backlight_driver.cpp
)

if (PCB STREQUAL TANGO)
  set(FIRMWARE_TARGET_SRC
    ${FIRMWARE_TARGET_SRC}
    led_driver.cpp
  )
endif()

# Lib targets (must use full paths relative to radio/src)
set(BOOTLOADER_TARGET_SRC
  targets/common/arm/stm32/pwr_driver.cpp
  targets/common/arm/stm32/delays_driver.cpp
  targets/common/arm/stm32/abnormal_reboot.cpp
  targets/tbs/${LCD_DRIVER}
  targets/tbs/board.cpp
)

add_library(board_bl OBJECT EXCLUDE_FROM_ALL
  ${BOOTLOADER_TARGET_SRC}
)

set(BOARD_SOURCES
  targets/tbs/startup_stm32f40_41xxx.s
  targets/tbs/${LCD_DRIVER}
  targets/tbs/board.cpp
  targets/tbs/backlight_driver.cpp
)

if (PCB STREQUAL TANGO)
  set(BOARD_SOURCES
    ${BOARD_SOURCES}
    targets/tbs/led_driver.cpp
  )
endif()

add_library(board OBJECT EXCLUDE_FROM_ALL
  ${BOARD_SOURCES}
)

set(FIRMWARE_SRC ${FIRMWARE_SRC} $<TARGET_OBJECTS:board>)
set(BOOTLOADER_SRC ${BOOTLOADER_SRC} $<TARGET_OBJECTS:board_bl>)
