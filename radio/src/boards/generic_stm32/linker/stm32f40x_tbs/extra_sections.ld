/* extra_sections.ld for TBS MAMBO — identical to stm32f40x/extra_sections.ld,
 * just re-rooted here so the TBS-specific TARGET_LINKER_DIR is
 * self-contained. */

PROVIDE(_heap_start = _eram);

INCLUDE section_ccm.ld

PROVIDE(_ccm_heap_start = _eccm);
PROVIDE(_ccm_heap_end = _main_stack_start);

/* Reserve heap space in RAM */
._user_heap(NOLOAD) :
{
  . = ALIGN(4);
  . = . + MIN_HEAP_SIZE;
  . = ALIGN(4);
} > REGION_RAM

/* Reserve stack space in CCM */
.stack(NOLOAD) :
{
  . = ALIGN(4);
  . = . + MAIN_STACK_SIZE;
  . = ALIGN(4);
} > CCM
