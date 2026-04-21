/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef _HAL_H_
#define _HAL_H_

  #if defined(RADIO_TANGO)
    #define LCD_W                           128
    #define LCD_H                           96
  #elif defined(RADIO_MAMBO)
    #define LCD_W                           128
    #define LCD_H                           64
  #endif

  #define LCD_CONTRAST_MIN                0
  #define LCD_CONTRAST_MAX                45
  #define LCD_CONTRAST_DEFAULT            25
  #define LCD_CONTRAST_OFFSET             130

  #define CPU_FREQ            168000000
  #define PERI1_FREQUENCY     42000000
  #define PERI2_FREQUENCY     84000000

  #define TIMER_MULT_APB1     2
  #define TIMER_MULT_APB2     2

  #define ADC_VREF_PREC2      330

  // Keys — gpio_t (GPIO_PIN(port, n)) form for modern stm32_gpio API.
  #define KEYS_GPIO_PIN_MENU                GPIO_PIN(GPIOD, 13)
  #define KEYS_GPIO_PIN_EXIT                GPIO_PIN(GPIOD, 14)
  #define KEYS_GPIO_PIN_PAGE                GPIO_PIN(GPIOD, 12)
  #define KEYS_GPIO_PIN_ENTER               GPIO_PIN(GPIOD, 4)

  // Rotary Encoder
  // The modern targets/common/arm/stm32/rotary_encoder_driver.cpp uses two
  // naming conventions at the same time:
  //   * ENC_GPIO_PIN_A/_B — gpio_t (GPIO_PIN(port, n)) — used by this
  //     target's keys_driver in read paths.
  //   * ROTARY_ENCODER_GPIO + _GPIO_PIN_A/_B — raw GPIOx port pointer
  //     and LL_GPIO_PIN_n bitmasks — used by LL_GPIO_Init / GPIOx->IDR
  //     inside the common driver.
  // Both pins are on GPIOA (PA.8 = A, PA.10 = B) so we use the single-port
  // branch of the common driver (ROTARY_ENCODER_GPIO without _A/_B split).
  #define ROTARY_ENCODER_NAVIGATION
  #define ROTARY_ENCODER_GRANULARITY        2

  #define ENC_GPIO_PIN_A                    GPIO_PIN(GPIOA, 8)
  #define ENC_GPIO_PIN_B                    GPIO_PIN(GPIOA, 10)

  #define ROTARY_ENCODER_GPIO               GPIOA
  #define ROTARY_ENCODER_GPIO_PIN_A         LL_GPIO_PIN_8
  #define ROTARY_ENCODER_GPIO_PIN_B         LL_GPIO_PIN_10

  // ROTARY_ENCODER_POSITION is read inside an ISR — go straight to IDR.
  // Produces { B_bit | A_bit } as a 2-bit value 0..3.
  #define ROTARY_ENCODER_POSITION() \
      (((GPIOA->IDR >> 9) & 0x02) | ((GPIOA->IDR >> 8) & 0x01))

  #define ROTARY_ENCODER_EXTI_LINE1         LL_EXTI_LINE_8
  #define ROTARY_ENCODER_EXTI_LINE2         LL_EXTI_LINE_10
  #define ROTARY_ENCODER_EXTI_PORT          LL_SYSCFG_EXTI_PORTA
  #define ROTARY_ENCODER_EXTI_SYS_LINE1     LL_SYSCFG_EXTI_LINE8
  #define ROTARY_ENCODER_EXTI_SYS_LINE2     LL_SYSCFG_EXTI_LINE10

  // TIM10 drives the 100 µs debounce/settle tick the common driver uses.
  #define ROTARY_ENCODER_TIMER              TIM10
  #define ROTARY_ENCODER_TIMER_IRQn         TIM1_UP_TIM10_IRQn
  #define ROTARY_ENCODER_TIMER_IRQHandler   TIM1_UP_TIM10_IRQHandler

  // AUX Serial
  #define AUX_SERIAL_USART                  UART4
  #define AUX_SERIAL_GPIO                   GPIOA
  #define AUX_SERIAL_TX_GPIO_PIN            LL_GPIO_PIN_0  // PA.00
  #define AUX_SERIAL_RX_GPIO_PIN            LL_GPIO_PIN_1  // PA.01
  #define AUX_SERIAL_GPIO_AF                LL_GPIO_AF_8
  #define AUX_SERIAL_USART_IRQn             UART4_IRQn
  #define AUX_SERIAL_USART_IRQHandler       UART4_IRQHandler

  // Telemetry
  #define TELEMETRY_DIR_INPUT             false
  #define TELEMETRY_USART                 USART6
  #define TELEMETRY_TX_GPIO               GPIO_PIN(GPIOC, 6)
  #define TELEMETRY_RX_GPIO               GPIO_PIN(GPIOC, 7)
  #define TELEMETRY_EXTI_PORT             LL_SYSCFG_EXTI_PORTC
  #define TELEMETRY_EXTI_SYS_LINE         LL_SYSCFG_EXTI_LINE9
  #define TELEMETRY_EXTI_LINE             LL_EXTI_LINE_9
  #define TELEMETRY_FIFO_SIZE             128
  #define TELEMETRY_SET_INPUT             false
  #define TELEMETRY_USART_IRQn            USART6_IRQn
  #define TELEMETRY_USART_IRQHandler      USART6_IRQHandler
  #ifndef TELEMETRY_USART_IRQ_PRIORITY
    #define TELEMETRY_USART_IRQ_PRIORITY    5
  #endif
  #define TELEMETRY_DMA                   DMA1
  #define TELEMETRY_DMA_Channel_TX        LL_DMA_CHANNEL_4
  #define TELEMETRY_DMA_Stream_TX         LL_DMA_STREAM_6
  #define TELEMETRY_DMA_TX_Stream_IRQ     DMA1_Stream6_IRQn
  #define TELEMETRY_DMA_TX_IRQHandler     DMA1_Stream6_IRQHandler
  #ifndef TELEMETRY_DMA_IRQ_PRIORITY
    #define TELEMETRY_DMA_IRQ_PRIORITY      5
  #endif

  // Internal Module (mapped to Crossfire for TBS)
  #define INTMODULE_TX_GPIO               GPIO_PIN(GPIOC, 6)
  #define INTMODULE_TX_GPIO_AF            LL_GPIO_AF_8
  #define INTMODULE_TIMER                 TIM8
  #define INTMODULE_TIMER_FREQ            (PERI2_FREQUENCY * TIMER_MULT_APB2)
  #define INTMODULE_TIMER_Channel         LL_TIM_CHANNEL_CH1
  #define INTMODULE_TIMER_IRQn            TIM8_CC_IRQn
  #define INTMODULE_TIMER_IRQHandler      TIM8_CC_IRQHandler
  #define INTMODULE_TIMER_DMA             DMA2
  #define INTMODULE_TIMER_DMA_STREAM      LL_DMA_STREAM_2
  #define INTMODULE_TIMER_DMA_CHANNEL     LL_DMA_CHANNEL_0
  #define INTMODULE_TIMER_DMA_STREAM_IRQn DMA2_Stream2_IRQn
  #define INTMODULE_TIMER_DMA_IRQHandler  DMA2_Stream2_IRQHandler

  // External Module
  #if defined(RADIO_TANGO)
    #define EXTMODULE_USART                 USART3
    #define EXTMODULE_GPIO                  GPIOD
    #define EXTMODULE_TX_GPIO_PIN           LL_GPIO_PIN_8  // PD.08
    #define EXTMODULE_RX_GPIO_PIN           LL_GPIO_PIN_9  // PD.09
    #define EXTMODULE_GPIO_AF               LL_GPIO_AF_7
    #define EXTMODULE_USART_TX_DMA_STREAM   LL_DMA_STREAM_3
    #define EXTMODULE_USART_TX_DMA_CHANNEL  LL_DMA_CHANNEL_4
  #elif defined(RADIO_MAMBO)
    #define EXTMODULE_USART                 USART1
    #define EXTMODULE_GPIO                  GPIOA
    #define EXTMODULE_TX_GPIO_PIN           LL_GPIO_PIN_9  // PA.09
    #define EXTMODULE_RX_GPIO_PIN           LL_GPIO_PIN_10 // PA.10
    #define EXTMODULE_GPIO_AF               LL_GPIO_AF_7
    #define EXTMODULE_USART_TX_DMA_STREAM   LL_DMA_STREAM_7
    #define EXTMODULE_USART_TX_DMA_CHANNEL  LL_DMA_CHANNEL_4
  #endif

  // USB
  #define USB_GPIO_AF                       LL_GPIO_AF_10
  #define USB_GPIO_DP                       GPIO_PIN(GPIOA, 12)
  #define USB_GPIO_DM                       GPIO_PIN(GPIOA, 11)

  // LCD driver
  #if defined(RADIO_TANGO)
    #define LCD_SPI_GPIO                    GPIOB
    #define LCD_MOSI_GPIO                   GPIO_PIN(GPIOB, 5)
    #define LCD_CLK_GPIO                    GPIO_PIN(GPIOB, 3)
    #define LCD_NCS_GPIO                    GPIO_PIN(GPIOD, 1)
    #define LCD_RST_GPIO                    GPIO_PIN(GPIOD, 3)
    #define LCD_DC_GPIO                     GPIO_PIN(GPIOD, 6)
    #define LCD_DMA                         DMA1
    #define LCD_DMA_Stream                  DMA1_Stream7
    #define LCD_DMA_Stream_IRQn             DMA1_Stream7_IRQn
    #define LCD_DMA_Stream_IRQHandler       DMA1_Stream7_IRQHandler
    #define LCD_DMA_FLAGS                   (DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7 | DMA_HIFCR_CTEIF7 | DMA_HIFCR_CDMEIF7 | DMA_HIFCR_CFEIF7)
    #define LCD_DMA_FLAG_INT                DMA_HIFCR_CTCIF7
    #define LCD_SPI                         SPI3
    #define LCD_GPIO_AF                     LL_GPIO_AF_6
    #define LCD_SPI_PRESCALER               LL_SPI_BAUDRATEPRESCALER_DIV2
  #elif defined(RADIO_MAMBO)
    #define LCD_SPI_GPIO                    GPIOB
    #define LCD_MOSI_GPIO                   GPIO_PIN(GPIOB, 5)
    #define LCD_CLK_GPIO                    GPIO_PIN(GPIOB, 3)
    #define LCD_A0_GPIO                     GPIO_PIN(GPIOD, 0)
    #define LCD_NCS_GPIO                    GPIO_PIN(GPIOD, 1)
    #define LCD_RST_GPIO                    GPIO_PIN(GPIOD, 3)
    #define LCD_DMA                         DMA1
    #define LCD_DMA_Stream                  DMA1_Stream7
    #define LCD_DMA_Stream_IRQn             DMA1_Stream7_IRQn
    #define LCD_DMA_Stream_IRQHandler       DMA1_Stream7_IRQHandler
    #define LCD_DMA_FLAGS                   (DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7 | DMA_HIFCR_CTEIF7 | DMA_HIFCR_CDMEIF7 | DMA_HIFCR_CFEIF7)
    #define LCD_DMA_FLAG_INT                DMA_HIFCR_CTCIF7
    #define LCD_SPI                         SPI3
    #define LCD_GPIO_AF                     LL_GPIO_AF_6
    #define LCD_SPI_PRESCALER               LL_SPI_BAUDRATEPRESCALER_DIV2
  #endif

  // SD
  #define SD_SDIO_DMA                       DMA2
  #define SD_SDIO_DMA_STREAM                DMA2_Stream3
  #define SD_SDIO_DMA_CHANNEL               LL_DMA_CHANNEL_4
  #define SD_SDIO_DMA_IRQn                  DMA2_Stream3_IRQn
  #define SD_SDIO_DMA_IRQHANDLER            DMA2_Stream3_IRQHandler
  #define SD_SDIO_FIFO_ADDRESS              ((uint32_t)0x40012C80)
  #define SD_SDIO_CLK_DIV(fq)               ((48000000 / (fq)) - 2)
  #define SD_SDIO_INIT_CLK_DIV            SD_SDIO_CLK_DIV(400000)
  #define SD_SDIO_TRANSFER_CLK_DIV        SD_SDIO_CLK_DIV(24000000)

  #define SD_PRESENT_GPIO                   GPIO_PIN(GPIOC, 5)
  #define STORAGE_USE_SDIO

  // Audio — DAC1 channel 1 on PA.4, driven by TIM6 through DMA1 Stream 5
  // (same pinout as X7 / XLITE family).
  #define AUDIO_OUTPUT_GPIO                 GPIO_PIN(GPIOA, 4)
  #define AUDIO_DMA                         DMA1
  #define AUDIO_DMA_Stream                  DMA1_Stream5
  #define AUDIO_DMA_Stream_IRQn             DMA1_Stream5_IRQn
  #define AUDIO_DMA_Stream_IRQHandler       DMA1_Stream5_IRQHandler
  #define AUDIO_TIMER                       TIM6
  // Mute pin (amp enable) — TANGO uses PD.5, MAMBO uses PE.0.
  #if defined(RADIO_TANGO)
    #define AUDIO_MUTE_GPIO                 GPIO_PIN(GPIOD, 5)
  #elif defined(RADIO_MAMBO)
    #define AUDIO_MUTE_GPIO                 GPIO_PIN(GPIOE, 0)
  #endif

  // Haptic
  #define HAPTIC_GPIO                       GPIO_PIN(GPIOB, 0)  // PB.00

  // Power / soft-power-latch (TBS Tango II schematics)
  // PB.14 = power switch read (active high — we invert in pwr_driver
  // via gpio_read)
  // PB.12 = "keep radio on" latch driven by firmware; set high to stay on,
  // clear low to cut power. Held by an external P-MOSFET gate circuit.
  #define PWR_SWITCH_GPIO                   GPIO_PIN(GPIOB, 14)
  #define PWR_ON_GPIO                       GPIO_PIN(GPIOB, 12)

  // Timers
  #define MS_TIMER                          TIM14
  #define MS_TIMER_IRQn                     TIM8_TRG_COM_TIM14_IRQn
  #define MS_TIMER_IRQHandler               TIM8_TRG_COM_TIM14_IRQHandler

  #define TIMER_2MHz_TIMER                  TIM7

  #define INTERRUPT_NOT_TIMER               TIM13
  #define INTERRUPT_NOT_IRQn                TIM8_UP_TIM13_IRQn
  #define INTERRUPT_NOT_IRQHandler          TIM8_UP_TIM13_IRQHandler

  #define INTERRUPT_EXTI_LINE               LL_EXTI_LINE_9 // For telemetry EXTI
  #define INTERRUPT_EXTI_IRQn               EXTI9_5_IRQn
  #define INTERRUPT_EXTI_IRQHandler         EXTI9_5_IRQHandler

#endif // _HAL_H_
