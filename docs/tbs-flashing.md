# TBS Tango II / Mambo — Flashing Runbook

> ⚠️ **Read "Prerequisites" at the end of this doc first.** The stock
> TBS bootloader expects a DIFFERENT flash layout than ours (48 KB vs
> 32 KB reserve). Flashing only `firmware.bin` on a stock radio will
> brick it until you re-flash via DFU. Always flash BOTH `bootloader.bin`
> and `firmware.bin` together.

After a `make firmware` build, the two artifacts you flash to the radio:

| File | Build path | Size | Flash address |
|---|---|---|---|
| Bootloader | `build/arm-none-eabi/bootloader.bin` | ~27 KB | `0x08000000` |
| Firmware | `build/arm-none-eabi/firmware.bin` | ~450 KB | `0x08008000` |

Both `.bin` files are raw images — no headers, no offset padding. The
firmware `.bin` starts at the firmware's `.isr_vector` and does **not**
include the bootloader region.

## Flash map (STM32F413, 1 MB)

```
0x08000000 ┌──────────────────┐
           │ bootloader.bin   │  BOOTLOADER_SIZE = 0x8000  (32 KB reserved)
0x08008000 ├──────────────────┤
           │ firmware.bin     │  vector table + .text + .data LMA
           │ (grows upward)   │
0x080C0020 ├──────────────────┤
           │ CRSF blob        │  CROSSFIRE_TASK_ADDRESS
           │ (TBS proprietary)│  ~256 KB reserved — do not overwrite
0x08100000 └──────────────────┘  end of 1 MB flash
```

The bootloader's `jumpTo(APP_START_ADDRESS)` lands exactly at `0x08008000`,
so both must agree on `BOOTLOADER_SIZE = 0x8000`. This is set in
`radio/src/boards/generic_stm32/linker/stm32f413/layout.ld` (linker) and
`radio/src/targets/tbs/board.h` (runtime).

## RAM

```
0x20000000                         320 KB SRAM (F413)
  .data / .ccm / .bss / .ram / .stack in order
0x2004FFFC  .reboot_buffer (4 bytes — abnormal_reboot.cpp)
0x20050000  top of stack
```

No CCM on F413 — the linker's `REGION_ALIAS("CCM", RAM)` maps it into the
same 320 KB block, which is why `.ccm` VMA lives at `0x200001D8` in the
section dump instead of `0x10000000`.

## Shared memory (CRSF blob ABI)

```
0x10000000  CCM RAM alias used by the blob-side shared-memory ABI
            (CrossfireSharedData in radio/src/io/crsf/crossfire.h).
            This region is currently UNUSED on our side because io/crsf/*.cpp
            is not compiled (Phase C work). When Phase C lands, the blob and
            firmware communicate through this region via xSemaphoreTake/Give
            trampolines (board.cpp tbs_trampoline_sem_take/give).
```

## Flashing — STM32CubeProgrammer (recommended)

With the radio in DFU mode (hold EXIT while inserting USB):

```bash
# Bootloader
STM32_Programmer_CLI -c port=usb1 -w build/arm-none-eabi/bootloader.bin 0x08000000

# Firmware
STM32_Programmer_CLI -c port=usb1 -w build/arm-none-eabi/firmware.bin 0x08008000

# Verify + reset
STM32_Programmer_CLI -c port=usb1 -rst
```

## Flashing — dfu-util

```bash
# Bootloader goes to the first DFU alt setting ("@Internal Flash /0x08000000/...")
dfu-util -a 0 -s 0x08000000 -D build/arm-none-eabi/bootloader.bin

# Firmware at +0x8000
dfu-util -a 0 -s 0x08008000 -D build/arm-none-eabi/firmware.bin
```

If you only have the firmware flashed (no bootloader) the radio will fail
to boot because the Cortex-M4 reset vector is at the start of flash
(`0x08000000`) — it fetches SP and PC from whatever is there. Flash the
bootloader first.

## SWD / ST-Link fallback

If DFU mode is unreachable (e.g. after a bad firmware bricks the radio),
connect an ST-Link to the Tango II's internal SWD pads and use
`openocd`:

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program build/arm-none-eabi/bootloader.bin 0x08000000 verify" \
  -c "program build/arm-none-eabi/firmware.bin  0x08008000 verify reset exit"
```

## First-boot checklist (Phase B.5)

After the first successful flash, verify in order:

1. Power latch (B.1) — does the radio stay on after release of the power
   button? If it flashes on then dies, `pwrPressed()` polarity is still
   wrong. Our driver expects PB.14 active-high with pull-down; confirm
   with a multimeter.
2. LCD splash (pre-existing `lcd_driver_spi.cpp`, untouched) — any
   pixels light up at boot?
3. MENU / EXIT / PAGE / ENTER keys (keys_driver.cpp) — the `gpio_t` form
   + single-arg `gpio_read` path should give correct active-low behavior.
4. Rotary encoder (B.2) — navigate the model menu; if direction is
   reversed, `#define ROTARY_ENCODER_INVERTED` in hal.h.
