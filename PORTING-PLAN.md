# TBS Tango II / Mambo - EdgeTX Porting Plan

This document outlines the step-by-step strategy for porting the TBS Tango II and Mambo radios from the legacy `tbs-merge` branch (Jan 2022) to the modern EdgeTX `main` head.

---

## Phase 1: Infrastructure & Build System
*Goal: Successfully trigger a compilation attempt for the new targets.*

- [x] **1.1 Register PCB Types**: Add `TANGO` and `MAMBO` to root `CMakeLists.txt`.
- [x] **1.2 Create Target Folder**: Setup `radio/src/targets/tbs/` with initial `CMakeLists.txt`.
- [x] **1.3 Scaffolding Files**: Copy `board.cpp/h`, `hal.h`, and architecture-specific files (`.s`, `.ld`, `malloc.c`).
- [ ] **1.4 Resolve Base Headers**: Fix includes in `board.cpp` to point to `edgetx.h` and modern HAL headers.

## Phase 2: Core Hardware Abstraction (HAL)
*Goal: Align with EdgeTX's Low-Level (LL) and HAL standards.*

- [ ] **2.1 Modernize GPIO**: Replace legacy `GPIO_Init` calls with `stm32_gpio.h` / `LL_GPIO` macros.
- [ ] **2.2 Clock Configuration**: Verify `SystemClock_Config` for the STM32F413/407 used in Tango II.
- [ ] **2.3 ADC Driver**: Port gimbal (Hall sensor) and battery voltage sensing to the EdgeTX `adc_driver.cpp` pattern.
- [ ] **2.4 Timer Drivers**: Port the 1ms and 2Mhz timers used for protocol synchronization.

## Phase 3: Monochrome Display Pipeline
*Goal: Get the UI rendering on the SPI display.*

- [ ] **3.1 Display Driver**: Port `lcd_driver_spi.cpp`.
- [ ] **3.2 DMA/Refresh Logic**: Update the display refresh loop to be task-safe for FreeRTOS.
- [ ] **3.3 Resolution Handling**: Ensure 128x64 (Mambo) and 128x96 (Tango II) layouts are correctly selected.

## Phase 4: Inputs & User Interface
*Goal: Enable buttons, switches, and navigation.*

- [ ] **4.1 Keys Driver**: Map the unique button matrix of the Tango II.
- [ ] **4.2 Rotary Encoder**: Port navigation logic for the Tango II thumbwheel/Mambo encoder.
- [ ] **4.3 Haptic & Audio**: Implement vibration and buzzer/DAC drivers using modern EdgeTX patterns.

## Phase 5: Internal Module (The "Proprietary" Layer)
*Goal: Enable Crossfire communication.*

- [ ] **5.1 Internal CRSF Driver**: Port `extmodule_driver.cpp` (which handles the internal module in TBS hardware).
- [ ] **5.2 CRSFShot Implementation**: Port the low-latency sync tasks.
- [ ] **5.3 Task Migration**: Move CRSF communication from legacy loops to dedicated FreeRTOS tasks (`radio/src/tasks/`).

## Phase 6: Storage & Data Integrity
*Goal: SD Card access and model saving.*

- [ ] **6.1 SDIO Driver**: Align `sdio_sd.c` with modern EdgeTX FS implementation.
- [ ] **6.2 EEPROM Layout**: Define the storage mapping (Tango II uses a specific sector of flash/SD for settings).

## Phase 7: Validation & Simulation
- [ ] **7.1 Simulator Support**: Add Tango II/Mambo profiles to EdgeTX Companion and Simulator.
- [ ] **7.2 Build Script**: Finalize `tools/build-tbs.py` for automated releases.

---

## Technical Notes
- **FreeRTOS**: The legacy code uses a custom scheduler. In modern EdgeTX, hardware initialization happens in `boardInit()`, and runtime logic must be moved to `main_task` or dedicated protocol tasks.
- **LL Drivers**: EdgeTX has moved away from `StdPeriph` toward `STM32 LL` (Low Layer) drivers to save flash space.
- **Tango II specific**: The internal module is technically "External" in the code logic but physically internal. This requires careful handling of `INTERNAL_MODULE_CRSF` vs `EXTERNAL_MODULE` defines.
