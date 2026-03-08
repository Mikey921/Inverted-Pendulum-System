# 🚀 倒立摆分布式控制与边缘网关系统 (Inverted Pendulum Edge Gateway)

![STM32](https://img.shields.io/badge/MCU-STM32F103-blue.svg)
![RTOS](https://img.shields.io/badge/OS-FreeRTOS-brightgreen.svg)
![Python](https://img.shields.io/badge/SCADA-Python_3.8+-yellow.svg)
![Build](https://img.shields.io/badge/Build-CMake_%7C_Keil-orange.svg)

本项目是一个基于 **STM32** 与 **FreeRTOS** 的工业级双向同步控制系统。通过将“底层控制节点”与“边缘路由网关”进行物理分离，并结合基于 Python 的 SCADA 交互终端，实现了对倒立摆物理姿态的微秒级高频采集与实时 PID 参数交互整定。



## ✨ 核心特性 (Key Features)

* **分布式双主站架构**：支持本地旋钮/按键与远程 Modbus 双向调参。首创 **“500ms 冷却屏蔽期”** 机制，完美解决工业多主站场景下的参数踩踏与覆盖冲突。
* **微秒级高速串口状态机**：摒弃低效的阻塞式任务轮询，下沉至 `RXNE` 硬件中断直读 `DR` 寄存器；配合 `ORE` (Overrun Error) 溢出防御与错误清除，实现 50ms 密集通信下的零丢包与绝对防锁死。
* **FreeRTOS 高可靠并发**：采用 `中断解析 -> 队列 (Queue) 投递 -> 任务互斥锁 (Mutex) 更新` 的空间隔离设计，彻底消除 ISR 阻塞、任务抢占引发的内核断言崩溃 (HardFault) 与数据撕裂 (Data Tearing)。
* **跨平台 SCADA 交互终端**：基于 Python 多线程架构实现无阻塞的实时数据流监控，提供类似 Linux 终端的命令行交互界面，支持毫秒级状态同步与不停机 PID 参数下发。

---

## 🗂️ 系统架构与开发环境 (Architecture & Toolchains)

本项目由三个完全解耦的子系统组成，根据核心诉求分别采用了最契合的开发工具链：

### 1. 边缘路由网关 (`/Gateway_FreeRTOS`)
充当网络中枢，负责 Modbus RTU 协议解析、多任务并发调度、上下位机的高频数据路由分发及互斥锁管理。
* **硬件基座**: STM32F103 (韦东山平台)
* **底层驱动**: STM32 HAL 库 + FreeRTOS
* **构建系统**: VS Code + CMake + GCC ARM Toolchain

### 2. 底层物理节点 (`/Node_StdLib`)
充当智能执行器，专精于高实时性的 MPU6050 姿态解算、步进电机驱动、底层 PID 闭环运算与本地编码器交互。
* **硬件基座**: STM32F103 (江科大平台)
* **底层驱动**: STM32 标准外设库 (Standard Peripheral Library)
* **构建系统**: Keil MDK-ARM

### 3. 上位机监控终端 (`/Python_SCADA`)
充当 HMI 人机交互界面，提供终端全景指令集与实时状态数据流监控。
* **运行环境**: Python 3.8+
* **核心依赖**: `pymodbus`

---

## 🚀 快速启动 (Quick Start)

### 1. 硬件通信拓扑
* **网关与节点**：通过 USART 交叉接线 (`TX` 接 `RX`, `RX` 接 `TX`)，并确保两块核心板**共地 (GND)**。
* **网关与 PC**：网关通过 USB 转 RS485 (或 TTL) 模块连接至上位机 PC，并在设备管理器中确认 COM 端口号。

### 2. 固件编译与烧录
由于双核系统采用不同工具链，请分别进行独立编译：
* **编译网关**：在 VS Code 中打开 `Gateway_FreeRTOS` 目录，配置 CMake 插件 (选择 `GCC for arm-none-eabi`)，点击 Build 生成 `.elf`/`.hex`，通过 OpenOCD / J-Link 烧录。
* **编译节点**：使用 Keil MDK 打开 `Node_StdLib` 目录下的 `.uvprojx` 工程文件，一键编译并下载至底层节点。

### 3. 运行上位机监控系统
在终端中进入 Python 脚本目录，安装依赖并启动实时监控终端：
```bash
cd Python_SCADA
pip install pymodbus
python pendulum_monitor.py