5. VBAT reading (B.3) — displayed on the main screen. Should read in the
   3-4 V range; calibration is `TBS_BATT_SCALE` in `board.h`.
6. Audio beep on key press (B.4) — any sound from the speaker? Mute pin
   polarity may need flipping (`INVERTED_MUTE_PIN` in hal.h if silent).

**Expected broken on Phase B**:
- Sticks read as zero — the Hall gimbal module talks to the CRSF blob,
  not to our ADC. Phase C work.
- Internal module (CRSF TX) — same story; blob handles the UART.
- Telemetry — stubbed in `tbs_stubs.cpp` (telemetryPortInit is a no-op).
- USB mass storage / joystick — not wired yet.

If the radio boots to the model screen with working keys, VBAT, and
navigation, Phase B is done. Everything else is Phase C / D.

---

## Prerequisites — must do before Phase B.5

Two things will silently brick a live Tango II if you skip them. Both
were discovered by inspecting a production Tango II SD card + firmware
image.

### 1. Hardware revision check — is your chip F407 or F413?

Production TBS firmware is compiled for STM32F407 (128 KB SRAM, SP top
`0x20020000`) to cover both early PCB revisions (F407-native) and later
ones (F413, backwards-compatible). Our port is **F413-specific**:
`CMakeLists.txt` sets `CPU_TYPE_FULL = STM32F413xG`, giving us 320 KB
SRAM and the F413 peripheral map.

If you flash an F413 build on an F407-rev Tango II, at minimum you'll
lose SRAM (the linker places the stack at `0x20050000`, which is past
the end of F407's 128 KB SRAM — stack writes hit a bus fault). The
`tbs_verify_cpu_or_halt()` safety net in `boardInit()` reads
`DBGMCU->IDCODE` and refuses to latch power if the chip doesn't match
the build:

- F413/F423 → `DEV_ID = 0x463`
- F407/F417 → `DEV_ID = 0x413`

If you boot and the radio cuts power the instant you release the power
button, that's the check firing. **Solutions**:
1. Flash an F407-compatible build — requires changing `CPU_TYPE_FULL` in
   `radio/src/targets/tbs/CMakeLists.txt` to `STM32F407xE` and
   `TARGET_LINKER_DIR` to `stm32f40x_tbs` (MAMBO already does this).
2. Or visually inspect the LQFP100 chip silkscreen on the main PCB
   before flashing — reads either "STM32F407xx" or "STM32F413xx".

### 2. Always flash BOTH `bootloader.bin` AND `firmware.bin`

The stock TBS bootloader uses the tbs-merge memory layout:
`BOOTLOADER_SIZE = 0xC000` (48 KB), firmware starts at `0x0800C000`.
Our port uses the modern EdgeTX layout: `BOOTLOADER_SIZE = 0x8000`
(32 KB), firmware at `0x08008000`.

If you only flash our `firmware.bin` at `0x08008000` and leave the
stock bootloader in place:
1. On boot the stock bootloader jumps to `APP_START_ADDRESS = 0x0800C000`.
2. That address is 16 KB into our firmware — somewhere in the middle
   of `.text`. Not a valid Cortex-M vector table.
3. The chip fetches garbage as the initial SP and Reset_Handler →
   hard-fault → repeat forever.
4. Radio appears bricked. You'll need SWD to recover.

**Always flash both**. The runbook commands at the top of this doc flash
both in the correct order. If you want to be extra safe, also erase the
entire main flash first with `STM32_Programmer_CLI -c port=SWD -e all`
before programming — that wipes any stale blob from `0x080C0020` and
starts completely fresh.

### 3. Have a recovery plan

Before flashing a dev build for the first time, confirm you can get back
to the stock TBS firmware:
- Download the latest TBS Agent X (macOS/Windows/Linux) BEFORE you need
  it. Package ID is something like `TBS_AGENT_X_INSTALLER`.
- TBS Agent's "Firmware → Recovery" flow rewrites both the radio
  firmware AND the CRSF blob region. If our dev build overwrites the
  blob area (which `TARGET_FLASH_SIZE = 768K` + the `0xB8020` write
  ceiling in `bin_fw_files.cpp` prevent, but belt-and-suspenders), TBS
  Agent Recovery is the way back.
- An ST-Link V2 or V3 clone (~$10) + `openocd` gives you a
  bootloader-independent route to re-flash anything from scratch. Pads
  for SWDIO/SWCLK/NRST live under the battery cover on most Tango II
  revs.
