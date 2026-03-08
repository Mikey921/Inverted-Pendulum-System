#ifndef __MODBUS_H
#define __MODBUS_H

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stdint.h"

// === Modbus 基础配置 ===
#define SLAVE_ID        0x01    // [Modbus Poll设置 -> Slave ID] 从机地址，必须一致
#define MODBUS_BUF_SIZE 128     // 缓冲区大小

typedef enum {
    MB_STATE_IDLE = 0,    // 空閒/接收中，允許中斷寫入
    MB_STATE_PROCESSING   // 處理中，禁止中斷寫入 rx_buffer
} ModbusState_t;

// === 寄存器定义 (菜单) ===
// 对应 Modbus Poll [Read/Write Definition] -> [Address] 0 ~ 3
typedef struct {
    // 输入寄存器(只读)
    uint16_t current_angle;     // 寄存器 0: 当前角度 
    int16_t current_location;   // 寄存器 1: 当前位置 
    int16_t target_location;    // 寄存器 2: 目标位置

    // 角度环(内环)PID参数 - 放大1000倍
    int16_t angle_kp;           // 寄存器 3: 角度环(内环)kp
    int16_t angle_ki;           // 寄存器 4: 角度环(内环)ki
    int16_t angle_kd;           // 寄存器 5: 角度环(内环)kd

    // 位置环(外环)PID参数 - 放大1000倍
    int16_t location_kp;        // 寄存器 6: 位置环(外环)kp
    int16_t location_ki;        // 寄存器 7: 位置环(外环)ki
    int16_t location_kd;        // 寄存器 8: 位置环(外环)kd

    int16_t run_mode;           // 寄存器 9: 倒立摆自启摆模式 (0=关闭, 1=开启)
} System_Registers_t;

typedef struct {
  int16_t location;
  uint16_t angle; 
  int16_t target_location;
  uint16_t angle_kp;
  uint16_t angle_ki;
  uint16_t angle_kd;
  uint16_t location_kp;
  uint16_t location_ki;
  uint16_t location_kd;
  uint8_t run_mode;
} Data_t;

// === 全局变量 ===
extern System_Registers_t sys_regs;
extern uint8_t  rx_buffer[MODBUS_BUF_SIZE];
extern uint16_t rx_len;
extern ModbusState_t mb_state;
extern SemaphoreHandle_t g_Modbus_Rx_Sem;

void Modbus_Process(void);

#endif
