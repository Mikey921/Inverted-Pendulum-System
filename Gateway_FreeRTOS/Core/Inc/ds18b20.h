#ifndef __DS18B20_H
#define __DS18B20_H

#include "main.h"
#include "stdint.h"

// GPIO 操作宏
#define DQ_IN()  HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1)
#define DQ_OUT_0() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET)
#define DQ_OUT_1() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET)

void DS18B20_Delay_Init(void);
void DS18B20_Start_Convert(void);
int16_t DS18B20_Read_Result(void);

#endif
