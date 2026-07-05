/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.h
  * @brief   This file contains all the function prototypes for
  *          the i2c.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __I2C_H__
#define __I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN Private defines */
#define OLED_I2C_ADDRESS     0x78 //通过调整0R电阻,屏可以0x78和0x7A两个地址 -- 默认0x78

#define OLED_CHAR_EN_SIZE_8x8    1     //8*8英文字符
#define OLED_CHAR_EN_SIZE_8x16   2     //8*16英文字符

#define OLED_BLACK_ON_WHITE      1     //白底黑字
#define OLED_WHITE_ON_BLACK      0     //黑底白字

#define OLED_DISPLAY_FILL_BLACK  0x00  //熄灭所有
#define OLED_DISPLAY_FILL_WHILE  0xFF  //点亮所有
/* USER CODE END Private defines */

void MX_I2C1_Init(void);

/* USER CODE BEGIN Prototypes */
unsigned char OLED_Init(void);
// void OLED_SetPos(unsigned char x, unsigned char y);
// void OLED_Fill(unsigned char fill_Data);
// void OLED_Fill_Part(unsigned char fill_Data, unsigned char x1, unsigned char x2, unsigned char y1, unsigned char y2);
// void OLED_CLS(void);
// void OLED_ON(void);
// void OLED_OFF(void);
// void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize, unsigned char backlight);
// void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N, unsigned char backlight);
// void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char BMP[]);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H__ */

