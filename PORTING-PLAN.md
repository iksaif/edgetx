# TBS Tango II / Mambo — EdgeTX Porting Plan

This document tracks the step-by-step strategy for porting the TBS Tango II
(`PCB=TANGO`) and Mambo (`PCB=MAMBO`) radios from the legacy `tbs-merge`
branch (Jan 2022) onto modern EdgeTX `main`.

Context backup (original legacy sources) lives in
`context_backup/porting_context/` — `tbs-merge/` (the 2022 OpenTX merge attempt)
and `freedomtx/` (TBS's official OpenTX 2.3 fork).

---

## Status snapshot

- ✅ Toolchain: ARM GNU 15.2.rel1 + newlib, Python venv with Pillow/lz4/pydantic/jinja2/clang
- ✅ Submodules: FreeRTOS, lvgl, stb, uf2
- ✅ `cmake -DPCB=TANGO` configures
- ✅ Bootloader: `bootloader.elf` links (~655 KB)
- ✅ Firmware: **`firmware.elf` links** (~438 KB .text, fits 1 MB F413 comfortably)
- ⏳ Real hardware bring-up, internal CRSF, companion/sim, release

### Build helper
`tools/build-tango.sh` wraps venv activation, PATH setup, cmake re-config,
and `make firmware`. Pass `--errors` (default) for a filtered view.


---

## Architectural note: the CRSF blob

Tango II and Mambo originally rely on a **closed-source TBS task binary** at
flash offset `CROSSFIRE_TASK_ADDRESS = 0x080C0020`. EdgeTX exposes callbacks to
the blob via a `trampoline[]` table in CCM RAM at `SHARED_MEMORY_ADDRESS =
0x10000000` (`CrossfireSharedData` in `radio/src/io/crsf/crossfire.h`). The
blob drives the internal-module CRSF UART and drops received bytes into
`crsf_rx` / reads `crsf_tx`.

We are pursuing **path 2** first: keep the blob ABI, modernize everything
around it. **Path 1** (drop the blob, use EdgeTX's native `io/crsf/` stack) is
the eventual goal — all blob-specific code is kept contained in
`radio/src/targets/tbs/crsf_tasks.{cpp,h}` and the `trampoline[]` / `_rtos_*`
wrappers inside `board.cpp` to keep path-1 migration bounded.

---

## Phase A — Get `firmware.elf` to link

Goal: `make firmware` produces a `firmware.elf` for `PCB=TANGO`. Functionality
can be stubbed; the aim is a clean link, not a working radio.

**A.1 — Fix FreeRTOS include path for firmware TBS sources**
  - `tbs/crsf_tasks.cpp` (`#include "FreeRTOS.h"`) compiles under the `firmware`
    target, not the `board` obj lib, so our earlier
    `target_include_directories(board PRIVATE ${RTOS_DIR}/include)` does not
    apply.
  - Options: add a `target_include_directories` on the `firmware` target, or
    move the TBS-specific `TARGET_SRC` files into a dedicated obj lib with its
    own include path. The latter is cleaner.

**A.2 — Modernize `crsf_tasks.cpp`**
  - Replace legacy `task_create((FUNCPtr)CROSSFIRE_TASK_ADDRESS, "crossfire",
    stack, prio)` call with modern
    `task_create(&crossfireTaskId, entry, "crossfire", stack, CROSSFIRE_STACK_SIZE, prio)`.
    The blob's entry address is cast through a `task_func_t`.
  - `vTaskDelay(pdMS_TO_TICKS(10))` → `RTOS_WAIT_MS(10)` (from `os/sleep.h`).
  - `vTaskSuspendAll()` — no direct `os/` equivalent. Keep as-is inside the
    blob-specific section, document why.
  - `xSemaphoreCreateBinaryStatic` — needed to populate `crossfireSharedData.taskSem`
    with blob-visible handles. Keep raw FreeRTOS here since the blob reads
    these as plain `SemaphoreHandle_t`; this is part of the ABI.

**A.3 — Remove dead legacy drivers from the build**
  Verified unused outside TBS target:
  - `extmodule_driver.cpp` (416 LOC, legacy StdPeriph): not referenced
    outside `targets/tbs/` and `simu/` stubs. Modern EdgeTX routes
    external module through `hal/module_port.h` + `generic_stm32/module_ports.cpp`.
    **Drop from `TARGET_SRC`.**
  - `diskio.cpp` (404 LOC): superseded by `targets/common/arm/stm32/diskio_sdio.cpp`
    which is already in `FIRMWARE_SRC`. **Delete file.**

**A.4 — Stub / port drivers called by firmware**
  - `usbChargerInit()` — called by `board.cpp:230`. Small file (40 LOC).
    Port legacy StdPeriph `GPIO_Init` → `gpio_init(GPIO_PIN(port, n), mode, speed)`.
    Or stub to no-op for Phase A and defer.
  - `keys_driver.cpp` — already mostly modern but uses `gpio_init(PORT, PIN, ...)`
    4-arg form. Modern API is `gpio_init(gpio_t, mode, speed)`. Fix hal.h
    `KEYS_GPIO_REG_*` / `KEYS_GPIO_PIN_*` to produce `gpio_t` values via
    `GPIO_PIN(port, n)`, then fix call sites. Rotary encoder lines in
    `keysInit()` also use the legacy 4-arg form.
  - `telemetry_driver.cpp` (415 LOC, legacy): only `intCrsfTelemetryFifo`
    (a `Fifo<uint8_t>`) is referenced outside TBS (in `io/crsf/crossfire.cpp`).
    **Minimal fix:** define `intCrsfTelemetryFifo` in a new small
    `tbs/tbs_telemetry_stubs.cpp`, stub `telemetryPortInit()` if called,
    leave the rest for Phase B.
  - `usb_agent_driver.cpp` — TBS's HID-joystick implementation. Modern
    `common/arm/stm32/usb_driver.cpp` provides `usbJoystickUpdate()` already.
    Drop this file; if any symbol is needed, stub it.

**A.5 — Firmware link — whack remaining undefined references**
  Iterate; each missing symbol is either (a) provided by a common module we
  need to include, or (b) a TBS-specific function we stub.

**Exit criteria**: `ls build/arm-none-eabi/radio/src/firmware.elf` succeeds.

---

## Phase B — First boot / real hardware bring-up

Goal: radio boots to a legible screen. Input is not required to work yet.

**B.1 — ADC driver**
  Stubbed in A. `hw_defs/tango.json` already describes ADC1 channels for
  LH/LV/RV/RH/VBAT. Enable generic_stm32 generation by ensuring the JSON
  lists valid GPIO/channel for each stick (currently `null` — a blocker).
  Add channel mapping from the original hardware:
  - Sticks are Hall sensors — the legacy code uses `RADIO_CALIBRATION_HALL=YES`
    (already set in CMakeLists). Check `context_backup/porting_context/tbs-merge`
    for the actual ADC channel/pin mapping.
  - VBAT is already PB1/ADC1_IN9 per tango.json.

**B.2 — Clock configuration**
  `system_clock.c` is already modern LL. Verify against STM32F413 reference
  clocks on first boot.

**B.3 — LCD driver**
  `lcd_driver_spi.cpp` uses modern `stm32_spi` / `stm32_dma`. Should work.
  Verify SPI3 wiring and contrast via `lcdSetRefVolt`.

**B.4 — Keys + rotary encoder**
  After A.4 finishes keys_driver modernization, wire
  `targets/common/arm/stm32/rotary_encoder_driver.cpp` into build. Requires
  adding in hal.h: `ROTARY_ENCODER_GRANULARITY`,
  `ROTARY_ENCODER_GPIO_A/GPIO_B` + `PIN_A/PIN_B` in `gpio_t` form,
  `ROTARY_ENCODER_EXTI_LINE2` / `SYS_LINE2`, `ROTARY_ENCODER_EXTI_PORT`.

**B.5 — Haptic + backlight**
  ✅ Done in Phase A work.

**B.6 — Power**
  `pwrInit` / `pwrOn` / `pwrOff` / `pwrPressed` — referenced from board.cpp.
  Originally in a tbs `pwr_driver.cpp`? Not in tree. Need to locate or port
  from `context_backup/porting_context/tbs-merge/`. The common
  `common/arm/stm32/pwr_driver.cpp` might already cover it if `PWR_*`
  defines in hal.h match.

**B.7 — SD card**
  `common/arm/stm32/diskio_sdio.cpp` is in the build. Need `STORAGE_USE_SDIO`
  (already set) and SD pinout in hal.h. Probably works out of the box.

**Exit criteria**: radio boots to a splash + model screen on hardware.

---

## Phase C — Internal module (the CRSF blob)

**C.1 — Obtain the blob**
  - Download a current TBS Tango II firmware release from the TBS Agent /
    TBS Agent Lite download URL (or their forum's firmware section).
  - The release is usually a `.frsk` or `.bin` packed image. Unpack with
    the TBS flasher's extractor (or binwalk / hand-parse).
  - The CRSF task blob sits at flash offset `0x080C0020 - 0x08000000 =
    0x000C0020` inside the image. Dump starting there to end-of-flash.
  - Header sanity check: first u32 ≠ `0xFFFFFFFF` means a real blob, any
    other value means erased sector (no blob present).
  - ⚠️ Licensing: TBS firmware is redistributed under their EULA. Local
    dev/test is fine; verify before shipping anything that redistributes
    their blob.

**C.2 — Reverse-engineer the blob**
  - Tool: Ghidra (free) or IDA. Cortex-M4F, STM32F413 SVD makes peripheral
    names auto-resolve.
  - Load the extracted blob as raw binary at base `0x080C0020`, language
    `ARM:LE:32:Cortex`.
  - Apply `CrossfireSharedData` struct (from `io/crsf/crossfire.h`) as a
    data type overlay at `0x10000000` (CCM RAM). Apply `TRAMPOLINE_INDEX`
    enum. Ghidra will auto-label most call sites that hit the trampoline
    or the shared-memory FIFOs.
  - Goal: confirm the blob is pure CRSF (no vendor-secret logic we can't
    re-implement natively). Note CRSFShot timing specifics, any gimbal
    sampling cadence, and any flags published via
    `crossfireSharedData.crsfFlag`.
  - Reference: the CRSF protocol is fully open (public spec, ELRS /
    BetaFlight / EdgeTX native impls), so we're reversing glue not crypto.
  - Deliverable: a short doc under `docs/` summarizing what the blob does,
    enough to plan Phase D (native CRSF migration).

**C.3 — Wire up internal module glue**
  - `extmodule_driver.cpp` is dead in modern EdgeTX. The internal module
    in TBS hardware is called "external" for historical reasons in the
    `INTERNAL_MODULE_CRSF` flow. Use `generic_stm32/module_ports.cpp`
    via the `INTMODULE_*` / `EXTMODULE_*` defines we already have in hal.h.
  - Verify `intCrsfTelemetryFifo` is wired into the CRSF rx path.

**C.4 — FreeRTOS task for CRSF**
  - `crossfireTasksCreate()` spawns the blob task pointing to
    `CROSSFIRE_TASK_ADDRESS`. Confirm modern `task_create` accepts a
    pre-allocated stack and a function pointer at an arbitrary address.
  - Handle the case where the blob sector is erased
    (`*(uint32_t*)CROSSFIRE_TASK_ADDRESS == 0xFFFFFFFF`) gracefully — the
    radio should still boot without the internal module.

**Exit criteria**: CRSF telemetry is visible and channels go out via the
internal module.

---

## Phase D — Native CRSF migration (deferred, tracked for later)

Once the blob is understood (C.2), we can replace it with EdgeTX's native
`io/crsf/` stack:
- Remove `CROSSFIRE_TASK_ADDRESS` / `SHARED_MEMORY_ADDRESS` indirection.
- Drop `trampoline[]` and the `tbs_trampoline_sem_take/give` wrappers in
  `board.cpp`.
- Drop `crsf_tasks.{cpp,h}`.
- Reclaim the flash region `0x080C0020` for firmware or storage.
- Internal module becomes a regular USART-driven CRSF port just like on
  any other modern EdgeTX target.

---

## Phase E — Ecosystem

**E.1 — Simulator profiles**: add Tango II / Mambo to `companion/src/simulation/`.
**E.2 — Companion**: add board entries, reset defaults matching DEFAULT_SWITCH_CONFIG.
**E.3 — Release build**: `tools/build-tbs.py` or equivalent.
**E.4 — CI**: add TANGO / MAMBO to GitHub Actions build matrix.

---

## Technical notes

- **FreeRTOS wrappers**: Modern EdgeTX uses `os/task.h`, `os/sleep.h`,
  `os/async.h` as the primary API. Direct `xSemaphore*` / `vTask*` only
  where the blob ABI requires it (contained in `crsf_tasks.cpp` and
  `board.cpp` trampoline section).
- **LL vs StdPeriph**: All legacy `GPIO_Init` / `RCC_ApbxPeriphClockCmd` /
  `GPIO_PinAFConfig` must become `gpio_init` / `stm32_gpio_enable_clock` /
  `gpio_init_af`.
- **GPIO_PIN macro**: Modern `gpio_t` values are produced by
  `GPIO_PIN(GPIOx, n)`. Older hal.h style with split PORT+PIN macros is
  incompatible with the one-arg `gpio_init` / `gpio_read`.
- **TBS vendor names**: Internal module is physically internal but is often
  labelled "external" in old code. Be careful: modern EdgeTX has real
  `INTERNAL_MODULE` vs `EXTERNAL_MODULE` concepts that must match the
  physical connector layout.
