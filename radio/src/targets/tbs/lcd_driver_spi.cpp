/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hal/gpio.h"
#include "stm32_gpio.h"
#include "stm32_spi.h"
#include "stm32_dma.h"

#include "board.h"
#include "debug.h"
#include "lcd.h"

#include "hal/abnormal_reboot.h"
#include "timers_driver.h"

#if !defined(BOOT)
  #include "edgetx.h"
#endif

#define RESET_WAIT_DELAY_MS            300
#define WAIT_FOR_DMA_END()             do { } while (lcd_busy)

#define LCD_NCS_HIGH()  gpio_set(LCD_NCS_GPIO)
#define LCD_NCS_LOW()   gpio_clear(LCD_NCS_GPIO)

#if defined(RADIO_TANGO)
#define LCD_DC_HIGH()   gpio_set(LCD_DC_GPIO)
#define LCD_DC_LOW()    gpio_clear(LCD_DC_GPIO)
#elif defined(RADIO_MAMBO)
#define LCD_A0_HIGH()   gpio_set(LCD_A0_GPIO)
#define LCD_A0_LOW()    gpio_clear(LCD_A0_GPIO)
#endif

#define LCD_RST_HIGH()  gpio_set(LCD_RST_GPIO)
#define LCD_RST_LOW()   gpio_clear(LCD_RST_GPIO)

volatile bool lcd_busy = false;
bool lcd_on = false;
bool lcdInitFinished = false;

static void spiWrite(uint8_t byte)
{
#if defined(RADIO_TANGO)
  LCD_DC_LOW();
#elif defined(RADIO_MAMBO)
  LCD_A0_LOW();
#endif
  LCD_NCS_LOW();

  while ((LCD_SPI->SR & SPI_SR_TXE) == 0) { }
  (void)LCD_SPI->DR; // Clear receive
  LCD_SPI->DR = byte;
  while ((LCD_SPI->SR & SPI_SR_RXNE) == 0) { }

  LCD_NCS_HIGH();
}

void lcdWriteCommand(uint8_t command)
{
  spiWrite(command);
}

void lcdSetRefVolt(uint8_t val)
{
  if (val > 127) {
    val = 127;
  }
  uint16_t setVal = val + LCD_CONTRAST_OFFSET;
  lcdWriteCommand(0x81); // Set Vop
  lcdWriteCommand(setVal & 0x3F); // the lower 6 bits for EV
  lcdWriteCommand(0x20 | (setVal >> 6)); // and the higher 3 bits for ratio
}

static void spiWriteArg(uint8_t arg)
{
  spiWrite(arg);
}

void lcdHardwareInit()
{
  stm32_spi_enable_clock(LCD_SPI);
  gpio_init_af(LCD_MOSI_GPIO, LCD_GPIO_AF, GPIO_PIN_SPEED_HIGH);
  gpio_init_af(LCD_CLK_GPIO, LCD_GPIO_AF, GPIO_PIN_SPEED_HIGH);

  LCD_SPI->CR1 = 0;
  LCD_SPI->CR1 = SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_CPOL | SPI_CR1_CPHA | LCD_SPI_PRESCALER;
  LCD_SPI->CR2 = 0;
  LCD_SPI->CR1 |= SPI_CR1_MSTR;
  LCD_SPI->CR1 |= SPI_CR1_SPE;

  gpio_init(LCD_NCS_GPIO, GPIO_OUT, GPIO_PIN_SPEED_MEDIUM);
  LCD_NCS_HIGH();

  gpio_init(LCD_RST_GPIO, GPIO_OUT, GPIO_PIN_SPEED_MEDIUM);

#if defined(RADIO_TANGO)
  gpio_init(LCD_DC_GPIO, GPIO_OUT, GPIO_PIN_SPEED_HIGH);
#elif defined(RADIO_MAMBO)
  gpio_init(LCD_A0_GPIO, GPIO_OUT, GPIO_PIN_SPEED_HIGH);
#endif

  stm32_dma_enable_clock(LCD_DMA);
  LCD_DMA_Stream->CR &= ~DMA_SxCR_EN;
  LCD_DMA->HIFCR = LCD_DMA_FLAGS;
  LCD_DMA_Stream->CR =  DMA_SxCR_PL_0 | DMA_SxCR_MINC | DMA_SxCR_DIR_0;
  LCD_DMA_Stream->PAR = (uint32_t)&LCD_SPI->DR;
  LCD_DMA_Stream->FCR = 0x05;

  NVIC_EnableIRQ(LCD_DMA_Stream_IRQn);
}

extern "C" void LCD_DMA_Stream_IRQHandler()
{
  DEBUG_INTERRUPT(INT_LCD);

  LCD_DMA_Stream->CR &= ~DMA_SxCR_TCIE;
  LCD_DMA->HIFCR |= LCD_DMA_FLAG_INT;
  LCD_SPI->CR2 &= ~SPI_CR2_TXDMAEN;
  LCD_DMA_Stream->CR &= ~DMA_SxCR_EN;

  while (LCD_SPI->SR & SPI_SR_BSY) { }
  LCD_NCS_HIGH();
  lcd_busy = false;
}

void lcdOn()
{
  if (!lcd_on) {
    LCD_NCS_LOW();
#if defined(RADIO_TANGO)
    LCD_DC_LOW();
    lcdWriteCommand(0xFD); spiWriteArg(0x12);
    lcdWriteCommand(0xAF);
#elif defined(RADIO_MAMBO)
    LCD_A0_LOW();
    lcdWriteCommand(0xAF);
#endif
    LCD_NCS_HIGH();
    lcd_on = true;
  }
}

bool isLcdOn()
{
  return lcd_on;
}

void lcdOff()
{
  if (lcd_on) {
    WAIT_FOR_DMA_END();
    lcdWriteCommand(0xAE);
    delay_ms(3);
    lcd_on = false;
  }
}

void lcdRefreshWait()
{
  WAIT_FOR_DMA_END();
}

void lcdRefresh(bool wait)
{
  if (!lcd_on) return;
  WAIT_FOR_DMA_END();

#if defined(RADIO_TANGO)
  LCD_NCS_LOW();
  LCD_DC_LOW();
  lcdWriteCommand(0x75); spiWriteArg(0); spiWriteArg(LCD_H - 1);
  lcdWriteCommand(0x15); spiWriteArg(0); spiWriteArg((LCD_W / 2) - 1);
  LCD_DC_HIGH();
#elif defined(RADIO_MAMBO)
  LCD_NCS_LOW();
  LCD_A0_LOW();
  lcdWriteCommand(0xB0); spiWriteArg(0);
  lcdWriteCommand(0x10);
  lcdWriteCommand(0x00);
  LCD_A0_HIGH();
#endif

  lcd_busy = true;
  LCD_DMA_Stream->CR &= ~DMA_SxCR_EN;
  LCD_DMA->HIFCR = LCD_DMA_FLAGS;
  LCD_DMA_Stream->M0AR = (uint32_t)displayBuf;
#if defined(RADIO_TANGO)
  LCD_DMA_Stream->NDTR = (LCD_W * LCD_H) / 2;
#else
  LCD_DMA_Stream->NDTR = (LCD_W * LCD_H) / 8;
#endif
  LCD_DMA_Stream->CR |= DMA_SxCR_EN | DMA_SxCR_TCIE;
  LCD_SPI->CR2 |= SPI_CR2_TXDMAEN;

  if (wait) WAIT_FOR_DMA_END();
}

void lcdInit()
{
  lcdHardwareInit();
  LCD_RST_LOW();
  delay_ms(150);
  LCD_RST_HIGH();
}
