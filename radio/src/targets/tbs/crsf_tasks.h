/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 */

#pragma once

// Tasks that glue the TBS CRSF blob at CROSSFIRE_TASK_ADDRESS to EdgeTX.
// See PORTING-PLAN.md "Architectural note: the CRSF blob" — all blob-specific
// runtime state is kept in this translation unit (plus the trampoline
// wrappers in board.cpp) so that migrating to native CRSF (Phase D) is a
// bounded change.

#if !defined(BOOT)
#include "os/task.h"
#if defined(FREE_RTOS)
#include "os/task_freertos.h"
#endif

// FreeRTOS priorities. Use tskIDLE_PRIORITY as the base like tasks.h does.
#include <FreeRTOS/include/FreeRTOS.h>
#include <FreeRTOS/include/task.h>

#define CROSSFIRE_STACK_SIZE 512
#define SYSTEM_STACK_SIZE    512

#define CROSSFIRE_TASK_PRIO  (tskIDLE_PRIORITY + 3)
#define SYSTEM_TASK_PRIO     (tskIDLE_PRIORITY + 2)

#if !defined(SIMU)

extern task_handle_t crossfireTaskId;
extern StackType_t   crossfireStack[CROSSFIRE_STACK_SIZE];

extern task_handle_t systemTaskId;
extern StackType_t   systemStack[SYSTEM_STACK_SIZE];

void crossfireTasksCreate();
void crossfireTasksStart();
void crossfireTasksStop();

#endif
#endif
