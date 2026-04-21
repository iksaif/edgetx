/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_bus.h"

// TBS has 8 MHz HSE.
// For 168 MHz SYSCLK:
//   SYSCLK = (HSE / M) * N / P
//   168 = (8 / 4) * 168 / 2
//
// ⚠️ OVERCLOCK ON TANGO (STM32F413xG) — see datasheet DS11581:
//   F413 spec max HCLK = 100 MHz (CMSIS header RCC_MAX_FREQUENCY = 100 MHz,
//   scale-1 regulator). Running at 168 MHz exceeds every rated spec
//   (flash latency, regulator, PLL output). The legacy TBS firmware has
//   been shipping this config for years and the CRSF blob at
//   CROSSFIRE_TASK_ADDRESS was compiled assuming 168 MHz for CRSFShot
//   timing — dropping to spec-compliant 100 MHz will de-sync the blob.
//   MAMBO (STM32F407, max 168 MHz) is in spec at this setting.
//
//   DO NOT drop PLL_N to 100 without also re-timing the blob (Phase D
//   work, blob replacement). If you need a known-safe clock for a
//   dev-board without the blob, define TBS_SAFE_CLOCK at build time.
#if defined(TBS_SAFE_CLOCK)
// Spec-compliant 100 MHz for F413 dev/bringup without the blob. 3WS flash.
#define PLL_M   LL_RCC_PLLM_DIV_4
#define PLL_N   100
#define PLL_Q   LL_RCC_PLLQ_DIV_5   // 48 MHz USB: (8/4)*100/(4.something)...
#define SYSTEM_CLOCK_FLASH_LATENCY  LL_FLASH_LATENCY_3
#else
#define PLL_M   LL_RCC_PLLM_DIV_4
#define PLL_N   168
#define PLL_Q   LL_RCC_PLLQ_DIV_7
#define SYSTEM_CLOCK_FLASH_LATENCY  LL_FLASH_LATENCY_5
#endif

/**
  * @brief  System Clock Configuration for TBS (8MHz HSE)
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 168000000
  *            HCLK(Hz)                       = 168000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 4
  *            PLL_N                          = 168
  *            PLL_P                          = 2
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  */
void SystemClock_Config(void)
{
  /* Enable HSE oscillator */
  LL_RCC_HSE_Enable();
  while (LL_RCC_HSE_IsReady() != 1) {
  }

  /* Set FLASH latency — 5WS for 168 MHz (default), 3WS for the
     TBS_SAFE_CLOCK 100 MHz build. */
  LL_FLASH_SetLatency(SYSTEM_CLOCK_FLASH_LATENCY);

  /* Setup pre-fetch + caches */
  LL_FLASH_EnablePrefetch();
  LL_FLASH_EnableInstCache();
  LL_FLASH_EnableDataCache();

  /* Enable PWR clock */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* Setup voltage regulator */
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);

  /* Main PLL configuration and activation */
  LL_RCC_PLL_ConfigDomain_48M(LL_RCC_PLLSOURCE_HSE, PLL_M, PLL_N, PLL_Q);
  LL_RCC_PLL_Enable();
  while (LL_RCC_PLL_IsReady() != 1) {
  }

  /* Sysclk activation on the main PLL */
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
  }

  /* Set APB1 & APB2 prescaler */
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
}
