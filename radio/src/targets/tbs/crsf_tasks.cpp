/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 */

#include "edgetx.h"
#include "mixer_scheduler.h"
#include "crsf_tasks.h"

#if !defined(SIMU)

#include "io/crsf/crsf.h"
#include "io/crsf/crossfire.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

task_handle_t crossfireTaskId;
TASK_DEFINE_STACK(crossfireStack, CROSSFIRE_STACK_SIZE);

task_handle_t systemTaskId;
TASK_DEFINE_STACK(systemStack, SYSTEM_STACK_SIZE);

static void systemTask(void *pvParameters)
{
  static uint32_t getModelIdDelay = 0;
  volatile uint32_t delayCount = 0;
  bkregSetStatusFlag(CRSF_SET_MODEL_ID_PENDING);

  while (1) {
    if (getCrsfFlag(CRSF_FLAG_SHOW_BOOTLOADER_ICON)) {
      if (delayCount == 0) {
        delayCount = timersGetMsTick();
        vTaskSuspendAll();
        lcdOn();
        // drawDownload(); // TODO
        storageDirty(EE_GENERAL|EE_MODEL);
        storageCheck(true);
        sdDone();
      }
      if (timersGetMsTick() - delayCount >= 200) {
        NVIC_SystemReset();
      }
    }

    crsfSharedFifoHandler();
    // agentHandler(); // TODO

    if (bkregGetStatusFlag(CRSF_SET_MODEL_ID_PENDING) && timersGetMsTick() - getModelIdDelay > 1000) {
      crsfSetModelID();
      crsfGetModelID();
      if (currentCrsfModelId == g_model.header.modelId[INTERNAL_MODULE])
        bkregClrStatusFlag(CRSF_SET_MODEL_ID_PENDING);
      getModelIdDelay = timersGetMsTick();
    }
    
    if (g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_NONE && isMixerTaskScheduled()) {
      clearMixerTaskSchedule();
      mixerSchedulerISRTrigger();
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

typedef void (*FUNCPtr)(void *);
StaticSemaphore_t xSemaphoreBuffer[TASK_SEM_COUNT];

void crossfireTasksCreate()
{
  crossfireTaskId = task_create((FUNCPtr)CROSSFIRE_TASK_ADDRESS, "crossfire", crossfireStack, CROSSFIRE_TASK_PRIO);
  systemTaskId = task_create(systemTask, "system", systemStack, RTOS_SYS_TASK_PRIO);
}

void crossfireTasksStart()
{
  SemaphoreHandle_t taskSem[TASK_SEM_COUNT] = {0};
  
  // Test if crossfire task is available and start it
  if (*(uint32_t *)CROSSFIRE_TASK_ADDRESS != 0xFFFFFFFF) {
    crossfireTasksCreate();
    taskSem[XF_TASK_SEM] = xSemaphoreCreateBinaryStatic(&xSemaphoreBuffer[XF_TASK_SEM]);
    taskSem[CRSF_SD_TASK_SEM] = xSemaphoreCreateBinaryStatic(&xSemaphoreBuffer[CRSF_SD_TASK_SEM]);
    taskSem[BOOTLOADER_ICON_WAIT_SEM] = xSemaphoreCreateBinaryStatic(&xSemaphoreBuffer[BOOTLOADER_ICON_WAIT_SEM]);

    for (uint8_t i = 0; i < TASK_SEM_COUNT; i++) {
      crossfireSharedData.taskSem[i] = (uint32_t *)taskSem[i];
    }
  }
}

void crossfireTasksStop()
{
  // NVIC_DisableIRQ(INTERRUPT_EXTI_IRQn);
  // NVIC_DisableIRQ(INTERRUPT_NOT_TIMER_IRQn);
}

#endif
