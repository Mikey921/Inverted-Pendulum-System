#include "modbus.h"

extern UART_HandleTypeDef huart1;
extern SemaphoreHandle_t g_xUpdateTrigger;
extern SemaphoreHandle_t g_xMutex;
extern QueueHandle_t g_xQueue;

uint32_t g_param_cooldown = 0;

ModbusState_t mb_state = MB_STATE_IDLE;

// 初始化寄存器默认值: 当前角度, 当前位置, 目标位置, 内环Kp, 内环Ki, 内环Kd, 外环Kp, 外环Ki, 外环Kd, 自启摆模式
System_Registers_t sys_regs = {
    0, 0,           // 实时数据
    0,              // 目标位置
    300, 10, 400,   // 内环参数
    400, 0, 4000,   // 外环参数
    0               // 自启摆模式
};

// 定义接收缓冲区变量
uint8_t  rx_buffer[MODBUS_BUF_SIZE];
uint16_t rx_len = 0;
uint8_t  frame_received_flag = 0;
uint8_t  tx_buffer[MODBUS_BUF_SIZE];

// 计算 CRC16
uint16_t Modbus_CRC16(uint8_t *buf, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

// === 处理 03 功能码 (读数据) ===
// 对应 Modbus Poll 设置: Function = 03 Read Holding Registers
void Process_Read_03(void) {
    // 提取电脑想要读取的起始地址 (Address)
    uint16_t start_addr = (rx_buffer[2] << 8) | rx_buffer[3];
    // 提取电脑想要读取的数量 (Quantity)
    uint16_t quantity   = (rx_buffer[4] << 8) | rx_buffer[5];

    if (start_addr + quantity > 10) return; // 简单防越界

    // 组装回复报文头
    tx_buffer[0] = SLAVE_ID;
    tx_buffer[1] = 0x03;
    tx_buffer[2] = quantity * 2; // 字节数

    // 填充数据 (注意大小端转换)
    uint16_t *pRegs = (uint16_t*)&sys_regs;
    int idx = 3;
    for (int i = 0; i < quantity; i++) {
        uint16_t val = pRegs[start_addr + i];
        // Modbus 是大端模式 (高位在前)，STM32 是小端，所以要移位
        tx_buffer[idx++] = (val >> 8) & 0xFF;
        tx_buffer[idx++] = val & 0xFF;
    }

    // CRC
    uint16_t crc = Modbus_CRC16(tx_buffer, idx);
    tx_buffer[idx++] = crc & 0xFF;
    tx_buffer[idx++] = (crc >> 8) & 0xFF;

    // 发送 (自动流控模块不需要切换GPIO，直接发)
    HAL_UART_Transmit(&huart1, tx_buffer, idx, 100);
}

// === 处理 06 功能码 (写单个数据) ===
// 对应 Modbus Poll 操作: 双击表格里的数值 -> Send
void Process_Write_06(void) {
    // 提取写入地址和数值
    uint16_t addr = (rx_buffer[2] << 8) | rx_buffer[3];
    uint16_t val  = (rx_buffer[4] << 8) | rx_buffer[5];

    if (addr > 9) return;

    if (xSemaphoreTake(g_xMutex, portMAX_DELAY) == pdTRUE)
    {
        // 修改 STM32 内存里的数据
        uint16_t *pRegs = (uint16_t*)&sys_regs;
        pRegs[addr] = val;

        g_param_cooldown = HAL_GetTick() + 500;

        xQueueReset(g_xQueue);

        xSemaphoreGive(g_xMutex);
    }

    if (g_xUpdateTrigger != NULL) {
        xSemaphoreGive(g_xUpdateTrigger); 
    }

    // 原样返回请求报文作为确认
    HAL_UART_Transmit(&huart1, rx_buffer, 8, 100);
}

// === Modbus 主处理函数 ===
void Modbus_Process(void) {
    // 基础校验(最短4字节)
    if (rx_len < 4) return;
    // ID 校验
    if (rx_buffer[0] != SLAVE_ID) return;

    // CRC 校验
    uint16_t cal_crc = Modbus_CRC16(rx_buffer, rx_len - 2);
    uint16_t rec_crc = rx_buffer[rx_len - 2] | (rx_buffer[rx_len - 1] << 8);
    if (cal_crc != rec_crc) return;

    // 【关键物理延时】
    // 自动流控 RS485 模块需要时间从“接收态”切换到“发送态”
    // 如果不加这句，发出去的数据头会被吞掉，导致 Timeout
    vTaskDelay(pdMS_TO_TICKS(10));

    // 根据功能码分发任务
    switch (rx_buffer[1]) {
        case 0x03: Process_Read_03(); break;    // 读
        case 0x06: Process_Write_06(); break;   // 写
    }
}