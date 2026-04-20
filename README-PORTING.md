# EdgeTX TBS Tango II Porting Workspace

This directory is set up to facilitate the porting of TBS Tango II and Mambo support to a modern EdgeTX head.

## Structure
- `.` : Current EdgeTX `main` branch (v2.12/v2.13 head).
- `porting_context/tbs-merge/`: The last known attempt to merge TBS support into EdgeTX (from the `tbs-merge` branch, circa Jan 2022).
- `porting_context/freedomtx/`: The official TBS FreedomTX implementation (forked from OpenTX 2.3).

## Key Files to Port
1. **`targets/tbs/board.cpp` & `board.h`**: Hardware definitions and pin mapping.
2. **`targets/tbs/lcd_driver_spi.cpp`**: The monochrome 128x64 SPI display driver.
3. **`targets/tbs/extmodule_driver.cpp`**: Internal module (Crossfire) handling.
4. **`tools/build-tbs.py`**: Build script logic.

## Strategy
1. **HAL Alignment**: The files in `porting_context/tbs-merge/radio/src/targets/tbs/` need to be refactored to match the modern EdgeTX HAL in `radio/src/targets/common/arm/stm32/`.
2. **FreeRTOS**: Transition legacy task loops to the FreeRTOS task system used in modern EdgeTX.
3. **Internal Module**: Ensure `INTERNAL_MODULE_CRSF` is correctly enabled in the new board target.

## Build Command (Initial Target)
You will likely need to create a new `radio/src/targets/tbs/` directory and update the root `CMakeLists.txt` to include `TANGO` and `MAMBO` in the `PCB_TYPES`.
