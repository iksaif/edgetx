/*
 * STM32F407 memory layout for TBS MAMBO.
 *
 * Differs from generic_stm32/linker/stm32f40x/layout.ld in one way: the
 * first 1 KB of CCM is carved out as a separate SHAREDRAM region so it
 * can hold the CrossfireSharedData struct at 0x10000000 (the ABI the
 * proprietary CRSF blob expects). Without this split, .data/.bss get
 * placed at 0x10000000 and corrupt the blob-visible struct.
 *
 * TANGO (F413) doesn't need this layout because its generic stm32f413
 * linker already puts .data/.bss in RAM, not CCM.
 */

/* Highest address of the user mode stack */
STACK_ADDRESS = ORIGIN(CCM) + LENGTH(CCM); /* end of CCM */

/* Required amount of stack for interrupt stack (Main stack) */
MAIN_STACK_SIZE = 1024;

/* Maximum bootloader code size */
BOOTLOADER_SIZE = 0x8000;

/* Minimum Heap Size (indicative) */
MIN_HEAP_SIZE = 0;

/* Highest heap address */
HEAP_ADDRESS = ORIGIN(RAM) + LENGTH(RAM);

/* Specify the memory areas */
MEMORY
{
  FLASH (rx) :
      ORIGIN = 0x08000000,
      LENGTH = DEFINED(__flash_size) ? __flash_size : 1024K

  RAM       (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
  /* SHAREDRAM = first 1 KB of CCM, reserved for CrossfireSharedData.
     The blob expects the struct at EXACTLY 0x10000000; any linker-placed
     data here corrupts the ABI. Don't assign any REGION_ALIAS to it. */
  SHAREDRAM (xrw) : ORIGIN = 0x10000000, LENGTH = 1K
  CCM       (xrw) : ORIGIN = 0x10000400, LENGTH = 63K
}

REGION_ALIAS("REGION_BOOTLOADER",   FLASH);
REGION_ALIAS("REGION_ISR_VECT",     FLASH);
REGION_ALIAS("REGION_TEXT",         FLASH);
REGION_ALIAS("REGION_TEXT_STORAGE", FLASH);

REGION_ALIAS("REGION_DATA", CCM);
REGION_ALIAS("REGION_BSS",  CCM);
REGION_ALIAS("REGION_RAM",  RAM);
