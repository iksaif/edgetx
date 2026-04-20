/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 */

// TBS CRSF blob glue tasks. See PORTING-PLAN.md "Architectural note: the
// CRSF blob". This file deliberately keeps the blob ABI (raw FreeRTOS
// semaphores, static task at a fixed address) contained so that the eventual
// migration to EdgeTX's native io/crsf/ stack (Phase D) is a bounded change.

#include "crsf_tasks.h"

#if !defined(SIMU)

#include "os/task.h"
#include "os/sleep.h"
#include "os/time.h"

#include "io/crsf/crossfire.h"

#include <FreeRTOS/include/FreeRTOS.h>
#include <FreeRTOS/include/task.h>
#include <FreeRTOS/include/semphr.h>

task_handle_t crossfireTaskId;
StackType_t   crossfireStack[CROSSFIRE_STACK_SIZE] __attribute__((aligned(8)));

task_handle_t systemTaskId;
StackType_t   systemStack[SYSTEM_STACK_SIZE] __attribute__((aligned(8)));

// TODO(port Phase C): wire up the system task to run alongside the blob
// (CRSFShot, model-id negotiation, telemetry drain, etc.). For now it's a
// minimal loop so the task exists and yields.
static void systemTask()
{
  while (1) {
    sleep_ms(10);
  }
}

// The CRSF blob is statically linked at CROSSFIRE_TASK_ADDRESS in flash
// (see board.h). task_create takes a task_func_t; the blob's entry point is
// just a plain function there. If the sector is erased (no blob), skip
// everything so the radio can still boot for bring-up work.
void crossfireTasksCreate()
{
  if (*(uint32_t*)CROSSFIRE_TASK_ADDRESS == 0xFFFFFFFF) {
    return;
  }

  task_create(&crossfireTaskId,
              reinterpret_cast<task_func_t>(CROSSFIRE_TASK_ADDRESS),
              "crossfire", crossfireStack,
              CROSSFIRE_STACK_SIZE, CROSSFIRE_TASK_PRIO);

  task_create(&systemTaskId, systemTask, "system", systemStack,
              SYSTEM_STACK_SIZE, SYSTEM_TASK_PRIO);
}

// The CRSF blob expects taskSem[] to hold real FreeRTOS semaphore handles.
// We keep the static buffer here and publish pointers into
// crossfireSharedData so the blob can xSemaphoreTake/Give on them.
static StaticSemaphore_t xSemaphoreBuffer[TASK_SEM_COUNT];

void crossfireTasksStart()
{
  if (*(uint32_t*)CROSSFIRE_TASK_ADDRESS == 0xFFFFFFFF) {
    return;
  }

  crossfireTasksCreate();

  SemaphoreHandle_t taskSem[TASK_SEM_COUNT] = {0};
  taskSem[XF_TASK_SEM] =
    xSemaphoreCreateBinaryStatic(&xSemaphoreBuffer[XF_TASK_SEM]);
  taskSem[CRSF_SD_TASK_SEM] =
    xSemaphoreCreateBinaryStatic(&xSemaphoreBuffer[CRSF_SD_TASK_SEM]);
  taskSem[BOOTLOADER_ICON_WAIT_SEM] =
    xSemaphoreCreateBinaryStatic(&xSemaphoreBuffer[BOOTLOADER_ICON_WAIT_SEM]);

  for (uint8_t i = 0; i < TASK_SEM_COUNT; i++) {
    crossfireSharedData.taskSem[i] = (uint32_t*)taskSem[i];
  }
}

void crossfireTasksStop()
{
  // Deliberately left empty — task teardown is not exercised on reboot
  // (NVIC_SystemReset is used instead).
}

#endif
