#ifndef _board_h_
#define _board_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "stm32l4xx_hal.h"
#include "display.h"

/**
 * TFT stuff
 */

#if 1
#define TFT_W 80
#define TFT_H 160  // 162 on GRAM

#define TFT_OFFSET_SOURCE	26
#define TFT_OFFSET_GATE		1
#define TFT_BGR_FILTER
#else
#define TFT_W 128
#define TFT_H 160
#endif

#define SPI_BLOCK_XFER
#include "st7735.h"

#define LCD_CS1 LCD_CS_GPIO_Port->BSRR = LCD_CS_Pin
#define LCD_CS0 LCD_CS_GPIO_Port->BRR = LCD_CS_Pin
#define LCD_BKL1 LCD_BKL_GPIO_Port->BSRR = LCD_BKL_Pin
#define LCD_BKL0 LCD_BKL_GPIO_Port->BRR = LCD_BKL_Pin
#define LCD_RST1 LCD_RST_GPIO_Port->BSRR = LCD_RST_Pin
#define LCD_RST0 LCD_RST_GPIO_Port->BRR = LCD_RST_Pin
#define LCD_CD1 LCD_BKL_GPIO_Port->BSRR = LCD_CD_Pin
#define LCD_CD0 LCD_BKL_GPIO_Port->BRR = LCD_CD_Pin

#define LCD_PIN_INIT

static inline void SPI_Send(uint8_t data){
	HAL_SPI_Transmit(&hspi1, &data, 1, 1000);
}

static inline void SPI_Read(uint8_t *dst, uint32_t len){
	HAL_SPI_Receive(&hspi1, dst, len, 1000);
}

static inline void SPI_Write(uint8_t *dst, uint32_t len){
	HAL_SPI_Transmit(&hspi1, dst, len, 1000);
}

#define BTN1_PRESSED (HAL_GPIO_ReadPin(BTN1_GPIO_Port, BTN1_Pin) == GPIO_PIN_RESET)

//-----------------------------------------------------------
#define DelayMs HAL_Delay

#ifdef __cplusplus
}
#endif

#endif
