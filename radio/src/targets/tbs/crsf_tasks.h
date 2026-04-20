/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 */

#pragma once

#if !defined(BOOT)
#include "os/task.h"
#if defined(FREE_RTOS)
#include "os/task_freertos.h"
#endif

#define CROSSFIRE_STACK_SIZE 512
#define SYSTEM_STACK_SIZE 512

#define CROSSFIRE_TASK_PRIO (RTOS_LOWEST_TASK_PRIO + 3)

#if !defined(SIMU)

extern task_handle_t crossfireTaskId;
extern TASK_DEFINE_STACK(crossfireStack, CROSSFIRE_STACK_SIZE);

extern task_handle_t systemTaskId;
extern TASK_DEFINE_STACK(systemStack, SYSTEM_STACK_SIZE);

void crossfireTasksCreate();
void crossfireTasksStart();
void crossfireTasksStop();

#endif
#endif
