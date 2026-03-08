import time
import threading
import sys
from pymodbus.client import ModbusSerialClient

# ---------------- 配置参数 ----------------
PORT = 'COM4'  # 请确认你的串口号
BAUDRATE = 115200
SLAVE_ID = 1

# 初始化 Modbus RTU 客户端
client = ModbusSerialClient(
    port=PORT,
    baudrate=BAUDRATE,
    timeout=0.3,
    parity='N',
    stopbits=1,
    bytesize=8
)

# 线程同步标志位
pause_monitor = False


def to_int16(uint16_val):
    """处理补码，防止负数被截断为巨大的正数"""
    if uint16_val > 32767:
        return uint16_val - 65536
    return uint16_val


def parse_run_state(state_val):
    """【核心映射】将底层的 RunState 翻译为人类可读的物理状态"""
    if state_val == 0:
        return "\033[91mSTOP (0)\033[0m"  # 红色: 停止
    elif state_val == 21:
        return "\033[93mSWING(21)\033[0m"  # 黄色: 正在起摆
    elif state_val == 31:
        return "\033[96mCATCH(31)\033[0m"  # 青色: 抓取稳态中
    elif state_val == 4:
        return "\033[92mSTABLE(4)\033[0m"  # 绿色: 完美倒立
    else:
        return f"RUN ({state_val})"


def monitor_thread():
    """后台守护线程：负责疯狂读取并刷新终端屏幕"""
    global pause_monitor

    while True:
        if pause_monitor:
            time.sleep(0.1)
            continue

        try:
            # 读取 0~9 共 10 个寄存器
            response = client.read_holding_registers(address=0, count=10, device_id=SLAVE_ID)

            if not response.isError():
                regs = response.registers

                # 物理数据
                ang = to_int16(regs[0])
                loc = to_int16(regs[1])
                tar = to_int16(regs[2])

                # PID 数据 (还原小数)
                akp, aki, akd = to_int16(regs[3]) / 1000, to_int16(regs[4]) / 1000, to_int16(regs[5]) / 1000
                lkp, lki, lkd = to_int16(regs[6]) / 1000, to_int16(regs[7]) / 1000, to_int16(regs[8]) / 1000

                # 状态机映射
                run_mode_str = parse_run_state(regs[9])

                # 终端 UI 刷新
                output = (
                    f"\r[{run_mode_str}] "
                    f"姿态(角:{ang:<5} 位:{loc:<5} 目标:{tar:<5}) | "
                    f"内环(P:{akp:<5.3f} I:{aki:<5.3f} D:{akd:<5.3f}) | "
                    f"外环(P:{lkp:<5.3f} I:{lki:<5.3f} D:{lkd:<5.3f})   "
                )
                print(output, end="", flush=True)
            else:
                print("\r[!] 读取超时，网关无响应...          ", end="", flush=True)

        except Exception:
            pass

        time.sleep(0.1)


def execute_command(cmd_str):
    """解析并执行用户输入的控制指令"""
    parts = cmd_str.strip().lower().split()
    if not parts:
        return

    cmd = parts[0]

    # 指令字典映射 (指令 -> 寄存器地址)
    pid_map = {
        'tar': 2,
        'akp': 3, 'aki': 4, 'akd': 5,
        'lkp': 6, 'lki': 7, 'lkd': 8
    }

    try:
        if cmd == 'start':
            client.write_register(9, 21, device_id=SLAVE_ID)
            print("[+] 已下发启动指令 (RunState=21)！")

        elif cmd == 'stop':
            client.write_register(9, 0, device_id=SLAVE_ID)
            print("[+] 已下发停止指令 (RunState=0)！")

        elif cmd in pid_map and len(parts) == 2:
            addr = pid_map[cmd]
            # 物理量乘以 1000 转换为整数下发
            if cmd == 'tar':
                val = int(parts[1])
            else:
                val = int(float(parts[1]) * 1000)

            # 处理负数补码
            if val < 0:
                val = val + 65536

            client.write_register(addr, val, device_id=SLAVE_ID)
            print(f"[+] 参数已下发: {cmd.upper()} -> {parts[1]}")

        else:
            print("[-] 未知指令或参数格式错误。格式示例: start, stop, akp 0.45, tar 100")

    except Exception as e:
        print(f"[-] 下发失败: {e}")


if __name__ == "__main__":
    print(f"[*] 正在连接 {PORT} ...")
    if not client.connect():
        print("[!] 串口连接失败！")
        sys.exit(1)

    print("[*] 连接成功！按 Ctrl+C 进入【命令调参模式】\n")

    # 启动后台刷新线程
    t = threading.Thread(target=monitor_thread, daemon=True)
    t.start()

    # 【核心修复】：让整个交互系统运行在一个永不退出的循环中
    while True:
        try:
            # 1. 监控模式：主线程在此安静休眠，把舞台交给后台线程
            while not pause_monitor:
                time.sleep(0.5)

            # 2. 调参模式：当 pause_monitor 变为 True 时，接管控制台
            print("\n\n" + "=" * 50)
            print("🛠️ 进入命令模式 (支持指令: start, stop, akp/aki/akd <值>, lkp/lki/lkd <值>, tar <位置>, exit)")
            print("=" * 50)

            while pause_monitor:
                try:
                    cmd_input = input("Modbus> ")
                    if cmd_input.strip().lower() == 'exit':
                        print("[*] 退出程序。")
                        client.close()
                        sys.exit(0)
                    elif cmd_input.strip() == '':
                        # 直接回车，恢复监控
                        print("[*] 恢复实时监控...\n")
                        pause_monitor = False
                        break  # 跳出 input 循环，回到最外层继续睡眠
                    else:
                        execute_command(cmd_input)
                except KeyboardInterrupt:
                    # 防止在 Modbus> 提示符下误按 Ctrl+C 导致程序崩溃
                    print("\n[!] 若要退出请直接输入 exit，若要恢复监控请直接回车。")

        except KeyboardInterrupt:
            # 在高速刷屏监控时，捕捉到了用户的 Ctrl+C 打断
            pause_monitor = True
            time.sleep(0.2)  # 微微延迟，等待后台线程完成最后一次串口收发，防止总线冲突