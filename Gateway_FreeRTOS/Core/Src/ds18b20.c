#include "ds18b20.h"

extern TIM_HandleTypeDef htim3;

// 微秒延时
void delay_us(uint16_t us) {
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    HAL_TIM_Base_Start(&htim3);
    while (__HAL_TIM_GET_COUNTER(&htim3) < us);
    HAL_TIM_Base_Stop(&htim3);
}

void DS18B20_Delay_Init(void) {
    HAL_TIM_Base_Start(&htim3);
}

// 复位
uint8_t DS18B20_Reset(void) {
    uint8_t presence;
    DQ_OUT_0(); delay_us(480);
    DQ_OUT_1(); delay_us(30);
    presence = (DQ_IN() == 0) ? 1 : 0;
    delay_us(450);
    return presence;
}

// 写字节
void DS18B20_Write_Byte(uint8_t dat) {
    for (uint8_t i = 0; i < 8; i++) {
        if (dat & 0x01) { // 写1
            DQ_OUT_0(); delay_us(2);
            DQ_OUT_1(); delay_us(60);
        } else { // 写0
            DQ_OUT_0(); delay_us(60);
            DQ_OUT_1(); delay_us(2);
        }
        dat >>= 1;
    }
}

// 读字节
uint8_t DS18B20_Read_Byte(void) {
    uint8_t dat = 0;
    for (uint8_t i = 0; i < 8; i++) {
        dat >>= 1;
        DQ_OUT_0(); delay_us(2);
        DQ_OUT_1(); delay_us(10); // 采样
        if (DQ_IN()) dat |= 0x80;
        delay_us(50);
    }
    return dat;
}

void DS18B20_Start_Convert(void) {
    DS18B20_Reset();
    DS18B20_Write_Byte(0xCC); // Skip ROM
    DS18B20_Write_Byte(0x44); // Convert T
}

// 读取温度 (返回 x10 后的整数)
int16_t DS18B20_Read_Result(void) {
    // 读数据
    DS18B20_Reset();
    DS18B20_Write_Byte(0xCC);
    DS18B20_Write_Byte(0xBE); // Read Scratchpad
    
    uint8_t TL = DS18B20_Read_Byte();
    uint8_t TH = DS18B20_Read_Byte();
    
    int16_t temp = (TH << 8) | TL;
    return (int16_t)((float)temp * 0.625); // 0.0625 * 10
}